/*$Id$*/
/// Exception used when some data value in a Plates file being read in is out of range.
/**
 * @file
 *
 * Most recent change:
 * $Author$
 * $Date$
 *
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
 * Author:
 *   Mike Dowman <mdowman@geosci.usyd.edu.au>
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
