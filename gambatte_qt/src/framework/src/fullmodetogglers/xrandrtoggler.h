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

#ifndef XRANDRTOGGLER_H
#define XRANDRTOGGLER_H

#include "../fullmodetoggler.h"
#include "scoped_ptr.h"
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>

class XRandRToggler : public FullModeToggler {
public:
	static bool isUsable();
	XRandRToggler();
	virtual std::size_t currentResIndex(std::size_t /*screen*/) const { return fullResIndex_; }
	virtual std::size_t currentRateIndex(std::size_t /*screen*/) const { return fullRateIndex_; }
	virtual QRect const fullScreenRect(QWidget const *w) const;
	virtual bool isFullMode() const { return isFull_; }
	virtual void setMode(std::size_t screen, std::size_t resIndex, std::size_t rateIndex);
	virtual void setFullMode(bool enable);
	virtual void emitRate();
	virtual std::vector<ResInfo> const & modeVector(std::size_t /*screen*/) const { return infoVector_; }
	virtual void setScreen(QWidget const *) {}
	virtual std::size_t screen() const { return 0; }
	virtual std::size_t screens() const { return 1; }

signals:
	void rateChange(int newHz);

private:
	Q_OBJECT

	struct XRRDeleter;

	std::vector<ResInfo> infoVector_;
	scoped_ptr<XRRScreenConfiguration, XRRDeleter> const config_;
	std::size_t originalResIndex_;
	std::size_t fullResIndex_;
	Rotation rotation_;
	std::size_t fullRateIndex_;
	short originalRate_;
	bool isFull_;
};

#endif
