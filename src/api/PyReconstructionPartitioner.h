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

#ifndef GPLATES_API_PYRECONSTRUCTIONPARTITIONER_H
#define GPLATES_API_PYRECONSTRUCTIONPARTITIONER_H

#include "global/PreconditionViolationError.h"


#if !defined(GPLATES_NO_PYTHON)

namespace GPlatesApi
{
	/**
	 * The reconstruction times of a group of partitioning reconstruction geometries are not all the same.
	 */
	class DifferentTimesInPartitioningReconstructionGeometriesException :
			public GPlatesGlobal::PreconditionViolationError
	{
	public:
		explicit
		DifferentTimesInPartitioningReconstructionGeometriesException(
				const GPlatesUtils::CallStack::Trace &exception_source) :
			GPlatesGlobal::PreconditionViolationError(exception_source)
		{  }

		~DifferentTimesInPartitioningReconstructionGeometriesException() throw()
		{  }

	protected:

		virtual
		const char *
		exception_name() const
		{
			return "DifferentReconstructionTimesException";
		}

	};
}

#endif   // GPLATES_NO_PYTHON

#endif // GPLATES_API_PYRECONSTRUCTIONPARTITIONER_H
