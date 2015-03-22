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

#ifndef SOUNDDIALOG_H
#define SOUNDDIALOG_H

#include "dialoghelpers.h"
#include <QDialog>

class MainWindow;
class QSpinBox;

/**
  * A utility class that can optionally be used to provide a GUI for
  * configuring audio settings.
  */
class SoundDialog : public QDialog {
public:
	explicit SoundDialog(MainWindow const &mw, QWidget *parent = 0);
	virtual ~SoundDialog();
	std::size_t engineIndex() const { return engineSelector_.index(); }
	std::size_t resamplerNo() const { return resamplerSelector_.index(); }
	int rate() const { return rate_; }
	int latency() const { return latency_; };

public slots:
	virtual void accept();
	virtual void reject();

private:
	Q_OBJECT

	MainWindow const &mw_;
	PersistComboBox engineSelector_;
	PersistComboBox resamplerSelector_;
	QComboBox *const rateBox_;
	QSpinBox *const latencyBox_;
	QWidget *engineWidget_;
	int rate_;
	int latency_;

	void store();
	void restore();

private slots:
	void engineChange(int index);
	void rateIndexChange(int index);
};

void applySettings(MainWindow &mw, SoundDialog const &sd);

#endif
