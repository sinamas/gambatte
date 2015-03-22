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

#include "dwmcontrol.h"
#include "blitterwidget.h"

#ifdef Q_WS_WIN

#include <windows.h>
#include <algorithm>
#include <functional>

namespace {

typedef struct _UNSIGNED_RATIO {
  UINT32 uiNumerator;
  UINT32 uiDenominator;
} UNSIGNED_RATIO;

typedef enum _DWM_SOURCE_FRAME_SAMPLING {
  DWM_SOURCE_FRAME_SAMPLING_POINT      = 1,
  DWM_SOURCE_FRAME_SAMPLING_COVERAGE,
  DWM_SOURCE_FRAME_SAMPLING_LAST
} DWM_SOURCE_FRAME_SAMPLING;

typedef struct _DWM_PRESENT_PARAMETERS {
  UINT32                    cbSize;
  BOOL                      fQueue;
  ULONGLONG                 cRefreshStart;
  UINT                      cBuffer;
  BOOL                      fUseSourceRate;
  UNSIGNED_RATIO            rateSource;
  UINT                      cRefreshesPerFrame;
  DWM_SOURCE_FRAME_SAMPLING eSampling;
} DWM_PRESENT_PARAMETERS;

typedef HRESULT (WINAPI *DwmSetPresentParametersFunc)(HWND hwnd, DWM_PRESENT_PARAMETERS *pPresentParams);
typedef HRESULT (WINAPI *DwmIsCompositionEnabledFunc)(BOOL *pfEnabled);

class DwmApi {
public:
	DwmApi()
	: dwmapidll_(LoadLibraryA("dwmapi.dll"))
	, setPresentParameters_(dwmapidll_
	                        ? reinterpret_cast<DwmSetPresentParametersFunc>(
	                              GetProcAddress(dwmapidll_, "DwmSetPresentParameters"))
	                        : 0)
	, isCompositionEnabled_(dwmapidll_
	                        ? reinterpret_cast<DwmIsCompositionEnabledFunc>(
	                              GetProcAddress(dwmapidll_, "DwmIsCompositionEnabled"))
	                        : 0)
	{
	}

	~DwmApi() {
		if (dwmapidll_)
			FreeLibrary(dwmapidll_);
	}

	bool isUsable() const { return setPresentParameters_; }

	void setPresentParameters(HWND hwnd, DWM_PRESENT_PARAMETERS *pPresentParams) {
		if (setPresentParameters_)
			setPresentParameters_(hwnd, pPresentParams);
	}

	bool isCompositionEnabled() const {
		if (isCompositionEnabled_) {
			BOOL enabled = FALSE;
			isCompositionEnabled_(&enabled);
			return enabled;
		}

		return false;
	}

private:
	HMODULE const dwmapidll_;
	DwmSetPresentParametersFunc const setPresentParameters_;
	DwmIsCompositionEnabledFunc const isCompositionEnabled_;
};

static DwmApi dwmapi_;

static void setDwmTripleBuffer(HWND const hwnd, bool const enable) {
	DWM_PRESENT_PARAMETERS p;
	ZeroMemory(&p, sizeof p);
	p.cbSize = sizeof p;
	if (enable) {
		p.fQueue = true;
		p.cBuffer = 2;
		p.fUseSourceRate = false;
		p.cRefreshesPerFrame = 1;
		p.eSampling = DWM_SOURCE_FRAME_SAMPLING_POINT;
	}

	dwmapi_.setPresentParameters(hwnd, &p);
}

}

DwmControl::DwmControl(std::vector<BlitterWidget *> const &blitters)
: blitters_(blitters)
, refreshCnt_(1)
, tripleBuffer_(false)
{
}

void DwmControl::setDwmTripleBuffer(bool enable) {
	tripleBuffer_ = enable;

	if (dwmapi_.isCompositionEnabled()) {
		for (std::vector<BlitterWidget*>::const_iterator it =
				blitters_.begin(); it != blitters_.end(); ++it) {
			::setDwmTripleBuffer((*it)->hwnd(), tripleBuffer_);
		}
	}
}

// OpenGL freezes if minimized with triple buffer enabled
void DwmControl::hideEvent() {
	if (dwmapi_.isCompositionEnabled()) {
		for (std::vector<BlitterWidget*>::const_iterator it =
				blitters_.begin(); it != blitters_.end(); ++it) {
			::setDwmTripleBuffer((*it)->hwnd(), false);
		}
	}
}

void DwmControl::showEvent() {
	if (dwmapi_.isCompositionEnabled()) {
		for (std::vector<BlitterWidget*>::const_iterator it =
				blitters_.begin(); it != blitters_.end(); ++it) {
			::setDwmTripleBuffer((*it)->hwnd(), tripleBuffer_);
		}

		refreshCnt_ = 1;
	}
}

bool DwmControl::winEvent(void const *const msg) {
	enum { WM_DWMCOMPOSITIONCHANGED = 0x031E };

	if (static_cast<MSG const *>(msg)->message == WM_DWMCOMPOSITIONCHANGED) {
		std::for_each(blitters_.begin(), blitters_.end(),
		              std::mem_fun(&BlitterWidget::compositionEnabledChange));
		if (dwmapi_.isCompositionEnabled()) {
			for (std::vector<BlitterWidget*>::const_iterator it =
					blitters_.begin(); it != blitters_.end(); ++it) {
				::setDwmTripleBuffer((*it)->hwnd(), tripleBuffer_);
			}

			refreshCnt_ = 1;
		}

		return true;
	}

	return false;
}

// needed because stuff bugs out when composition is turned off/on
void DwmControl::tick() {
	if (refreshCnt_) {
		--refreshCnt_;

		if (dwmapi_.isCompositionEnabled()) {
			for (std::vector<BlitterWidget*>::const_iterator it =
					blitters_.begin(); it != blitters_.end(); ++it) {
				::setDwmTripleBuffer((*it)->hwnd(), tripleBuffer_);
			}
		}
	}
}

void DwmControl::hwndChange(BlitterWidget *blitter) {
	if (dwmapi_.isCompositionEnabled())
		::setDwmTripleBuffer(blitter->hwnd(), tripleBuffer_);
}

bool DwmControl::hasDwmCapability() {
	return dwmapi_.isUsable();
}

bool DwmControl::isCompositingEnabled() {
	return dwmapi_.isCompositionEnabled();
}

#else

DwmControl::DwmControl(std::vector<BlitterWidget *> const &) {}
void DwmControl::setDwmTripleBuffer(bool) {}
void DwmControl::hideEvent() {}
void DwmControl::showEvent() {}
bool DwmControl::winEvent(void const *) { return false; }
void DwmControl::tick() {}
void DwmControl::hwndChange(BlitterWidget *) {}
bool DwmControl::hasDwmCapability() { return false; }
bool DwmControl::isCompositingEnabled() { return false; }

#endif
