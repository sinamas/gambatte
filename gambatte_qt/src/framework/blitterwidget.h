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
#ifndef BLITTERWIDGET_H
#define BLITTERWIDGET_H

#include "pixelbuffersetter.h"
#include <QWidget>
#include <QString>
#include <memory>
#include <usec.h>

class QHBoxLayout;

class FtEst {
	enum { UPSHIFT= 5 };
	enum { UP = 1 << UPSHIFT };

	long frameTime;
	long ft;
	long ftAvg;
	long ftVar;
	usec_t last;
	unsigned count;

public:
	FtEst(long frameTime = 0) { init(frameTime); }
	void init(long frameTime);
	void update(usec_t t);
	long est() const { return (ftAvg + UP / 2) >> UPSHIFT; }
	long var() const { return (ftVar + UP / 2) >> UPSHIFT; }
};

class BlitterWidget : public QWidget {
	Q_OBJECT

	class Impl;

	Impl *const impl;
	long ft;

protected:
	PixelBufferSetter setPixelBuffer;
	
public:
	struct Estimate {
		long est;
		long var;
	};

	const QString nameString;
	const bool integerOnlyScaler;
	
private:
	bool paused;
	
	virtual void privSetPaused(const bool paused) { setUpdatesEnabled(paused); }

public:
	BlitterWidget(PixelBufferSetter setPixelBuffer,
	              const QString &name,
	              bool integerOnlyScaler = false,
	              QWidget *parent = 0);
	virtual ~BlitterWidget();
	bool isPaused() const { return paused; }
	void setPaused(const bool paused) { this->paused = paused; privSetPaused(paused); }

	virtual void init() {}
	virtual void uninit() {}
	virtual void blit() = 0;
	virtual bool isUnusable() const { return false; }
	virtual void setBufferDimensions(unsigned width, unsigned height) = 0;
	virtual void setCorrectedGeometry(int w, int h, int new_w, int new_h) { setGeometry((w - new_w) >> 1, (h - new_h) >> 1, new_w, new_h); }
	virtual void setFrameTime(const long ft) { this->ft = ft; }
	long frameTime() const { return ft; }
	virtual const Estimate frameTimeEst() const { static const Estimate e = { 0, 0 }; return e; }
	virtual long sync(long ft);
	virtual QWidget* settingsWidget() { return NULL; }
	virtual void setExclusive(bool) {}

// 	public slots:
	virtual void acceptSettings() {}
	virtual void rejectSettings() {}

public slots:
	virtual void rateChange(int /*newHz*/) {}
//	virtual void modeChange() {}
};

#endif
