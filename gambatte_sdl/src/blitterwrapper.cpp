/***************************************************************************
 *   Copyright (C) 2009 by Sindre Aamås                                    *
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
#include "blitterwrapper.h"
#include "videolink/vfilterinfo.h"
#include "videolink/rgb32conv.h"

const BlitterWrapper::Buf BlitterWrapper::inBuf() const {
	Buf buf;
	
	if (VideoLink *const gblink = vfilter.get() ? vfilter.get() : cconvert.get()) {
		buf.pixels = static_cast<gambatte::uint_least32_t*>(gblink->inBuf());
		buf.pitch  = gblink->inPitch();
	} else {
		const SdlBlitter::PixelBuffer &pxbuf = blitter.inBuffer();
		buf.pixels = static_cast<gambatte::uint_least32_t*>(pxbuf.pixels);
		buf.pitch = pxbuf.pitch;
	}
	
	return buf;
}

void BlitterWrapper::draw() {
	const SdlBlitter::PixelBuffer &pb = blitter.inBuffer();

	if (void *const pbdata = pb.pixels) {
		if (vfilter.get()) {
			if (cconvert.get()) {
				vfilter->draw(cconvert->inBuf(), cconvert->inPitch());
				cconvert->draw(pbdata, pb.pitch);
			} else
				vfilter->draw(pbdata, pb.pitch);
		} else if (cconvert.get())
			cconvert->draw(pbdata, pb.pitch);
	}
	
	blitter.draw();
}

void BlitterWrapper::init() {
	const VfilterInfo vfinfo = VfilterInfo::get(vsrci);
	vfilter.reset(vfinfo.create());
	blitter.setBufferDimensions(vfinfo.outWidth, vfinfo.outHeight);
	cconvert.reset(Rgb32Conv::create(static_cast<Rgb32Conv::PixelFormat>(blitter.inBuffer().format),
			vfinfo.outWidth, vfinfo.outHeight));
	
}
