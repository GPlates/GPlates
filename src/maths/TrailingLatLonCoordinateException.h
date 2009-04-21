/* $Id$ */

/**
 * \file 
 * File specific comments.
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

#ifndef GPLATES_MATHS_TRAILINGLATLONCOORDINATEEXCEPTION_H
#define GPLATES_MATHS_TRAILINGLATLONCOORDINATEEXCEPTION_H

#include "global/ExternalResourceFailureException.h"


namespace GPlatesMaths
{
	/**
	 * This is the exception thrown when a sequence of doubles, whose elements are to be paired
	 * into (lat, lon) coordinate-pairs, encounters a trailing coordinate.
	 */
	class TrailingLatLonCoordinateException:
			public GPlatesGlobal::ExternalResourceFailureException
	{
	public:
		typedef unsigned long size_type;

		/**
		 * @param trailing_coord_ is the trailing coordinate.
		 * @param sequence_len_ is the length of the sequence in question.
		 */
		TrailingLatLonCoordinateException(
				const GPlatesUtils::CallStack::Trace &exception_source,
				const double &trailing_coord_,
				size_type sequence_len_) :
			GPlatesGlobal::ExternalResourceFailureException(exception_source),
			d_trailing_coord(trailing_coord_),
			d_sequence_len(sequence_len_)
		{  }

		const double &
		trailing_coord() const
		{
			return d_trailing_coord;
		}

		size_type
		sequence_len() const
		{
			return d_sequence_len;
		}
	protected:
		virtual
		const char *
		exception_name() const
		{
			// FIXME:  This function should really be defined in a .cc file.
			return "TrailingLatLonCoordinateException";
		}

		virtual
		void
		write_message(
				std::ostream &os) const;

	private:
		const double d_trailing_coord;
		const size_type d_sequence_len;
	};
}

#endif  // GPLATES_MATHS_TRAILINGLATLONCOORDINATEEXCEPTION_H
