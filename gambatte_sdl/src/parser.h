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

#include <map>
#include <cstring>

class Parser {
public:
	class Option {
		const char *const s;
		const int nArgs;
		const char c;
		
	public:
		Option(const char *s, char c = 0, int nArgs = 0);
		virtual ~Option() {}
		char getChar() const { return c; }
		const char* getStr() const { return s; }
		int neededArgs() const { return nArgs; }
		virtual void exec(const char *const */*argv*/, int /*index*/) {}
	};
	
private:
	struct StrLess {
		bool operator()(const char *const l, const char *const r) const {
			return std::strcmp(l, r) < 0;
		}
	};
	
	typedef std::map<char,Option*> smap_t;
	typedef std::map<const char*,Option*,StrLess> lmap_t;

	smap_t sMap;
	lmap_t lMap;

	void addLong(Option *o);
	void addShort(Option *o);
	int parseLong(int argc, const char *const *argv, int index);
	int parseShort(int argc, const char *const *argv, int index);
	
public:
	void add(Option *o);
	int parse(int argc, const char *const *argv, int index);
};

#endif
