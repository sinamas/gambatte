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
#ifndef PALETTEDIALOG_H
#define PALETTEDIALOG_H

class QListView;
class QPushButton;
class QSettings;

#include <QDialog>
#include <QColor>
#include <QFrame>
#include <QPoint>
#include <QSize>
#include <QGroupBox>
#include <QModelIndex>
#include <QString>

class ColorPicker : public QFrame {
	Q_OBJECT
	
	QWidget *const w;
	QPoint dragStartPosition;
	
	const QColor& getQColor() const;
	void requestColor();
	void setColor(const QColor &color);
	
protected:
	void dragEnterEvent(QDragEnterEvent *e);
	void dropEvent(QDropEvent *e);
	void mouseMoveEvent(QMouseEvent *e);
	void mousePressEvent(QMouseEvent *e);
	void mouseReleaseEvent(QMouseEvent *e);
	void keyReleaseEvent(QKeyEvent *e);
	
public:
	ColorPicker(QRgb color = 0xFFFFFF, QWidget *parent = 0);
	QRgb getColor() const;
	void setColor(QRgb rgb32);
	QSize sizeHint() const { return QSize(4*6, 3*6); }

signals:
	void colorChanged();
};

class ColorQuad : public QGroupBox {
	Q_OBJECT
	
	ColorPicker* picker[4];
	
private slots:
	void pickerChanged();
	
protected:
	void dragEnterEvent(QDragEnterEvent *e);
	void dropEvent(QDropEvent *e);
	void mousePressEvent(QMouseEvent *e);
	
public:
	ColorQuad(const QString &label, QWidget *parent = 0);
	QRgb getColor(unsigned index) const { return picker[index & 3]->getColor(); }
	void setColor(unsigned index, QRgb color) { picker[index & 3]->setColor(color); }
	
signals:
	void colorChanged();
};

class PaletteDialog : public QDialog {
	Q_OBJECT
	
	const PaletteDialog *const global;
	QListView *const listView;
	QPushButton *const rmSchemeButton;
	ColorQuad *quads[3];
	QRgb currentColors[3][4];
	QString defaultScheme;
	QString schemeString;
	QString savedir;
	QString settingsFile;
	
	void saveSettings(QSettings &settings);
	void loadSettings(QSettings &settings);
	void saveToSettingsFile();
	void setSchemeList();
	void store();
	void restore();
	
private slots:
	void rmScheme();
	void saveScheme();
	void schemeChanged(const QModelIndex &current, const QModelIndex &previous);
	
public:
	explicit PaletteDialog(const QString &savepath, const PaletteDialog *global = 0, QWidget *parent = 0);
	~PaletteDialog();
	QRgb getColor(unsigned palnum, unsigned colornum) const { return currentColors[palnum < 3 ? palnum : 2][colornum & 3]; }
	void externalChange();
	void setSettingsFile(const QString &filename, const QString &romTitle);

public slots:
	void accept();
	void reject();
};

#endif
