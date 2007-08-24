/***************************************************************************
 *   Copyright (C) 2007 by Sindre Aamås                                    *
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
#include "qglblitter.h"

#include <QSizePolicy>
#include <QVBoxLayout>
#include <QCheckBox>
#include <QResizeEvent>
#include <QSettings>
#include <QGLWidget>

#include <algorithm>

#ifdef PLATFORM_WIN32
#include <GL/glext.h>
#endif

#ifdef PLATFORM_UNIX
#include "../x11getprocaddress.h"
#endif

class QGLSubWidget : public QGLWidget {
	unsigned textureRes;
	unsigned inWidth;
	unsigned inHeight;
	unsigned swapInterval;
	bool initialized;
	bool keepRatio;
	bool integerScaling;
	bool bf;
	
protected:
	void initializeGL();
	void paintGL();
	void resizeGL(int w, int h);
	
public:
	QGLSubWidget(unsigned swapInterval, bool bf, QWidget *parent = 0);
	~QGLSubWidget();
	
	void blit();
	void blitFront();
	
	unsigned getInWidth() const {
		return inWidth;
	}
	
	unsigned getInHeight() const {
		return inHeight;
	}
	
	unsigned getSwapInterval() const {
		return swapInterval;
	}
	
	bool keepsRatio() {
		return keepRatio;
	}
	
	bool scalesByInteger() {
		return integerScaling;
	}
	
	void setBilinearFiltering(bool on);
	void setBufferDimensions(unsigned w, unsigned h);
	
	void setIntegerScaling(const bool on) {
		integerScaling = on;
		
		if (initialized) {
			makeCurrent();
			resizeGL(width(), height());
		}
	}
	
	void setKeepRatio(const bool on) {
		keepRatio = on;
		
		if (initialized) {
			makeCurrent();
			resizeGL(width(), height());
		}
	}
	
	void updateTexture(uint32_t *buffer);
	
	void uninit();
};

static const QGLFormat getQGLFormat(const unsigned swapInterval) {
	QGLFormat f;
	
#ifndef PLATFORM_UNIX
	f.setSwapInterval(swapInterval);
#endif
	
	return f;
}

QGLSubWidget::QGLSubWidget(const unsigned swapInterval_in, const bool bf_in, QWidget *parent) :
	QGLWidget(getQGLFormat(swapInterval_in), parent),
	inWidth(160),
	inHeight(144),
	swapInterval(swapInterval_in),
	initialized(false),
	keepRatio(true),
	integerScaling(false),
	bf(bf_in)
{
	setAutoBufferSwap(false);
// 	setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
	
// 	QSettings settings;
// 	settings.beginGroup("qglsubwidget");
// 	bf = settings.value("bf", true).toBool();
// 	settings.endGroup();
}

QGLSubWidget::~QGLSubWidget() {
	uninit();
	
// 	QSettings settings;
// 	settings.beginGroup("qglsubwidget");
// 	settings.setValue("bf", bf);
// 	settings.endGroup();
}

void QGLSubWidget::blit() {
	glCallList(1);
	glFlush();
}

void QGLSubWidget::initializeGL() {
// 	void *libHandle = dlopen("libgl.so", RTLD_LAZY);
	
// 	glXGetVideoSyncSGI_ptr = (glXGetVideoSyncSGI_Func) dlsym(libHandle, "glXGetVideoSyncSGI");
// 	glXWaitVideoSyncSGI_ptr = (glXWaitVideoSyncSGI_Func) dlsym(libHandle, "glXWaitVideoSyncSGI");
	
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, bf ? GL_LINEAR : GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, bf ? GL_LINEAR : GL_NEAREST);
	glShadeModel(GL_FLAT);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_DITHER);
	
	glClearColor(0.0, 0.0, 0.0, 0.0);
	
	glNewList(1, GL_COMPILE);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, -0.2); glVertex2f(0.0, -0.2);
	glTexCoord2f(0.0, 1.0); glVertex2f(0.0, 1.0);
	glTexCoord2f(1.2, 1.0); glVertex2f(1.2, 1.0);
	glTexCoord2f(1.2, -0.2); glVertex2f(1.2, -0.2);
	glEnd();
	glEndList();
	
#ifdef PLATFORM_UNIX
	if (swapInterval) {
		static void (*const glXSwapIntervalSGI)(int) = reinterpret_cast<void (*)(int)>(x11GetProcAddress("glXSwapIntervalSGI"));
		
		if (glXSwapIntervalSGI) {
			glXSwapIntervalSGI(swapInterval);
		} else
			swapInterval = 0;
	}
#endif
	
	initialized = true;
}

void QGLSubWidget::blitFront() {
	GLint drawBuffer;
	glGetIntegerv(GL_DRAW_BUFFER, &drawBuffer);
	glDrawBuffer(GL_FRONT);
	blit();
	glDrawBuffer(drawBuffer);
}

void QGLSubWidget::paintGL() {
	if (swapInterval)
		blitFront();
	else {
		blit();
		swapBuffers();
	}
}

void QGLSubWidget::resizeGL(const int w, const int h) {
	if (keepRatio) {
		{
			GLint drawBuffer;
			glGetIntegerv(GL_DRAW_BUFFER, &drawBuffer);
			glDrawBuffer(GL_FRONT_AND_BACK);
			glClear(GL_COLOR_BUFFER_BIT);
			glDrawBuffer(drawBuffer);
		}
		
		if (integerScaling) {
			const unsigned int scale_factor = std::min(w / inWidth, h / inHeight);
			
			const int new_w = inWidth * scale_factor;
			const int new_h = inHeight * scale_factor;
			
			glViewport(w - new_w >> 1, h - new_h >> 1, new_w, new_h);
		} else {
			if (w * 9 > h * 10) {
				const int new_w = (h * 20 + 9) / 18;
				glViewport(w - new_w >> 1, 0, new_w, h);
			} else {
				const int new_h = (w * 9 + 5) / 10;
				glViewport(0, h - new_h >> 1, w, new_h);
			}
		}
	} else {
		glViewport(0, 0, w, h);
	}
}

void QGLSubWidget::setBilinearFiltering(const bool on) {
	const bool oldbf = bf;
	bf = on;
	
	if (bf != oldbf && initialized) {
		makeCurrent();
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, bf ? GL_LINEAR : GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, bf ? GL_LINEAR : GL_NEAREST);
	}
}

void QGLSubWidget::setBufferDimensions(const unsigned int width, const unsigned int height) {
	inWidth = width;
	inHeight = height;
	
	textureRes = std::max(width, height);
	
	//Next power of 2
	--textureRes;
	textureRes |= textureRes >> 1;
	textureRes |= textureRes >> 2;
	textureRes |= textureRes >> 4;
	textureRes |= textureRes >> 8;
	textureRes |= textureRes >> 16;
	++textureRes;
	
	if (!initialized)
		glInit();
	else
		makeCurrent();
	
	glLoadIdentity();
	
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, textureRes, textureRes, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, 0);
	double dres = textureRes;
	glOrtho(0, width / dres, 1, 1.0 - height / dres, -1, 1);
}

void QGLSubWidget::uninit() {
	initialized = false;
}

void QGLSubWidget::updateTexture(uint32_t *const buffer) {
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, textureRes - inHeight, inWidth, inHeight, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, buffer);
}

QGLBlitter::QGLBlitter(QWidget *parent) :
	BlitterWidget(QString("OpenGL"), false, true, parent),
	confWidget(new QWidget),
	vsyncBox(new QCheckBox(QString("Sync to vertical blank in 60, 119 and 120 Hz modes"))),
	bfBox(new QCheckBox(QString("Bilinear filtering"))),
	buffer(NULL),
	hz(0)
{
// 	setLayout(new QVBoxLayout);
// 	layout()->setMargin(0);
// 	layout()->setSpacing(0);
// 	layout()->addWidget(subWidget);
	
	QSettings settings;
	settings.beginGroup("qglblitter");
	vsync = settings.value("vsync", false).toBool();
	bf = settings.value("bf", true).toBool();
	settings.endGroup();
	
	confWidget->setLayout(new QVBoxLayout);
	confWidget->layout()->setMargin(0);
	confWidget->layout()->addWidget(vsyncBox);
	confWidget->layout()->addWidget(bfBox);
	vsyncBox->setChecked(vsync);
	bfBox->setChecked(bf);
	
	subWidget = new QGLSubWidget(0, bf, this);
}

QGLBlitter::~QGLBlitter() {
	uninit();
	
// 	delete subWidget;
	delete confWidget;
	
	QSettings settings;
	settings.beginGroup("qglblitter");
	settings.setValue("vsync", vsync);
	settings.setValue("bf", bf);
	settings.endGroup();
}

void QGLBlitter::resizeEvent(QResizeEvent *const event) {
	subWidget->setGeometry(0, 0, event->size().width(), event->size().height());
}

void QGLBlitter::uninit() {
	subWidget->uninit();
	
	delete[] buffer;
	buffer = NULL;
}

bool QGLBlitter::isUnusable() {
	return !subWidget->isValid();
}

void QGLBlitter::keepAspectRatio(const bool enable) {
	subWidget->setKeepRatio(enable);
}

bool QGLBlitter::keepsAspectRatio() {
	return subWidget->keepsRatio();
}

void QGLBlitter::scaleByInteger(const bool enable) {
	subWidget->setIntegerScaling(enable);
}

bool QGLBlitter::scalesByInteger() {
	return subWidget->scalesByInteger();
}

void QGLBlitter::setBufferDimensions(const unsigned int width, const unsigned int height) {
	delete []buffer;
	buffer = new uint32_t[width * height];
	
	subWidget->setBufferDimensions(width, height);
}

const PixelBuffer QGLBlitter::inBuffer() {
	PixelBuffer pixb;
	pixb.format = PixelBuffer::RGB32;
	pixb.pixels = buffer;
	pixb.pitch = subWidget->getInWidth();
	
	return pixb;
}

void QGLBlitter::blit() {
	if (buffer) {
		subWidget->makeCurrent();
		subWidget->updateTexture(buffer);
	}
}

const BlitterWidget::Rational QGLBlitter::frameTime() const {
	if (subWidget->getSwapInterval()) {
		Rational r = { subWidget->getSwapInterval(), hz };
		return r;
	}
	
	return BlitterWidget::frameTime();
}

int QGLBlitter::sync(const bool turbo) {
	subWidget->makeCurrent();
	
	if (subWidget->getSwapInterval() && turbo)
		subWidget->blitFront();
	else {
		subWidget->blit();
		subWidget->swapBuffers();
	}
	
	if (subWidget->getSwapInterval())
		return 0;
	
	return BlitterWidget::sync(turbo);
}

void QGLBlitter::resetSubWidget() {
	unsigned swapInterval = 0;
	
	if (vsync) {
		if (hz == 60)
			swapInterval = 1;
		else if (hz == 119 || hz == 120)
			swapInterval = 2;
	}
	
	if (swapInterval == subWidget->getSwapInterval())
		return;
	
	const bool keepRatio = subWidget->keepsRatio();
	const bool integerScaling = subWidget->scalesByInteger();
	const unsigned w = subWidget->getInWidth();
	const unsigned h = subWidget->getInHeight();
	
// 	subWidget->hide();
	delete subWidget;
	subWidget = new QGLSubWidget(swapInterval, bf, this);
	subWidget->setKeepRatio(keepRatio);
	subWidget->setIntegerScaling(integerScaling);
	subWidget->setGeometry(0, 0, width(), height());
	subWidget->show();
	
	if (buffer)
		subWidget->setBufferDimensions(w, h);
}

void QGLBlitter::acceptSettings() {
	bf = bfBox->isChecked();
	subWidget->setBilinearFiltering(bf);
	
	vsync = vsyncBox->isChecked();
	resetSubWidget();
}

void QGLBlitter::rejectSettings() {
	vsyncBox->setChecked(vsync);
	bfBox->setChecked(bf);
}

void QGLBlitter::rateChange(const int hz) {
	this->hz = hz;
	resetSubWidget();
}
