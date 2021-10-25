/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
 * Copyright (C) 2012 Geological Survey of Norway
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

#ifndef GPLATES_FILE_IO_FILELOADABORTEDEXCEPTION_H
#define GPLATES_FILE_IO_FILELOADABORTEDEXCEPTION_H


#include "global/GPlatesException.h"

namespace GPlatesFileIO
{
	/**
	 * Should be thrown when a file load is aborted by the user.
	 */
	class FileLoadAbortedException : public GPlatesGlobal::Exception
	{
	public:
		/**
			 * @param msg is a message describing the situation.
			 */
		FileLoadAbortedException(
				const GPlatesUtils::CallStack::Trace &exception_source,
				const char *msg,
				const QString &filename_) :
			Exception(exception_source),
			d_msg(msg),
			d_filename(filename_)
		{  }

		~FileLoadAbortedException() throw() { }

		/**
			 * Return the filename of the file for which loading was aborted.
			 */
		const QString &
		filename() const
		{
			return d_filename;
		}

	protected:
		virtual const char *
		exception_name() const {
			return "FileLoadAbortedException";
		}

		virtual
		void
		write_message(
				std::ostream &os) const
		{
			write_string_message(os, d_msg);
		}



	private:
		std::string d_msg;

		/**
			 * The filename of the file for which loading was aborted.
			 */
		QString d_filename;
	};
}

#endif // GPLATES_FILE_IO_FILELOADABORTEDEXCEPTION_H
