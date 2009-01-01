/***************************************************************************
 *   Copyright (C) 2008 by Sindre Aam�s                                    *
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
#include "direct3dblitter.h"
#include "../gdisettings.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QSettings>
#include <QApplication>
#include <QStyle>
#include <iostream>

class ModeLock {
	HMONITOR monitor;
	DEVMODE originalMode;

public:
	ModeLock(HMONITOR monitor) : monitor(monitor) {
		ZeroMemory(&originalMode, sizeof(DEVMODE));
		originalMode.dmSize = sizeof(DEVMODE);
		gdiSettings.enumDisplaySettings(monitor, ENUM_REGISTRY_SETTINGS, &originalMode);

		DEVMODE devmode;
		ZeroMemory(&devmode, sizeof(DEVMODE));
		devmode.dmSize = sizeof(DEVMODE);
		gdiSettings.enumDisplaySettings(monitor, ENUM_CURRENT_SETTINGS, &devmode);
		gdiSettings.changeDisplaySettings(monitor, &devmode, CDS_NORESET | CDS_UPDATEREGISTRY);
	}

	~ModeLock() {
		gdiSettings.changeDisplaySettings(monitor, &originalMode, CDS_NORESET | CDS_UPDATEREGISTRY);
	}
};

struct Vertex {
	float x, y, z, rhw, u, v;
};

Direct3DBlitter::Direct3DBlitter(PixelBufferSetter setPixelBuffer, QWidget *parent) :
	BlitterWidget(setPixelBuffer, "Direct3D", parent),
	confWidget(new QWidget),
	adapterSelector(new QComboBox),
	vblankBox(new QCheckBox("Sync to vertical blank in 60 and 120 Hz modes")),
	flippingBox(new QCheckBox("Exclusive full screen")),
	vblankblitBox(new QCheckBox("Only update during vertical blank")),
	triplebufBox(new QCheckBox("Triple buffering")),
	bfBox(new QCheckBox("Bilinear filtering")),
	d3d9handle(NULL),
	direct3DCreate9(NULL),
	d3d(NULL),
	device(NULL),
	vertexBuffer(NULL),
	texture(NULL),
	inWidth(1),
	inHeight(1),
	textRes(1),
	backBufferWidth(1),
	backBufferHeight(1),
	clear(0),
	hz(0),
	swapInterval(0),
	adapterIndex(0),
	exclusive(false),
	windowed(false),
	drawn(false),
	vblank(false),
	flipping(false),
	vblankblit(false),
	triplebuf(false),
	bf(true)
{
	setAttribute(Qt::WA_PaintOnScreen, true);

	if ((d3d9handle = LoadLibraryA("d3d9.dll")))
		direct3DCreate9 = (Direct3DCreate9Ptr)GetProcAddress(d3d9handle, "Direct3DCreate9");

	if (direct3DCreate9) {
		if ((d3d = direct3DCreate9(D3D_SDK_VERSION))) {
			const unsigned adapterCount = d3d->GetAdapterCount();
			D3DADAPTER_IDENTIFIER9 adapterId;

			for (unsigned i = 0; i < adapterCount; ++i) {
				if (FAILED(d3d->GetAdapterIdentifier(i, 0, &adapterId)))
					break;

				adapterSelector->addItem(adapterId.Description);
			}
		}
	}

	if (adapterSelector->count() < 1)
		adapterSelector->addItem(QString());

	QSettings settings;
	settings.beginGroup("direct3dblitter");

	if ((adapterIndex = settings.value("adapterIndex", adapterIndex).toUInt()) >= static_cast<unsigned>(adapterSelector->count()))
		adapterIndex = 0;

	vblank = settings.value("vblank", vblank).toBool();
	flipping = settings.value("flipping", flipping).toBool();
	vblankblit = settings.value("vblankblit", vblankblit).toBool();
	triplebuf = settings.value("triplebuf", triplebuf).toBool();
	bf = settings.value("bf", bf).toBool();
	settings.endGroup();

	{
		QVBoxLayout *mainLayout = new QVBoxLayout;
		mainLayout->setMargin(0);

		if (adapterSelector->count() > 1) {
			QHBoxLayout *const hlayout = new QHBoxLayout;
			hlayout->addWidget(new QLabel(QString(tr("Direct3D adapter:"))));
			hlayout->addWidget(adapterSelector);
			mainLayout->addLayout(hlayout);
		}

		mainLayout->addWidget(vblankBox);
		mainLayout->addWidget(flippingBox);
		mainLayout->addWidget(vblankblitBox);
		mainLayout->addWidget(triplebufBox);
		mainLayout->addWidget(bfBox);
		confWidget->setLayout(mainLayout);
	}

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
	settings.setValue("vblank", vblank);
	settings.setValue("flipping", flipping);
	settings.setValue("vblankblit", vblankblit);
	settings.setValue("triplebuf", triplebuf);
	settings.setValue("bf", bf);
	settings.endGroup();
}

void Direct3DBlitter::getPresentParams(D3DPRESENT_PARAMETERS *const presentParams) const {
	D3DDISPLAYMODE displayMode;
	bool excl = exclusive & flipping;

	if (gdiSettings.monitorFromWindow &&
			d3d->GetAdapterMonitor(adapterIndex) != gdiSettings.monitorFromWindow(parentWidget()->parentWidget()->winId(), GdiSettings::MON_DEFAULTTONEAREST))
		excl = false;

	if (FAILED(d3d->GetAdapterDisplayMode(adapterIndex, &displayMode))) {
		excl = false;
		displayMode.Format = D3DFMT_UNKNOWN;
		std::cout << "d3d->GetAdapterDisplayMode failed" << std::endl;
	}

	presentParams->BackBufferWidth = excl ? displayMode.Width : width();
	presentParams->BackBufferHeight = excl ? displayMode.Height : height();
	presentParams->BackBufferFormat = displayMode.Format;
	presentParams->BackBufferCount = triplebuf ? 2 : 1;
	presentParams->MultiSampleType = D3DMULTISAMPLE_NONE;
	presentParams->MultiSampleQuality = 0;
	presentParams->SwapEffect = excl ? D3DSWAPEFFECT_FLIP : D3DSWAPEFFECT_DISCARD;
	presentParams->hDeviceWindow = excl ? parentWidget()->parentWidget()->winId() : winId();
	presentParams->Windowed = excl ? FALSE : TRUE;
	presentParams->EnableAutoDepthStencil = FALSE;
	presentParams->AutoDepthStencilFormat = D3DFMT_UNKNOWN;
	presentParams->Flags = 0;
	presentParams->FullScreen_RefreshRateInHz = excl ? displayMode.RefreshRate : 0;
	presentParams->PresentationInterval = swapInterval == 2 ? D3DPRESENT_INTERVAL_TWO :
			(swapInterval == 1 || vblankblit ? D3DPRESENT_INTERVAL_ONE : D3DPRESENT_INTERVAL_IMMEDIATE);
}

void Direct3DBlitter::lockTexture() {
	const RECT rect = { left: 0, top: 0, right: inWidth, bottom: inHeight };
	D3DLOCKED_RECT lockedrect;
	lockedrect.pBits = NULL;

	if (texture)
		texture->LockRect(0, &lockedrect, &rect, D3DLOCK_NOSYSLOCK);

	setPixelBuffer(lockedrect.pBits, MediaSource::RGB32, lockedrect.Pitch >> 2);
}

void Direct3DBlitter::setVertexBuffer() {
	if (!vertexBuffer)
		return;

	Vertex *vertices = NULL;

	if (!FAILED(vertexBuffer->Lock(0, 0, (VOID**)&vertices, 0))) {
		const unsigned xoffset = backBufferWidth > static_cast<unsigned>(width()) ? backBufferWidth - width() >> 1 : 0;
		const unsigned yoffset = backBufferHeight > static_cast<unsigned>(height()) ? backBufferHeight - height() >> 1 : 0;

		vertices[0].x = xoffset;
		vertices[0].y = yoffset + height();
		vertices[0].z = 0.0f;
		vertices[0].rhw = 1.0f;
		vertices[0].u = 0.0f;
		vertices[0].v = static_cast<float>(inHeight) / textRes;

		vertices[1].x = xoffset;
		vertices[1].y = yoffset;
		vertices[1].z = 0.0f;
		vertices[1].rhw = 1.0f;
		vertices[1].u = 0.0f;
		vertices[1].v = 0.0f;

		vertices[2].x = xoffset + width();
		vertices[2].y = yoffset + height();
		vertices[2].z = 0.0f;
		vertices[2].rhw = 1.0f;
		vertices[2].u = static_cast<float>(inWidth) / textRes;
		vertices[2].v = static_cast<float>(inHeight) / textRes;

		vertices[3].x = xoffset + width();
		vertices[3].y = yoffset;
		vertices[3].z = 0.0f;
		vertices[3].rhw = 1.0f;
		vertices[3].u = static_cast<float>(inWidth) / textRes;
		vertices[3].v = 0.0f;

		vertexBuffer->Unlock();
	} else
		std::cout << "vertexBuffer->Lock failed" << std::endl;
}

void Direct3DBlitter::setFilter() {
	if (device) {
		device->SetSamplerState(0, D3DSAMP_MINFILTER, bf ? D3DTEXF_LINEAR : D3DTEXF_POINT);
		device->SetSamplerState(0, D3DSAMP_MAGFILTER, bf ? D3DTEXF_LINEAR : D3DTEXF_POINT);
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
		device->SetTexture(0, NULL);

		D3DPRESENT_PARAMETERS presentParams;

		getPresentParams(&presentParams);

		if (FAILED(device->Reset(&presentParams)) && FAILED(device->Reset(&presentParams))) {
			if (texture) {
				setPixelBuffer(NULL, MediaSource::RGB32, 0);
				texture->Release();
				texture = NULL;
			}

			if (vertexBuffer) {
				vertexBuffer->Release();
				vertexBuffer = NULL;
			}

			device->Release();
			device = NULL;

			std::cout << "device->Reset failed" << std::endl;
		} else {
			backBufferWidth = presentParams.BackBufferWidth;
			backBufferHeight = presentParams.BackBufferHeight;
			windowed = presentParams.Windowed;
			clear = presentParams.BackBufferCount + 1;
			setDeviceState();
			device->SetTexture(0, texture);
			setVertexBuffer();
		}
	}

	drawn = false;
}

void Direct3DBlitter::exclusiveChange() {
	if (isVisible()) {
		if (exclusive & flipping) {
			if (windowed)
				resetDevice();
		} else if (!windowed) {
			ModeLock mlock(d3d->GetAdapterMonitor(adapterIndex));
			resetDevice();
			SetWindowPos(parentWidget()->parentWidget()->winId(), HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
		}
	}
}

void Direct3DBlitter::setSwapInterval(const unsigned hz1, const unsigned hz2) {
	const unsigned newSwapInterval = vblank ? hz == hz1 * 2 || hz == hz2 ? 2 : (hz == hz1 ? 1 : 0) : 0;

	if (newSwapInterval != swapInterval) {
		swapInterval = newSwapInterval;
		resetDevice();
		ftEst.init(hz ? swapInterval * 1000000 / hz : 0);
	}
}

void Direct3DBlitter::setSwapInterval() {
	setSwapInterval((1000000 + (frameTime() >> 1)) / frameTime(), (1000000 * 2 + (frameTime() >> 1)) / frameTime());
}

void Direct3DBlitter::draw() {
	if (device) {
		if (clear && !drawn) {
			--clear;
			device->Clear(0, NULL, D3DCLEAR_TARGET, D3DCOLOR_ARGB(0xFF, 0, 0, 0), 1.0, 0);
		}

		if (SUCCEEDED(device->BeginScene())) {
			device->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
			device->EndScene();
		}
	}
}

void Direct3DBlitter::present() {
	if (device && device->TestCooperativeLevel() == D3DERR_DEVICENOTRESET)
		resetDevice();

	if (device) {
		IDirect3DSwapChain9 *swapChain = NULL;

		device->GetSwapChain(0, &swapChain);

		if (swapChain) {
			swapChain->Present(NULL, NULL, 0, NULL, vblankblit && !swapInterval ? 1 : 0);
			swapChain->Release();
		} else
			device->Present(NULL, NULL, 0, NULL);
	}

	drawn = false;
}

void Direct3DBlitter::init() {
	{
		D3DPRESENT_PARAMETERS presentParams;

		getPresentParams(&presentParams);

		for (unsigned n = 2; n--;) {
			if (!FAILED(d3d->CreateDevice(adapterIndex, D3DDEVTYPE_HAL,
					parentWidget()->parentWidget()->winId(), D3DCREATE_SOFTWARE_VERTEXPROCESSING, &presentParams, &device))) {
				break;
			}
		}

		backBufferWidth = presentParams.BackBufferWidth;
		backBufferHeight = presentParams.BackBufferHeight;
		windowed = presentParams.Windowed;
		clear = presentParams.BackBufferCount + 1;
	}

	if (!device) {
		std::cout << "d3d->CreateDevice failed" << std::endl;
	} else if (FAILED(device->CreateVertexBuffer(sizeof(Vertex) * 4, 0, D3DFVF_XYZRHW | D3DFVF_TEX1, D3DPOOL_MANAGED, &vertexBuffer, NULL))) {
		std::cout << "device->CreateVertexBuffer failed" << std::endl;
	}

	setDeviceState();

	drawn = false;
}

void Direct3DBlitter::uninit() {
	if (device) {
		device->SetStreamSource(0, NULL, 0, 0);
		device->SetTexture(0, NULL);
	}

	if (texture) {
		texture->UnlockRect(0);
		texture->Release();
		texture = NULL;
	}

	if (vertexBuffer) {
		vertexBuffer->Release();
		vertexBuffer = NULL;
	}

	if (device) {
		device->Release();
		device = NULL;
	}
}

void Direct3DBlitter::setBufferDimensions(unsigned w, unsigned h) {
	inWidth = w;
	inHeight = h;

	if (device) {
		device->SetTexture(0, NULL);

		if (texture) {
			texture->UnlockRect(0);
			texture->Release();
			texture = NULL;
		}

		textRes = std::max(w, h);

		--textRes;
		textRes |= textRes >> 1;
		textRes |= textRes >> 2;
		textRes |= textRes >> 4;
		textRes |= textRes >> 8;
		++textRes;

		if (FAILED(device->CreateTexture(textRes, textRes, 1, 0, D3DFMT_X8R8G8B8, D3DPOOL_MANAGED, &texture, NULL)))
			std::cout << "device->CreateTexture failed" << std::endl;

		device->SetTexture(0, texture);
	}

	lockTexture();
	setVertexBuffer();
}

void Direct3DBlitter::blit() {
	if (texture) {
		texture->UnlockRect(0);

		draw();
		drawn = true;

		lockTexture();
	}
}

long Direct3DBlitter::sync(const long ft) {
	if (!drawn)
		draw();

	if (!swapInterval)
		BlitterWidget::sync(ft);

	present();

	if (swapInterval)
		ftEst.update(getusecs());

	if (!device)
		return -1;

	return 0;
}

void Direct3DBlitter::setFrameTime(const long ft) {
	BlitterWidget::setFrameTime(ft);

	const unsigned hz1 = (1000000 + (ft >> 1)) / ft;
	const unsigned hz2 = (1000000 * 2 + (ft >> 1)) / ft;

	{
		QString text("Sync to vertical blank in " + QString::number(hz1));

		if (hz2 != hz1 * 2)
			text += ", " + QString::number(hz2);

		vblankBox->setText(text + " and " + QString::number(hz1 * 2) + " Hz modes");
	}

	setSwapInterval(hz1, hz2);
}

const BlitterWidget::Estimate Direct3DBlitter::frameTimeEst() const {
	if (swapInterval) {
		const Estimate est = { ftEst.est(), ftEst.var() };
		return est;
	}

	return BlitterWidget::frameTimeEst();
}

/*const BlitterWidget::Rational Direct3DBlitter::frameTime() const {
	if (swapInterval)
		return Rational(swapInterval, hz);

	return BlitterWidget::frameTime();
}*/

