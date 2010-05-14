/* $Id$ */

/**
 * @file 
 * Contains the implementation of the VGPRenderSettings class.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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

#include "VGPRenderSettings.h"


const double
GPlatesGui::VGPRenderSettings::INITIAL_VGP_DELTA_T = 5.;


GPlatesGui::VGPRenderSettings::VGPRenderSettings() :
	d_vgp_visibility_setting(DELTA_T_AROUND_AGE),
	d_vgp_delta_t(INITIAL_VGP_DELTA_T),
	d_vgp_earliest_time(GPlatesPropertyValues::GeoTimeInstant::create_distant_past()),
	d_vgp_latest_time(GPlatesPropertyValues::GeoTimeInstant::create_distant_future()),
	d_should_draw_circular_error(true)
{  }
