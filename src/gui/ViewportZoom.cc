/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2004, 2005, 2006, 2007, 2010 The University of Sydney, Australia
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
GPlatesGui::ViewportZoom::s_max_zoom_level = 40;


const double
GPlatesGui::ViewportZoom::s_min_zoom_percent = 100.0;


const double
GPlatesGui::ViewportZoom::s_max_zoom_percent = 10000.0;


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
		emit zoom_changed();
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

