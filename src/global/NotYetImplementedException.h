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
 *   Hamish Law <hlaw@geosci.usyd.edu.au>
 *   James Boyden <jboyden@geosci.usyd.edu.au>
 */

#ifndef _GPLATES_GLOBAL_NOTYETIMPLEMENTEDEXCEPTION_H_
#define _GPLATES_GLOBAL_NOTYETIMPLEMENTEDEXCEPTION_H_

#include "Exception.h"

namespace GPlatesGlobal
{
	/**
	 * Should be thrown when a function that has not YET been
	 * implemented is called.
	 */
	class NotYetImplementedException : public Exception
	{
		public:
			/**
			 * @param fname is the name of the function which is
			 * not yet implemented.   
			 */
			explicit
			NotYetImplementedException(const char *fname)
				: _fname(fname) {  }

			virtual
			~NotYetImplementedException() {  }

		protected:
			virtual const char *
			ExceptionName() const {

				return "NotYetImplementedException";
			}

			virtual std::string
			Message() const { return _fname; }

		private:
			std::string _fname;
	};
}

#endif  // _GPLATES_GLOBAL_NOTYETIMPLEMENTEDEXCEPTION_H_
