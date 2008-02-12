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

#include "videodialog.h"

class QGLBlitter::SubWidget : public QGLWidget {
public:
	int corrected_w;
	int corrected_h;
	
private:
	unsigned textureRes;
	unsigned inWidth;
	unsigned inHeight;
	unsigned swapInterval;
	bool initialized;
	bool bf;
	
protected:
	void initializeGL();
	void paintGL();
	void resizeGL(int w, int h);
	
public:
	SubWidget(unsigned swapInterval, bool bf, QWidget *parent = 0);
	~SubWidget();
	
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
	
	void setBilinearFiltering(bool on);
	void setBufferDimensions(unsigned w, unsigned h);
	
	void forcedResize() {
		if (initialized) {
			makeCurrent();
			resizeGL(width(), height());
		}
	}
	
	void updateTexture(quint32 *buffer);
	
	void uninit();
};

static const QGLFormat getQGLFormat(const unsigned swapInterval) {
	QGLFormat f;
	
#ifndef PLATFORM_UNIX
	f.setSwapInterval(swapInterval);
#endif
	
	return f;
}

QGLBlitter::SubWidget::SubWidget(const unsigned swapInterval_in, const bool bf_in, QWidget *parent) :
	QGLWidget(getQGLFormat(swapInterval_in), parent),
	corrected_w(160),
	corrected_h(144),
	inWidth(160),
	inHeight(144),
	swapInterval(swapInterval_in),
	initialized(false),
	bf(bf_in)
{
	setAutoBufferSwap(false);
// 	setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
	
// 	QSettings settings;
// 	settings.beginGroup("qglsubwidget");
// 	bf = settings.value("bf", true).toBool();
// 	settings.endGroup();
}

QGLBlitter::SubWidget::~SubWidget() {
	uninit();
	
// 	QSettings settings;
// 	settings.beginGroup("qglsubwidget");
// 	settings.setValue("bf", bf);
// 	settings.endGroup();
}

void QGLBlitter::SubWidget::blit() {
	glCallList(1);
	glFlush();
}

void QGLBlitter::SubWidget::initializeGL() {
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
	glTexCoord2f(0.0, 0.0); glVertex2f(0.0, 0.0);
	glTexCoord2f(0.0, 1.0); glVertex2f(0.0, 1.0);
	glTexCoord2f(1.0, 1.0); glVertex2f(1.0, 1.0);
	glTexCoord2f(1.0, 0.0); glVertex2f(1.0, 0.0);
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

void QGLBlitter::SubWidget::blitFront() {
	GLint drawBuffer;
	glGetIntegerv(GL_DRAW_BUFFER, &drawBuffer);
	glDrawBuffer(GL_FRONT);
	blit();
	glDrawBuffer(drawBuffer);
}

void QGLBlitter::SubWidget::paintGL() {
	if (swapInterval)
		blitFront();
	else {
		blit();
		swapBuffers();
	}
}

void QGLBlitter::SubWidget::resizeGL(const int w, const int h) {
	glViewport(0, 0, w, h);
	
	if (corrected_w != w || corrected_h != h) {
		{
			GLint drawBuffer;
			glGetIntegerv(GL_DRAW_BUFFER, &drawBuffer);
			glDrawBuffer(GL_FRONT_AND_BACK);
			glClear(GL_COLOR_BUFFER_BIT);
			glDrawBuffer(drawBuffer);
		}
		
		glViewport(w - corrected_w >> 1, h - corrected_h >> 1, corrected_w, corrected_h);
	}
}

void QGLBlitter::SubWidget::setBilinearFiltering(const bool on) {
	const bool oldbf = bf;
	bf = on;
	
	if (bf != oldbf && initialized) {
		makeCurrent();
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, bf ? GL_LINEAR : GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, bf ? GL_LINEAR : GL_NEAREST);
	}
}

void QGLBlitter::SubWidget::setBufferDimensions(const unsigned int width, const unsigned int height) {
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

void QGLBlitter::SubWidget::uninit() {
	initialized = false;
}

void QGLBlitter::SubWidget::updateTexture(quint32 *const buffer) {
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, textureRes - inHeight, inWidth, inHeight, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, buffer);
}

QGLBlitter::QGLBlitter(PixelBufferSetter setPixelBuffer, QWidget *parent) :
	BlitterWidget(setPixelBuffer, QString("OpenGL"), false, parent),
	confWidget(new QWidget),
	vsyncBox(new QCheckBox(QString("Sync to vertical blank in 60, 119 and 120 Hz modes"))),
	bfBox(new QCheckBox(QString("Bilinear filtering"))),
	buffer(NULL),
	hz(0),
	hz1(60),
	hz2(119)
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
	
	subWidget = new SubWidget(0, bf, this);
}

QGLBlitter::~QGLBlitter() {
	uninit();
	
// 	delete subWidget;
	
	QSettings settings;
	settings.beginGroup("qglblitter");
	settings.setValue("vsync", vsync);
	settings.setValue("bf", bf);
	settings.endGroup();
}

void QGLBlitter::resizeEvent(QResizeEvent *const event) {
	subWidget->setGeometry(QRect(QPoint(0, 0), event->size()));
}

void QGLBlitter::uninit() {
	subWidget->uninit();
	
	delete []buffer;
	buffer = NULL;
}

bool QGLBlitter::isUnusable() const {
	return !subWidget->isValid();
}

void QGLBlitter::setBufferDimensions(const unsigned int width, const unsigned int height) {
	delete []buffer;
	buffer = new quint32[width * height];
	
	subWidget->setBufferDimensions(width, height);
	setPixelBuffer(buffer, MediaSource::RGB32, width);
}

void QGLBlitter::blit() {
	if (buffer) {
		subWidget->makeCurrent();
		subWidget->updateTexture(buffer);
	}
}

void QGLBlitter::setCorrectedGeometry(int w, int h, int new_w, int new_h) {
	subWidget->corrected_w = new_w;
	subWidget->corrected_h = new_h;
	
	const QRect geo(0, 0, w, h);
	
	if (geometry() != geo)
		setGeometry(geo);
	else
		subWidget->forcedResize();
}

void QGLBlitter::setFrameTime(Rational ft) {
	BlitterWidget::setFrameTime(ft);
	
	hz1 = (ft.denominator + (ft.numerator >> 1)) / ft.numerator;
	hz2 = (ft.denominator * 2 + (ft.numerator >> 1)) / ft.numerator;
	
	QString text("Sync to vertical blank in ");
	text += QString::number(hz1);
	
	if (hz2 != hz1 * 2) {
		text += ", ";
		text += QString::number(hz2);
	}
	
	text += " and ";
	text += QString::number(hz1 * 2);
	text += " Hz modes";
	
	vsyncBox->setText(text);
	resetSubWidget();
}

const BlitterWidget::Rational QGLBlitter::frameTime() const {
	if (subWidget->getSwapInterval()) {
		return Rational(subWidget->getSwapInterval(), hz);
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
		if (hz == hz1)
			swapInterval = 1;
		else if (hz == hz2 || hz == hz1 * 2)
			swapInterval = 2;
	}
	
	if (swapInterval == subWidget->getSwapInterval())
		return;
	
	const unsigned corrected_w = subWidget->corrected_w;
	const unsigned corrected_h = subWidget->corrected_h;
	const unsigned w = subWidget->getInWidth();
	const unsigned h = subWidget->getInHeight();
	
// 	subWidget->hide();
	delete subWidget;
	subWidget = new SubWidget(swapInterval, bf, this);
	subWidget->corrected_w = corrected_w;
	subWidget->corrected_h = corrected_h;
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
