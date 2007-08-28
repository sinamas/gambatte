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
#ifndef PARSER_H
#define PARSER_H

#include <vector>

class Parser {
public:
	class Option {
		const char *const s;
		const unsigned hash;
		const int nArgs;
		
	public:
		Option(const char *s, int nArgs = 0);
		virtual ~Option() {}
		unsigned getHash() const { return hash; }
		const char* getStr() const { return s; }
		int neededArgs() const { return nArgs; }
		virtual void exec(const char *const */*argv*/, int /*index*/) {}
	};
	
private:
	Option *t[256];
	std::vector<Option*> v;
	bool sorted;

	void addLong(Option *o);
	void addShort(Option *o);
	int parseLong(int argc, const char *const *argv, int index);
	int parseShort(int argc, const char *const *argv, int index);
	
public:
	Parser();
	void add(Option *o);
	int parse(int argc,const char *const *argv, int index);
};

#endif
