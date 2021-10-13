/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008, 2010 The University of Sydney, Australia
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
#include <math.h>
#include <QDebug>
#include "ColourSpectrum.h"

GPlatesGui::ColourSpectrum::ColourSpectrum(
		const GPlatesGui::Colour& upper_colour,
		const GPlatesGui::Colour& lower_colour,
		const double upper_bound,
		const double lower_bound) :
	d_upper_colour(upper_colour),
	d_lower_colour(lower_colour),
	d_upper_bound(upper_bound),
	d_lower_bound(lower_bound)
{
	if(d_upper_bound < d_lower_bound)
		qWarning() << "The upper bound is less than the lower bound.";
}

#if 0
GPlatesGui::ColourSpectrum::ColourSpectrum() 
{
	using namespace boost::assign;
	d_colours +=
			Colour(1, 0, 0),
			Colour(1, 1, 0),
			Colour(0, 1, 0),
			Colour(0, 1, 1),
			Colour(0, 0, 1),
			Colour(1, 0, 1)
			;
}
#endif 

boost::optional<GPlatesGui::Colour>
GPlatesGui::ColourSpectrum::get_colour_at(double position) const
{

	if(position > d_upper_bound || position < d_lower_bound)
		return boost::none;
	
	double position_in_range = (position - d_lower_bound) / (d_upper_bound - d_lower_bound);

	return Colour::linearly_interpolate(
			d_upper_colour,
			d_lower_colour,
			position_in_range);
}

