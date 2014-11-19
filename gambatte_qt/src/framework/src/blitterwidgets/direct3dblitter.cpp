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

#include "direct3dblitter.h"
#include "../dwmcontrol.h"
#include "../gdisettings.h"
#include "uncopyable.h"
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QSettings>
#include <QStyle>
#include <QVBoxLayout>
#include <iostream>

namespace {

class ModeLock : Uncopyable {
public:
	explicit ModeLock(HMONITOR monitor) : monitor(monitor) {
		ZeroMemory(&originalMode, sizeof originalMode);
		originalMode.dmSize = sizeof originalMode;
		gdiSettings.enumDisplaySettings(monitor, ENUM_REGISTRY_SETTINGS, &originalMode);

		DEVMODE devmode;
		ZeroMemory(&devmode, sizeof devmode);
		devmode.dmSize = sizeof devmode;
		gdiSettings.enumDisplaySettings(monitor, ENUM_CURRENT_SETTINGS, &devmode);
		gdiSettings.changeDisplaySettings(monitor, &devmode,
		                                  CDS_NORESET | CDS_UPDATEREGISTRY);
	}

	~ModeLock() {
		gdiSettings.changeDisplaySettings(monitor, &originalMode,
		                                  CDS_NORESET | CDS_UPDATEREGISTRY);
	}

private:
	HMONITOR monitor;
	DEVMODE originalMode;
};

struct Vertex {
	float x, y, z, rhw, u, v;
};

}

Direct3DBlitter::Direct3DBlitter(VideoBufferLocker vbl, QWidget *parent)
: BlitterWidget(vbl, "Direct3D", 2, parent)
, confWidget(new QWidget)
, adapterSelector(new QComboBox(confWidget.get()))
, vblankblit_(new QCheckBox(tr("Wait for vertical blank"), confWidget.get()),
              "direct3dblitter/vblankblit", false)
, flipping_(new QCheckBox(tr("Exclusive full screen"), confWidget.get()),
            "direct3dblitter/flipping", false)
, vblankflip_(new QCheckBox(tr("Flip during vertical blank"), confWidget.get()),
              "direct3dblitter/vblankflip", true)
, triplebuf_(new QCheckBox(tr("Triple buffering"), confWidget.get()),
             "direct3dblitter/triplebuf", false)
, bf_(new QCheckBox(tr("Bilinear filtering"), confWidget.get()),
      "direct3dblitter/bf", true)
, d3d9handle()
, d3d()
, device()
, vertexBuffer()
, stexture()
, vtexture()
, lastblank(0)
, backBufferWidth(1)
, backBufferHeight(1)
, clear(0)
, dhz(600)
, swapInterval(0)
, adapterIndex(0)
, exclusive(false)
, windowed(false)
, drawn(false)
{
	setAttribute(Qt::WA_PaintOnScreen, true);

	if ((d3d9handle = LoadLibraryA("d3d9.dll"))) {
		typedef IDirect3D9 * (WINAPI *Direct3DCreate9Ptr)(UINT);
		Direct3DCreate9Ptr const direct3DCreate9 =
			reinterpret_cast<Direct3DCreate9Ptr>(GetProcAddress(d3d9handle,
			                                                    "Direct3DCreate9"));
		if (direct3DCreate9 && (d3d = direct3DCreate9(D3D_SDK_VERSION))) {
			unsigned const adapterCount = d3d->GetAdapterCount();
			for (unsigned i = 0; i < adapterCount; ++i) {
				D3DADAPTER_IDENTIFIER9 adapterId;
				if (FAILED(d3d->GetAdapterIdentifier(i, 0, &adapterId)))
					break;

				adapterSelector->addItem(adapterId.Description);
			}
		}
	}

	if (adapterSelector->count() < 1)
		adapterSelector->addItem(QString());

	QSettings settings;
	adapterIndex = settings.value("direct3dblitter/adapterIndex", adapterIndex).toUInt();
	if (adapterIndex >= static_cast<uint>(adapterSelector->count()))
		adapterIndex = 0;

	{
		QVBoxLayout *const mainLayout = new QVBoxLayout(confWidget.get());
		mainLayout->setMargin(0);

		if (adapterSelector->count() > 1) {
			QHBoxLayout *hlayout = new QHBoxLayout;
			mainLayout->addLayout(hlayout);
			hlayout->addWidget(new QLabel(tr("Direct3D adapter:")));
			hlayout->addWidget(adapterSelector);
		} else
			adapterSelector->hide();

		mainLayout->addWidget(vblankblit_.checkBox());
		vblankblit_.checkBox()->setToolTip(
			tr("Prevents tearing. Does not work well on all systems.\n"
			   "Ignored when exclusive full screen or DWM composition is active."));
		mainLayout->addWidget(flipping_.checkBox());
		flipping_.checkBox()->setToolTip(tr("Grabs device for better performance when full screen."));

		{
			QHBoxLayout *l = new QHBoxLayout;
			mainLayout->addLayout(l);
			l->addSpacing(QApplication::style()->pixelMetric(QStyle::PM_LayoutLeftMargin));
			l->addWidget(vblankflip_.checkBox());
			vblankflip_.checkBox()->setToolTip(tr("Prevents tearing. Recommended."));
		}

		mainLayout->addWidget(triplebuf_.checkBox());
		triplebuf_.checkBox()->setToolTip(
			tr("Attempts to improve video flow at the cost of increased latency."));
		mainLayout->addWidget(bf_.checkBox());
	}

	vblankflip_.checkBox()->setEnabled(flipping_.checkBox()->isChecked());
	connect(flipping_.checkBox(), SIGNAL(toggled(bool)),
	        vblankflip_.checkBox(), SLOT(setEnabled(bool)));
	rejectSettings();
}

