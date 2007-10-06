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
#include "parser.h"

Parser::Option::Option(const char *const s, const char c, const int nArgs) : s(s), nArgs(nArgs), c(c) {}

void Parser::addLong(Option *const o) {
	lMap.insert(std::pair<const char*,Option*>(o->getStr(), o));
}

int Parser::parseLong(const int argc, const char *const *const argv, const int index) {
	lmap_t::iterator it = lMap.find(argv[index] + 2);
	
	if (it == lMap.end())
		return 0;
	
	Option &e = *(it->second);
	
	if (e.neededArgs() >= argc - index)
		return 0;
		
	e.exec(argv, index);
	
	return index + e.neededArgs();
}

void Parser::addShort(Option *const o) {
	sMap.insert(std::pair<char,Option*>(o->getChar(), o));
}

int Parser::parseShort(const int argc, const char *const *const argv, const int index) {
	const char *s = argv[index];
	++s;
	
	if (!(*s))
		return 0;

	do {
		const smap_t::iterator it = sMap.find(*s);
		
		if (it == sMap.end())
			return 0;
		
		Option &e = *(it->second);
			
		if (e.neededArgs()) {
			if (s[1] || e.neededArgs() >= argc - index)
				return 0;
				
			e.exec(argv, index);
			return index + e.neededArgs();
		}
		
		e.exec(argv, index);
	} while (*++s);
	
	return index;
}

void Parser::add(Option *const o) {
	addLong(o);
	
	if (o->getChar())
		addShort(o);
}

int Parser::parse(const int argc, const char *const *const argv, const int index) {
	return (argv[index][1] == '-') ? parseLong(argc, argv, index) : parseShort(argc, argv, index);
}
