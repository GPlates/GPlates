/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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

#ifndef GPLATES_FILEIO_ERROROPENINGPIPEFROMGZIPEXCEPTION_H
#define GPLATES_FILEIO_ERROROPENINGPIPEFROMGZIPEXCEPTION_H

#include <QString>

#include "global/GPlatesException.h"


namespace GPlatesFileIO
{
	/**
	 * This exception is thrown when GPlates cannot start the 'gzip' program
	 * to do on-the-fly compression to read a compressed GPML file.
	 */
	class ErrorOpeningPipeFromGzipException :
			public GPlatesGlobal::Exception
	{
	public:
		/**
		 * Instantiate an exception for a file named @a filename.
		 */
		explicit
		ErrorOpeningPipeFromGzipException(
				const GPlatesUtils::CallStack::Trace &exception_source,
				const QString &command_,
				const QString &filename_):
			Exception(exception_source),
			d_command(command_),
			d_filename(filename_)
		{  }

		/**
		 * Return the command which could not be executed.
		 */
		const QString &
		command() const
		{
			return d_command;
		}

		/**
		 * Return the filename of the file which couldn't be opened for reading.
		 */
		const QString &
		filename() const
		{
			return d_filename;
		}

	protected:
		virtual
		const char *
		exception_name() const
		{
			return "ErrorOpeningPipeFromGzipException";
		}

		virtual
		void
		write_message(
				std::ostream &os) const;

	private:
		/**
		 * The command which could not be executed.
		 */
		QString d_command;
	
		/**
		 * The filename of the file which couldn't be opened for reading.
		 */
		QString d_filename;
	};
}
#endif  // GPLATES_FILEIO_ERROROPENINGPIPEFROMGZIPEXCEPTION_H
