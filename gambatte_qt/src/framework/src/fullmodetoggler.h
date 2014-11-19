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

#ifndef FULLMODETOGGLER_H
#define FULLMODETOGGLER_H

#include "resinfo.h"
#include <QObject>
#include <QRect>
#include <QString>
#include <QWidget>
#include <vector>

class FullModeToggler : public QObject {
	Q_OBJECT

public:
	virtual ~FullModeToggler() {}
	virtual std::size_t currentResIndex(std::size_t screen) const = 0;
	virtual std::size_t currentRateIndex(std::size_t screen) const = 0;
	virtual QRect const fullScreenRect(QWidget const *w) const { return w->geometry(); }
	virtual bool isFullMode() const = 0;
	virtual void setMode(std::size_t screen, std::size_t resIndex, std::size_t rateIndex) = 0;
	virtual void setFullMode(bool enable) = 0;
	virtual void emitRate() = 0;
	virtual std::vector<ResInfo> const & modeVector(std::size_t screen) const = 0;
	virtual void setScreen(QWidget const *widget) = 0;
	virtual std::size_t screen() const = 0;
	virtual std::size_t screens() const = 0;
	virtual QString const screenName(std::size_t screen) const { return QString::number(screen + 1); }

signals:
	void rateChange(int newHz);
};

#endif
