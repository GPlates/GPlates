/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2004, 2005, 2006 The University of Sydney, Australia
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

#ifndef _GPLATES_GLOBAL_ALREADYINITIALISEDSINGLETONEXCEPTION_H_
#define _GPLATES_GLOBAL_ALREADYINITIALISEDSINGLETONEXCEPTION_H_

#include "Exception.h"

namespace GPlatesGlobal
{
	/**
	 * Should be thrown when an attempt is made to instantiate a singleton
	 * class which has already been initialised.
	 */
	class AlreadyInitialisedSingletonException : public Exception
	{
		public:
			/**
			 * @param msg is a message describing the situation.
			 */
			explicit
			AlreadyInitialisedSingletonException(const char *msg)
				: _msg(msg) {  }

			virtual
			~AlreadyInitialisedSingletonException() {  }

		protected:
			virtual const char *
			ExceptionName() const {

				return "AlreadyInitialisedSingletonException";
			}

			virtual std::string
			Message() const { return _msg; }

		private:
			std::string _msg;
	};
}

#endif  // _GPLATES_GLOBAL_ALREADYINITIALISEDSINGLETONEXCEPTION_H_
