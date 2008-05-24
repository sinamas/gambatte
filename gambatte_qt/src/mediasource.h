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
#include <QString>

//TODO: stop dictating audio/video formats.
//      Choice based on:
//       - formats supported by the source, listed by priority
//       - formats supported natively in the engine, list of priorities from the engine
//       - fastest estimated software conversion fallback
//       - only consider formats that have at least detail level of min(maxSorceDetailLvl, maxOutDetailLvl)
//         (note that the source may support formats of higher details than it needs)

class MediaSource {
public:
	/**
	  * Formats that you'll have to deal with if a BlitterWidget decides to give you a buffer with such a format.
	  *
	  * @enum RGB32 Native endian RGB with 8 bits pr color. rmask: 0xff0000, gmask: 0x00ff00, bmask: 0x0000ff
	  *             You may want to keep the top 8 bits at 0 (not sure).
	  *
	  * @enum RGB16 Native endian RGB with 5. 6. and 5 bits for red, green and blue respectively.
	  *             rmask: 0xf800, gmask: 0x07e0 , bmask: 0x001f
	  *
	  * @enum UYVY Big endian UYVY, 8 bits pr field. Normally two horizontal neighbour pixels share U and V,
	  *            but this expects video at 2x width to avoid chroma loss. One pixel is made up of
	  *            U, Y, V and Y (the same value) again for a total of 32 bits pr pixel.
	  *            umask: 0xff000000, ymask: 0x00ff00ff, vmask: 0x0000ff00 (big endian)
	  *            umask: 0x000000ff, ymask: 0xff00ff00, vmask: 0x00ff0000 (little endian)
	  */
	enum PixelFormat { RGB32, RGB16, UYVY };
	
	struct VideoSourceInfo {
		// label used in the video dialog combobox.
		QString label;
		
		// The size of the buffer given through setPixelBuffer depends on these.
		// (so does scaling calculations and other things)
		unsigned width;
		unsigned height;
	};
	
	struct ButtonInfo {
		// Label used in input settings dialog. If this is empty the button won't be configurable, but will use the defaultKey.
		QString label;
		
		// Tab label used in input settings dialog.
		QString category;
		
		// Default Qt::Key. Use Qt::Key_unknown for none.
		int defaultKey;
		
		// Default alternate Qt::Key. Use Qt::Key_unknown for none.
		int defaultAltKey;
	};
	
	/**
	  * Reimplement to get buttonPress events for buttons of corresponding index to the
	  * buttonInfos given to MainWindow.
	  */
	virtual void buttonPressEvent(unsigned /*buttonIndex*/) {}
	virtual void buttonReleaseEvent(unsigned /*buttonIndex*/) {}
	
	/**
	  * Called by MainWindow to set the pixel buffer according to the video source selcted,
	  * and the active BlitterWidget. Can be assumed to have enough space to fit the
	  * dimensions of the current video source in the given format.
	  *
	  * @param pixels Pointer to start of buffer.
	  * @param format Format of the buffer given.
	  * @param pitch Distance from the start of one line in the buffer to the next in pixels (as opposed
	  *              to bytes which is more common).
	  */
	virtual void setPixelBuffer(void *pixels, PixelFormat format, unsigned pitch) = 0;
	
	/**
	  * Called by MainWindow to set the buffer to output audio samples to (always in native endian
	  * signed 16-bit interleaved stereo for now.) Can be assumed to be big enough to have space
	  * for the number of samples requested on each update call. Also gives the currently active
	  * sample rate. which may differ slightly from the alternatives given to MainWindow, since
	  * some engines set a near sample rate if they can't give an exact native one. sampleRate
	  * may be somewhat redundant information, since the number of samples wanted pr frame is given to
	  * the update method, but it's there for convenience (mainly for fixed sample rate sources).
	  */
	virtual void setSampleBuffer(qint16 *sampleBuffer, unsigned sampleRate) = 0;
	
	/**
	  * Sets video source to the one indicated by the corresponding index of the VideoSourceInfos
	  * given to MainWindow. setPixelBuffer will be called if a buffer change is necessary.
	  * Different video sources can have different dimensions and can for instance be used
	  * to select between video filters.
	  */
	virtual void setVideoSource(unsigned videoSourceIndex) = 0;
	
	/**
	  * Called at a time interval given by the frameTime given to MainWindow. Approximately one
	  * frame of video should be generated in the pixel buffer (followed by a call to MainWindow::blit)
	  * pr update for best results. The number of audio samples requested should be outputted to the
	  * (start of the) sample buffer. It's advised to adjust how much you update according to how many
	  * samples are requested, rather than strictly doing one frame of video pr update, unless you
	  * do resampling or dynamically generate samples in a way that allows you to always output the
	  * right number of samples pr video frame. If the number of video frames generated pr update is
	  * far off from one pr update, you could try adjusting the frame time (MainWindow::setFrameTime).
	  * Obviously, if the sample rate selected for a fixed sample rate source is wrong, you can
	  * expect the number of samples requested to be way off.
	  *
	  * @param samples The number of stereo samples that should be written to sampleBuffer.
	  */
	virtual void update(unsigned samples) = 0;
	virtual ~MediaSource() {}
};

#endif
