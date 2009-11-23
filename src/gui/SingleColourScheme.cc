/* $Id$ */

/**
 * @file 
 * Contains the implementation of the SingleColourScheme class.
 *
 * Most recent change:
 *   $Date$
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

#include "SingleColourScheme.h"

GPlatesGui::SingleColourScheme::SingleColourScheme()
{
	d_colour = boost::optional<Colour>(DEFAULT_COLOUR);
}

GPlatesGui::SingleColourScheme::SingleColourScheme(
		const Colour &colour)
{
	d_colour = boost::optional<Colour>(colour);
}

boost::optional<GPlatesGui::Colour>
GPlatesGui::SingleColourScheme::get_colour(
		const GPlatesModel::ReconstructionGeometry &) const
{
	return d_colour;
}

const GPlatesGui::Colour
GPlatesGui::SingleColourScheme::DEFAULT_COLOUR = GPlatesGui::Colour::get_white();
