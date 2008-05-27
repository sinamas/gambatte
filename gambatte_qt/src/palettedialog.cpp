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
#include "palettedialog.h"

#include <QColor>
#include <QPalette>
#include <QColorDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QApplication>
#include <QDropEvent>
#include <QDragEnterEvent>
#include <QMimeData>
#include <QByteArray>
#include <QDataStream>
#include <QListView>
#include <QStringListModel>
#include <QDir>
#include <QSettings>
#include <QInputDialog>
#include <QMessageBox>

ColorPicker::ColorPicker(QRgb color, QWidget *parent)
: QFrame(parent), w(new QWidget)
{
	setAcceptDrops(true);
	w->setAutoFillBackground(true);
	
	setLayout(new QVBoxLayout);
	layout()->setMargin(0);
	layout()->addWidget(w);
	
	setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
	setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
	setFocusPolicy(Qt::StrongFocus);
	setColor(color);
}

const QColor& ColorPicker::getQColor() const {
	return w->palette().color(QPalette::Background);
}

void ColorPicker::requestColor() {
	QColor c = QColorDialog::getColor(QColor(getColor()), this);
	
	if (c.isValid()) {
		setColor(c);
		emit colorChanged();
	}
}

void ColorPicker::setColor(const QColor &color) {
	QPalette p(w->palette());
	p.setColor(QPalette::Background, color);
	w->setPalette(p);
}

void ColorPicker::dragEnterEvent(QDragEnterEvent *e) {
	if (e->mimeData()->hasColor() && e->source() != this)
		e->acceptProposedAction();
}

void ColorPicker::dropEvent(QDropEvent *e) {
	e->setDropAction(Qt::CopyAction);
	e->accept();
	setColor(qvariant_cast<QColor>(e->mimeData()->colorData()));
	emit colorChanged();
}

void ColorPicker::mousePressEvent(QMouseEvent *e) {
	dragStartPosition = e->pos();
}

void ColorPicker::mouseMoveEvent(QMouseEvent *e) {
	if ((e->pos() - dragStartPosition).manhattanLength() < QApplication::startDragDistance())
		return;
	
	QDrag *const drag = new QDrag(this);
	QMimeData *const mimeData = new QMimeData;
	mimeData->setColorData(getQColor());
	drag->setMimeData(mimeData);
	drag->exec(Qt::CopyAction);
}

void ColorPicker::mouseReleaseEvent(QMouseEvent *e) {
	e->ignore();
	
	if (e->x() <= width() && e->y() <= height()) {
		e->accept();
		requestColor();
	}
}

void ColorPicker::keyReleaseEvent(QKeyEvent *e) {
	e->ignore();
	
	if (e->key() == Qt::Key_Space) {
		e->accept();
		requestColor();
	}
}

QRgb ColorPicker::getColor() const {
	return getQColor().rgb() & 0xFFFFFF;
}

void ColorPicker::setColor(QRgb rgb32) {
	setColor(QColor(rgb32));
}

ColorQuad::ColorQuad(const QString &label, QWidget *parent)
: QGroupBox(label, parent)
{
	setAcceptDrops(true);
	setLayout(new QHBoxLayout);
	
	for (int i = 0; i < 4; ++i) {
		layout()->addWidget(picker[i] = new ColorPicker);
		connect(picker[i], SIGNAL(colorChanged()), this, SLOT(pickerChanged()));
	}
	
// 	setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
}

void ColorQuad::pickerChanged() {
	emit colorChanged();
}

void ColorQuad::dragEnterEvent(QDragEnterEvent *e) {
	if (e->mimeData()->hasFormat("application/x-colorquad") && e->source() != this)
		e->acceptProposedAction();
}

void ColorQuad::dropEvent(QDropEvent *e) {
	e->setDropAction(Qt::CopyAction);
	e->accept();
		
	QDataStream dataStream(e->mimeData()->data("application/x-colorquad"));
	QRgb color;
	
	for (int i = 0; i < 4; ++i) {
		dataStream >> color;
		setColor(i, color);
	}
	
	pickerChanged();
}

void ColorQuad::mousePressEvent(QMouseEvent */*e*/) {
	QByteArray itemData;
	QDataStream dataStream(&itemData, QIODevice::WriteOnly);
	
	for (int i = 0; i < 4; ++i)
		dataStream << getColor(i);
	
	QMimeData *const mimeData = new QMimeData;
	mimeData->setData("application/x-colorquad", itemData);
	
	QDrag *const drag = new QDrag(this);
	drag->setMimeData(mimeData);
	drag->exec(Qt::CopyAction);
}

