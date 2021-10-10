/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006, 2007 The University of Sydney, Australia
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
#include <boost/current_function.hpp>

#define GPLATES_EXCEPTION_SOURCE __FILE__, __LINE__, BOOST_CURRENT_FUNCTION


namespace GPlatesGlobal
{
	/**
	 * This is the base class of all exceptions in GPlates.
	 */
	class Exception
	{
		public:
			// FIXME:  This class should have a constructor which accepts:
			//  - a const char *filename
			//  - an int line-number
			//  - a const char *funcname
			// or possibly an instance of a class (which will look like
			// CallStackTracker) whose constructor accepts these items).

			virtual
			~Exception() {  }

			/**
			 * Write the name and message of an exception into the supplied output
			 * stream @a os.
			 *
			 * It is not intended that these messages be internationalised for users --
			 * they are purely for debugging output when an exception is caught at the
			 * base-most frame of the function call stack.
			 */
			void
			write(
					std::ostream &os) const;

		protected:
			/**
			 * @return The name of this Exception.
			 *
			 * FIXME:  This should be renamed to 'exception_name'.
			 */
			virtual
			const char *
			ExceptionName() const = 0;

			/**
			 * @return The Exception's message as a string.
			 *
			 * FIXME:  Rather than creating a string for the message, there should be a
			 * 'write_message' function which accepts this same ostream.
			 */
			virtual
			std::string
			Message() const
			{
				return std::string();
			}

			virtual
			void
			write_message(
					std::ostream &os) const
			{  }
	};


	/**
	 * Insert a string representation of the exception @a ex into the output stream @a os.
	 */
	inline
	std::ostream &
	operator<<(
			std::ostream &os,
			const Exception &ex)
	{
		ex.write(os);
		return os;
	}
}

#endif  // GPLATES_GLOBAL_EXCEPTION_H
