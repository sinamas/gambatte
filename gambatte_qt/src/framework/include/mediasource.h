//
//   Copyright (C) 2007 by sinamas <sinamas at users.sourceforge.net>
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

#ifndef MEDIASOURCE_H
#define MEDIASOURCE_H

#include "SDL_event.h"
#include <QtGlobal>
#include <cstddef>

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
	  * @param overUpdate The soundBuf passed to the update function is set to a size
	  *                   that depends on the number of audio samples per video frame,
	  *                   so that it should be possible to fit one video frame's worth
	  *                   of audio in the buffer. overUpdate is added to this size.
	  *                   This is useful if a MediaSource is unable to update for an
	  *                   exact amount of audio samples. For instance if, the MediaSource
	  *                   may internally update for up to N samples more than requested
	  *                   overUpdate should be set to N to ensure that it is possible to
	  *                   fit N extra samples per video frame. Otherwise there may be
	  *                   multiple updates per video frame.
	  */
	explicit MediaSource(std::size_t overUpdate = 0) : overUpdate(overUpdate) {}
public:
	std::size_t const overUpdate;

	// Reimplement to receive input events.
	// joystickEvent is called from the worker thread, while keyPress/ReleaseEvent
	// are called from the GUI thread.
	virtual void keyPressEvent(QKeyEvent const *) {}
	virtual void keyReleaseEvent(QKeyEvent const *) {}
	virtual void joystickEvent(SDL_Event const &) {}

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
	  * @param soundBuf Audio buffer to write 16-bit stereo samples to. 32-bit aligned.
	  * @param samples  In: Size of soundBuf in number of stereo samples.
	  *                Out: Number of stereo samples written to soundBuf.
	  * @return The number of stereo samples that should be output before the video frame is
	  '         displayed. Or a negative number if no new video frame should be displayed.
	  */
	virtual std::ptrdiff_t update(PixelBuffer const &frameBuf,
	                              qint16 *soundBuf, std::size_t &samples) = 0;

	/**
	  * This is called after update returns, but only when it is clear that the frame will not
	  * be skipped. This gives an opportunity to delay heavy video work until it is clear that
	  * it will be used. For instance, if the audio buffer is low, we may want to skip updating
	  * video to avoid underruns. Or if we are fast-forwarding we do not want to waste time
	  * updating video unnecessarily.
	  *
	  * Heavy post-processing of video is a good candidate for this method.
	  */
	virtual void generateVideoFrame(PixelBuffer const &/*frameBuf*/) {}

	virtual ~MediaSource() {}
};

#endif