class ImmutableStringListModel : public QStringListModel {
public:
	ImmutableStringListModel(QObject *parent = 0) : QStringListModel(parent) {}
	ImmutableStringListModel(const QStringList &strings, QObject *parent = 0) : QStringListModel(strings, parent) {}
	Qt::ItemFlags flags(const QModelIndex &index) const { return QStringListModel::flags(index) & ~Qt::ItemIsEditable; }
};

static void setSchemeList(const QString &savedir, const bool hasGlobal, QStringListModel *const model) {
	QDir dir(savedir, "*.pal", QDir::Name | QDir::IgnoreCase, QDir::Files | QDir::Readable);
	QStringList dirlisting(dir.entryList());
	
	for (QStringList::iterator it = dirlisting.begin(); it != dirlisting.end(); ++it)
		it->chop(4);
	
	model->setStringList((hasGlobal ? QStringList("Global Palette") : QStringList()) + QStringList("Current Scheme") + QStringList("Default Grey") + dirlisting);
}

static const QModelIndex schemeIndexOf(const QAbstractItemModel *const model, const QString &schemeStr) {
	const int rows = model->rowCount();
	
	for (int i = 0; i < rows; ++i) {
		if (model->index(i, 0).data().toString() == schemeStr)
			return model->index(i, 0);
	}
	
	for (int i = 0; i < rows; ++i) {
		if (model->index(i, 0).data().toString() == "Current Scheme")
			return model->index(i, 0);
	}
	
	return QModelIndex();
}

PaletteDialog::PaletteDialog(const QString &savepath, const PaletteDialog *global, QWidget *parent)
: QDialog(parent), global(global), listView(new QListView), rmSchemeButton(new QPushButton("Remove Scheme"))
{
	setWindowTitle(global ? "Current ROM Palette" : "Global Palette");
	
	QBoxLayout *const mainLayout = new QVBoxLayout;
	
	{
		QBoxLayout *const topLayout = new QHBoxLayout;
		
		{
			QGroupBox *const lframe = new QGroupBox("Scheme");
			QBoxLayout *const frameLayout = new QVBoxLayout;
			savedir = savepath + "/";
			QDir::root().mkpath(savedir + "stored/");
			listView->setModel(new ImmutableStringListModel);
			setSchemeList();
			frameLayout->addWidget(listView);
			
			{
				QPushButton *const saveButton = new QPushButton("Save Scheme...");
				connect(saveButton, SIGNAL(clicked()), this, SLOT(saveScheme()));
				frameLayout->addWidget(saveButton);
			}
			
			connect(rmSchemeButton, SIGNAL(clicked()), this, SLOT(rmScheme()));
			frameLayout->addWidget(rmSchemeButton);
			
			lframe->setLayout(frameLayout);
			topLayout->addWidget(lframe);
		}
		
		{
			QBoxLayout *const vLayout = new QVBoxLayout;
			vLayout->addWidget(quads[0] = new ColorQuad("Background"));
			vLayout->addWidget(quads[1] = new ColorQuad("Sprite 1"));
			vLayout->addWidget(quads[2] = new ColorQuad("Sprite 2"));
			topLayout->addLayout(vLayout);
// 			topLayout->setAlignment(vLayout, Qt::AlignTop);
		}
		
		mainLayout->addLayout(topLayout);
	}
	
	{
		QPushButton *const okButton = new QPushButton(tr("OK"));
		QPushButton *const cancelButton = new QPushButton(tr("Cancel"));
		
		okButton->setDefault(true);
		
		connect(okButton, SIGNAL(clicked()), this, SLOT(accept()));
		connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
		
		QBoxLayout *const hLayout = new QHBoxLayout;
		hLayout->addWidget(okButton);
		hLayout->addWidget(cancelButton);
		mainLayout->addLayout(hLayout);
		mainLayout->setAlignment(hLayout, Qt::AlignBottom | Qt::AlignRight);
	}
	
	setLayout(mainLayout);
	
	for (int i = 0; i < 3; ++i)
		connect(quads[i], SIGNAL(colorChanged()), listView->selectionModel(), SLOT(clear()));
	
	connect(listView->selectionModel(), SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)), this, SLOT(schemeChanged(const QModelIndex&, const QModelIndex&)));
	
	if (global) {
		schemeString = "Global Palette";
		restore();
		store();
	} else {
		QSettings settings;
		settings.beginGroup("palette");
		loadSettings(settings);
		settings.endGroup();
	}
}

PaletteDialog::~PaletteDialog() {
	if (global) {
		saveToSettingsFile();
	} else {
		QSettings settings;
		settings.beginGroup("palette");
		saveSettings(settings);
		settings.endGroup();
	}
}

void PaletteDialog::saveSettings(QSettings &settings) {
	settings.setValue("slectedScheme", schemeString);
	
	for (unsigned i = 0; i < 3; ++i)
		for (unsigned j = 0; j < 4; ++j)
			settings.setValue(quads[i]->title() + QString::number(j), currentColors[i][j]);
}

