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

#include "gdisettings.h"
#include <QtGlobal>

GdiSettings::GdiSettings()
: user32handle()
, getMonitorInfo()
, changeDisplaySettingsEx()
, monitorFromWindow()
, monitorFromPoint()
{
	if ((user32handle = LoadLibraryA("user32.dll"))) {
		QT_WA({getMonitorInfo = reinterpret_cast<GetMonInfo>(
				GetProcAddress(user32handle, "GetMonitorInfoW"));},
		      {getMonitorInfo = reinterpret_cast<GetMonInfo>(
				GetProcAddress(user32handle, "GetMonitorInfoA"));});
		QT_WA({changeDisplaySettingsEx = reinterpret_cast<ChangeGdiSettingsEx>(
				GetProcAddress(user32handle, "ChangeDisplaySettingsExW"));},
		      {changeDisplaySettingsEx = reinterpret_cast<ChangeGdiSettingsEx>(
				GetProcAddress(user32handle, "ChangeDisplaySettingsExA"));});
		monitorFromWindow = reinterpret_cast<MonFromWindow>(
			GetProcAddress(user32handle, "MonitorFromWindow"));
		monitorFromPoint = reinterpret_cast<MonFromPoint>(
			GetProcAddress(user32handle, "MonitorFromPoint"));
	}
}

GdiSettings::~GdiSettings() {
	if (user32handle)
		FreeLibrary(user32handle);
}

TCHAR* GdiSettings::getMonitorName(HMONITOR monitor, GdiSettings::MonInfo *minfo) const {
	if (getMonitorInfo && monitor) {
		minfo->cbSize = sizeof *minfo;
		getMonitorInfo(monitor, minfo);
		return minfo->szDevice;
	}

	return 0;
}

BOOL GdiSettings::enumDisplaySettings(HMONITOR monitor, DWORD iModeNum, LPDEVMODE devmode) const {
	MonInfo minfo;
	return EnumDisplaySettings(getMonitorName(monitor, &minfo), iModeNum, devmode);
}

LONG GdiSettings::changeDisplaySettings(HMONITOR monitor, LPDEVMODE devmode, DWORD dwflags) const {
	MonInfo minfo;
	return changeDisplaySettingsEx
	     ? changeDisplaySettingsEx(getMonitorName(monitor, &minfo), devmode, 0, dwflags, 0)
	     : ChangeDisplaySettings(devmode, dwflags);
}

GdiSettings const gdiSettings;
