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

#ifndef BLITTERCONF_H
#define BLITTERCONF_H

class BlitterWidget;
class QString;
class QWidget;

class ConstBlitterConf {
public:
	explicit ConstBlitterConf(BlitterWidget const *blitter) : blitter_(blitter) {}
	QString const & nameString() const;
	unsigned maxSwapInterval() const;
	QWidget * settingsWidget() const;
	void rejectSettings() const;
	bool operator==(ConstBlitterConf r) const { return blitter_ == r.blitter_; }
	bool operator!=(ConstBlitterConf r) const { return blitter_ != r.blitter_; }

private:
	BlitterWidget const *blitter_;
};

class BlitterConf {
public:
	explicit BlitterConf(BlitterWidget *blitter) : blitter_(blitter) {}
	QString const & nameString() const;
	unsigned maxSwapInterval() const;
	QWidget * settingsWidget() const;
	void acceptSettings() const;
	void rejectSettings() const;
	bool operator==(BlitterConf r) const { return blitter_ == r.blitter_; }
	bool operator!=(BlitterConf r) const { return blitter_ != r.blitter_; }
	operator ConstBlitterConf() const { return ConstBlitterConf(blitter_); }

private:
	BlitterWidget *blitter_;
};

#endif
