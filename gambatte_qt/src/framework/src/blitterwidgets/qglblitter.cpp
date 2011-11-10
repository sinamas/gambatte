/***************************************************************************
 *   Copyright (C) 2007 by Sindre Aam�s                                    *
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
#include <cstring>

#ifdef PLATFORM_WIN32
#include <GL/glext.h>
#endif

#ifdef PLATFORM_UNIX
#include "../x11getprocaddress.h"
#endif

class QGLBlitter::SubWidget : public QGLWidget {
public:
	int corrected_w;
	int corrected_h;

private:
	unsigned textureRes;
	unsigned inWidth;
	unsigned inHeight;
	unsigned swapInterval;
	unsigned clear;
	bool initialized;
	bool bf;

public:
	bool blitted;

protected:
	void initializeGL();
	void paintGL();
	void resizeGL(int w, int h);

public:
	SubWidget(unsigned swapInterval, bool bf, QGLBlitter *parent);

	void blit();
	//void blitFront();

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
// 			makeCurrent();
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

QGLBlitter::SubWidget::SubWidget(const unsigned swapInterval_in, const bool bf_in, QGLBlitter *parent) :
	QGLWidget(getQGLFormat(swapInterval_in), parent),
	corrected_w(160),
	corrected_h(144),
	textureRes(0x100),
	inWidth(160),
	inHeight(144),
	swapInterval(swapInterval_in),
	clear(2),
	initialized(false),
	bf(bf_in),
	blitted(false)
{
	setAutoBufferSwap(false);
	setMouseTracking(true);
// 	setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
}

void QGLBlitter::SubWidget::blit() {
	if (clear) {
		--clear;
		glClear(GL_COLOR_BUFFER_BIT);
	}

	glCallList(1);
	glFlush();
	blitted = true;
}

void QGLBlitter::SubWidget::initializeGL() {
// 	void *libHandle = dlopen("libgl.so", RTLD_LAZY);

// 	glXGetVideoSyncSGI_ptr = (glXGetVideoSyncSGI_Func) dlsym(libHandle, "glXGetVideoSyncSGI");
// 	glXWaitVideoSyncSGI_ptr = (glXWaitVideoSyncSGI_Func) dlsym(libHandle, "glXWaitVideoSyncSGI");

	glEnable(GL_CULL_FACE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, bf ? GL_LINEAR : GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, bf ? GL_LINEAR : GL_NEAREST);
	glShadeModel(GL_FLAT);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_DITHER);

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

/*void QGLBlitter::SubWidget::blitFront() {
	GLint drawBuffer = GL_BACK;
	glGetIntegerv(GL_DRAW_BUFFER, &drawBuffer);

	if (clear) {
		--clear;
		glClear(GL_COLOR_BUFFER_BIT);
	}

	glDrawBuffer(GL_FRONT);

	if (clear) {
		--clear;
		glClear(GL_COLOR_BUFFER_BIT);
	}

	glCallList(1);
	glDrawBuffer(drawBuffer);
	glFlush();
}*/

void QGLBlitter::SubWidget::paintGL() {
	clear = 2;

	if (reinterpret_cast<const QGLBlitter*>(parentWidget())->isPaused()) {
// 	if (swapInterval)
// 		blitFront();
// 	else {
		if (!blitted)
			blit();

		swapBuffers();
		blitted = false;
// 	}
	}
}

void QGLBlitter::SubWidget::resizeGL(const int w, const int h) {
	glViewport(0, 0, w, h);

	const unsigned itop = (h - corrected_h) >> 1;
	const unsigned ileft = (w - corrected_w) >> 1;

	const double top = static_cast<double>(itop) / h;
	const double left = static_cast<double>(ileft) / w;
	const double bot = static_cast<double>(itop + corrected_h) / h;
	const double right = static_cast<double>(ileft + corrected_w) / w;

	const double ttop = 1.0 - static_cast<double>(inHeight) / textureRes;
	const double tright = static_cast<double>(inWidth) / textureRes;

	glNewList(1, GL_COMPILE);
	glBegin(GL_QUADS);
	glTexCoord2f(0.0, ttop); glVertex2f(left, top);
	glTexCoord2f(0.0, 1.0); glVertex2f(left, bot);
	glTexCoord2f(tright, 1.0); glVertex2f(right, bot);
	glTexCoord2f(tright, ttop); glVertex2f(right, top);
	glEnd();
	glEndList();

	clear = 2;
}

void QGLBlitter::SubWidget::setBilinearFiltering(const bool on) {
	const bool oldbf = bf;
	bf = on;

	if (bf != oldbf && initialized) {
// 		makeCurrent();
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
// 	else
// 		makeCurrent();

	glLoadIdentity();

	{
		const Array<quint32> nulltexture(textureRes * textureRes); // avoids bilinear filter border garbage
		std::memset(nulltexture, 0, nulltexture.size() * sizeof(quint32));
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textureRes, textureRes, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, nulltexture);
	}

	glOrtho(0, 1, 1, 0, -1, 1);

	resizeGL(this->width(), this->height());
}

void QGLBlitter::SubWidget::uninit() {
	initialized = false;
}

