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

#ifndef GPLATES_MATHS_INVALIDLATLONCOORDINATEEXCEPTION_H
#define GPLATES_MATHS_INVALIDLATLONCOORDINATEEXCEPTION_H

#include "global/ExternalResourceFailureException.h"


namespace GPlatesMaths
{
	/**
	 * This is the exception thrown when a sequence of doubles, whose elements are to be paired
	 * into (lat, lon) coordinate-pairs, contains an invalid latitude coordinate or an invalid
	 * longitude coordinate.
	 */
	class InvalidLatLonCoordinateException:
			public GPlatesGlobal::ExternalResourceFailureException
	{
	public:
		typedef unsigned long size_type;

		enum CoordinateType
		{
			LatitudeCoord,
			LongitudeCoord
		};

		/**
		 * @param invalid_coord_ is the invalid coordinate.
		 * @param coordinate_type_ is whether the invalid coordinate is a latitude coord or
		 * a longitude coord.
		 * @param coord_index_ is the length of the sequence in question.
		 */
		InvalidLatLonCoordinateException(
				const GPlatesUtils::CallStack::Trace &exception_source,
				const double &invalid_coord_,
				CoordinateType coordinate_type_,
				size_type coord_index_):
			ExternalResourceFailureException(exception_source),
			d_invalid_coord(invalid_coord_),
			d_coordinate_type(coordinate_type_),
			d_coord_index(coord_index_)
		{  }

		virtual
		~InvalidLatLonCoordinateException()
		{  }

		const double &
		invalid_coord() const
		{
			return d_invalid_coord;
		}

		CoordinateType
		coordinate_type() const
		{
			return d_coordinate_type;
		}

		size_type
		coord_index() const
		{
			return d_coord_index;
		}
	protected:
		virtual
		const char *
		exception_name() const
		{
			// FIXME:  This function should really be defined in a .cc file.
			return "InvalidLatLonCoordinateException";
		}

		virtual
		void
		write_message(
				std::ostream &os) const;

	private:
		const double d_invalid_coord;
		CoordinateType d_coordinate_type;
		const size_type d_coord_index;
	};
}

#endif  // GPLATES_MATHS_INVALIDLATLONCOORDINATEEXCEPTION_H
