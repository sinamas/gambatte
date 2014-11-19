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

#ifndef XF86VIDMODETOGGLER_H
#define XF86VIDMODETOGGLER_H

#include "../fullmodetoggler.h"
#include <QCoreApplication>
#include <X11/Xlib.h>
#include <X11/extensions/xf86vmode.h>

class Xf86VidModeToggler : public FullModeToggler {
public:
	static bool isUsable();
	explicit Xf86VidModeToggler(WId winId);
	virtual ~Xf86VidModeToggler();
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
	virtual bool eventFilter(QObject *obj, QEvent *ev);

signals:
	void rateChange(int newHz);

private:
	Q_OBJECT

	XF86VidModeModeInfo **modesinfo_;
	XF86VidModeModeInfo originalMode_;
	std::vector<ResInfo> infoVector_;
	int modecount_;
	std::size_t fullResIndex_;
	std::size_t fullRateIndex_;
	int originalvportx_;
	int originalvporty_;
	WId winId_;
	bool isFull_;
};

#endif
