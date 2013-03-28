/***************************************************************************
 *   Copyright (C) 2011 by Sindre Aamås                                    *
 *   sinamas@users.sourceforge.net                                         *
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
#ifndef CHEATDIALOG_H
#define CHEATDIALOG_H

#include <QDialog>
#include <QString>
#include <vector>

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
	explicit GetCheatInput(QString const &desc, QString const &code, QWidget *parent);
	QString const codeText() const;
	QString const descText() const;

private:
	Q_OBJECT

	class QLineEdit *const codeEdit_;
	class QLineEdit *const descEdit_;
	class QPushButton *const okButton_;

private slots:
	void codeTextEdited(QString const &);
};

class CheatDialog : public QDialog {
public:
	explicit CheatDialog(QString const &savefile, QWidget *parent = 0);
	~CheatDialog();
	QString const cheats() const;
	void setGameName(QString const &name);

public slots:
	virtual void accept();
	virtual void reject();

private:
	Q_OBJECT

	class QListView *const view_;
	class QPushButton *const editButton_;
	class QPushButton *const rmButton_;
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
	void selectionChanged(class QModelIndex const &current, class QModelIndex const &last);
};

#endif
