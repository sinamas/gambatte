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
#ifndef MEDIASOURCE_H
#define MEDIASOURCE_H

#include <QtGlobal>
#include "SDL_event.h"

struct PixelBuffer;
class QKeyEvent;

/**
  * A MediaSource is a source of audio/video content.
  * It produces audio/video upon requests to update from MainWindow.
  * Inherit from it and implement its virtual methods. The implementation decides what
  * kind of audio/video content to produce, and how to react to input events.
  */
class MediaSource {
protected:
	/**
	  * @param overupdate The soundBuf passed to the update function is set to a size
	  *                   that depends on the number of audio samples per video frame,
	  *                   so that it should be possible to fit one video frame's worth
	  *                   of audio in the buffer. overupdate is added to this size.
	  *                   This is useful if a MediaSource is unable to update for an
	  *                   exact amount of audio samples. For instance if, the MediaSource
	  *                   may internally update for up to N samples more than requested
	  *                   overupdate should be set to N to ensure that it is possible to
	  *                   fit N extra samples per video frame. Otherwise there may be
	  *                   multiple updates per video frame.
	  */
	explicit MediaSource(const unsigned overupdate = 0) : overupdate(overupdate) {}
public:
	const unsigned overupdate;
	
	// Reimplement to get input events. The InputDialog class may be useful.
	// joystickEvent is called from the worker thread, while keyPress/ReleaseEvent
	// are called from the GUI thread.
	virtual void keyPressEvent(const QKeyEvent *) {}
	virtual void keyReleaseEvent(const QKeyEvent *) {}
	virtual void joystickEvent(const SDL_Event&) {}
	
	/**
	  * Update until a new frame of video, or 'samples' audio samples have been produced.
	  * May postpone writing to frameBuf until generateVideoFrame is called.
	  *
	  * Be aware that there can be a delay between requests for a new video format
	  * until the frame buffer has changed. IOW frameBuf may be of an
	  * unexpected size at times. The data field of frameBuf can be 0 when
	  * the frame buffer is locked by someone else.
	  *
	  * Called in a separate (worker) thread from the GUI thread.
	  *
	  * @param frameBuf Frame buffer to be filled with video data.
	  * @param soundBuf Audio buffer to write 16-bit stereo samples to.
	  * @param samples In: Size of soundBuf in number of stereo samples. Out: Number of stereo samples written to soundBuf.
	  * @return The number of stereo samples that should be output before the video frame is displayed. Or a negative number if no new video frame should be displayed.
	  */
	virtual long update(const PixelBuffer &frameBuf, qint16 *soundBuf, long &samples) = 0;
	
	/**
	  * This is called after update returns, but only when it is clear that the frame won't be skipped.
	  * This gives an opportunity to delay heavy video work until it is clear that it will be used.
	  * For instance, if the audio buffer is low, we may want to skip updating video to avoid underruns.
	  * Or if we're fast forwarding we don't want to waste time updating video unnecessarily.
	  * Heavy post-processing of video is a good candidate for this method.
	  */
	virtual void generateVideoFrame(const PixelBuffer &/*fb*/) {}
	
	virtual ~MediaSource() {}
};

#endif
