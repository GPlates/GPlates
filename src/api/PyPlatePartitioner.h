/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2015 The University of Sydney, Australia
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

#ifndef GPLATES_API_PYPLATEPARTITIONER_H
#define GPLATES_API_PYPLATEPARTITIONER_H

#include "global/PreconditionViolationError.h"


#if !defined(GPLATES_NO_PYTHON)

namespace GPlatesApi
{
	/**
	 * The reconstruction times of a group of partitioning plates are not all the same.
	 */
	class DifferentTimesInPartitioningPlatesException :
			public GPlatesGlobal::PreconditionViolationError
	{
	public:
		explicit
		DifferentTimesInPartitioningPlatesException(
				const GPlatesUtils::CallStack::Trace &exception_source) :
			GPlatesGlobal::PreconditionViolationError(exception_source)
		{  }

		~DifferentTimesInPartitioningPlatesException() throw()
		{  }

	protected:

		virtual
		const char *
		exception_name() const
		{
			return "DifferentTimesInPartitioningPlatesException";
		}

	};


	/**
	 * Enumerated ways to sort partitioning plates.
	 */
	namespace SortPartitioningPlates
	{
		enum Value
		{
			// Group in order of resolved topological networks then resolved topological boundaries
			// then reconstructed static polygons, but with no sorting within each group.
			BY_PARTITION_TYPE,

			// Same as @a BY_PARTITION_TYPE but also sort by plate ID (from highest to lowest)
			// within each partition type group.
			BY_PARTITION_TYPE_THEN_PLATE_ID,

			// Same as @a BY_PARTITION_TYPE but also sort by plate area (from highest to lowest)
			// within each partition type group.
			BY_PARTITION_TYPE_THEN_PLATE_AREA,

			// Sort by plate ID (from highest to lowest), but no grouping by partition type.
			BY_PLATE_ID,

			// Sort by plate area (from highest to lowest), but no grouping by partition type.
			BY_PLATE_AREA
		};
	};
}

#endif   // GPLATES_NO_PYTHON

#endif // GPLATES_API_PYPLATEPARTITIONER_H
