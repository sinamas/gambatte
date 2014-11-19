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

#ifndef AUDIOENGINE_H
#define AUDIOENGINE_H

#include <QMutex>
#include <QMutexLocker>
#include <QString>
#include <cstddef>

class QWidget;

class AudioEngine {
public:
	struct BufferState {
		enum { not_supported = std::size_t(-1) };
		std::size_t fromUnderrun;
		std::size_t fromOverflow;
	};

	QString const & nameString() const { return nameString_; }
	long rate() const { return rate_; }
	void acceptSettings() { QMutexLocker l(&mut_); doAcceptSettings(); }

	long init(long rate, int msLatency) {
		QMutexLocker l(&mut_);
		rate_ = rate = doInit(rate, msLatency);
		if (rate < 0)
			uninit();

		return rate;
	}

	virtual ~AudioEngine() {}
	virtual void uninit() {}
	virtual int write(void *buffer, std::size_t samples) = 0;
	virtual long rateEstimate() const { return rate_; }

	virtual BufferState bufferState() const {
		BufferState s = { BufferState::not_supported,
		                  BufferState::not_supported };
		return s;
	}

	virtual void pause() {}

	/** @return success */
	virtual bool flushPausedBuffers() const { return false; }

	virtual int write(void *buffer, std::size_t samples,
	                  BufferState &preBufState_out, long &rate_out)
	{
		preBufState_out = bufferState();
		int const ret = write(buffer, samples);
		rate_out = rateEstimate();
		return ret;
	}

	virtual QWidget * settingsWidget() const { return 0; }
	virtual void rejectSettings() const {}

protected:
	AudioEngine(QString const &name)
	: nameString_(name)
	, rate_(0)
	{
	}

	virtual long doInit(long rate, int msLatency) = 0;
	virtual void doAcceptSettings() {}

private:
	QMutex mut_;
	QString const nameString_;
	long rate_;
};

#endif
