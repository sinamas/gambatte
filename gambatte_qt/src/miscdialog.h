//
//   Copyright (C) 2009 by sinamas <sinamas at users.sourceforge.net>
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

#ifndef MISCDIALOG_H
#define MISCDIALOG_H

#include "dialoghelpers.h"
#include "fpsselector.h"
#include "pathselector.h"
#include <QDialog>

class QSpinBox;

class MiscDialog : public QDialog {
public:
	explicit MiscDialog(QString const &savePath, QWidget *parent = 0);
	virtual ~MiscDialog();
	int turboSpeed() const { return turboSpeed_; }
	bool pauseOnDialogs() const { return pauseOnDialogs_.value() | pauseOnFocusOut_.value(); }
	bool pauseOnFocusOut() const { return pauseOnFocusOut_.value(); }
	bool dwmTripleBuf() const { return dwmTripleBuf_.value(); }
	bool multicartCompat() const { return multicartCompat_.value(); }
	QSize const baseFps() const { return fpsSelector_.value(); }
	QString const & savePath() const { return savepathSelector_.value(); }

public slots:
	virtual void accept();
	virtual void reject();

private:
	QSpinBox *const turboSpeedBox;
	PersistCheckBox pauseOnDialogs_;
	PersistCheckBox pauseOnFocusOut_;
	FpsSelector fpsSelector_;
	PersistCheckBox dwmTripleBuf_;
	PersistCheckBox multicartCompat_;
	PathSelector savepathSelector_;
	int turboSpeed_;

	void restore();
};

#endif
