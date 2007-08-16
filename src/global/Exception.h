/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006 The University of Sydney, Australia
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

#ifndef GPLATES_GLOBAL_EXCEPTION_H
#define GPLATES_GLOBAL_EXCEPTION_H

#include <iosfwd>
#include <string>

namespace GPlatesGlobal
{
	/**
	 * Generic exception.
	 */
	class Exception
	{
		public:
			virtual
			~Exception() {  }

			/**
			 * Insert the name and message (if it exists) of this
			 * Exception into the given output stream.
			 */
			void
			Write(std::ostream &os) const;

		protected:
			/**
			 * @return The name of this Exception.
			 */
			virtual const char *
			ExceptionName() const = 0;

			/**
			 * @return The Exception's message as a string.
			 */
			virtual std::string
			Message() const = 0;
	};

	/**
	 * Synonym/shorthand for Exception::Write().
	 */
	inline std::ostream &
	operator<<(std::ostream &os, const Exception &ex) {

		ex.Write(os);
		return os;
	}
}

#endif  // GPLATES_GLOBAL_EXCEPTION_H
