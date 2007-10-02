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

#include <vector>
#include <boost/shared_ptr.hpp>
#include "ReadErrorOccurrence.h"

namespace GPlatesFileIO
{
	struct ReadErrorAccumulation
	{
		ReadErrorAccumulation()
		{  }

		/**
		 * A warning is the result of a problem which doesn't cause data loss (when the
		 * data is being loaded), but which the user should nevertheless be notified of.
		 *
		 * There may be any number of warnings in this accumulation.
		 */
		std::vector<ReadErrorOccurrence> d_warnings;

		/**
		 * After a recoverable error, reading from file can continue, but some amount of
		 * data (a feature? a property of a feature? etc.) simply had to be discarded
		 * because it was hopelessly malformed.
		 *
		 * There may be any number of recoverable errors in this accumulation.
		 */
		std::vector<ReadErrorOccurrence> d_recoverable_errors;

		/**
		 * After a terminating error, reading from file (or other data source) simply
		 * cannot continue.
		 *
		 * There may be zero or one terminating errors in this accumulation.
		 */
		boost::shared_ptr<ReadErrorOccurrence> d_terminating_error;
	};
}

#endif  // GPLATES_FILEIO_READERRORACCUMULATION_H
