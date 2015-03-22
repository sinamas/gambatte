//
//   Copyright (C) 2007 by sinamas <sinamas at users.sourceforge.net>
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

#include "parser.h"
#include <utility>

Parser::Option::Option(char const *s, char c, int nArgs)
: s_(s)
, nArgs_(nArgs)
, c_(c)
{
}

void Parser::addLong(Option *o) {
	lMap.insert(std::make_pair(o->str(), o));
}

int Parser::parseLong(int const argc, char const *const argv[], int const index) const {
	lmap_t::const_iterator it = lMap.find(argv[index] + 2);
	if (it == lMap.end())
		return 0;

	Option &e = *(it->second);
	if (e.neededArgs() >= argc - index)
		return 0;

	e.exec(argv, index);
	return index + e.neededArgs();
}

void Parser::addShort(Option *o) {
	sMap.insert(std::make_pair(o->character(), o));
}

int Parser::parseShort(int const argc, char const *const argv[], int const index) const {
	char const *s = argv[index];
	++s;

	if (!(*s))
		return 0;

	do {
		smap_t::const_iterator const it = sMap.find(*s);
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

void Parser::add(Option *o) {
	addLong(o);

	if (o->character())
		addShort(o);
}

int Parser::parse(int argc, char const *const *argv, int index) const {
	return argv[index][1] == '-'
	     ? parseLong(argc, argv, index)
	     : parseShort(argc, argv, index);
}
