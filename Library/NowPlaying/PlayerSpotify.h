/* Copyright (C) 2011 Rainmeter Project Developers
 *
 * This Source Code Form is subject to the terms of the GNU General Public
 * License; either version 2 of the License, or (at your option) any later
 * version. If a copy of the GPL was not distributed with this file, You can
 * obtain one at <https://www.gnu.org/licenses/gpl-2.0.html>. */

#ifndef __PLAYERSPOTIFY_H__
#define __PLAYERSPOTIFY_H__

#include <locale>
#include <codecvt>
#include "Player.h"

class PlayerSpotify : public Player
{
public:
	virtual ~PlayerSpotify();

	static Player* Create();

	virtual void Pause() { return Play(); }
	virtual void Play();
	virtual void Stop();
	virtual void Next();
	virtual void Previous();
	virtual void ClosePlayer();
	virtual void OpenPlayer(std::wstring& path);
	virtual void UpdateData();

protected:
	PlayerSpotify();

private:
	enum SPOTIFYCOMMAND
	{
		SPOTIFY_MUTE		= 0x80000,
		SPOTIFY_VOLUMEDOWN	= 0x90000,
		SPOTIFY_VOLUMEUP	= 0xA0000,
		SPOTIFY_NEXT		= 0xB0000,
		SPOTIFY_PREV		= 0xC0000,
		SPOTIFY_STOP		= 0xD0000,
		SPOTIFY_PLAYPAUSE	= 0xE0000
	};

	bool CheckWindow();
	bool PlayerSpotify::GetTokens();

	static Player* c_Player;

	HWND m_Window;
	DWORD m_LastCheckTime;
	const std::wstring m_baseURL = L"http://127.0.0.1:4380/";
	const std::wstring m_csrfURL = L"simplecsrf/token.json";
	const std::wstring m_statusURL = L"remote/status.json";
	const std::wstring m_openidURL = L"https://open.spotify.com/token";
	const LPWSTR originHeader = L"User-Agent: Mozilla/5.0 (Windows NT 10.0; WOW64; rv:43.0) Gecko/20100101 Firefox/43.0\r\nOrigin: https://open.spotify.com\r\n\r\n\r\n";
	std::wstring csrfToken, openidToken;
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
};

#endif
