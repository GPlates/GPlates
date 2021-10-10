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
 *  (under the name "InvalidPolylineContainsOnlyOnePointException.h")
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

#ifndef GPLATES_MATHS_INVALIDPOLYLINECONTAINSONLYONEPOINTEXCEPTION_H
#define GPLATES_MATHS_INVALIDPOLYLINECONTAINSONLYONEPOINTEXCEPTION_H

#include "global/InternalObjectInconsistencyException.h"


namespace GPlatesMaths
{
	/**
	 * The exception thrown when a polyline is found to contain zero points.
	 */
	class InvalidPolylineContainsOnlyOnePointException:
			public GPlatesGlobal::InternalObjectInconsistencyException
	{
	public:
		/**
		 * Instantiate the exception.
		 */
		InvalidPolylineContainsOnlyOnePointException(
				const char *filename,
				int line_num):
			d_filename(filename),
			d_line_num(line_num)
		{  }

		virtual
		~InvalidPolylineContainsOnlyOnePointException()
		{  }

	protected:
		virtual
		const char *
		ExceptionName() const
		{
			return "InvalidPolylineContainsOnlyOnePointException";
		}

	private:
		const char *d_filename;
		int d_line_num;
	};
}

#endif  // GPLATES_MATHS_INVALIDPOLYLINECONTAINSONLYONEPOINTEXCEPTION_H
