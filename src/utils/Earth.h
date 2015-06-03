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

#ifndef GPLATES_UTILS_EARTH_H
#define GPLATES_UTILS_EARTH_H

namespace GPlatesUtils
{
	/**
	 * Various Earth-related parameters.
	 */
	class Earth
	{
	public:
		/**
		 * Equatorial radius (in kms) - in WGS-84 coordinate system.
		 */
		static const double EQUATORIAL_RADIUS_KMS;

		/**
		 * Polar radius (in kms) - in WGS-84 coordinate system.
		 */
		static const double POLAR_RADIUS_KMS;

		/**
		 * Mean radius (in kms) - in WGS-84 coordinate system.
		 */
		static const double MEAN_RADIUS_KMS;
	};
}

#endif // GPLATES_UTILS_EARTH_H
