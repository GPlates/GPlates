/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2004, 2006, 2007 The University of Sydney, Australia
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

#ifndef GPLATES_MATHS_INVALIDLATLONEXCEPTION_H
#define GPLATES_MATHS_INVALIDLATLONEXCEPTION_H

#include <iosfwd>
#include "global/PreconditionViolationError.h"


namespace GPlatesMaths
{
	/**
	 * This is the exception thrown when an attempt is made to instantiate a LatLonPoint using
	 * either an invalid latitude or an invalid longitude (or both, though this exception can
	 * only report one problem).
	 */
	class InvalidLatLonException:
			public GPlatesGlobal::PreconditionViolationError
	{
	public:
		enum LatOrLon
		{
			Latitude,
			Longitude
		};

		/**
		 * @param invalid_value_ is the invalid value.
		 * @param lat_or_lon_ is whether the invalid value is an invalid latitude or an
		 * invalid longitude.
		 */
		InvalidLatLonException(
				const GPlatesUtils::CallStack::Trace &exception_source,
				const double &invalid_value_,
				LatOrLon lat_or_lon_) :
			GPlatesGlobal::PreconditionViolationError(exception_source),
			d_invalid_value(invalid_value_),
			d_lat_or_lon(lat_or_lon_)
		{  }

		virtual
		~InvalidLatLonException()
		{  }

		const double &
		invalid_value() const
		{
			return d_invalid_value;
		}

		LatOrLon
		lat_or_lon() const
		{
			return d_lat_or_lon;
		}
	protected:
		virtual
		const char *
		exception_name() const
		{
			// FIXME:  This function should really be defined in a .cc file.
			return "InvalidLatLonException";
		}

		virtual
		void
		write_message(
				std::ostream &os) const;

	private:
		const double d_invalid_value;
		LatOrLon d_lat_or_lon;
	};
}

#endif  // GPLATES_MATHS_INVALIDLATLONEXCEPTION_H
