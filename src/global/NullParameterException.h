/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
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
 */

#ifndef _GPLATES_GLOBAL_NULLPARAMETEREXCEPTION_H_
#define _GPLATES_GLOBAL_NULLPARAMETEREXCEPTION_H_

#include "Exception.h"

namespace GPlatesGlobal
{
	/**
	 * Should be thrown when a function is invoked with a NULL-pointer
	 * parameter when that parameter may not be NULL.
	 */
	class NullParameterException : public Exception
	{
		public:
			/**
			 * @param msg is a message describing the situation.
			 */
			explicit
			NullParameterException(const char *msg)
				: _msg(msg) {  }

			virtual
			~NullParameterException() {  }

		protected:
			virtual const char *
			ExceptionName() const {

				return "NullParameterException";
			}

			virtual std::string
			Message() const { return _msg; }

		private:
			std::string _msg;
	};
}

#endif  // _GPLATES_GLOBAL_NULLPARAMETEREXCEPTION_H_
