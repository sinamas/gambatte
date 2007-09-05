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

#include <algorithm>
#include <cstring>

struct OptionLess {
	bool operator()(const Parser::Option *const l, const Parser::Option *const r) const {
		return l->getHash() < r->getHash();
	}
};

static unsigned computeHash(const char *s) {
	unsigned hash = 0;
	
	while (*s) {
		hash += *s++;
		hash += hash << 10;
		hash ^= hash >> 6;
	}
	
	hash += hash << 3;
	hash ^= hash >> 11;
	hash += hash << 15;
	
	return hash;
}

Parser::Option::Option(const char *const s, int nArgs) : s(s), hash(computeHash(s)), nArgs(nArgs) {}

Parser::Parser() : sorted(false) {
	std::memset(t, 0, sizeof(t));
}

void Parser::addLong(Option *const o) {
	v.push_back(o);
	sorted = false;
}

int Parser::parseLong(const int argc, const char *const *const argv, const int index) {
	if (!sorted)
		std::sort(v.begin(), v.end(), OptionLess());
	
	sorted = true;
	
	Option o(argv[index]);
	std::vector<Option*>::iterator it = lower_bound(v.begin(), v.end(), &o, OptionLess());
	
	while (it != v.end() && (*it)->getHash() == o.getHash()) {
		if (!std::strcmp((*it)->getStr(), o.getStr())) {
			if ((*it)->neededArgs() >= argc - index)
				return 0;
				
			(*it)->exec(argv, index);
			return index + (*it)->neededArgs();
		}
		
		++it;
	}
	
	return 0;
}

void Parser::addShort(Option *const o) {
	t[static_cast<unsigned char>(o->getStr()[1])] = o;
}

int Parser::parseShort(const int argc, const char *const *const argv, const int index) {
	const char *s = argv[index];
	++s;
	
	if (!(*s))
		return 0;

	do {
		Option *const o = t[static_cast<unsigned char>(*s)];
		
		if (!o)
			return 0;
			
		if (o->neededArgs()) {
			if (s[1] || o->neededArgs() >= argc - index)
				return 0;
				
			o->exec(argv, index);
			return index + o->neededArgs();
		}
		
		o->exec(argv, index);
	} while (*++s);
	
	return index;
}

void Parser::add(Option *const o) {
	(o->getStr()[1] == '-') ? addLong(o) : addShort(o);
}

int Parser::parse(const int argc, const char *const *const argv, const int index) {
	return (argv[index][1] == '-') ? parseLong(argc, argv, index) : parseShort(argc, argv, index);
}
