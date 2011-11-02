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
#include "cheatdialog.h"
#include <QAbstractListModel>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QListView>
#include <QMap>
#include <QPushButton>
#include <QRegExp>
#include <QRegExpValidator>
#include <QSettings>
#include <QStringList>
#include <QVariant>
#include <QVBoxLayout>
#include <algorithm>
#include <memory>

namespace {

struct CheatItemLess {
	bool operator()(const CheatListItem &lhs, const CheatListItem &rhs) const {
		return lhs.label + lhs.code < rhs.label + rhs.code;
	}
};

class CheatListModel : public QAbstractListModel {
	std::vector<CheatListItem> items_;
	
public:
	explicit CheatListModel(QObject *parent = 0) : QAbstractListModel(parent) {}
	explicit CheatListModel(const std::vector<CheatListItem> &items, QObject *parent = 0)
	: QAbstractListModel(parent), items_(items)
	{
	}
	
	virtual int rowCount(const QModelIndex&) const { return items_.size(); }
	
	virtual Qt::ItemFlags flags(const QModelIndex &index) const {
		return QAbstractListModel::flags(index) | Qt::ItemIsUserCheckable;
	}
	
	virtual QVariant data(const QModelIndex &index, const int role) const {
		if (static_cast<std::size_t>(index.row()) < items_.size()) {
			switch (role) {
			case Qt::DisplayRole: return items_[index.row()].label + " (" + items_[index.row()].code + ")";
			case Qt::CheckStateRole: return items_[index.row()].checked ? Qt::Checked : Qt::Unchecked;
			}
		}
		
		return QVariant();
	}
	
	virtual bool setData(const QModelIndex &index, const QVariant &value, const int role) {
		if (static_cast<std::size_t>(index.row()) < items_.size() && role == Qt::CheckStateRole) {
			items_[index.row()].checked = value.toBool();
			return true;
		}
		
		return false;
	}
	
	const std::vector<CheatListItem> & items() const { return items_; }
};

}

GetCheatInput::GetCheatInput(const QString &desc, const QString &code, QWidget *const parent)
: QDialog(parent),
  codeEdit_(new QLineEdit(code)),
  descEdit_(new QLineEdit(desc)),
  okButton_(new QPushButton(tr("OK")))
{
	QVBoxLayout *const l = new QVBoxLayout;
	setLayout(l);
	l->addWidget(new QLabel(tr("Description:")));
	l->addWidget(descEdit_);
	l->addWidget(new QLabel(tr("GG/GS Code:")));
	l->addWidget(codeEdit_);
	
	const QString cheatre("((01[0-9a-fA-F]{6,6})|([0-9a-fA-F]{3,3}-[0-9a-fA-F]{3,3}(-[0-9a-fA-F]{3,3})?))");
	codeEdit_->setValidator(new QRegExpValidator(QRegExp(cheatre + "(;" + cheatre + ")*"), codeEdit_));
	codeEdit_->setToolTip(tr("Game Genie: hhh-hhh-hhh;...\nGame Shark: 01hhhhhh;..."));
	codeEdit_->setWhatsThis(codeEdit_->toolTip());
	
	QHBoxLayout *const hl = new QHBoxLayout;
	l->addLayout(hl);
	l->setAlignment(hl, Qt::AlignBottom | Qt::AlignRight);
	hl->addWidget(okButton_);
	QPushButton *const cancelButton = new QPushButton(tr("Cancel"));
	hl->addWidget(cancelButton);
	
	okButton_->setEnabled(codeEdit_->hasAcceptableInput());
	connect(codeEdit_, SIGNAL(textChanged(const QString&)), this, SLOT(codeTextEdited(const QString&)));
	connect(okButton_, SIGNAL(clicked()), this, SLOT(accept()));
	connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
}

void GetCheatInput::codeTextEdited(const QString&) {
	okButton_->setEnabled(codeEdit_->hasAcceptableInput());
}

const QString GetCheatInput::codeText() const {
	return codeEdit_->text().toUpper();
}

const QString GetCheatInput::descText() const {
	return descEdit_->text();
}

CheatDialog::CheatDialog(const QString &savefile, QWidget *const parent)
: QDialog(parent),
  view_(new QListView()),
  editButton_(new QPushButton(tr("Edit..."))),
  rmButton_(new QPushButton(tr("Remove"))),
  savefile_(savefile)
{
	setWindowTitle("Cheats");
	
	QVBoxLayout *const mainLayout = new QVBoxLayout;
	setLayout(mainLayout);
	QVBoxLayout *const viewLayout = new QVBoxLayout;
	mainLayout->addLayout(viewLayout);
	viewLayout->addWidget(view_);
	resetViewModel(items_);
	
	{
		QPushButton *const addButton = new QPushButton("Add...");
		viewLayout->addWidget(addButton);
		connect(addButton, SIGNAL(clicked()), this, SLOT(addCheat()));
	}
	
	viewLayout->addWidget(editButton_);
	connect(editButton_, SIGNAL(clicked()), this, SLOT(editCheat()));
	
	viewLayout->addWidget(rmButton_);
	connect(rmButton_, SIGNAL(clicked()), this, SLOT(removeCheat()));
	
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
}