Direct3DBlitter::~Direct3DBlitter() {
	uninit();

	if (d3d)
		d3d->Release();
	if (d3d9handle)
		FreeLibrary(d3d9handle);

	QSettings settings;
	settings.beginGroup("direct3dblitter");
	settings.setValue("adapterIndex", adapterIndex);
	settings.endGroup();
}

void Direct3DBlitter::getPresentParams(D3DPRESENT_PARAMETERS *const presentParams) const {
	bool excl = exclusive && flipping_.value();
	if (gdiSettings.monitorFromWindow
			&& d3d->GetAdapterMonitor(adapterIndex)
			   != gdiSettings.monitorFromWindow(parentWidget()->parentWidget()->winId(),
			                                    GdiSettings::MON_DEFAULTTONEAREST)) {
		excl = false;
	}

	D3DDISPLAYMODE displayMode;
	if (FAILED(d3d->GetAdapterDisplayMode(adapterIndex, &displayMode))) {
		excl = false;
		displayMode.Format = D3DFMT_UNKNOWN;
		std::cerr << "d3d->GetAdapterDisplayMode failed" << std::endl;
	}

	presentParams->BackBufferWidth = excl ? displayMode.Width : width();
	presentParams->BackBufferHeight = excl ? displayMode.Height : height();
	presentParams->BackBufferFormat = displayMode.Format;
	presentParams->BackBufferCount = triplebuf_.value() ? 2 : 1;
	presentParams->MultiSampleType = D3DMULTISAMPLE_NONE;
	presentParams->MultiSampleQuality = 0;
	presentParams->SwapEffect = excl ? D3DSWAPEFFECT_FLIP : D3DSWAPEFFECT_DISCARD;
	presentParams->hDeviceWindow = excl ? parentWidget()->parentWidget()->winId() : winId();
	presentParams->Windowed = excl ? FALSE : TRUE;
	presentParams->EnableAutoDepthStencil = FALSE;
	presentParams->AutoDepthStencilFormat = D3DFMT_UNKNOWN;
	presentParams->Flags = 0;
	presentParams->FullScreen_RefreshRateInHz = excl ? displayMode.RefreshRate : 0;

	if (swapInterval > 0) {
		presentParams->PresentationInterval = swapInterval > 1
		                                    ? D3DPRESENT_INTERVAL_TWO
		                                    : D3DPRESENT_INTERVAL_ONE;
	} else {
		bool vblankPresent = excl
			? vblankflip_.value()
			: vblankblit_.value() && !DwmControl::isCompositingEnabled();
		presentParams->PresentationInterval = vblankPresent 
		                                    ? D3DPRESENT_INTERVAL_ONE
		                                    : D3DPRESENT_INTERVAL_IMMEDIATE;
	}
}

