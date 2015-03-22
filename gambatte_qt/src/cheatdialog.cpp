//
//   Copyright (C) 2011 by sinamas <sinamas at users.sourceforge.net>
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

#include "cheatdialog.h"
#include "dialoghelpers.h"
#include "scoped_ptr.h"
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

namespace {

struct CheatItemLess {
	bool operator()(CheatListItem const &lhs, CheatListItem const &rhs) const {
		return lhs.label + lhs.code < rhs.label + rhs.code;
	}
};

class CheatListModel : public QAbstractListModel {
public:
	explicit CheatListModel(QObject *parent = 0) : QAbstractListModel(parent) {}
	explicit CheatListModel(std::vector<CheatListItem> const &items, QObject *parent = 0)
	: QAbstractListModel(parent), items_(items)
	{
	}

	virtual int rowCount(QModelIndex const &) const { return items_.size(); }

	virtual Qt::ItemFlags flags(QModelIndex const &index) const {
		return QAbstractListModel::flags(index) | Qt::ItemIsUserCheckable;
	}

	virtual QVariant data(QModelIndex const &index, int const role) const {
		if (static_cast<std::size_t>(index.row()) < items_.size()) {
			switch (role) {
			case Qt::DisplayRole:
				return items_[index.row()].label
				     + " (" + items_[index.row()].code + ')';
			case Qt::CheckStateRole:
				return items_[index.row()].checked
				     ? Qt::Checked
				     : Qt::Unchecked;
			}
		}

		return QVariant();
	}

	virtual bool setData(QModelIndex const &index, QVariant const &value, int role) {
		if (static_cast<std::size_t>(index.row()) < items_.size()
				&& role == Qt::CheckStateRole) {
			items_[index.row()].checked = value.toBool();
			return true;
		}

		return false;
	}

	std::vector<CheatListItem> const & items() const { return items_; }

private:
	std::vector<CheatListItem> items_;
};

} // anon ns

GetCheatInput::GetCheatInput(QString const &desc, QString const &code, QWidget *parent)
: QDialog(parent)
, descEdit_(new QLineEdit(desc, this))
, codeEdit_(new QLineEdit(code, this))
, okButton_(new QPushButton(tr("OK"), this))
{
	QVBoxLayout *const l = new QVBoxLayout(this);
	l->addWidget(new QLabel(tr("Description:")));
	l->addWidget(descEdit_);
	l->addWidget(new QLabel(tr("GG/GS Code:")));
	l->addWidget(codeEdit_);

	QString const cheatre("((01[0-9a-fA-F]{6,6})|([0-9a-fA-F]{3,3}-[0-9a-fA-F]{3,3}(-[0-9a-fA-F]{3,3})?))");
	codeEdit_->setValidator(new QRegExpValidator(QRegExp(cheatre + "(;" + cheatre + ")*"),
	                                             codeEdit_));
	codeEdit_->setToolTip(tr("Game Genie: hhh-hhh-hhh;...\nGame Shark: 01hhhhhh;..."));
	codeEdit_->setWhatsThis(codeEdit_->toolTip());

	QHBoxLayout *const hl = addLayout(l, new QHBoxLayout,
	                                  Qt::AlignBottom | Qt::AlignRight);
	hl->addWidget(okButton_);
	QPushButton *const cancelButton = addWidget(hl, new QPushButton(tr("Cancel")));
	okButton_->setEnabled(codeEdit_->hasAcceptableInput());
	connect(codeEdit_, SIGNAL(textChanged(QString const &)),
	        this, SLOT(codeTextEdited()));
	connect(okButton_, SIGNAL(clicked()), this, SLOT(accept()));
	connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
}

void GetCheatInput::codeTextEdited() {
	okButton_->setEnabled(codeEdit_->hasAcceptableInput());
}

QString const GetCheatInput::codeText() const {
	return codeEdit_->text().toUpper();
}

QString const GetCheatInput::descText() const {
	return descEdit_->text();
}

CheatDialog::CheatDialog(QString const &savefile, QWidget *parent)
: QDialog(parent)
, view_(new QListView(this))
, editButton_(new QPushButton(tr("Edit..."), this))
, rmButton_(new QPushButton(tr("Remove"), this))
, savefile_(savefile)
{
	setWindowTitle("Cheats");

	QVBoxLayout *const mainLayout = new QVBoxLayout(this);
	QVBoxLayout *const viewLayout = addLayout(mainLayout, new QVBoxLayout);
	viewLayout->addWidget(view_);
	resetViewModel(items_);

	QPushButton *const addButton = addWidget(viewLayout, new QPushButton(tr("Add...")));
	connect(addButton,  SIGNAL(clicked()), this, SLOT(addCheat()));
	editButton_->setParent(0); // tab order reparent
	viewLayout->addWidget(editButton_);
	connect(editButton_, SIGNAL(clicked()), this, SLOT(editCheat()));
	rmButton_->setParent(0); // tab order reparent
	viewLayout->addWidget(rmButton_);
	connect(rmButton_, SIGNAL(clicked()), this, SLOT(removeCheat()));

	QBoxLayout *const hLayout = addLayout(mainLayout, new QHBoxLayout,
	                                      Qt::AlignBottom | Qt::AlignRight);
	QPushButton *const okButton = addWidget(hLayout, new QPushButton(tr("OK")));
	QPushButton *const cancelButton = addWidget(hLayout, new QPushButton(tr("Cancel")));
	okButton->setDefault(true);
	connect(okButton, SIGNAL(clicked()), this, SLOT(accept()));
	connect(cancelButton, SIGNAL(clicked()), this, SLOT(reject()));
}

