/* $Id$ */

/**
 * @file 
 * Contains the implementation of the ColourNameSet class.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#include "ColourNameSet.h"

boost::optional<GPlatesGui::Colour>
GPlatesGui::ColourNameSet::get_colour(
		const std::string &name) const
{
	std::map<std::string, Colour>::const_iterator iter = colours.find(name);
	if (iter == colours.end())
	{
		return boost::none;
	}
	else
	{
		return boost::optional<Colour>(iter->second);
	}
}

void
GPlatesGui::ColourNameSet::insert_colour(
		const std::string &name,
		int r,
		int g,
		int b)
{
	colours.insert(std::make_pair(
				name,
				Colour(
					static_cast<GLfloat>(r) / 255.0f,
					static_cast<GLfloat>(g) / 255.0f,
					static_cast<GLfloat>(b) / 255.0f)));
}