void Direct3DBlitter::lockTexture(SetBuffer setInputBuffer) {
	RECT const rect = { 0, 0, inBuffer().width, inBuffer().height };
	D3DLOCKED_RECT lockedrect;
	lockedrect.pBits = 0;
	if (stexture)
		stexture->LockRect(0, &lockedrect, &rect, D3DLOCK_NOSYSLOCK);

	setInputBuffer(lockedrect.pBits, PixelBuffer::RGB32, lockedrect.Pitch >> 2);
}

unsigned Direct3DBlitter::textureSizeFromInBufferSize() const {
	unsigned textRes = std::max(inBuffer().width, inBuffer().height);
	--textRes;
	textRes |= textRes >> 1;
	textRes |= textRes >> 2;
	textRes |= textRes >> 4;
	textRes |= textRes >> 8;
	++textRes;

	return textRes;
}

void Direct3DBlitter::setVertexBuffer() {
	if (!vertexBuffer)
		return;

	Vertex *vertices = 0;
	if (!FAILED(vertexBuffer->Lock(0, 0, reinterpret_cast<VOID **>(&vertices), 0))) {
		unsigned const textRes = textureSizeFromInBufferSize();
		unsigned const xoffset = backBufferWidth > static_cast<unsigned>(width())
		                       ? (backBufferWidth - width()) >> 1
		                       : 0;
		unsigned const yoffset = backBufferHeight > static_cast<unsigned>(height())
		                       ? (backBufferHeight - height()) >> 1
		                       : 0;

		vertices[0].x = xoffset - 0.5f;
		vertices[0].y = yoffset + height() - 0.5f;
		vertices[0].z = 0.0f;
		vertices[0].rhw = 1.0f;
		vertices[0].u = 0.0f;
		vertices[0].v = static_cast<float>(inBuffer().height) / textRes;

		vertices[1].x = xoffset - 0.5f;
		vertices[1].y = yoffset - 0.5f;
		vertices[1].z = 0.0f;
		vertices[1].rhw = 1.0f;
		vertices[1].u = 0.0f;
		vertices[1].v = 0.0f;

		vertices[2].x = xoffset + width() - 0.5f;
		vertices[2].y = yoffset + height() - 0.5f;
		vertices[2].z = 0.0f;
		vertices[2].rhw = 1.0f;
		vertices[2].u = static_cast<float>(inBuffer().width) / textRes;
		vertices[2].v = static_cast<float>(inBuffer().height) / textRes;

		vertices[3].x = xoffset + width() - 0.5f;
		vertices[3].y = yoffset - 0.5f;
		vertices[3].z = 0.0f;
		vertices[3].rhw = 1.0f;
		vertices[3].u = static_cast<float>(inBuffer().width) / textRes;
		vertices[3].v = 0.0f;

		vertexBuffer->Unlock();
	} else
		std::cerr << "vertexBuffer->Lock failed" << std::endl;
}

void Direct3DBlitter::setFilter() {
	if (device) {
		device->SetSamplerState(0, D3DSAMP_MINFILTER,
		                        bf_.value() ? D3DTEXF_LINEAR : D3DTEXF_POINT);
		device->SetSamplerState(0, D3DSAMP_MAGFILTER,
		                        bf_.value() ? D3DTEXF_LINEAR : D3DTEXF_POINT);
	}
}

void Direct3DBlitter::setDeviceState() {
	if (device) {
		device->SetFVF(D3DFVF_XYZRHW | D3DFVF_TEX1);
		if (vertexBuffer)
			device->SetStreamSource(0, vertexBuffer, 0, sizeof(Vertex));

		setFilter();
	}
}

void Direct3DBlitter::resetDevice() {
	if (device && device->TestCooperativeLevel() != D3DERR_DEVICELOST) {
		device->SetTexture(0, 0);
		if (vtexture) {
			vtexture->Release();
			vtexture = 0;
		}

		D3DPRESENT_PARAMETERS presentParams;
		getPresentParams(&presentParams);
		if (FAILED(device->Reset(&presentParams))
				&& FAILED(device->Reset(&presentParams))) {
			if (stexture) {
				BufferLock lock(*this);
				SetBuffer setInputBuffer(lock);
				setInputBuffer(0, PixelBuffer::RGB32, 0);
				stexture->Release();
				stexture = 0;
			}

			if (vertexBuffer) {
				vertexBuffer->Release();
				vertexBuffer = 0;
			}

			device->Release();
			device = 0;

			std::cerr << "device->Reset failed" << std::endl;
		} else {
			backBufferWidth = presentParams.BackBufferWidth;
			backBufferHeight = presentParams.BackBufferHeight;
			windowed = presentParams.Windowed;
			clear = presentParams.BackBufferCount + 1;
			setDeviceState();
			setVideoTexture();
			setVertexBuffer();
		}
	}

	drawn = false;
}

