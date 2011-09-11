/***************************************************************************
 *   Copyright (C) 2008 by Sindre Aamås                                    *
 *   aamas@stud.ntnu.no                                                    *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License version 2 as     *
 *   published by the Free Software Foundation.                            *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License version 2 for more details.                *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   version 2 along with this program; if not, write to the               *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/
#include "gdisettings.h"
#include <QtGlobal>

GdiSettings::GdiSettings() : user32handle(NULL), getMonitorInfo(NULL), changeDisplaySettingsEx(NULL), monitorFromWindow(NULL), monitorFromPoint(NULL) {
	if ((user32handle = LoadLibraryA("user32.dll"))) {
		QT_WA({getMonitorInfo = (GetMonInfo)GetProcAddress(user32handle, "GetMonitorInfoW");},
		      {getMonitorInfo = (GetMonInfo)GetProcAddress(user32handle, "GetMonitorInfoA");});
		
		QT_WA({changeDisplaySettingsEx = (ChangeGdiSettingsEx)GetProcAddress(user32handle, "ChangeDisplaySettingsExW");},
		      {changeDisplaySettingsEx = (ChangeGdiSettingsEx)GetProcAddress(user32handle, "ChangeDisplaySettingsExA");});
		
		monitorFromWindow = (MonFromWindow)GetProcAddress(user32handle, "MonitorFromWindow");
		monitorFromPoint = (MonFromPoint)GetProcAddress(user32handle, "MonitorFromPoint");
	}
}

GdiSettings::~GdiSettings() {
	if (user32handle)
		FreeLibrary(user32handle);
}

TCHAR* GdiSettings::getMonitorName(HMONITOR monitor, GdiSettings::MonInfo *minfo) const {
	if (getMonitorInfo && monitor) {
		minfo->cbSize = sizeof(MonInfo);
		getMonitorInfo(monitor, minfo);
		
		return minfo->szDevice;
	}
	
	return NULL;
}

BOOL GdiSettings::enumDisplaySettings(HMONITOR monitor, DWORD iModeNum, LPDEVMODE devmode) const {
	MonInfo minfo;
	
	return EnumDisplaySettings(getMonitorName(monitor, &minfo), iModeNum, devmode);
}

LONG GdiSettings::changeDisplaySettings(HMONITOR monitor, LPDEVMODE devmode, DWORD dwflags) const {
	MonInfo minfo;
	
	return changeDisplaySettingsEx ?
		changeDisplaySettingsEx(getMonitorName(monitor, &minfo), devmode, NULL, dwflags, NULL) :
		ChangeDisplaySettings(devmode, dwflags);
}

const GdiSettings gdiSettings;
