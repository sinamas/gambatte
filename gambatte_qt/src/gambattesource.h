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
#ifndef GAMBATTESOURCE_H
#define GAMBATTESOURCE_H

#include "mediasource.h"
#include "inputdialog.h"
#include "pixelbuffer.h"
#include "scoped_ptr.h"
#include "videodialog.h"
#include "videolink/videolink.h"
#include <gambatte.h>
#include <pakinfo.h>
#include <QObject>
#include <cstring>

class GambatteSource : public QObject, public MediaSource {
	Q_OBJECT

	struct GbVidBuf;
	struct GetInput : public gambatte::InputGetter {
		unsigned is;
		GetInput() : is(0) {}
		unsigned operator()() { return is; }
	};

	gambatte::GB gb;
	GetInput inputGetter;
	InputDialog *const inputDialog_;
	scoped_ptr<VideoLink> cconvert;
	scoped_ptr<VideoLink> vfilter;
	PixelBuffer::PixelFormat pxformat;
	unsigned vsrci;
	bool inputState[10];
	volatile bool dpadUp, dpadDown;
	volatile bool dpadLeft, dpadRight;
	volatile bool dpadUpLast, dpadLeftLast;

	InputDialog* createInputDialog();
	const GbVidBuf setPixelBuffer(void *pixels, PixelBuffer::PixelFormat format, unsigned pitch);
	void keyPressEvent(const QKeyEvent *);
	void keyReleaseEvent(const QKeyEvent *);
	void joystickEvent(const SDL_Event&);

	void emitSetTurbo(bool on) { emit setTurbo(on); }
	void emitPause() { emit togglePause(); }
	void emitFrameStep() { emit frameStep(); }
	void emitDecFrameRate() { emit decFrameRate(); }
	void emitIncFrameRate() { emit incFrameRate(); }
	void emitResetFrameRate() { emit resetFrameRate(); }
	void emitPrevStateSlot() { emit prevStateSlot(); }
	void emitNextStateSlot() { emit nextStateSlot(); }
	void emitSaveState() { emit saveStateSignal(); }
	void emitLoadState() { emit loadStateSignal(); }
	void emitQuit() { emit quit(); }

public:
	GambatteSource();

	const std::vector<VideoDialog::VideoSourceInfo> generateVideoSourceInfos();

	gambatte::LoadRes load(std::string const &romfile, unsigned flags) { return gb.load(romfile, flags); }
	void setGameGenie(const std::string &codes) { gb.setGameGenie(codes); }
	void setGameShark(const std::string &codes) { gb.setGameShark(codes); }
	void reset() { gb.reset(); }
	void setDmgPaletteColor(unsigned palNum, unsigned colorNum, unsigned rgb32) { gb.setDmgPaletteColor(palNum, colorNum, rgb32); }
	void setSavedir(const std::string &sdir) { gb.setSaveDir(sdir); }
	bool isCgb() const { return gb.isCgb(); }
	const std::string romTitle() const { return gb.romTitle(); }
	gambatte::PakInfo const pakInfo() const { return gb.pakInfo(); }
	void selectState(int n) { gb.selectState(n); }
	int currentState() const { return gb.currentState(); }
	void saveState(const PixelBuffer &fb, const std::string &filepath);
	void loadState(const std::string &filepath) { gb.loadState(filepath); }
	QDialog* inputDialog() const { return inputDialog_; }

	//overrides
	void buttonPressEvent(unsigned buttonIndex);
	void buttonReleaseEvent(unsigned buttonIndex);
	void setVideoSource(unsigned videoSourceIndex);// { gb.setVideoFilter(videoSourceIndex); }
	long update(const PixelBuffer &fb, qint16 *soundBuf, long &samples);
	void generateVideoFrame(const PixelBuffer &fb);

// public slots:
	void saveState(const PixelBuffer &fb);
	void loadState() { gb.loadState(); }

signals:
	void setTurbo(bool on);
	void togglePause();
	void frameStep();
	void decFrameRate();
	void incFrameRate();
	void resetFrameRate();
	void prevStateSlot();
	void nextStateSlot();
	void saveStateSignal();
	void loadStateSignal();
	void quit();
};

#endif
