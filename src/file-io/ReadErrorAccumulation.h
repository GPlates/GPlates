/* $Id$ */

/**
 * \file 
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

#ifndef GPLATES_FILEIO_READERRORACCUMULATION_H
#define GPLATES_FILEIO_READERRORACCUMULATION_H

#include <algorithm>
#include <iterator>
#include <vector>
#include <boost/shared_ptr.hpp>
#include "ReadErrorOccurrence.h"

namespace GPlatesFileIO
{
	struct ReadErrorAccumulation
	{
		typedef std::vector<ReadErrorOccurrence> read_error_collection_type;
		typedef read_error_collection_type::const_iterator read_error_collection_const_iterator;
		typedef read_error_collection_type::size_type size_type;

		ReadErrorAccumulation()
		{  }

		/**
		 * A warning is the result of a problem which doesn't cause data loss (when the
		 * data is being loaded), but which the user should nevertheless be notified of.
		 *
		 * There may be any number of warnings in this accumulation.
		 */
		read_error_collection_type d_warnings;

		/**
		 * After a recoverable error, reading from file can continue, but some amount of
		 * data (a feature? a property of a feature? etc.) simply had to be discarded
		 * because it was hopelessly malformed.
		 *
		 * There may be any number of recoverable errors in this accumulation.
		 */
		read_error_collection_type d_recoverable_errors;

		/**
		 * After a terminating error, reading from file (or other data source) simply
		 * cannot continue.
		 *
		 * There can only be zero or one terminating errors per file, but there may be
		 * multiple terminating errors in this accumulation.
		 */
		read_error_collection_type d_terminating_errors;
		
		/**
		 * A failure to begin indicates a fatal error before the parser could access
		 * any data from the file, e.g. the file does not exist. No data has been loaded.
		 * No data corruption will occur, but the user must be notified about the problem.
		 *
		 * There can only be one failure to begin per file, but there may be any number
		 * of them in this accumulation.
		 */
		read_error_collection_type d_failures_to_begin;

		/**
		 * Returns whether the ReadErrorAccumulation contains no errors or warnings.
		 */
		bool
		is_empty() const
		{
			return (d_warnings.empty() && d_recoverable_errors.empty() &&
					d_terminating_errors.empty() && d_failures_to_begin.empty());
		}
		
		/**
		 * The combined size of all read error collections in this ReadErrorAccumulation.
		 */
		size_type
		size() const
		{
			return (d_warnings.size() + d_recoverable_errors.size() +
					d_terminating_errors.size() + d_failures_to_begin.size());
		}
		
		void
		clear()
		{
			d_warnings.clear();
			d_recoverable_errors.clear();
			d_terminating_errors.clear();
			d_failures_to_begin.clear();
		}

		/**
		 * Appends warnings and errors of @a errors into 'this'.
		 */
		void
		accumulate(
				const ReadErrorAccumulation &errors)
		{
			std::copy(errors.d_warnings.begin(), errors.d_warnings.end(),
					std::back_inserter(d_warnings));
			std::copy(errors.d_recoverable_errors.begin(), errors.d_recoverable_errors.end(),
					std::back_inserter(d_recoverable_errors));
			std::copy(errors.d_terminating_errors.begin(), errors.d_terminating_errors.end(),
					std::back_inserter(d_terminating_errors));
			std::copy(errors.d_failures_to_begin.begin(), errors.d_failures_to_begin.end(),
					std::back_inserter(d_failures_to_begin));
		}
	};
}

#endif  // GPLATES_FILEIO_READERRORACCUMULATION_H
