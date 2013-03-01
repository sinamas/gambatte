/***************************************************************************
 *   Copyright (C) 2007 by Sindre Aam�s                                    *
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

#ifdef PLATFORM_UNIX
#include "../x11getprocaddress.h"
#endif

#ifdef PLATFORM_WIN32
#include <GL/glext.h>
#endif

#include <QCheckBox>
#include <QGLWidget>
#include <QResizeEvent>
#include <QSettings>
#include <QSizePolicy>
#include <QVBoxLayout>
#include <algorithm>
#include <cstring>
#include <vector>

enum { max_buffer_cnt = 3 };

class QGLBlitter::SubWidget : public QGLWidget {
public:
	SubWidget(unsigned swapInterval, bool bf, QGLBlitter *parent);
	void blit();
	QSize const correctedSize() const { return correctedSize_; }
	unsigned swapInterval() const { return swapInterval_; }
	void setBilinearFiltering(bool on);
	void setTextureSize(QSize const &size);
	void setCorrectedSize(QSize const &size) { correctedSize_ = size; }

	void forcedResize() {
		if (initialized_)
			resizeGL(width(), height());
	}

	void sync(FtEst &ftEst);
	void updateTexture(quint32 const *data);
	void uninit();

protected:
	virtual void initializeGL();
	virtual void paintGL();
	virtual void resizeGL(int w, int h);

private:
	QSize correctedSize_;
	QSize inSize_;
	unsigned swapInterval_;
	unsigned clear_;
	bool initialized_;
	bool bf_;
	bool blitted_;

	unsigned textureRes() const;
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

QGLBlitter::SubWidget::SubWidget(unsigned const swapInterval, bool const bf, QGLBlitter *parent)
: QGLWidget(getQGLFormat(swapInterval), parent)
, correctedSize_(160, 144)
, inSize_(160, 144)
, swapInterval_(swapInterval)
, clear_(max_buffer_cnt)
, initialized_(false)
, bf_(bf)
, blitted_(false)
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

void QGLBlitter::SubWidget::blit() {
	if (clear_) {
		--clear_;
		glClear(GL_COLOR_BUFFER_BIT);
	}

	glCallList(1);
	glFlush();
	blitted_ = true;
}

void QGLBlitter::SubWidget::initializeGL() {
	glEnable(GL_CULL_FACE);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, bf_ ? GL_LINEAR : GL_NEAREST);
	glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, bf_ ? GL_LINEAR : GL_NEAREST);
	glShadeModel(GL_FLAT);
	glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	glEnable(GL_TEXTURE_2D);
	glDisable(GL_DITHER);

#ifdef PLATFORM_UNIX
	if (swapInterval_) {
		static void (*const glXSwapIntervalSGI)(int) =
			reinterpret_cast<void (*)(int)>(x11GetProcAddress("glXSwapIntervalSGI"));
		if (glXSwapIntervalSGI) {
			glXSwapIntervalSGI(swapInterval_);
		} else
			swapInterval_ = 0;
	}
#endif

	initialized_ = true;
}

void QGLBlitter::SubWidget::paintGL() {
	clear_ = max_buffer_cnt;

	if (static_cast<QGLBlitter const *>(parentWidget())->isPaused()) {
		if (!blitted_)
			blit();

		swapBuffers();
		blitted_ = false;
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
	glTexCoord2f(0.0, ttop); glVertex2f(left, top);
	glTexCoord2f(0.0, 1.0); glVertex2f(left, bot);
	glTexCoord2f(tright, 1.0); glVertex2f(right, bot);
	glTexCoord2f(tright, ttop); glVertex2f(right, top);
	glEnd();
	glEndList();
}

void QGLBlitter::SubWidget::setBilinearFiltering(bool const on) {
	bool const oldbf = bf_;
	bf_ = on;

	if (bf_ != oldbf && initialized_) {
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, bf_ ? GL_LINEAR : GL_NEAREST);
		glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, bf_ ? GL_LINEAR : GL_NEAREST);
	}
}

void QGLBlitter::SubWidget::setTextureSize(QSize const &size) {
	inSize_ = size;

	if (!initialized_)
		glInit();

	glLoadIdentity();

	{
		// 0-initialize texture to avoid filter border garbage
		std::vector<quint32> const nulltexture(std::size_t(textureRes()) * textureRes());
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, textureRes(), textureRes(), 0, GL_BGRA,
		             GL_UNSIGNED_INT_8_8_8_8_REV, &nulltexture[0]);
	}

	glOrtho(0, 1, 1, 0, -1, 1);
	resizeGL(width(), height());
}

void QGLBlitter::SubWidget::sync(FtEst &ftEst) {
	swapBuffers();

	if (swapInterval_)
		ftEst.update(getusecs());

	if (!blitted_)
		blit();

	blitted_ = false;
}

void QGLBlitter::SubWidget::uninit() {
	initialized_ = false;
}

void QGLBlitter::SubWidget::updateTexture(quint32 const *data) {
	glTexSubImage2D(GL_TEXTURE_2D, 0, 0, textureRes() - inSize_.height(), inSize_.width(),
	                inSize_.height(), GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, data);
}

QGLBlitter::QGLBlitter(VideoBufferLocker vbl, DwmControlHwndChange hwndChange, QWidget *parent)
: BlitterWidget(vbl, QString("OpenGL"), 2, parent)
, hwndChange_(hwndChange)
, confWidget_(new QWidget)
, vsync_(new QCheckBox(QString("Wait for vertical blank")), "qglblitter/vsync", false)
, bf_(new QCheckBox(QString("Bilinear filtering")), "qglblitter/bf", true)
, swapInterval_(0)
, dhz_(600)
, subWidget_(new SubWidget(0, bf_.value(), this))
{
	confWidget_->setLayout(new QVBoxLayout);
	confWidget_->layout()->setMargin(0);
	confWidget_->layout()->addWidget(vsync_.checkBox());
	confWidget_->layout()->addWidget(bf_.checkBox());
}

QGLBlitter::~QGLBlitter() {
}

void QGLBlitter::resizeEvent(QResizeEvent *const event) {
	subWidget_->setGeometry(QRect(QPoint(0, 0), event->size()));
}

void QGLBlitter::uninit() {
	subWidget_->uninit();
	buffer_.reset();
}

bool QGLBlitter::isUnusable() const {
	return !subWidget_->isValid();
}

void QGLBlitter::setBufferDimensions(unsigned const width, unsigned const height) {
	buffer_.reset(std::size_t(width) * height * 2);
	subWidget_->setTextureSize(QSize(width, height));
	setPixelBuffer(buffer_, PixelBuffer::RGB32, width);
}

void QGLBlitter::blit() {
	if (buffer_) {
		setPixelBuffer(inBuffer().data == buffer_ ? buffer_ + buffer_.size() / 2 : buffer_,
		               inBuffer().pixelFormat, inBuffer().pitch);
	}
}

void QGLBlitter::draw() {
	if (buffer_)
		subWidget_->updateTexture(inBuffer().data == buffer_ ? buffer_ + buffer_.size() / 2 : buffer_);

	if (subWidget_->doubleBuffer())
		subWidget_->blit();
}

void QGLBlitter::setCorrectedGeometry(int w, int h, int correctedw, int correctedh) {
	subWidget_->setCorrectedSize(QSize(correctedw, correctedh));

	QRect const geo(0, 0, w, h);
	if (geometry() != geo)
		setGeometry(geo);
	else
		subWidget_->forcedResize();
}

long QGLBlitter::frameTimeEst() const {
	if (subWidget_->swapInterval() && swapInterval_)
		return ftEst_.est();

	return BlitterWidget::frameTimeEst();
}

long QGLBlitter::sync() {
	subWidget_->sync(ftEst_);
	return 0;
}

void QGLBlitter::resetSubWidget() {
	unsigned const swapInterval = swapInterval_
		? swapInterval_
		: vsync_.value() && !DwmControl::isCompositingEnabled();

	if (swapInterval == subWidget_->swapInterval())
		return;

	QSize const correctedSize = subWidget_->correctedSize();
	subWidget_.reset();
	subWidget_.reset(new SubWidget(swapInterval, bf_.value(), this));
	subWidget_->setCorrectedSize(correctedSize);
	subWidget_->setGeometry(rect());
	subWidget_->show();
	ftEst_.init(swapInterval * 10000000 / dhz_);

	if (buffer_)
		subWidget_->setTextureSize(QSize(inBuffer().width, inBuffer().height));

	hwndChange_(this);
}

void QGLBlitter::acceptSettings() {
	bf_.accept();
	subWidget_->setBilinearFiltering(bf_.value());
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

void QGLBlitter::rateChange(int const dhz) {
	dhz_ = dhz ? dhz : 600;
	ftEst_.init(subWidget_->swapInterval() * 10000000 / dhz_);
}

WId QGLBlitter::hwnd() const {
	return subWidget_->winId();
}