void QGLBlitter::SubWidget::updateTexture(quint32 *const buffer) {
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, textureRes - inHeight, inWidth, inHeight, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, buffer);
}

QGLBlitter::QGLBlitter(VideoBufferLocker vbl, DwmControlHwndChange hwndChange, QWidget *parent) :
	BlitterWidget(vbl, QString("OpenGL"), 2, parent),
	hwndChange_(hwndChange),
	confWidget(new QWidget),
	vsync_(new QCheckBox(QString("Wait for vertical blank")), "qglblitter/vsync", false),
	bf_(new QCheckBox(QString("Bilinear filtering")), "qglblitter/bf", true),
	swapInterval_(0),
	dhz(600),
	subWidget(new SubWidget(0, bf_.value(), this))
{
// 	setLayout(new QVBoxLayout);
// 	layout()->setMargin(0);
// 	layout()->setSpacing(0);
// 	layout()->addWidget(subWidget);

	confWidget->setLayout(new QVBoxLayout);
	confWidget->layout()->setMargin(0);
	confWidget->layout()->addWidget(vsync_.checkBox());
	confWidget->layout()->addWidget(bf_.checkBox());
}

QGLBlitter::~QGLBlitter() {
}

void QGLBlitter::resizeEvent(QResizeEvent *const event) {
	subWidget->setGeometry(QRect(QPoint(0, 0), event->size()));
}

void QGLBlitter::uninit() {
	subWidget->uninit();
	buffer.reset();
}

bool QGLBlitter::isUnusable() const {
	return !subWidget->isValid();
}

void QGLBlitter::setBufferDimensions(const unsigned int width, const unsigned int height) {
	buffer.reset(width * height * 2);

	subWidget->setBufferDimensions(width, height);
	setPixelBuffer(buffer, PixelBuffer::RGB32, width);
}

void QGLBlitter::blit() {
	if (buffer)
		setPixelBuffer(inBuffer().data == buffer ? buffer + buffer.size() / 2 : buffer, inBuffer().pixelFormat, inBuffer().pitch);
}

void QGLBlitter::draw() {
	if (buffer)
		subWidget->updateTexture(inBuffer().data == buffer ? buffer + buffer.size() / 2 : buffer);
	
	if (subWidget->doubleBuffer())
		subWidget->blit();
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

/*void QGLBlitter::setFrameTime(const long ft) {
	BlitterWidget::setFrameTime(ft);

	hz1 = (1000000 + (ft >> 1)) / ft;
	hz2 = (1000000 * 2 + (ft >> 1)) / ft;

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
}*/

long QGLBlitter::frameTimeEst() const {
	if (subWidget->getSwapInterval() && swapInterval_) {
		return ftEst.est();
	}

	return BlitterWidget::frameTimeEst();
}

/*const BlitterWidget::Rational QGLBlitter::frameTime() const {
	if (subWidget->getSwapInterval()) {
		return Rational(subWidget->getSwapInterval(), hz);
	}

	return BlitterWidget::frameTime();
}*/

long QGLBlitter::sync() {
// 	subWidget->makeCurrent();
	subWidget->swapBuffers();

	if (subWidget->getSwapInterval())
		ftEst.update(getusecs());

	if (!subWidget->blitted)
		subWidget->blit();

	subWidget->blitted = false;

	return 0;
}

void QGLBlitter::resetSubWidget() {
// 	unsigned swapInterval = 0;
//
// 	if (vsync) {
// 		if (hz == hz1)
// 			swapInterval = 1;
// 		else if (hz == hz2 || hz == hz1 * 2)
// 			swapInterval = 2;
// 	}
	const unsigned swapInterval = swapInterval_ ? swapInterval_ : vsync_.value() && !DwmControl::isCompositingEnabled();

	if (swapInterval == subWidget->getSwapInterval())
		return;

	const unsigned corrected_w = subWidget->corrected_w;
	const unsigned corrected_h = subWidget->corrected_h;
	const unsigned w = subWidget->getInWidth();
	const unsigned h = subWidget->getInHeight();

// 	subWidget->hide();
	subWidget.reset();
	subWidget.reset(new SubWidget(swapInterval, bf_.value(), this));
	subWidget->corrected_w = corrected_w;
	subWidget->corrected_h = corrected_h;
	subWidget->setGeometry(0, 0, width(), height());
	subWidget->show();
	ftEst.init(swapInterval * 10000000 / dhz);

	if (buffer)
		subWidget->setBufferDimensions(w, h);

	hwndChange_(this);
}

void QGLBlitter::acceptSettings() {
	bf_.accept();
	subWidget->setBilinearFiltering(bf_.value());
	vsync_.accept();
	resetSubWidget();
}

void QGLBlitter::rejectSettings() const {
	vsync_.reject();
	bf_.reject();
}

void QGLBlitter::setSwapInterval(unsigned si) {
	swapInterval_ = si;
	resetSubWidget();
}

void QGLBlitter::compositionEnabledChange() {
	resetSubWidget();
}

void QGLBlitter::rateChange(const int dhz) {
	this->dhz = dhz ? dhz : 600;
	ftEst.init(subWidget->getSwapInterval() * 10000000 / this->dhz);
}

WId QGLBlitter::hwnd() const {
	return subWidget->winId();
}
