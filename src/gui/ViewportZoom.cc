/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2004, 2005, 2006, 2007, 2010 The University of Sydney, Australia
 * Copyright (C) 2011 Geological Survey of Norway
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


const int
GPlatesGui::ViewportZoom::s_min_zoom_level = 0;


const int
GPlatesGui::ViewportZoom::s_max_zoom_level = 60;


const double
GPlatesGui::ViewportZoom::s_min_zoom_percent = 100.0;


// NOTE: When increasing the maximum zoom percent, be sure to change the maximum zoom level such
// that the original max zoom level and original max percent still match up.
// This means that the various zoom control widgets and zoom keyboard shortcuts (that adjust zoom
// *level*) will still change the zoom at the same rate (ie, change-in-zoom-percent per unit time).
//
// For example, when the max zoom percent was increased from 10,000 to 100,000 the corresponding
// max zoom level was increased from 40 to 60 because...
//
//    zoom_percent(level=40) = pow(10, (40 - 0) / (60 - 0) * (log10(100,000) - log10(100)) + log10(100))
//                           = pow(10, 4 / 6 * (5 - 2) + 2)
//                           = pow(10, 4)
//                           = 10,000
//
// ...and so a zoom level of 40 corresponds to a zoom percent of 10,000 which were the old maximums.
//
const double
GPlatesGui::ViewportZoom::s_max_zoom_percent = 100000.0;


double
GPlatesGui::ViewportZoom::min_zoom_power()
{
	static const double MIN_ZOOM_POWER = std::log10(s_min_zoom_percent);
	return MIN_ZOOM_POWER;
}


double
GPlatesGui::ViewportZoom::max_zoom_power()
{
	static const double MAX_ZOOM_POWER = std::log10(s_max_zoom_percent);
	return MAX_ZOOM_POWER;
}


GPlatesGui::ViewportZoom::ViewportZoom() :
	d_zoom_percent(s_min_zoom_percent)
{  }


double
GPlatesGui::ViewportZoom::zoom_level() const
{
	double curr_power = std::log10(d_zoom_percent);
	return (curr_power - min_zoom_power()) / (max_zoom_power() - min_zoom_power()) *
		(s_max_zoom_level - s_min_zoom_level) + s_min_zoom_level;
}


void
GPlatesGui::ViewportZoom::zoom_in(
		double num_levels)
{
	double curr_zoom_level = zoom_level();
	set_zoom_level(curr_zoom_level + num_levels);
}


void
GPlatesGui::ViewportZoom::zoom_out(
		double num_levels)
{
	double curr_zoom_level = zoom_level();
	set_zoom_level(curr_zoom_level - num_levels);
}


void
GPlatesGui::ViewportZoom::reset_zoom()
{
	set_zoom_percent(s_min_zoom_percent);
}


void
GPlatesGui::ViewportZoom::set_zoom_percent(
		double new_zoom_percent)
{
	// First, ensure the values lie within the valid zoom percent range.
	if (new_zoom_percent > s_max_zoom_percent)
	{
		new_zoom_percent = s_max_zoom_percent;
	}
	if (new_zoom_percent < s_min_zoom_percent)
	{
		new_zoom_percent = s_min_zoom_percent;
	}

	if (GPlatesMaths::Real(new_zoom_percent) != d_zoom_percent)
	{
		d_zoom_percent = new_zoom_percent;
		Q_EMIT zoom_changed();
		Q_EMIT send_zoom_to_stdout(d_zoom_percent);
	}
}


void
GPlatesGui::ViewportZoom::set_zoom_level(
		double new_zoom_level)
{
	double power = (new_zoom_level - s_min_zoom_level) / (s_max_zoom_level - s_min_zoom_level) *
		(max_zoom_power() - min_zoom_power()) + min_zoom_power();
	set_zoom_percent(std::pow(10.0, power));
}

