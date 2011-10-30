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
#ifndef CHEATDIALOG_H
#define CHEATDIALOG_H

#include <QDialog>
#include <QString>
#include <vector>

struct CheatListItem {
	QString label;
	QString code;
	bool checked;
	
	CheatListItem(const QString &label, const QString &code, const bool checked)
	: label(label), code(code), checked(checked)
	{
	}
};

class GetCheatInput : public QDialog {
	Q_OBJECT
	
	class QLineEdit *const codeEdit_;
	class QLineEdit *const descEdit_;
	class QPushButton *const okButton_;
	
private slots:
	void codeTextEdited(const QString&);
	
public:
	explicit GetCheatInput(const QString &desc, const QString &code, QWidget *parent);
	const QString codeText() const;
	const QString descText() const;
};

class CheatDialog : public QDialog {
	Q_OBJECT
	
	class QListView *const view_;
	class QPushButton *const editButton_;
	class QPushButton *const rmButton_;
	std::vector<CheatListItem> items_;
	const QString savefile_;
	QString gamename_;
	
	void loadFromSettingsFile();
	void saveToSettingsFile();
	void resetViewModel(const std::vector<CheatListItem> &items);
	void resetViewModel(const std::vector<CheatListItem> &items, int newCurRow);
	
private slots:
	void addCheat();
	void editCheat();
	void removeCheat();
	void selectionChanged(const class QModelIndex &current, const class QModelIndex &last);
	
public:
	explicit CheatDialog(const QString &savefile, QWidget *parent = 0);
	~CheatDialog();
	const QString cheats() const;
	void setGameName(const QString &name);
	
public slots:
	virtual void accept();
	virtual void reject();
};

#endif
