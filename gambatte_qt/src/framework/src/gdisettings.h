//
//   Copyright (C) 2008 by sinamas <sinamas at users.sourceforge.net>
//
//   This program is free software; you can redistribute it and/or modify
//   it under the terms of the GNU General Public License version 2 as
//   published by the Free Software Foundation.
//
//   This program is distributed in the hope that it will be useful,
//   but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//   GNU General Public License version 2 for more details.
//
//   You should have received a copy of the GNU General Public License
//   version 2 along with this program; if not, write to the
//   Free Software Foundation, Inc.,
//   51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.
//

#ifndef GDISETTINGS_H_
#define GDISETTINGS_H_

#include "uncopyable.h"
#include <windows.h>

class GdiSettings : Uncopyable {
private:
	struct MonInfo {
		DWORD  cbSize;
		RECT   rcMonitor;
		RECT   rcWork;
		DWORD  dwFlags;
		TCHAR  szDevice[CCHDEVICENAME];
	};

	typedef BOOL (WINAPI *GetMonInfo)(HMONITOR, MonInfo *);
	typedef HMONITOR (WINAPI *MonFromWindow)(HWND, DWORD);
	typedef HMONITOR (WINAPI *MonFromPoint)(POINT pt, DWORD dwFlags);
	typedef LONG (WINAPI *ChangeGdiSettingsEx)(LPCTSTR, LPDEVMODE, HWND, DWORD, LPVOID);

	HMODULE user32handle;
	GetMonInfo getMonitorInfo;
	ChangeGdiSettingsEx changeDisplaySettingsEx;

public:
	enum { MON_DEFAULTTONEAREST = 2 };

	MonFromWindow monitorFromWindow;
	MonFromPoint monitorFromPoint;

	GdiSettings();
	~GdiSettings();
	BOOL enumDisplaySettings(HMONITOR monitor, DWORD iModeNum, LPDEVMODE devmode) const;
	LONG changeDisplaySettings(HMONITOR monitor, LPDEVMODE devmode, DWORD dwflags) const;

private:
	TCHAR * getMonitorName(HMONITOR monitor, MonInfo *minfo) const;
};

extern GdiSettings const gdiSettings;

#endif /*GDISETTINGS_H_*/
