/*$Id$*/
/// Exception used when some data value in a Plates file being read in is out of range.
/**
 * @file
 *
 * Most recent change:
 * $Date$
 *
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

#ifndef _GPLATES_FILEIO_INVALIDDATAEXCEPTION_H_
#define _GPLATES_FILEIO_INVALIDDATAEXCEPTION_H_

#include "FileIOException.h"

namespace GPlatesFileIO {

	/**
	 * The Exception thrown when the input data is invalid.
	 */
	class InvalidDataException : public FileIOException
	{
		public:
			/**
			 * @param msg is the description of the problem.
			 */
			explicit
			InvalidDataException(const char* msg) : _msg(msg) { }

			virtual
			~InvalidDataException() { }

		protected:
			virtual const char *
			ExceptionName() const { return "InvalidDataException"; }

			virtual std::string
			Message() const { return _msg; }

		private:
			std::string _msg;
	};

}

#endif  // _GPLATES_FILEIO_INVALIDDATAEXCEPTION_H_
