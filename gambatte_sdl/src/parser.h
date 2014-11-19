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

#ifndef PARSER_H
#define PARSER_H

#include <cstring>
#include <map>

class Parser {
public:
	class Option {
	public:
		explicit Option(char const *s, char c = 0, int nArgs = 0);
		virtual ~Option() {}
		virtual void exec(char const *const *argv, int index) = 0;
		char character() const { return c_; }
		char const * str() const { return s_; }
		int neededArgs() const { return nArgs_; }

	private:
		char const *const s_;
		int const nArgs_;
		char const c_;
	};

	void add(Option *o);
	int parse(int argc, char const *const *argv, int index) const;

private:
	struct StrLess {
		bool operator()(char const *l, char const *r) const {
			return std::strcmp(l, r) < 0;
		}
	};

	typedef std::map<char, Option *> smap_t;
	typedef std::map<char const *, Option *, StrLess> lmap_t;

	smap_t sMap;
	lmap_t lMap;

	void addLong(Option *o);
	void addShort(Option *o);
	int parseLong(int argc, char const *const *argv, int index) const;
	int parseShort(int argc, char const *const *argv, int index) const;
};

#endif
