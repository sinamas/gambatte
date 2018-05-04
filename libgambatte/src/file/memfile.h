/***************************************************************************
Copyright (C) 2007 by Nach
http://nsrt.edgeemu.com

Copyright (C) 2007-2011 by sinamas <sinamas at users.sourceforge.net>
sinamas@users.sourceforge.net

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License version 2 as
published by the Free Software Foundation.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License version 2 for more details.

You should have received a copy of the GNU General Public License
version 2 along with this program; if not, write to the
Free Software Foundation, Inc.,
51 Franklin St, Fifth Floor, Boston, MA  02110-1301, USA.
***************************************************************************/
#ifndef GAMBATTE_MEMFILE_H
#define GAMBATTE_MEMFILE_H

#include <string.h>
#include "file.h"

namespace gambatte {

class MemFile : public File {
public:
	explicit MemFile(const void *data, size_t size)
	: data_(static_cast<const char*>(data))
	, size_(size)
	, offset_(0)
	{
	}

	virtual void read(char *buffer, size_t amount) {
		size_t remaining = size_ - offset_;
		size_t bytes_to_read = amount < remaining ? amount : remaining;
		memcpy(buffer, data_ + offset_, bytes_to_read);
		offset_ += bytes_to_read;
	}

	virtual void rewind() { offset_ = 0; }
	virtual size_t size() const { return size_; };
	virtual bool fail() const { return false; }

private:
	const char *data_;
	size_t size_;
	size_t offset_;
};

}

#endif
