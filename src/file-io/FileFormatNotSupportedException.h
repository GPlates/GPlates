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

#ifndef GPLATES_FILE_IO_FILEFORMATNOTSUPPORTEDEXCEPTION_H
#define GPLATES_FILE_IO_FILEFORMATNOTSUPPORTEDEXCEPTION_H

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
			FileFormatNotSupportedException(
					const GPlatesUtils::CallStack::Trace &exception_source,
					const char *msg) :
				Exception(exception_source),
				_msg(msg)
			{  }
			
			~FileFormatNotSupportedException() throw() { }
		protected:
			virtual const char *
			exception_name() const {

				return "FileFormatNotSupportedException";
			}

			virtual
			void
			write_message(
					std::ostream &os) const
			{
				write_string_message(os, _msg);
			}

		private:
			std::string _msg;
	};
}

#endif  // GPLATES_FILE_IO_FILEFORMATNOTSUPPORTEDEXCEPTION_H
