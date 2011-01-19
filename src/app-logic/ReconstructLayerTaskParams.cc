/* $Id$ */


/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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
 
#include "ReconstructLayerTaskParams.h"


const double
GPlatesAppLogic::ReconstructLayerTaskParams::INITIAL_VGP_DELTA_T = 5.;


GPlatesAppLogic::ReconstructLayerTaskParams::ReconstructLayerTaskParams() :
	d_vgp_visibility_setting(DELTA_T_AROUND_AGE),
	d_vgp_earliest_time(GPlatesPropertyValues::GeoTimeInstant::create_distant_past()),
	d_vgp_latest_time(GPlatesPropertyValues::GeoTimeInstant::create_distant_future()),
	d_vgp_delta_t(INITIAL_VGP_DELTA_T)
{
}


bool
GPlatesAppLogic::ReconstructLayerTaskParams::should_draw_vgp(
		double current_time,
		const boost::optional<double> &age) const
{
	// Check the render settings and use them to decide if the vgp should be drawn for 
	// the current time.
	
	GPlatesPropertyValues::GeoTimeInstant geo_time = 
							GPlatesPropertyValues::GeoTimeInstant(current_time);

	switch(d_vgp_visibility_setting)
	{
		case ALWAYS_VISIBLE:
			return true;
		case TIME_WINDOW:
			if ( geo_time.is_later_than_or_coincident_with(d_vgp_earliest_time) && 
				 geo_time.is_earlier_than_or_coincident_with(d_vgp_latest_time) )
			{
				return true;
			}
			break;
		case DELTA_T_AROUND_AGE:
			if (age)
			{
				GPlatesPropertyValues::GeoTimeInstant earliest_time =
					GPlatesPropertyValues::GeoTimeInstant(*age + d_vgp_delta_t);
				GPlatesPropertyValues::GeoTimeInstant latest_time =
					GPlatesPropertyValues::GeoTimeInstant(*age - d_vgp_delta_t);
				
				if ((geo_time.is_later_than_or_coincident_with(earliest_time)) &&
					(geo_time.is_earlier_than_or_coincident_with(latest_time)))
				{
					return true;
				}
			}
			break;			
	}
	return false;
}

