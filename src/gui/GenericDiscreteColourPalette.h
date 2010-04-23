/* $Id: GenericContinuousColourPalette.h 7043 2009-11-19 03:54:25Z elau $ */

/**
 * @file 
 * Contains the definition of the GenericDiscreteColourPalette class.
 *
 * Most recent change:
 *   $Date: 2009-11-19 14:54:25 +1100 (Thu, 19 Nov 2009) $
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

#ifndef GPLATES_GUI_GENERICDISCRETECOLOURPALETTE_H
#define GPLATES_GUI_GENERICDISCRETECOLOURPALETTE_H

#include <map>
#include <boost/optional.hpp>

#include "Colour.h"
#include "DiscreteColourPalette.h"


namespace GPlatesGui
{
	
	/**
	 * GenericDiscreteColourPalette is a discrete colour palette that maps
	 * arbitrary values (of one type) to a colour.
	 *
	 * The primary design consideration for this class is that it should act as a
	 * data structure for the in-memory representation of a "categorical" CPT file.
	 *
	 * A description of a "categorical" CPT file can be found at
	 * http://gmt.soest.hawaii.edu/gmt/doc/gmt/html/GMT_Docs/node68.html
	 */
	template<class T>
	class GenericDiscreteColourPalette :
		public DiscreteColourPalette<T>
	{
	public:

		/**
		 * Retrieves a Colour based on the @a value given.
		 */
		virtual
		boost::optional<Colour>
		get_colour(
			const T &value) const
		{
			if (d_mapping.empty())
			{
				return d_nan_colour;
			}

			// Return background colour if before first value.
			if (value < d_mapping.begin()->first)
			{
				return d_background_colour;
			}

			// Return foreground colour if after last value.
			if (value > d_mapping.rbegin()->first)
			{
				return d_foreground_colour;
			}

			// Else try and find the value in the map, else return NaN colour.
			std::map<T, Colour>::const_iterator iter = d_mapping.find(value);
			if (iter == d_mapping.end())
			{
				return d_nan_colour;
			}
			else
			{
				return iter->second;
			}
		}

		void
		add_mapping(
				const T &value,
				const Colour &colour)
		{
			d_mapping.insert(make_pair(value, colour));
		}

		void
		set_background_colour(
				const Colour &colour)
		{
			d_background_colour = colour;
		}

		void
		set_foreground_colour(
				const Colour &colour)
		{
			d_foreground_colour = colour;
		}

		void
		set_nan_colour(
				const Colour &colour)
		{
			d_nan_colour = colour;
		}

	private:

		std::map<T, Colour> d_mapping;

		boost::optional<Colour> d_background_colour;

		boost::optional<Colour> d_foreground_colour;

		boost::optional<Colour> d_nan_colour;

	};
}

#endif  /* GPLATES_GUI_GENERICDISCRETECOLOURPALETTE_H */
