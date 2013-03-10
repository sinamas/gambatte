/***************************************************************************
 *   Copyright (C) 2007 by Sindre Aamås                                    *
 *   sinamas@users.sourceforge.net                                         *
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

#ifdef PLATFORM_WIN32
#include <GL/glext.h>
#endif

#include <QCheckBox>
#include <QGLWidget>
#include <QSettings>
#include <QSizePolicy>
#include <QVBoxLayout>
#include <algorithm>
#include <cstring>
#include <vector>

enum { max_buffer_cnt = 3 };

class QGLBlitter::SubWidget : public QGLWidget {
public:
	SubWidget(QSize const &correctedSize, QSize const &textureSize,
	          unsigned swapInterval, int dhzRefreshRate, bool bf, QGLBlitter *parent);
	long frameTimeEst() const { return ftEst_.est(); }
	unsigned swapInterval() const { return swapInterval_; }
	void setBilinearFiltering(bool on);
	void setTextureSize(QSize const &size);
	void setCorrectedSize(QSize const &size) { correctedSize_ = size; }
	void setRefreshRate(int dhz) { ftEst_.init(swapInterval_ * 10000000 / dhz); }

	void forcedResize() {
		if (initialized_) {
			makeCurrent();
			resizeGL(width(), height());
		}
	}

	void prepare() {
		if (doubleBuffer())
			draw();
	}

	void present();
	void updateTexture(quint32 const *data);

protected:
	virtual void initializeGL();
	virtual void paintGL();
	virtual void resizeGL(int w, int h);

private:
	FtEst ftEst_;
	QSize correctedSize_;
	QSize inSize_;
	unsigned swapInterval_;
	unsigned clear_;
	bool initialized_;
	bool bf_;

	unsigned textureRes() const;
	void draw();
};

static QGLFormat const getQGLFormat(unsigned const swapInterval) {
	QGLFormat f;
#ifndef PLATFORM_UNIX
	f.setSwapInterval(swapInterval);
#else
	(void) swapInterval;
#endif

	return f;
}

QGLBlitter::SubWidget::SubWidget(QSize const &correctedSize, QSize const &textureSize,
                                 unsigned swapInterval, int dhzRefreshRate, bool bf,
                                 QGLBlitter *parent)
: QGLWidget(getQGLFormat(swapInterval), parent)
, ftEst_(swapInterval * 10000000 / dhzRefreshRate)
, correctedSize_(correctedSize)
, inSize_(textureSize)
, swapInterval_(swapInterval)
, clear_(max_buffer_cnt)
, initialized_(false)
, bf_(bf)
{
	setAutoBufferSwap(false);
	setMouseTracking(true);
}

static unsigned ceiledPow2(unsigned v) {
	--v;
	v |= v >> 1;
	v |= v >> 2;
	v |= v >> 4;
	v |= v >> 8;
	++v;

	return v;
}

unsigned QGLBlitter::SubWidget::textureRes() const {
	return ceiledPow2(std::max(inSize_.width(), inSize_.height()));
}

void QGLBlitter::SubWidget::draw() {
	if (clear_) {
		--clear_;
		glClear(GL_COLOR_BUFFER_BIT);
	}

	glCallList(1);
	glFlush();
}

void QGLBlitter::SubWidget::initializeGL() {
	glEnable(GL_CULL_FACE);
	glShadeModel(GL_FLAT);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_DITHER);

#ifdef PLATFORM_UNIX
	if (swapInterval_ && context()) {
		static int (*const glXSwapIntervalSGI)(int) =
			reinterpret_cast<int (*)(int)>(context()->getProcAddress("glXSwapIntervalSGI"));
		if (!glXSwapIntervalSGI || glXSwapIntervalSGI(swapInterval_) != 0)
			swapInterval_ = 0;
	}
#endif

	glLoadIdentity();
	glOrtho(0, 1, 1, 0, -1, 1);

	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, bf_ ? GL_LINEAR : GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, bf_ ? GL_LINEAR : GL_NEAREST);

	{
		// 0-initialize texture to avoid filter border garbage
		std::vector<quint32> const nulltexture(std::size_t(textureRes()) * textureRes());
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textureRes(), textureRes(), 0, GL_BGRA,
		             GL_UNSIGNED_INT_8_8_8_8_REV, &nulltexture[0]);
	}

	resizeGL(width(), height());
	initialized_ = true;
}

void QGLBlitter::SubWidget::paintGL() {
	clear_ = max_buffer_cnt;

	if (static_cast<QGLBlitter const *>(parentWidget())->isPaused()) {
		draw();
		swapBuffers();
	}
}

void QGLBlitter::SubWidget::resizeGL(int const w, int const h) {
	clear_ = max_buffer_cnt;
	glViewport(0, 0, w, h);

	int const itop  = (h - correctedSize_.height()) >> 1;
	int const ileft = (w - correctedSize_.width()) >> 1;
	double const top   = double(itop) / h;
	double const left  = double(ileft) / w;
	double const bot   = double(itop + correctedSize_.height()) / h;
	double const right = double(ileft + correctedSize_.width()) / w;
	double const ttop   = 1.0 - double(inSize_.height()) / textureRes();
	double const tright = double(inSize_.width()) / textureRes();

	glNewList(1, GL_COMPILE);
	glBegin(GL_QUADS);
	glTexCoord2f(   0.0, ttop); glVertex2f( left, top);
	glTexCoord2f(   0.0,  1.0); glVertex2f( left, bot);
	glTexCoord2f(tright,  1.0); glVertex2f(right, bot);
	glTexCoord2f(tright, ttop); glVertex2f(right, top);
	glEnd();
	glEndList();
}

void QGLBlitter::SubWidget::setBilinearFiltering(bool const on) {
	bool const oldbf = bf_;
	bf_ = on;

	if (bf_ != oldbf && initialized_) {
		makeCurrent();
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, bf_ ? GL_LINEAR : GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, bf_ ? GL_LINEAR : GL_NEAREST);
	}
}

void QGLBlitter::SubWidget::setTextureSize(QSize const &size) {
	inSize_ = size;
	if (initialized_)
		glInit();
}

void QGLBlitter::SubWidget::present() {
	swapBuffers();

	if (swapInterval_)
		ftEst_.update(getusecs());

	if (!doubleBuffer())
		draw();
}

void QGLBlitter::SubWidget::updateTexture(quint32 const *data) {
	if (!initialized_) {
		glInit();
	} else if (QGLContext::currentContext() != context())
		makeCurrent();

	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, textureRes() - inSize_.height(), inSize_.width(),
	                inSize_.height(), GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, data);
}

QGLBlitter::QGLBlitter(VideoBufferLocker vbl, DwmControlHwndChange hwndChange, QWidget *parent)
: BlitterWidget(vbl, QString("OpenGL"), 2, parent)
, hwndChange_(hwndChange)
, confWidget_(new QWidget)
, vsync_(new QCheckBox(QString("Wait for vertical blank")), "qglblitter/vsync", false)
, bf_(new QCheckBox(QString("Bilinear filtering")), "qglblitter/bf", true)
, correctedSize_(size())
, swapInterval_(0)
, dhz_(600)
{
	confWidget_->setLayout(new QVBoxLayout);
	confWidget_->layout()->setMargin(0);
	confWidget_->layout()->addWidget(vsync_.checkBox());
	confWidget_->layout()->addWidget(bf_.checkBox());
}

QGLBlitter::~QGLBlitter() {
}

void QGLBlitter::resizeEvent(QResizeEvent *) {
	if (subWidget_)
		subWidget_->setGeometry(rect());
}

void QGLBlitter::uninit() {
	subWidget_.reset();
	buffer_.reset();
}

bool QGLBlitter::isUnusable() const {
	return !QGLFormat::hasOpenGL();
}

void QGLBlitter::setBufferDimensions(unsigned const width, unsigned const height) {
	buffer_.reset(std::size_t(width) * height * 2);
	setPixelBuffer(buffer_, PixelBuffer::RGB32, width);

	if (subWidget_) {
		subWidget_->setTextureSize(QSize(width, height));
	} else
		createNewSubWidget(calcSubWidgetSwapInterval());

}

void QGLBlitter::blit() {
	setPixelBuffer(inBuffer().data == buffer_ ? buffer_ + buffer_.size() / 2 : buffer_,
	               inBuffer().pixelFormat, inBuffer().pitch);
}

void QGLBlitter::draw() {
	subWidget_->updateTexture(inBuffer().data == buffer_
	                        ? buffer_ + buffer_.size() / 2
	                        : buffer_);
	subWidget_->prepare();
}

void QGLBlitter::setCorrectedGeometry(int w, int h, int correctedw, int correctedh) {
	QRect const geo(0, 0, w, h);
	correctedSize_ = QSize(correctedw, correctedh);
	if (subWidget_)
		subWidget_->setCorrectedSize(correctedSize_);

	if (geometry() != geo) {
		setGeometry(geo);
	} else if (subWidget_)
		subWidget_->forcedResize();
}

long QGLBlitter::frameTimeEst() const {
	if (subWidget_ && subWidget_->swapInterval() && swapInterval_)
		return subWidget_->frameTimeEst();

	return BlitterWidget::frameTimeEst();
}

long QGLBlitter::sync() {
	subWidget_->present();
	return 0;
}

unsigned QGLBlitter::calcSubWidgetSwapInterval() const {
	return swapInterval_
	     ? swapInterval_
	     : vsync_.value() && !DwmControl::isCompositingEnabled();
}

void QGLBlitter::createNewSubWidget(unsigned const swapInterval) {
	subWidget_.reset();
	subWidget_.reset(new SubWidget(correctedSize_,
	                               QSize(inBuffer().width, inBuffer().height),
	                               swapInterval, dhz_, bf_.value(), this));
	subWidget_->setGeometry(rect());
	subWidget_->show();
	hwndChange_(this);
}

void QGLBlitter::updateSubWidgetSwapInterval() {
	unsigned swapInterval = calcSubWidgetSwapInterval();
	if (subWidget_ && subWidget_->swapInterval() != swapInterval)
		createNewSubWidget(swapInterval);
}

void QGLBlitter::acceptSettings() {
	bf_.accept();
	vsync_.accept();

	if (subWidget_) {
		updateSubWidgetSwapInterval();
		subWidget_->setBilinearFiltering(bf_.value());
	}
}

void QGLBlitter::rejectSettings() const {
	vsync_.reject();
	bf_.reject();
}

void QGLBlitter::setSwapInterval(unsigned si) {
	swapInterval_ = si;
	updateSubWidgetSwapInterval();
}

void QGLBlitter::compositionEnabledChange() {
	updateSubWidgetSwapInterval();
}

void QGLBlitter::rateChange(int const dhz) {
	dhz_ = dhz ? dhz : 600;

	if (subWidget_)
		subWidget_->setRefreshRate(dhz_);
}

WId QGLBlitter::hwnd() const {
	if (subWidget_)
		return subWidget_->winId();

	return BlitterWidget::hwnd();
}
