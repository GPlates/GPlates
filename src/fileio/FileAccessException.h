/*$Id$*/

/**
 * @file
 *
 * Most recent change:
 * $Author$
 * $Date$
 *
 * Copyright (C) 2004 The GPlates Consortium
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
 *   Dave Symonds <ds@geosci.usyd.edu.au>
 */

#ifndef _GPLATES_FILEIO_FILEACCESSEXCEPTION_H_
#define _GPLATES_FILEIO_FILEACCESSEXCEPTION_H_

#include "FileIOException.h"

namespace GPlatesFileIO {

	/**
	 * The Exception thrown when there is a problem with accessing a file,
	 * whether due to I/O problems or file permissions.
	 */
	class FileAccessException : public FileIOException
	{
		public:

			/**
			 * @param msg is a description of the problem.
			 */
			FileAccessException (const char *msg) : _msg(msg) { }

			virtual ~FileAccessException() { }

		protected:
			virtual const char *ExceptionName () const
					{ return "FileAccessException"; }

			virtual std::string Message () const { return _msg; }
 
		private:
			std::string _msg;
	};
}

#endif  // _GPLATES_FILEIO_FILEACCESSEXCEPTION_H_
