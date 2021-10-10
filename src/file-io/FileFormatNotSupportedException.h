/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2007 The University of Sydney, Australia
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

#ifndef GPLATES_GLOBAL_FILEFORMATNOTSUPPORTEDEXCEPTION_H
#define GPLATES_GLOBAL_FILEFORMATNOTSUPPORTEDEXCEPTION_H

#include "global/GPlatesException.h"

namespace GPlatesFileIO
{
	/**
	 * Thrown when the user attempts to save a file in a format that is not
	 * yet supported.
	 */
	class FileFormatNotSupportedException : public GPlatesGlobal::Exception
	{
		public:
			/**
			 * @param msg is a message describing the situation.
			 */
			explicit
			FileFormatNotSupportedException(const char *msg)
				: _msg(msg) {  }

			virtual
			~FileFormatNotSupportedException() {  }

		protected:
			virtual const char *
			ExceptionName() const {

				return "FileFormatNotSupportedException";
			}

			virtual std::string
			Message() const { return _msg; }

		private:
			std::string _msg;
	};
}

#endif  // GPLATES_GLOBAL_FILEFORMATNOTSUPPORTEDEXCEPTION_H
