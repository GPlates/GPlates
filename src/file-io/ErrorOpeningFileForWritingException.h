/* $Id$ */

/**
 * @file
 * Contains the declaration of the class ErrorOpeningFileForWritingException.
 *
 * Most recent change:
 *   $Date$
 *
 * Copyright (C) 2007 James Boyden <jboy@jboy.id.au>
 *
 * This file is derived from the header file
 * "ErrorOpeningFileForReadingException.hh", which is part of the
 * ReconTreeViewer software:
 *  http://sourceforge.net/projects/recontreeviewer/
 *
 * ReconTreeViewer is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License, version 2, as published
 * by the Free Software Foundation.
 *
 * ReconTreeViewer is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef GPLATES_FILEIO_ERROROPENINGFILEFORWRITINGEXCEPTION_H
#define GPLATES_FILEIO_ERROROPENINGFILEFORWRITINGEXCEPTION_H

#include <QString>

#include "global/GPlatesException.h"


namespace GPlatesFileIO
{
	/**
	 * This exception is thrown when an error is encountered while attempting to open a file
	 * for writing.
	 */
	class ErrorOpeningFileForWritingException :
			public GPlatesGlobal::Exception
	{
	public:
		/**
		 * Instantiate an exception for a file named @a filename.
		 */
		ErrorOpeningFileForWritingException(
				const GPlatesUtils::CallStack::Trace &exception_source,
				const QString &filename_) :
			Exception(exception_source),
			d_filename(filename_)
		{  }

		/**
		 * Return the filename of the file which couldn't be opened for writing.
		 */
		const QString &
		filename() const
		{
			return d_filename;
		}

	protected:
		virtual const char *
		exception_name() const {

			return "ErrorOpeningFileForWritingException";
		}

		virtual
		void
		write_message(
				std::ostream &os) const;

	private:
		/**
		 * The filename of the file which couldn't be opened for writing.
		 */
		QString d_filename;
	};
}
#endif  // GPLATES_FILEIO_ERROROPENINGFILEFORWRITINGEXCEPTION_H
