/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2004, 2005, 2006 The University of Sydney, Australia
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

#include <cmath>
#include "ViewportZoom.h"

#define MIN_ZOOM_POWER 1.0
#define MAX_ZOOM_POWER 2.0

#define NUM_POWER_INCRS 20


const double
GPlatesGui::ViewportZoom::m_min_zoom_power = MIN_ZOOM_POWER;


const double
GPlatesGui::ViewportZoom::m_max_zoom_power = MAX_ZOOM_POWER;


const double
GPlatesGui::ViewportZoom::m_zoom_power_delta =
 ((MAX_ZOOM_POWER - MIN_ZOOM_POWER) / NUM_POWER_INCRS);


const double
GPlatesGui::ViewportZoom::m_initial_zoom_power = MIN_ZOOM_POWER;


void
GPlatesGui::ViewportZoom::zoom_in() {

	double incred_zoom_power = m_zoom_power + m_zoom_power_delta;

	// Allow for a bit of accumulated floating-point error...
	if (incred_zoom_power < (m_max_zoom_power + (m_zoom_power_delta / 2))) {

		m_zoom_power = incred_zoom_power;
		update_zoom_params();
	}
}


void
GPlatesGui::ViewportZoom::zoom_out() {

	double decred_zoom_power = m_zoom_power - m_zoom_power_delta;

	// Allow for a bit of accumulated floating-point error...
	if (decred_zoom_power > (m_min_zoom_power - (m_zoom_power_delta / 2))) {

		m_zoom_power = decred_zoom_power;
		update_zoom_params();
	}
}


void
GPlatesGui::ViewportZoom::reset_zoom() {

	m_zoom_power = m_initial_zoom_power;
	update_zoom_params();
}


void
GPlatesGui::ViewportZoom::update_zoom_params() {

	double f_zoom_percent = std::pow(10.0, m_zoom_power + 1.0);

	m_zoom_percent = static_cast< unsigned >(f_zoom_percent + 0.5);
	m_zoom_factor = f_zoom_percent / 100.0;
}
