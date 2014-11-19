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

#ifndef CHEATDIALOG_H
#define CHEATDIALOG_H

#include <QDialog>
#include <QString>
#include <vector>

class QLineEdit;
class QListView;
class QModelIndex;
class QPushButton;

struct CheatListItem {
	QString label;
	QString code;
	bool checked;

	CheatListItem(QString const &label, QString const &code, bool checked)
	: label(label), code(code), checked(checked)
	{
	}
};

class GetCheatInput : public QDialog {
public:
	GetCheatInput(QString const &desc, QString const &code, QWidget *parent);
	QString const codeText() const;
	QString const descText() const;

private:
	Q_OBJECT

	QLineEdit *const descEdit_;
	QLineEdit *const codeEdit_;
	QPushButton *const okButton_;

private slots:
	void codeTextEdited();
};

class CheatDialog : public QDialog {
public:
	explicit CheatDialog(QString const &savefile, QWidget *parent = 0);
	virtual ~CheatDialog();
	QString const cheats() const;
	void setGameName(QString const &name);

public slots:
	virtual void accept();
	virtual void reject();

private:
	Q_OBJECT

	QListView *const view_;
	QPushButton *const editButton_;
	QPushButton *const rmButton_;
	std::vector<CheatListItem> items_;
	QString const savefile_;
	QString gamename_;

	void loadFromSettingsFile();
	void saveToSettingsFile();
	void resetViewModel(std::vector<CheatListItem> const &items);
	void resetViewModel(std::vector<CheatListItem> const &items, int newCurRow);

private slots:
	void addCheat();
	void editCheat();
	void removeCheat();
	void selectionChanged(QModelIndex const &current);
};

#endif
