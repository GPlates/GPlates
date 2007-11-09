/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2004, 2005, 2006, 2007 The University of Sydney, Australia
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
#include "maths/types.h"

#define MIN_ZOOM_POWER 1.0
#define MAX_ZOOM_POWER 3.0
#define ZOOM_POWER_DELTA 0.05


const int
GPlatesGui::ViewportZoom::s_min_zoom_level = 0;


const int
GPlatesGui::ViewportZoom::s_max_zoom_level =
 static_cast<int>(((MAX_ZOOM_POWER - MIN_ZOOM_POWER) / ZOOM_POWER_DELTA) + 0.5);


const int
GPlatesGui::ViewportZoom::s_initial_zoom_level = 0;


const double
GPlatesGui::ViewportZoom::s_min_zoom_percent = 100.0;


const double
GPlatesGui::ViewportZoom::s_max_zoom_percent = 10000.0;


namespace
{
	inline
	double
	calc_zoom_power_from_level(
			int level)
	{
		return (1.0 + level * ZOOM_POWER_DELTA);
	}


	inline
	double
	calc_zoom_percent_from_power(
			const double &power)
	{
		return std::pow(10.0, power + 1.0);
	}


	inline
	double
	calc_zoom_power_from_percent(
			const double &percent)
	{
		return std::log10(percent) - 1.0;
	}


	inline
	double
	calc_in_between_zoom_level_from_power(
			const double &power)
	{
		return ((power - 1.0) / ZOOM_POWER_DELTA);
	}


	inline
	int
	calc_next_zoom_level_in(
			const double &power)
	{
		// To calcalute the next zoom level "in" (ie, zoomed in), we want to round the zoom
		// level UP.
		return static_cast<int>(calc_in_between_zoom_level_from_power(power) + 0.5);
	}


	inline
	int
	calc_next_zoom_level_out(
			const double &power)
	{
		// To calcalute the next zoom level "out" (ie, zoomed out), we want to round the
		// zoom level DOWN.
		return static_cast<int>(calc_in_between_zoom_level_from_power(power) - 0.5);
	}
}


void
GPlatesGui::ViewportZoom::zoom_in()
{
	if (d_zoom_percent_is_synced_with_level) {
		// Since the zoom-percent is synced, let's just increment the zoom level normally,
		// then update the zoom percent.
		if (d_zoom_level < s_max_zoom_level) {
			++d_zoom_level;
			update_zoom_percent_from_level();
		}
	} else {
		// We need to calculate the next zoom-level "in".
		int next_level_in = calc_next_zoom_level_in(calc_zoom_power_from_percent(d_zoom_percent));
		if (next_level_in <= s_max_zoom_level) {
			d_zoom_level = next_level_in;
			update_zoom_percent_from_level();
		}
	}
}


void
GPlatesGui::ViewportZoom::zoom_out()
{
	if (d_zoom_percent_is_synced_with_level) {

		// Since the zoom-percent is synced, let's just decrement the zoom level normally,
		// then update the zoom percent.
		if (d_zoom_level > s_min_zoom_level) {

			--d_zoom_level;
			update_zoom_percent_from_level();
		}
	} else {

		// We need to calculate the next zoom-level "out".
		int next_level_out = calc_next_zoom_level_out(calc_zoom_power_from_percent(d_zoom_percent));

		if (next_level_out >= s_min_zoom_level) {

			d_zoom_level = next_level_out;
			update_zoom_percent_from_level();
		}
	}
}


void
GPlatesGui::ViewportZoom::reset_zoom()
{
	d_zoom_level = s_initial_zoom_level;
	update_zoom_percent_from_level();
}


void
GPlatesGui::ViewportZoom::set_zoom(
		double new_zoom_percent)
{
	// First, ensure the values lie within the valid zoom percent range.
	if (new_zoom_percent > s_max_zoom_percent) {
		new_zoom_percent = s_max_zoom_percent;
	}
	if (new_zoom_percent < s_min_zoom_percent) {
		new_zoom_percent = s_min_zoom_percent;
	}

	if (GPlatesMaths::Real(new_zoom_percent) != d_zoom_percent) {
		d_zoom_percent = new_zoom_percent;
		d_zoom_percent_is_synced_with_level = false;
	}
}


void
GPlatesGui::ViewportZoom::update_zoom_percent_from_level()
{
	d_zoom_percent = calc_zoom_percent_from_power(calc_zoom_power_from_level(d_zoom_level));
	d_zoom_percent_is_synced_with_level = true;
}