void Direct3DBlitter::exclusiveChange() {
	if (isVisible()) {
		if (exclusive && flipping_.value()) {
			if (windowed)
				resetDevice();
		} else if (!windowed) {
			ModeLock mlock(d3d->GetAdapterMonitor(adapterIndex));
			resetDevice();
			SetWindowPos(parentWidget()->parentWidget()->winId(),
			             HWND_NOTOPMOST, 0, 0, 0, 0,
			             SWP_NOMOVE | SWP_NOSIZE);
		}
	}
}

void Direct3DBlitter::setSwapInterval(unsigned const newSwapInterval) {
	if (newSwapInterval != swapInterval) {
		swapInterval = newSwapInterval;
		resetDevice();
		ftEst = FtEst(swapInterval * 10000000 / dhz);
	}
}

void Direct3DBlitter::draw() {
	if (device) {
		if (clear && !drawn) {
			--clear;
			device->Clear(0, 0, D3DCLEAR_TARGET,
			              D3DCOLOR_ARGB(0xFF, 0, 0, 0),
			              1.0, 0);
		}

		if (SUCCEEDED(device->BeginScene())) {
			device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
			device->EndScene();
		}
	}

	drawn = true;
}

void Direct3DBlitter::doPresent() {
	if (device && device->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
		resetDevice();

	if (device) {
		if (swapInterval) {
			unsigned long const estft = ftEst.est();
			usec_t const swaplimit = getusecs() - lastblank > estft - estft / 32
			                       ? estft * 2 - 5000000 / dhz
			                       : 0;
			device->Present(0, 0, 0, 0);

			usec_t now = getusecs();
			if (now - lastblank < swaplimit) {
				drawn = false;
				draw();
				device->Present(0, 0, 0, 0);
				now = getusecs();
			}

			lastblank = now;
		} else {
			IDirect3DSwapChain9 *swapChain = 0;
			device->GetSwapChain(0, &swapChain);

			if (swapChain) {
				enum { DONOTWAIT = 1 };

				bool vblankPresent = windowed
				                   ? vblankblit_.value()
				                   : vblankflip_.value();
				DWORD flags = vblankPresent ? DONOTWAIT : 0;
				swapChain->Present(0, 0, 0, 0, flags);
				swapChain->Release();
			} else
				device->Present(0, 0, 0, 0);
		}
	}

	drawn = false;
}

void Direct3DBlitter::init() {
	{
		D3DPRESENT_PARAMETERS presentParams;
		getPresentParams(&presentParams);

		// Omitting the D3DCREATE_FPU_PRESERVE will cause problems with floating point
		// code elsewhere. For instance WASAPI screws up cursor time stamps.
		for (int n = 2; n--;) {
			if (!FAILED(d3d->CreateDevice(
					adapterIndex, D3DDEVTYPE_HAL,
					parentWidget()->parentWidget()->winId(),
					D3DCREATE_FPU_PRESERVE | D3DCREATE_SOFTWARE_VERTEXPROCESSING,
					&presentParams, &device))) {
				break;
			}
		}

		backBufferWidth = presentParams.BackBufferWidth;
		backBufferHeight = presentParams.BackBufferHeight;
		windowed = presentParams.Windowed;
		clear = presentParams.BackBufferCount + 1;
	}

	if (!device) {
		std::cerr << "d3d->CreateDevice failed" << std::endl;
	} else if (FAILED(device->CreateVertexBuffer(sizeof(Vertex) * 4, 0,
	                                             D3DFVF_XYZRHW | D3DFVF_TEX1,
	                                             D3DPOOL_MANAGED, &vertexBuffer, 0))) {
		std::cerr << "device->CreateVertexBuffer failed" << std::endl;
	}

	setDeviceState();

	drawn = false;
}

void Direct3DBlitter::uninit() {
	if (device) {
		device->SetStreamSource(0, 0, 0, 0);
		device->SetTexture(0, 0);
	}
	if (stexture) {
		stexture->UnlockRect(0);
		stexture->Release();
		stexture = 0;
	}
	if (vtexture) {
		vtexture->Release();
		vtexture = 0;
	}
	if (vertexBuffer) {
		vertexBuffer->Release();
		vertexBuffer = 0;
	}
	if (device) {
		device->Release();
		device = 0;
	}
}

void Direct3DBlitter::setVideoTexture() {
	if (device) {
		device->SetTexture(0, 0);

		if (vtexture) {
			vtexture->Release();
			vtexture = 0;
		}

		unsigned const textRes = textureSizeFromInBufferSize();
		if (FAILED(device->CreateTexture(textRes, textRes, 1, 0,
		                                 D3DFMT_X8R8G8B8, D3DPOOL_DEFAULT, &vtexture, 0))) {
			std::cerr << "device->CreateTexture failed" << std::endl;
		}

		device->SetTexture(0, vtexture);
	}
}

void Direct3DBlitter::setBufferDimensions(unsigned w, unsigned h, SetBuffer setInputBuffer) {
	if (device) {
		if (stexture) {
			stexture->UnlockRect(0);
			stexture->Release();
			stexture = 0;
		}

		unsigned const textRes = textureSizeFromInBufferSize();
		if (FAILED(device->CreateTexture(textRes, textRes, 1, 0,
		                                 D3DFMT_X8R8G8B8, D3DPOOL_SYSTEMMEM, &stexture, 0))) {
			std::cerr << "device->CreateTexture failed" << std::endl;
		}
	}

	lockTexture(setInputBuffer);
	setVideoTexture();
	setVertexBuffer();
}

void Direct3DBlitter::consumeBuffer(SetBuffer setInputBuffer) {
	if (device && stexture && vtexture) {
		stexture->UnlockRect(0);
		device->UpdateTexture(stexture, vtexture);
		lockTexture(setInputBuffer);
	}
}

int Direct3DBlitter::present() {
	if (!drawn)
		draw();

	doPresent();
	if (swapInterval)
		ftEst.update(getusecs());

	if (!device)
		return -1;

	return 0;
}

long Direct3DBlitter::frameTimeEst() const {
	if (swapInterval)
		return ftEst.est();

	return BlitterWidget::frameTimeEst();
}

void Direct3DBlitter::setExclusive(bool const exclusive) {
	if (exclusive != this->exclusive) {
		this->exclusive = exclusive;
		exclusiveChange();
	}
}

void Direct3DBlitter::acceptSettings() {
	if (static_cast<int>(adapterIndex) != adapterSelector->currentIndex()) {
		adapterIndex = adapterSelector->currentIndex();
		if (isVisible()) {
			BufferLock lock(*this);
			SetBuffer setInputBuffer(lock);
			uninit();
			setInputBuffer(0, PixelBuffer::RGB32, 0);
			init();
			setBufferDimensions(inBuffer().width, inBuffer().height, setInputBuffer);
		}
	}

	bool const oldFlipping = flipping_.value();
	flipping_.accept();
	if (flipping_.value() != oldFlipping)
		exclusiveChange();

	vblankblit_.accept();
	vblankflip_.accept();
	triplebuf_.accept();
	bf_.accept();
	resetDevice();
}

void Direct3DBlitter::rejectSettings() const {
	adapterSelector->setCurrentIndex(adapterIndex);
	flipping_.reject();
	vblankblit_.reject();
	vblankflip_.reject();
	triplebuf_.reject();
	bf_.reject();
}

void Direct3DBlitter::rateChange(int const dhz) {
	this->dhz = dhz ? dhz : 600;
	ftEst = FtEst(swapInterval * 10000000 / this->dhz);
}

void Direct3DBlitter::compositionEnabledChange() {
	if (windowed && vblankblit_.value())
		resetDevice();
}

void Direct3DBlitter::resizeEvent(QResizeEvent *) {
	resetDevice();
}

void Direct3DBlitter::paintEvent(QPaintEvent *) {
	if (!drawn)
		draw();

	doPresent();
}
