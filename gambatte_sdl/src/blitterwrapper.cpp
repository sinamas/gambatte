//
//   Copyright (C) 2009 by sinamas <sinamas at users.sourceforge.net>
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

#include "blitterwrapper.h"
#include "videolink/rgb32conv.h"
#include "videolink/vfilterinfo.h"
#include "videolink/videolink.h"

BlitterWrapper::BlitterWrapper(VfilterInfo const &vfinfo, int scale, bool yuv, bool full)
: blitter_(vfinfo.outWidth, vfinfo.outHeight, scale, yuv, full)
, cconvert_(Rgb32Conv::create(static_cast<Rgb32Conv::PixelFormat>(blitter_.inBuffer().format),
                              vfinfo.outWidth, vfinfo.outHeight))
, vfilter_(vfinfo.create())
{
}

BlitterWrapper::~BlitterWrapper() {
}

BlitterWrapper::Buf BlitterWrapper::inBuf() const {
	Buf buf;
	if (VideoLink *const gblink = vfilter_ ? vfilter_.get() : cconvert_.get()) {
		buf.pixels = static_cast<gambatte::uint_least32_t *>(gblink->inBuf());
		buf.pitch  = gblink->inPitch();
	} else {
		SdlBlitter::PixelBuffer const &pxbuf = blitter_.inBuffer();
		buf.pixels = static_cast<gambatte::uint_least32_t *>(pxbuf.pixels);
		buf.pitch = pxbuf.pitch;
	}

	return buf;
}

void BlitterWrapper::draw() {
	SdlBlitter::PixelBuffer const &pb = blitter_.inBuffer();
	if (pb.pixels) {
		if (vfilter_) {
			vfilter_->draw(cconvert_ ? cconvert_->inBuf()   : pb.pixels,
			               cconvert_ ? cconvert_->inPitch() : pb.pitch);
		}
		if (cconvert_)
			cconvert_->draw(pb.pixels, pb.pitch);
	}

	blitter_.draw();
}
