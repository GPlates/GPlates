/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Author$
 *   $Date$
 * 
 * Copyright (C) 2003 The GPlates Consortium
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Authors:
 *   Hamish Law <hlaw@es.usyd.edu.au>
 *   James Boyden <jboyden@es.usyd.edu.au>
 */

#ifndef _GPLATES_GLOBAL_EXCEPTION_H_
#define _GPLATES_EXCEPTION_H_

#include <iostream>
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
			 * Exception into the given stream.
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
			 * @return True if there is a message associated with
			 *   Exception.
			 */
			virtual bool
			HasMessage() const = 0;

			/**
			 * @return The Exception's messages as a string.
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

#endif  // _GPLATES_GLOBAL_EXCEPTION_H_
