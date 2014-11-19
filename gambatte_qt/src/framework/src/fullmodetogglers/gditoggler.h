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

#ifndef GDITOGGLER_H_
#define GDITOGGLER_H_

#include "../fullmodetoggler.h"
#include "scoped_ptr.h"

class GdiToggler : public FullModeToggler {
public:
	GdiToggler();
	virtual ~GdiToggler();
	virtual std::size_t currentResIndex(std::size_t screen) const { return fullResIndex[screen]; }
	virtual std::size_t currentRateIndex(std::size_t screen) const { return fullRateIndex[screen]; }
	virtual bool isFullMode() const { return isFull; }
	virtual void setMode(std::size_t screen, std::size_t resIndex, std::size_t rateIndex);
	virtual void setFullMode(bool enable);
	virtual void emitRate();
	virtual std::vector<ResInfo> const & modeVector(std::size_t screen) const { return infoVector[screen]; }
	virtual void setScreen(QWidget const *widget);
	virtual std::size_t screen() const { return widgetScreen; }
	virtual std::size_t screens() const { return infoVector.size(); }

signals:
	void rateChange(int newHz);

private:
	Q_OBJECT

	class MultiMon;

	scoped_ptr<MultiMon> const mon;
	std::vector<std::vector<ResInfo> > infoVector;
	std::vector<std::size_t> fullResIndex;
	std::vector<std::size_t> fullRateIndex;
	std::size_t widgetScreen;
	bool isFull;
};

#endif /*GDITOGGLER_H_*/
