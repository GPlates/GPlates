/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006 The University of Sydney, Australia
 *  (under the name "InvalidPolyLineException.h")
 * Copyright (C) 2006 The University of Sydney, Australia
 *  (under the name "InvalidPolylineException.h")
 * Copyright (C) 2008 The University of Sydney, Australia
 *  (under the name "InvalidPolylineContainsZeroPointsException.h")
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

#ifndef GPLATES_MATHS_INVALIDPOLYLINECONTAINSZEROPOINTSEXCEPTION_H
#define GPLATES_MATHS_INVALIDPOLYLINECONTAINSZEROPOINTSEXCEPTION_H

#include "global/InternalObjectInconsistencyException.h"


namespace GPlatesMaths
{
	/**
	 * The exception thrown when a polyline is found to contain zero points.
	 */
	class InvalidPolylineContainsZeroPointsException:
			public GPlatesGlobal::InternalObjectInconsistencyException
	{
	public:
		/**
		 * Instantiate the exception.
		 */
		InvalidPolylineContainsZeroPointsException(
				const GPlatesUtils::CallStack::Trace &exception_source) :
			GPlatesGlobal::InternalObjectInconsistencyException(exception_source),
			d_filename(exception_source.get_filename()),
			d_line_num(exception_source.get_line_num())
		{  }

	protected:
		virtual
		const char *
		exception_name() const
		{
			return "InvalidPolylineContainsZeroPointsException";
		}

	private:
		const char *d_filename;
		int d_line_num;
	};
}

#endif  // GPLATES_MATHS_INVALIDPOLYLINECONTAINSZEROPOINTSEXCEPTION_H