void PaletteDialog::loadSettings(QSettings &settings) {
	schemeString = settings.value("slectedScheme", global ? "Global Palette" : "Default Grey").toString();
	
	for (unsigned i = 0; i < 3; ++i)
		for (unsigned j = 0; j < 4; ++j)
			currentColors[i][j] = qvariant_cast<QRgb>(settings.value(quads[i]->title() + QString::number(j), (3 - (j & 3)) * 85 * 0x010101));
	
	restore();
	store();
}

void PaletteDialog::saveToSettingsFile() {
	if (!settingsFile.isEmpty()) {
		if (schemeString == "Global Palette") {
			QDir(savedir).remove(settingsFile);
		} else {
			QSettings settings(savedir + settingsFile, QSettings::IniFormat);
			saveSettings(settings);
		}
	}
}

void PaletteDialog::setSchemeList() {
	::setSchemeList(savedir + "stored/", global, reinterpret_cast<QStringListModel*>(listView->model()));
}

void PaletteDialog::rmScheme() {
	{
		QDir(savedir + "stored/").remove(listView->currentIndex().data().toString() + ".pal");
	}
	
	listView->selectionModel()->clear();
	setSchemeList();
}

void PaletteDialog::saveScheme() {
	bool ok;
	const QString &text = QInputDialog::getText(this, "Save Scheme", "Scheme name:",
	                                            QLineEdit::Normal, QString(), &ok);
	
	if (!ok)
		return;
	
	if (text.isEmpty() || "Global Palette" == text || "Current Scheme" == text || "Default Grey" == text ||
			text.size() > 200 || text.contains(QRegExp("[" + QRegExp::escape("<>:\"/\\|?*") + "]"))) {
		QMessageBox::information(this, "Invalid scheme name", "Invalid scheme name.");
		return;
	}
	
	{
		QSettings settings(savedir + "stored/" + text + ".pal", QSettings::IniFormat);
		
		for (unsigned i = 0; i < 3; ++i)
			for (unsigned j = 0; j < 4; ++j)
				settings.setValue(quads[i]->title() + QString::number(j), quads[i]->getColor(j));
	}
	
	setSchemeList();
	
	listView->setCurrentIndex(schemeIndexOf(listView->model(), text));
}

void PaletteDialog::schemeChanged(const QModelIndex &current, const QModelIndex &/*previous*/) {
	rmSchemeButton->setEnabled(false);
	
	if (!current.isValid())
		return;
	
	const QString &str = current.data().toString();
	
	if ("Global Palette" == str) {
		for (unsigned i = 0; i < 3; ++i)
			for (unsigned j = 0; j < 4; ++j)
				quads[i]->setColor(j, global->getColor(i, j));
	} else if ("Current Scheme" == str) {
		for (unsigned i = 0; i < 3; ++i)
			for (unsigned j = 0; j < 4; ++j)
				quads[i]->setColor(j, currentColors[i][j]);
	} else if("Default Grey" == str) {
		for (unsigned i = 0; i < 3; ++i)
			for (unsigned j = 0; j < 4; ++j)
				quads[i]->setColor(j, (3 - (j & 3)) * 85 * 0x010101);
	} else {
		QSettings settings(savedir + "stored/" + str + ".pal", QSettings::IniFormat);
		
		for (unsigned i = 0; i < 3; ++i)
			for (unsigned j = 0; j < 4; ++j)
				quads[i]->setColor(j, qvariant_cast<QRgb>(settings.value(quads[i]->title() + QString::number(j), 0)));
		
		rmSchemeButton->setEnabled(true);
	}
}

void PaletteDialog::store() {
	for (unsigned i = 0; i < 3; ++i)
		for (unsigned j = 0; j < 4; ++j)
			currentColors[i][j] = quads[i]->getColor(j);
	
	if (!listView->currentIndex().isValid())
		listView->setCurrentIndex(schemeIndexOf(listView->model(), "Current Scheme")); //obs: will emit currentChanged()
	
	schemeString = listView->currentIndex().data().toString();
}

void PaletteDialog::restore() {
	listView->setCurrentIndex(schemeIndexOf(listView->model(), schemeString));
}

void PaletteDialog::externalChange() {
	setSchemeList();
	restore();
	store();
}

void PaletteDialog::setSettingsFile(const QString &filename) {
	saveToSettingsFile();
	
	settingsFile = filename;
	
	QSettings settings(savedir + settingsFile, QSettings::IniFormat);
	loadSettings(settings);
}

void PaletteDialog::accept() {
	store();
	QDialog::accept();
}

void PaletteDialog::reject() {
	restore();
// 	QDialog::reject();
	QDialog::accept();
}
