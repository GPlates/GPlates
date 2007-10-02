/* $Id$ */

/**
 * @file
 * Contains the definition of the class ErrorOpeningFileForReadingException.
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

#ifndef GPLATES_FILEIO_ERROROPENINGFILEFORREADINGEXCEPTION_H
#define GPLATES_FILEIO_ERROROPENINGFILEFORREADINGEXCEPTION_H

#include <string>

namespace GPlatesFileIO
{
	/**
	 * This exception is thrown when an error is encountered while attempting to open a file
	 * for reading.
	 */
	class ErrorOpeningFileForReadingException
	{
	public:
		/**
		 * Instantiate an exception for a file named @a filename.
		 */
		explicit
		ErrorOpeningFileForReadingException(
				const std::string &filename_):
			d_filename(filename_)
		{  }

		/**
		 * Return the filename of the file which couldn't be opened for reading.
		 */
		const std::string &
		filename() const
		{
			return d_filename;
		}
	private:
		/**
		 * The filename of the file which couldn't be opened for reading.
		 */
		std::string d_filename;
	};
}
#endif  // GPLATES_FILEIO_ERROROPENINGFILEFORREADINGEXCEPTION_H
