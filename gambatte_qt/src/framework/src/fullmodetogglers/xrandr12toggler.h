//
//   Copyright (C) 2008 by sinamas <sinamas at users.sourceforge.net>
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

#ifndef XRANDR12TOGGLER_H_
#define XRANDR12TOGGLER_H_

#include "../fullmodetoggler.h"
#include "scoped_ptr.h"
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>

class XRandR12Toggler : public FullModeToggler {
public:
	static bool isUsable();
	XRandR12Toggler();
	virtual std::size_t currentResIndex(std::size_t screen) const { return fullResIndex_[screen]; }
	virtual std::size_t currentRateIndex(std::size_t screen) const { return fullRateIndex_[screen]; }
	virtual QRect const fullScreenRect(QWidget const *w) const;
	virtual bool isFullMode() const { return isFull_; }
	virtual void setMode(std::size_t screen, std::size_t resIndex, std::size_t rateIndex);
	virtual void setFullMode(bool enable);
	virtual void emitRate();
	virtual std::vector<ResInfo> const & modeVector(std::size_t screen) const { return infoVector_[screen]; }
	virtual void setScreen(QWidget const *widget);
	virtual std::size_t screen() const { return widgetScreen_; }
	virtual std::size_t screens() const { return infoVector_.size(); }
	virtual QString const screenName(std::size_t screen) const;

signals:
	void rateChange(int newHz);

private:
	Q_OBJECT

	struct XRRDeleter;

	RRMode originalMode_;
	scoped_ptr<XRRScreenResources, XRRDeleter> const resources_;
	std::vector< std::vector<ResInfo> > infoVector_;
	std::vector<std::size_t> fullResIndex_;
	std::vector<std::size_t> fullRateIndex_;
	std::size_t widgetScreen_;
	bool isFull_;
};

#endif

