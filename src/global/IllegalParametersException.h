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
 *   Dave Symonds <ds@geosci.usyd.edu.au>
 */

#ifndef _GPLATES_GLOBAL_ILLEGALPARAMETERSEXCEPTION_H_
#define _GPLATES_GLOBAL_ILLEGALPARAMETERSEXCEPTION_H_

#include "Exception.h"

namespace GPlatesGlobal
{
	/**
	 * Should be thrown when a method is called with illegal or
	 * unreasonable parameters.
	 */
	class IllegalParametersException : public Exception
	{
		public:
			/**
			 * @param msg is a message describing the situation.
			 */
			IllegalParametersException(const char *msg)
				: _msg(msg) {  }

			virtual
			~IllegalParametersException() {  }

		protected:
			virtual const char *
			ExceptionName() const {
				return "IllegalParametersException";
			}

			virtual std::string
			Message() const { return _msg; }

		private:
			std::string _msg;
	};
}

#endif  // _GPLATES_GLOBAL_ILLEGALPARAMETERSEXCEPTION_H_