CheatDialog::~CheatDialog() {
	saveToSettingsFile();
}

void CheatDialog::loadFromSettingsFile() {
	items_.clear();

	if (!gamename_.isEmpty()) {
		QSettings settings(savefile_, QSettings::IniFormat);
		settings.beginGroup(gamename_);

		foreach (QString const &key, settings.childKeys()) {
			QStringList const &l = settings.value(key).toStringList();
			if (1 < l.size()) {
				CheatListItem item(l[0], l[1],
				                   2 < l.size() && l[2] == "on");
				items_.push_back(item);
			}
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

void CheatDialog::resetViewModel(std::vector<CheatListItem> const &items) {
	resetViewModel(items, view_->currentIndex().row());
}

void CheatDialog::resetViewModel(std::vector<CheatListItem> const &items, int const newCurRow) {
	scoped_ptr<QAbstractItemModel> const oldModel(view_->model());
	view_->setModel(new CheatListModel(items, this));
	view_->setCurrentIndex(view_->model()->index(newCurRow, 0, QModelIndex()));
	selectionChanged(view_->selectionModel()->currentIndex());
	connect(view_->selectionModel(),
	        SIGNAL(currentChanged(QModelIndex const &, QModelIndex const &)),
	        this, SLOT(selectionChanged(QModelIndex const &)));
}

void CheatDialog::addCheat() {
	GetCheatInput getCheatDialog(QString(), QString(), this);
	getCheatDialog.setWindowTitle(tr("Add Cheat"));
	if (getCheatDialog.exec()) {
		std::vector<CheatListItem> items =
			static_cast<CheatListModel *>(view_->model())->items();
		CheatListItem const item(getCheatDialog.descText(),
		                         getCheatDialog.codeText(),
		                         false);
		std::vector<CheatListItem>::iterator it =
			items.insert(std::lower_bound(items.begin(), items.end(), item,
			                              CheatItemLess()),
			             item);
		resetViewModel(items, it - items.begin());
	}
}

void CheatDialog::editCheat() {
	std::size_t const row = view_->selectionModel()->currentIndex().row();
	std::vector<CheatListItem> items = static_cast<CheatListModel *>(view_->model())->items();

	if (row < items.size()) {
		GetCheatInput getCheatDialog(items[row].label, items[row].code, this);
		getCheatDialog.setWindowTitle(tr("Edit Cheat"));
		if (getCheatDialog.exec()) {
			CheatListItem const item(getCheatDialog.descText(),
			                         getCheatDialog.codeText(),
			                         items[row].checked);
			items.erase(items.begin() + row);

			std::vector<CheatListItem>::iterator it =
				items.insert(std::lower_bound(items.begin(), items.end(), item,
				                              CheatItemLess()),
				             item);
			resetViewModel(items, it - items.begin());
		}
	}
}

void CheatDialog::removeCheat() {
	if (view_->selectionModel()->currentIndex().isValid()) {
		std::size_t const row = view_->selectionModel()->currentIndex().row();
		std::vector<CheatListItem> items =
			static_cast<CheatListModel *>(view_->model())->items();
		if (row < items.size()) {
			items.erase(items.begin() + row);
			resetViewModel(items, row);
		}
	}
}

void CheatDialog::selectionChanged(QModelIndex const &current) {
	editButton_->setEnabled(current.isValid());
	rmButton_->setEnabled(current.isValid());
}

QString const CheatDialog::cheats() const {
	QString s;
	for (std::size_t i = 0; i < items_.size(); ++i) {
		if (items_[i].checked)
			s += items_[i].code + ';';
	}

	return s;
}

void CheatDialog::setGameName(QString const &name) {
	saveToSettingsFile();
	gamename_ = name;
	loadFromSettingsFile();
}

void CheatDialog::accept() {
	items_ = static_cast<CheatListModel *>(view_->model())->items();
	QDialog::accept();
}

void CheatDialog::reject() {
	resetViewModel(items_);
	QDialog::reject();
}
