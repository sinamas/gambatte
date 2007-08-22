/***************************************************************************
 *   Copyright (C) 2007 by Sindre Aamås                                    *
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
#ifndef BLITTERCONTAINER_H
#define BLITTERCONTAINER_H

#include <QWidget>

class BlitterWidget;
class FullResToggler;

class BlitterContainer : public QWidget {
	const FullResToggler &resToggler;
	BlitterWidget *blitter;
	
	void doLayout(int w, int h);
	
protected:
	void resizeEvent(QResizeEvent *event);
	void hideEvent(QHideEvent *event);
	
public:
	BlitterContainer(const FullResToggler &fullResToggler, QWidget *parent = 0);
	~BlitterContainer();
	void setBlitter(BlitterWidget *blitter);
	void updateLayout() { doLayout(width(), height()); }
};

#endif
