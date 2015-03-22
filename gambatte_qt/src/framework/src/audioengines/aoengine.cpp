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

#include "aoengine.h"
#include "../audioengine.h"
#include <ao/ao.h>

namespace {

class AoEngine : public AudioEngine {
public:
	AoEngine()
	: AudioEngine("Libao")
	, aoDevice_()
	{
	}

	virtual ~AoEngine() { uninit(); }
	virtual void uninit();

	virtual int write(void *buffer, std::size_t samples) {
		if (ao_play(aoDevice_, static_cast<char *>(buffer), samples * 4) == 0)
			return -1;

		return 0;
	}

protected:
	virtual long doInit(long rate, int latency);

private:
	ao_device *aoDevice_;
};

long AoEngine::doInit(long const rate, int /*latency*/) {
	ao_initialize();

	ao_sample_format sampleFormat = { 16, int(rate), 2, AO_FMT_NATIVE, 0 };
	int aoDriverId = ao_default_driver_id();
	if (aoDriverId != -1)
		aoDevice_ = ao_open_live(aoDriverId, &sampleFormat, 0);

	if (!aoDevice_) {
		ao_shutdown();
		return -1;
	}

	return sampleFormat.rate;
}

void AoEngine::uninit() {
	if (aoDevice_) {
		ao_close(aoDevice_);
		aoDevice_ = 0;
		ao_shutdown();
	}
}

} // anon ns

transfer_ptr<AudioEngine> createAoEngine() {
	return transfer_ptr<AudioEngine>(new AoEngine);
}
