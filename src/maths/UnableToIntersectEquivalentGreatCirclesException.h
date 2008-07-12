/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
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

#ifndef GPLATES_MATHS_UNABLETOINTERSECTEQUIVALENTGREATCIRCLESEXCEPTION_H
#define GPLATES_MATHS_UNABLETOINTERSECTEQUIVALENTGREATCIRCLESEXCEPTION_H

// FIXME:  When the definition of 'write_message' moves to a .cc file, replace this with <iosfwd>.
#include <ostream>
#include "global/PreconditionViolationError.h"
#include "GreatCircleArc.h"


namespace GPlatesMaths
{
	/**
	 * This is the exception thrown when an attempt is made to calculate the intersection of
	 * two great-circles which are equivalent.
	 */
	class UnableToIntersectEquivalentGreatCirclesException:
			public GPlatesGlobal::PreconditionViolationError
	{
	public:
		UnableToIntersectEquivalentGreatCirclesException(
				const GreatCircleArc &arc1_,
				const GreatCircleArc &arc2_):
			d_arc1(arc1_),
			d_arc2(arc2_)
		{  }

		virtual
		~UnableToIntersectEquivalentGreatCirclesException()
		{  }

	protected:
		virtual
		const char *
		ExceptionName() const
		{
			// FIXME:  This function should really be defined in a .cc file.
			return "UnableToIntersectEquivalentGreatCirclesException";
		}

		virtual
		void
		write_message(
				std::ostream &os) const
		{
			// FIXME:  This function should really be defined in a .cc file.
		}
	private:
		GreatCircleArc d_arc1;
		GreatCircleArc d_arc2;
	};
}

#endif  // GPLATES_MATHS_UNABLETOINTERSECTEQUIVALENTGREATCIRCLESEXCEPTION_H
