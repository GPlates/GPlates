/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2009 Geological Survey of Norway
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

#ifndef GPLATES_FILEIO_OGREXCEPTION_H
#define GPLATES_FILEIO_OGREXCEPTION_H

#include "global/GPlatesException.h"

namespace GPlatesFileIO
{
	/**
	 *  Thrown on encountering an OGR error. 
	 */
	class OgrException :
		public GPlatesGlobal::Exception
	{
		public:
			/**
			 * @param msg is a message describing the situation.
			 */
			explicit
			OgrException(
				const GPlatesUtils::CallStack::Trace &exception_source,
				const char *msg):
				Exception(exception_source),
				d_msg(msg) {  }

			virtual
			~OgrException() {}

			virtual const char *
			exception_name() const {
				return "OgrException";
			}

			virtual
			void
			write_message(
				std::ostream &os) const 
			{ 
				write_string_message(os,d_msg); 
			}

		private:
			std::string d_msg;

	};

}

#endif // GPLATES_FILEIO_OGREXCEPTION_H