CheatDialog::~CheatDialog() {
	saveToSettingsFile();
}

void CheatDialog::loadFromSettingsFile() {
	items_.clear();
	
	if (!gamename_.isEmpty()) {
		QSettings settings(savefile_, QSettings::IniFormat);
		settings.beginGroup(gamename_);
		
		foreach (const QString &key, settings.childKeys()) {
			const QStringList &l = settings.value(key).toStringList();
			
			if (1 < l.size())
				items_.push_back(CheatListItem(l[0], l[1], 2 < l.size() && l[2] == "on"));
		}
		
		std::sort(items_.begin(), items_.end(), CheatItemLess());
	}
	
	resetViewModel(items_);
}

void CheatDialog::saveToSettingsFile() {
	if (!gamename_.isEmpty()) {
		QSettings settings(savefile_, QSettings::IniFormat);
		settings.beginGroup(gamename_);
		settings.remove("");
		
		for (std::size_t i = 0; i < items_.size(); ++i) {
			QStringList l;
			l.append(items_[i].label);
			l.append(items_[i].code);
			
			if (items_[i].checked)
				l.append("on");
			
			settings.setValue(QString::number(i), l);
		}
	}
}

void CheatDialog::resetViewModel(const std::vector<CheatListItem> &items) {
	resetViewModel(items, view_->currentIndex().row());
}

void CheatDialog::resetViewModel(const std::vector<CheatListItem> &items, const int newCurRow) {
	const std::auto_ptr<QAbstractItemModel> oldModel(view_->model());
	view_->setModel(new CheatListModel(items, this));
	view_->setCurrentIndex(view_->model()->index(newCurRow, 0, QModelIndex()));
	selectionChanged(view_->selectionModel()->currentIndex(), QModelIndex());
	connect(view_->selectionModel(), SIGNAL(currentChanged(const QModelIndex&, const QModelIndex&)),
					this, SLOT(selectionChanged(const QModelIndex&, const QModelIndex&)));
}

void CheatDialog::addCheat() {
	const std::auto_ptr<GetCheatInput> getCheatDialog(new GetCheatInput(QString(), QString(), this));
	getCheatDialog->setWindowTitle(tr("Add Cheat"));
	
	if (getCheatDialog->exec()) {
		std::vector<CheatListItem> items = reinterpret_cast<CheatListModel*>(view_->model())->items();
		const CheatListItem item(getCheatDialog->descText(), getCheatDialog->codeText(), false);
		const std::vector<CheatListItem>::iterator it =
				items.insert(std::lower_bound(items.begin(), items.end(), item, CheatItemLess()), item);
		
		resetViewModel(items, it - items.begin());
	}
}

void CheatDialog::editCheat() {
	const std::size_t row = view_->selectionModel()->currentIndex().row();
	std::vector<CheatListItem> items = reinterpret_cast<CheatListModel*>(view_->model())->items();
	
	if (row < items.size()) {
		const std::auto_ptr<GetCheatInput> getCheatDialog(new GetCheatInput(items[row].label, items[row].code, this));
		getCheatDialog->setWindowTitle(tr("Edit Cheat"));
		
		if (getCheatDialog->exec()) {
			const CheatListItem item(getCheatDialog->descText(), getCheatDialog->codeText(), items[row].checked);
			items.erase(items.begin() + row);
			
			const std::vector<CheatListItem>::iterator it =
					items.insert(std::lower_bound(items.begin(), items.end(), item, CheatItemLess()), item);
			
			resetViewModel(items, it - items.begin());
		}
	}
}

void CheatDialog::removeCheat() {
	if (view_->selectionModel()->currentIndex().isValid()) {
		const std::size_t row = view_->selectionModel()->currentIndex().row();
		std::vector<CheatListItem> items = reinterpret_cast<CheatListModel*>(view_->model())->items();
		
		if (row < items.size()) {
			items.erase(items.begin() + row);
			resetViewModel(items, row);
		}
	}
}

void CheatDialog::selectionChanged(const QModelIndex &current, const QModelIndex&) {
	editButton_->setEnabled(current.isValid());
	rmButton_->setEnabled(current.isValid());
}

const QString CheatDialog::cheats() const {
	QString s;
	
	for (std::size_t i = 0; i < items_.size(); ++i) {
		if (items_[i].checked)
			s += items_[i].code + ";";
	}
	
	return s;
}

void CheatDialog::setGameName(const QString &name) {
	saveToSettingsFile();
	gamename_ = name;
	loadFromSettingsFile();
}

void CheatDialog::accept() {
	items_ = reinterpret_cast<CheatListModel*>(view_->model())->items();
	QDialog::accept();
}

void CheatDialog::reject() {
	resetViewModel(items_);
	QDialog::reject();
}