void Direct3DBlitter::setExclusive(const bool exclusive) {
	if (exclusive != this->exclusive) {
		this->exclusive = exclusive;
		exclusiveChange();
	}
}

void Direct3DBlitter::acceptSettings() {
	if (static_cast<int>(adapterIndex) != adapterSelector->currentIndex()) {
		adapterIndex = adapterSelector->currentIndex();

		if (isVisible()) {
			uninit();
			setPixelBuffer(NULL, MediaSource::RGB32, 0);
			init();
			setBufferDimensions(inWidth, inHeight);
		}
	}

	if (vblank != vblankBox->isChecked()) {
		vblank = vblankBox->isChecked();
		setSwapInterval();
	}

	if (flipping != flippingBox->isChecked()) {
		flipping = flippingBox->isChecked();
		exclusiveChange();
	}

	vblankblit = vblankblitBox->isChecked();
	triplebuf = triplebufBox->isChecked();
	bf = bfBox->isChecked();
	resetDevice();
}

void Direct3DBlitter::rejectSettings() {
	adapterSelector->setCurrentIndex(adapterIndex);
	vblankBox->setChecked(vblank);
	flippingBox->setChecked(flipping);
	vblankblitBox->setChecked(vblankblit);
	triplebufBox->setChecked(triplebuf);
	bfBox->setChecked(bf);
}

void Direct3DBlitter::rateChange(int hz) {
	this->hz = hz;
	setSwapInterval();
}

void Direct3DBlitter::resizeEvent(QResizeEvent */*e*/) {
	resetDevice();
}

void Direct3DBlitter::paintEvent(QPaintEvent */*e*/) {
	if (!drawn)
		draw();

	present();
}
