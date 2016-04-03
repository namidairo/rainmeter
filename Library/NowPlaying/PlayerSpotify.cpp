/* Copyright (C) 2011 Rainmeter Project Developers
*
* This Source Code Form is subject to the terms of the GNU General Public
* License; either version 2 of the License, or (at your option) any later
* version. If a copy of the GPL was not distributed with this file, You can
* obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#include "StdAfx.h"
#include "PlayerSpotify.h"
#include "json/json.hpp"

using JSON = nlohmann::json;

Player* PlayerSpotify::c_Player = nullptr;

/*
** Constructor.
**
*/
PlayerSpotify::PlayerSpotify() : Player(),
m_Window(),
m_LastCheckTime(0)
{
}

/*
** Destructor.
**
*/
PlayerSpotify::~PlayerSpotify()
{
	c_Player = nullptr;
}

/*
** Creates a shared class object.
**
*/
Player* PlayerSpotify::Create()
{
	if (!c_Player)
	{
		c_Player = new PlayerSpotify();
	}

	return c_Player;
}

/*
** Try to find Spotify periodically.
**
*/
bool PlayerSpotify::CheckWindow()
{
	DWORD time = GetTickCount();

	// Try to find Spotify window every 5 seconds
	if (time - m_LastCheckTime > 5000)
	{
		m_LastCheckTime = time;

		m_Window = FindWindow(L"SpotifyMainWindow", nullptr);
		if (m_Window)
		{
			if (csrfToken.empty() || openidToken.empty()) {
				//Get tokens
				if (!GetTokens()) {
					m_Initialized = false;
				}
			}
			m_Initialized = true;
		}
		else
		{
			m_Initialized = false;
		}
	}
	return m_Initialized;
}

/*
** Called during each update of the main measure.
**
*/
void PlayerSpotify::UpdateData()
{
	if (m_Initialized || CheckWindow())
	{
		auto statusParams = L"?oauth=" + openidToken + L"&csrf=" + csrfToken;
		auto statusJson = Internet::DownloadUrl(m_baseURL + m_statusURL + statusParams, CP_UTF8, originHeader, true);
		if (statusJson.empty())
		{
			m_Initialized = false;
			ClearData();
			return;
		}

		auto j = JSON::parse(statusJson);

		if (!j["error"].is_null())
		{
			m_Initialized = false;
			ClearData();
			return;
		}

		if (j["open_graph_state"]["private_session"].get<bool>())
		{
			ClearData();
			return;
		}

		if (!j["track"].is_null())
		{
			m_Title = converter.from_bytes(j["track"]["track_resource"]["name"].get<std::string>());
			m_Artist = converter.from_bytes(j["track"]["artist_resource"]["name"].get<std::string>());
			m_Album = converter.from_bytes(j["track"]["album_resource"]["name"].get<std::string>());
			m_Duration = j["track"]["length"].get<int>();
			m_Position = j["playing_position"].get<int>();
			m_Volume = j["volume"].get<int>() * 100;
			m_State = j["playing"].get<bool>() ? STATE_PLAYING : STATE_PAUSED;
			m_Shuffle = j["shuffle"].get<bool>();
			m_Repeat = j["repeat"].get<bool>();
		}

	}
}

/*
** Gets the CSRF and OpenID tokens from the host and the Spotify site respectively.
**
*/
bool PlayerSpotify::GetTokens()
{
	auto csrfJson = Internet::DownloadUrl(m_baseURL + m_csrfURL, CP_UTF8, originHeader, true);

	if(csrfJson.empty())
	{
		return false;
	}

	auto j = JSON::parse(csrfJson);

	csrfToken = converter.from_bytes(j["token"].get<std::string>());

	if (csrfToken.empty()) {
		return false;
	}

	//Grab OpenID token
	auto openidJson = Internet::DownloadUrl(m_openidURL, CP_UTF8, originHeader, true);
	j = JSON::parse(openidJson);
	openidToken = converter.from_bytes(j["t"].get<std::string>());
	if (openidJson.empty()) {
		return false;
	}
	return true;
}

/*
** Handles the Play bang.
**
*/
void PlayerSpotify::Play()
{
	SendMessage(m_Window, WM_APPCOMMAND, 0, SPOTIFY_PLAYPAUSE);
}

/*
** Handles the Stop bang.
**
*/
void PlayerSpotify::Stop()
{
	SendMessage(m_Window, WM_APPCOMMAND, 0, SPOTIFY_STOP);
}

/*
** Handles the Next bang.
**
*/
void PlayerSpotify::Next()
{
	SendMessage(m_Window, WM_APPCOMMAND, 0, SPOTIFY_NEXT);
}

/*
** Handles the Previous bang.
**
*/
void PlayerSpotify::Previous()
{
	SendMessage(m_Window, WM_APPCOMMAND, 0, SPOTIFY_PREV);
}


/*
** Handles the ClosePlayer bang.
**
*/
void PlayerSpotify::ClosePlayer()
{
	// A little harsh...
	DWORD pID;
	GetWindowThreadProcessId(m_Window, &pID);
	HANDLE hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, pID);
	if (hProcess)
	{
		TerminateProcess(hProcess, 0);
		CloseHandle(hProcess);
	}
}

/*
** Handles the OpenPlayer bang.
**
*/
void PlayerSpotify::OpenPlayer(std::wstring& path)
{
	if (!m_Initialized)
	{
		if (path.empty())
		{
			// Gotta figure out where Spotify is located at
			HKEY hKey;
			RegOpenKeyEx(HKEY_CLASSES_ROOT,
				L"spotify\\DefaultIcon",
				0,
				KEY_QUERY_VALUE,
				&hKey);

			DWORD size = 512;
			WCHAR* data = new WCHAR[size];
			DWORD type = 0;

			if (RegQueryValueEx(hKey,
				nullptr,
				nullptr,
				(LPDWORD)&type,
				(LPBYTE)data,
				(LPDWORD)&size) == ERROR_SUCCESS)
			{
				if (type == REG_SZ)
				{
					path = data;
					path.erase(0, 1);				// Get rid of the leading quote
					path.resize(path.length() - 3);	// And the ",0 at the end
					ShellExecute(nullptr, L"open", path.c_str(), nullptr, nullptr, SW_SHOW);
				}
			}

			delete[] data;
			RegCloseKey(hKey);
		}
		else
		{
			ShellExecute(nullptr, L"open", path.c_str(), nullptr, nullptr, SW_SHOW);
		}
	}
	else
	{
		// Already active, restore the window
		ShowWindow(m_Window, SW_SHOWNORMAL);
		BringWindowToTop(m_Window);
	}
}
