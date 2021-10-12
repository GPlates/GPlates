/* $Id$ */

/**
 * \file 
 * Contains the implementation of the CptColourPalette class.
 *
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2009, 2010 The University of Sydney, Australia
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

#include "CptColourPalette.h"


GPlatesGui::ColourSlice::ColourSlice(
		value_type lower_value_,
		boost::optional<Colour> lower_colour_,
		value_type upper_value_,
		boost::optional<Colour> upper_colour_,
		ColourScaleAnnotation::Type annotation_,
		boost::optional<QString> label_) :
	d_lower_value(lower_value_),
	d_upper_value(upper_value_),
	d_lower_colour(lower_colour_),
	d_upper_colour(upper_colour_),
	d_annotation(annotation_),
	d_label(label_)
{
}


bool
GPlatesGui::ColourSlice::can_handle(
		value_type value) const
{
	return d_lower_value <= value && value <= d_upper_value;
}


boost::optional<GPlatesGui::Colour>
GPlatesGui::ColourSlice::get_colour(
		value_type value) const
{
	if (d_lower_colour && d_upper_colour)
	{
		value_type position = (value - d_lower_value) / (d_upper_value - d_lower_value);
		return Colour::linearly_interpolate(
				*d_lower_colour,
				*d_upper_colour,
				position.dval());
	}
	else
	{
		return boost::none;
	}
}


bool
GPlatesGui::operator<(
		ColourSlice::value_type value,
		const ColourSlice &colour_slice)
{
	return value < colour_slice.lower_value();
}


bool
GPlatesGui::operator>(
		ColourSlice::value_type value,
		const ColourSlice &colour_slice)
{
	return value > colour_slice.upper_value();
}

