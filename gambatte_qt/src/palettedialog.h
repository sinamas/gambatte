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

#ifndef PALETTEDIALOG_H
#define PALETTEDIALOG_H

#include <QColor>
#include <QDialog>
#include <QFrame>
#include <QGroupBox>
#include <QPoint>
#include <QSize>
#include <QString>
#include <algorithm>

class QListView;
class QModelIndex;
class QPushButton;
class QSettings;

class ColorPicker : public QFrame {
public:
	explicit ColorPicker(QRgb color = 0xFFFFFF, QWidget *parent = 0);
	QRgb color() const;
	void setColor(QRgb rgb32);
	virtual QSize sizeHint() const { return QSize(4 * 6, 3 * 6); }

signals:
	void colorChanged();

protected:
	virtual void dragEnterEvent(QDragEnterEvent *e);
	virtual void dropEvent(QDropEvent *e);
	virtual void mouseMoveEvent(QMouseEvent *e);
	virtual void mousePressEvent(QMouseEvent *e);
	virtual void mouseReleaseEvent(QMouseEvent *e);
	virtual void keyReleaseEvent(QKeyEvent *e);

private:
	Q_OBJECT

	QWidget *const w_;
	QPoint dragStartPosition_;

	QColor const & getQColor() const;
	void requestColor();
	void setColor(QColor const &color);
};

class ColorQuad : public QGroupBox {
public:
	explicit ColorQuad(QString const &label, QWidget *parent = 0);
	QRgb color(int index) const { return picker_[index & 3]->color(); }
	void setColor(int index, QRgb color) { picker_[index & 3]->setColor(color); }

signals:
	void colorChanged();

protected:
	virtual void dragEnterEvent(QDragEnterEvent *e);
	virtual void dropEvent(QDropEvent *e);
	virtual void mousePressEvent(QMouseEvent *e);

private:
	Q_OBJECT

	ColorPicker * picker_[4];

private slots:
	void pickerChanged();
};

class PaletteDialog : public QDialog {
public:
	explicit PaletteDialog(QString const &savepath,
	                       PaletteDialog const *global = 0,
	                       QWidget *parent = 0);
	virtual ~PaletteDialog();

	QRgb color(int palnum, int colornum) const {
		return currentColors_[std::min(palnum, 2)][colornum & 3];
	}

	void externalChange();
	void setSettingsFile(QString const &filename, QString const &romTitle);

public slots:
	virtual void accept();
	virtual void reject();

private:
	Q_OBJECT

	QString const savedir_;
	PaletteDialog const *const global_;
	QListView *const listView_;
	QPushButton *const rmSchemeButton_;
	ColorQuad *quads_[3];
	QRgb currentColors_[3][4];
	QString defaultScheme_;
	QString schemeString_;
	QString settingsFile_;

	void saveSettings(QSettings &settings);
	void loadSettings(QSettings &settings);
	void saveToSettingsFile();
	void setSchemeList();
	void store();
	void restore();

private slots:
	void rmScheme();
	void saveScheme();
	void schemeChanged(QModelIndex const &current);
};

#endif
