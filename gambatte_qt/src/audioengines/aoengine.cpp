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
#include "aoengine.h"

AoEngine::AoEngine() : aoDevice(NULL) {}

AoEngine::~AoEngine() {
	uninit();
}

int AoEngine::init() {
	ao_initialize();
	
	aoDevice = NULL;
	
	ao_sample_format sampleFormat = { 16, 48000, 2, AO_FMT_NATIVE };
	
	int aoDriverId = ao_default_driver_id();
	
	if (aoDriverId != -1) {
		if ((aoDevice = ao_open_live(aoDriverId, &sampleFormat, NULL)) == NULL) {
			sampleFormat.rate = 44100;
			aoDevice = ao_open_live(aoDriverId, &sampleFormat, NULL);
		}
	}
	
	if (aoDevice == NULL) {
		ao_shutdown();
		return -1;
	}
	
	return sampleFormat.rate;
}

void AoEngine::uninit() {
	if (aoDevice) {
		ao_close(aoDevice);
		aoDevice = NULL;
		
		ao_shutdown();
	}
}

int AoEngine::write(void *const buffer, const unsigned samples) {
	if (ao_play(aoDevice, reinterpret_cast<char*>(buffer), samples * 4) == 0)
		return -1;
	
	return 0;
}
