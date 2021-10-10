/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2005, 2006 The University of Sydney, Australia
 *
 * This file is part of GPlates.
 *
 * GPlates is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 2, as published by
 * the Free Software Foundation.
 *
 * GPlates is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef GPLATES_GLOBAL_INTERNALINCONSISTENCYEXCEPTION_H
#define GPLATES_GLOBAL_INTERNALINCONSISTENCYEXCEPTION_H

#include "GPlatesException.h"

namespace GPlatesGlobal {

	/**
	 * Should be thrown when an unexpected internal inconsistency is
	 * detected.
	 * 
	 * As good programmers, we program defensively, double-checking
	 * everything which the compiler doesn't explicitly guarantee, right?
	 * Of course.
	 *
	 * Like maybe we're expecting a particular item to be contained in a
	 * list.  But instead of just doing a @a std::find on the list and
	 * assuming the iterator points to the expected item, we first ensure
	 * the iterator isn't @a list::end (which would mean the expected item
	 * was not found).  After all: we're hosed either way, but when we're
	 * digging through the smouldering rubble trying to work out what went
	 * wrong, we'll be glad of the extra information (an exception with a
	 * (hopefully) descriptive message instead of an uninformative segfault
	 * or, worse yet, some kind of delayed reaction).
	 */
	class InternalInconsistencyException: public Exception {

	 public:

		/**
		 * @param file is the filename as provided by __FILE__.
		 * @param line is the line number as provided by __LINE__.
		 * @param msg is a message describing the situation.
		 */
		InternalInconsistencyException(
		 const std::string &file,
		 int line,
		 const std::string &msg) :
		 m_file(file),
		 m_line(line),
		 m_msg(msg) {  }

		virtual
		~InternalInconsistencyException() {  }

	 protected:

		virtual
		const char *
		ExceptionName() const {

			return "InternalInconsistencyException";
		}

		virtual
		std::string
		Message() const;

	 private:

		std::string m_file;
		int m_line;
		std::string m_msg;

	};
}

#endif  // GPLATES_GLOBAL_INTERNALINCONSISTENCYEXCEPTION_H
