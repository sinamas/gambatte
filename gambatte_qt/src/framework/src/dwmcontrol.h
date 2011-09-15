/***************************************************************************
 *   Copyright (C) 2011 by Sindre Aamås                                    *
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
#ifndef DWMCONTROL_H_
#define DWMCONTROL_H_

#include <vector>
#include <QWidget>

class BlitterWidget;

class DwmControl {
	const std::vector<BlitterWidget*> blitters_;
	int refreshCnt_;
	bool tripleBuffer_;

public:
	explicit DwmControl(const std::vector<BlitterWidget*> &blitters);
	void setDwmTripleBuffer(bool enable);
	void hideEvent();
	void showEvent();
	/** @return compositionChange */
	bool winEvent(const void *msg);
	void tick();
	void hwndChange(BlitterWidget *blitter);

	static bool hasDwmCapability();
	static bool isCompositingEnabled();
};

class DwmControlHwndChange {
	DwmControl &dwmc_;
public:
	explicit DwmControlHwndChange(DwmControl &dwmc) : dwmc_(dwmc) {}
	void operator()(BlitterWidget *blitter) const { dwmc_.hwndChange(blitter); }
};

#endif /* DWMCONTROL_H_ */
