/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
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

#ifndef GPLATES_FILE_IO_ERRORWRITINGFEATURECOLLECTIONTOFILEFORMATEXCEPTION_H
#define GPLATES_FILE_IO_ERRORWRITINGFEATURECOLLECTIONTOFILEFORMATEXCEPTION_H

#include <string>

#include "global/GPlatesException.h"


namespace GPlatesFileIO
{
	/**
	 * An exception during writing of feature collection data to a file format.
	 *
	 * TODO: We need a write-errors system perhaps similar to the existing read-errors system
	 * in order to report errors back to the user. We don't currently have a write-errors system
	 * because the data being written out is from the model and hence already verified.
	 * However, there are a few situations where the limitations of a particular file format
	 * prevent it from writing out valid data (eg, 5-digit plate ids for the PLATES line format).
	 * We probably don't need a full system like the read-errors but we need something.
	 *
	 * In the meantime, a general write exception is used to report these kinds of write errors.
	 * This exception can be thrown by the low-level file IO code. The exception then propagates up
	 * to the GUI level which then reports the error.
	 * The downside of this is the entire file write gets aborted - the GUI level
	 * will also remove the file in case it was (most likely) partially written.
	 */
	class ErrorWritingFeatureCollectionToFileFormatException :
			public GPlatesGlobal::Exception
	{
	public:

		/**
		 * @param msg is a description of the conditions
		 * in which the problem occurs.
		 */
		ErrorWritingFeatureCollectionToFileFormatException(
				const GPlatesUtils::CallStack::Trace &exception_source,
				const char *msg) :
			GPlatesGlobal::Exception(exception_source),
			d_msg(msg)
		{  }

		~ErrorWritingFeatureCollectionToFileFormatException() throw() { }

	protected:

		virtual const char *
		exception_name() const
		{

			return "ErrorWritingFeatureCollectionToFileFormatException";
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

	};
}

#endif // GPLATES_FILE_IO_ERRORWRITINGFEATURECOLLECTIONTOFILEFORMATEXCEPTION_H
