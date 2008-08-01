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

#ifndef _GPLATES_GLOBAL_UNSUPPORTEDFUNCTIONEXCEPTION_H_
#define _GPLATES_GLOBAL_UNSUPPORTEDFUNCTIONEXCEPTION_H_

#include "GPlatesException.h"

namespace GPlatesGlobal
{
	/**
	 * Should be thrown when a function that has purposely not been
	 * implemented is called.
	 * @see GeologicalData::Add(), GeologicalData::Remove().
	 */
	class UnsupportedFunctionException : public Exception
	{
		public:
			/**
			 * @param fname is the name of the function which is
			 * not supported.
			 */
			explicit
			UnsupportedFunctionException(const char *fname)
				: _fname(fname) {  }

			virtual
			~UnsupportedFunctionException() {  }

		protected:
			virtual const char *
			ExceptionName() const {

				return "UnsupportedFunctionException";
			}

			virtual std::string
			Message() const { return _fname; }

		private:
			std::string _fname;
	};
}

#endif  // _GPLATES_GLOBAL_UNSUPPORTEDFUNCTIONEXCEPTION_H_
