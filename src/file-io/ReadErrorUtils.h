/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

#ifndef GPLATES_FILE_IO_READERRORUTILS_H
#define GPLATES_FILE_IO_READERRORUTILS_H

#include <map>
#include <string>

#include "ReadErrorAccumulation.h"


namespace GPlatesFileIO
{
	namespace ReadErrorUtils
	{
		// Map of Filename -> Error collection for reporting all errors of a particular type for each file.
		typedef std::map<std::string, ReadErrorAccumulation::read_error_collection_type> errors_by_file_map_type;
		typedef errors_by_file_map_type::const_iterator errors_by_file_map_const_iterator;

		// Map of ReadErrors::Description -> Error collection for reporting all errors of a particular type for each error code.
		typedef std::map<ReadErrors::Description, ReadErrorAccumulation::read_error_collection_type> errors_by_type_map_type;
		typedef errors_by_type_map_type::const_iterator errors_by_type_map_const_iterator;


		/**
		 * Builds a string summarising the number of errors in each error category.
		 */
		const QString
		build_summary_string(
				const ReadErrorAccumulation &read_errors);


		/**
		 * Takes a read_error_collection_type and creates a map of read_error_collection_types,
		 * grouped by filename.
		 */
		void
		group_read_errors_by_file(
				errors_by_file_map_type &errors_by_file,
				const ReadErrorAccumulation::read_error_collection_type &errors);


		/**
		 * Takes a read_error_collection_type and creates a map of read_error_collection_types,
		 * grouped by error type (the ReadErrors::Description enum).
		 */
		void
		group_read_errors_by_type(
				errors_by_type_map_type &errors_by_type,
				const ReadErrorAccumulation::read_error_collection_type &errors);
	}
}

#endif // GPLATES_FILE_IO_READERRORUTILS_H
