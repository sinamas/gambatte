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
#ifndef BLITTERCONF_H
#define BLITTERCONF_H

#include "blitterwidget.h"

class ConstBlitterConf {
	const BlitterWidget *const blitter;
	
public:
	explicit ConstBlitterConf(const BlitterWidget *const blitter) : blitter(blitter) {}
	const QString& nameString() const { return blitter->nameString(); }
	unsigned maxSwapInterval() const { return blitter->maxSwapInterval(); }
	QWidget* settingsWidget() const { return blitter->settingsWidget(); }
	void rejectSettings() const { blitter->rejectSettings(); }
	
	bool operator==(ConstBlitterConf r) const { return blitter == r.blitter; }
	bool operator!=(ConstBlitterConf r) const { return blitter != r.blitter; }
};

class BlitterConf {
	BlitterWidget *const blitter;
	
public:
	explicit BlitterConf(BlitterWidget *const blitter) : blitter(blitter) {}
	const QString& nameString() const { return blitter->nameString(); }
	unsigned maxSwapInterval() const { return blitter->maxSwapInterval(); }
	QWidget* settingsWidget() const { return blitter->settingsWidget(); }
	void acceptSettings() const { blitter->acceptSettings(); }
	void rejectSettings() const { blitter->rejectSettings(); }
	
	bool operator==(BlitterConf r) const { return blitter == r.blitter; }
	bool operator!=(BlitterConf r) const { return blitter != r.blitter; }
	operator ConstBlitterConf() { return ConstBlitterConf(blitter); }
	operator const ConstBlitterConf() const { return ConstBlitterConf(blitter); }
};

#endif
