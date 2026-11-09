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

#ifndef GPLATES_API_PYGREATCIRCLEARC_H
#define GPLATES_API_PYGREATCIRCLEARC_H

#include "global/PreconditionViolationError.h"


namespace GPlatesApi
{
	/**
	 * The smallest tessellation threshold, in radians, for which the number of segments tessellating
	 * a great circle arc (at most PI long) fits an 'int' (about 1.5e-9 radians).
	 */
	double
	get_minimum_tessellate_radians();

	/**
	 * Raises Python 'ValueError' if @a tessellate_radians is not positive (including NaN), or is
	 * less than @a get_minimum_tessellate_radians.
	 *
	 * Used by the 'to_tessellated()' methods. Note that, below that, a small threshold still means
	 * many points (that is the caller's cost).
	 */
	void
	raise_value_error_if_invalid_tessellate_radians(
			const double &tessellate_radians);


	/**
	 * Cannot calculate great circle normal for zero length arc.
	 */
	class IndeterminateGreatCircleArcNormalException :
			public GPlatesGlobal::PreconditionViolationError
	{
	public:
		explicit
		IndeterminateGreatCircleArcNormalException(
				const GPlatesUtils::CallStack::Trace &exception_source) :
			GPlatesGlobal::PreconditionViolationError(exception_source)
		{  }

		~IndeterminateGreatCircleArcNormalException() throw()
		{  }

	protected:

		virtual
		const char *
		exception_name() const
		{
			return "IndeterminateGreatCircleArcNormalException";
		}

	};

	/**
	 * Cannot calculate great circle arc direction for zero length arc.
	 */
	class IndeterminateGreatCircleArcDirectionException :
			public GPlatesGlobal::PreconditionViolationError
	{
	public:
		explicit
		IndeterminateGreatCircleArcDirectionException(
				const GPlatesUtils::CallStack::Trace &exception_source) :
			GPlatesGlobal::PreconditionViolationError(exception_source)
		{  }

		~IndeterminateGreatCircleArcDirectionException() throw()
		{  }

	protected:

		virtual
		const char *
		exception_name() const
		{
			return "IndeterminateGreatCircleArcDirectionException";
		}

	};
}

#endif // GPLATES_API_PYGREATCIRCLEARC_H
