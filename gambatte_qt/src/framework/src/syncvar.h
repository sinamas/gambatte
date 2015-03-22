//
//   Copyright (C) 2009 by sinamas <sinamas at users.sourceforge.net>
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

#ifndef SYNC_VAR_H
#define SYNC_VAR_H

#include <QMutex>
#include <QWaitCondition>

class SyncVar {
public:
	class Locked : Uncopyable {
	public:
		Locked(SyncVar &sv) : sv(sv) { sv.mut_.lock(); }
		~Locked() { sv.mut_.unlock(); }
		unsigned get() const { return sv.var_; }
		void set(unsigned var) { sv.var_ = var; sv.cond_.wakeAll(); }
		bool wait(unsigned long time = ULONG_MAX) { return sv.cond_.wait(&sv.mut_, time); }

	private:
		SyncVar &sv;
	};

	explicit SyncVar(unsigned var = 0) : var_(var) {}

private:
	QMutex mut_;
	QWaitCondition cond_;
	unsigned var_;
};

#endif
