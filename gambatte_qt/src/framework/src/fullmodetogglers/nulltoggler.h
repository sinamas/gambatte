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

#ifndef NULLTOGGLER_H
#define NULLTOGGLER_H

#include "../fullmodetoggler.h"

class NullToggler : public FullModeToggler {
public:
	NullToggler() : fullRes(false) {}
	virtual std::size_t currentResIndex(std::size_t /*screen*/) const { return 0; }
	virtual std::size_t currentRateIndex(std::size_t /*screen*/) const { return 0; }
	virtual bool isFullMode() const { return fullRes; }
	virtual void setMode(std::size_t /*screen*/, std::size_t /*resIndex*/, std::size_t /*rateIndex*/) {}
	virtual void setFullMode(bool enable) { fullRes = enable; }
	virtual void emitRate() { emit rateChange(0); }
	virtual std::vector<ResInfo> const & modeVector(std::size_t /*screen*/) const { return nullVector; }
	virtual void setScreen(QWidget const *) {}
	virtual std::size_t screen() const { return 0; }
	virtual std::size_t screens() const { return 0; }

signals:
	void rateChange(int newHz);

private:
	Q_OBJECT

	std::vector<ResInfo> const nullVector;
	bool fullRes;
};

#endif
