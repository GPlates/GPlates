/* $Id$ */

/**
 * \file 
 * Interface for querying the proximity threshold based on position on globe.
 * $Revision$
 * $Date$
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

#ifndef GPLATES_VIEWOPERATIONS_QUERYPROXIMITYTHRESHOLD_H
#define GPLATES_VIEWOPERATIONS_QUERYPROXIMITYTHRESHOLD_H


namespace GPlatesViewOperations
{
	/**
	 * Interface for querying the proximity threshold based on position on globe.
	 */
	class QueryProximityThreshold
	{
	public:
		virtual
		~QueryProximityThreshold()
		{  }

		/**
		 * The proximity inclusion threshold is a measure of how close a geometry must be
		 * to a click-point be considered "hit" by the click.
		 *
		 * This will depend on the projection of the globe.
		 * For 3D projections the horizon of the globe will need a larger threshold
		 * than the centre of the globe.
		 * For 2D projections the threshold will vary with the 'stretch' around the
		 * clicked-point.
		 */
		virtual
		double
		current_proximity_inclusion_threshold(
				const GPlatesMaths::PointOnSphere &click_pos_on_globe) const = 0;
	};
}

#endif // GPLATES_VIEWOPERATIONS_QUERYPROXIMITYTHRESHOLD_H
