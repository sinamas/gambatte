//
//   Copyright (C) 2011 by sinamas <sinamas at users.sourceforge.net>
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

#include "mmpriority.h"

#ifdef Q_WS_WIN

#include <windows.h>
#include <iostream>

namespace {

typedef HANDLE (WINAPI *AvSetMmThreadCharacteristicsFunc)(const char *TaskName,LPDWORD TaskIndex);
typedef BOOL (WINAPI *AvRevertMmThreadCharacteristicsFunc)(HANDLE AvrtHandle);

class Avrt : Uncopyable {
	const HMODULE avrtdll_;
	const AvSetMmThreadCharacteristicsFunc setMmThreadCharacteristics_;
	const AvRevertMmThreadCharacteristicsFunc revertMmThreadCharacteristics_;

public:
	Avrt()
	: avrtdll_(LoadLibraryA("avrt.dll")),
	  setMmThreadCharacteristics_(avrtdll_
			  ? (AvSetMmThreadCharacteristicsFunc) GetProcAddress(avrtdll_, "AvSetMmThreadCharacteristicsA") : 0),
	  revertMmThreadCharacteristics_(avrtdll_
			  ? (AvRevertMmThreadCharacteristicsFunc) GetProcAddress(avrtdll_, "AvRevertMmThreadCharacteristics") : 0)
	{
	}

	~Avrt() {
		if (avrtdll_)
			FreeLibrary(avrtdll_);
	}

	HANDLE setMmThreadCharacteristics(const char *TaskName) const {
		if (setMmThreadCharacteristics_) {
			DWORD TaskIndex = 0;
			const HANDLE handle = setMmThreadCharacteristics_(TaskName, &TaskIndex);

			if (!handle)
				std::cout << "AvSetMmThreadCharacteristics failed: " << GetLastError() << "\r\n";

			return handle;
		}

		return 0;
	}

	BOOL revertMmThreadCharacteristics(HANDLE AvrtHandle) const {
		return revertMmThreadCharacteristics_ ? revertMmThreadCharacteristics_(AvrtHandle) : true;
	}

	bool isUsable() const { return setMmThreadCharacteristics_; }
};

static const Avrt avrt_;

}

SetThreadPriorityAudio::SetThreadPriorityAudio()
: handle_(avrt_.setMmThreadCharacteristics("Audio"))
{
}

SetThreadPriorityAudio::~SetThreadPriorityAudio()
{
	if (handle_)
		avrt_.revertMmThreadCharacteristics(handle_);
}

#else

SetThreadPriorityAudio::SetThreadPriorityAudio() {}
SetThreadPriorityAudio::~SetThreadPriorityAudio() {}

#endif
