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

#include "DiscreteColourPalette.h"
#include <map>

namespace GPlatesGui
{
	
	/**
	 * GenericDiscreteColourPalette is a discrete colour palette that maps
	 * arbitrary values (of one type) to a colour.
	 */
	template <class T>
	class GenericDiscreteColourPalette :
		public DiscreteColourPalette<T>
	{
	public:

		/**
		 * Constructs an instance of GenericContinuousColourPalette
		 */
		GenericContinuousColourPalette()
		{
		}

		/**
		 * Constructs an instance of GenericContinuousColourPalette
		 * @param mapping A mapping of values to colours.
		 */
		explicit
		GenericContinuousColourPalette(
				const std::map<T, Colour> &mapping) :
			d_mapping(mapping)
		{
		}

		/**
		 * Retrieves a Colour based on the @a value given.
		 */
		virtual
		boost::optional<Colour>
		get_colour(
			const T &value) const
		{
			std::map<T, Colour>::iterator iter = d_mapping.find(value);
			if (iter == d_mapping.end())
			{
				return boost::none;
			}
			else
			{
				return boost::optional<Colour>(iter->second);
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
		remove_mapping(
				const T &value)
		{
			d_mapping.erase(value);
		}

	private:

		std::map<T, Colour> d_mapping;

	};
}

#endif  /* GPLATES_GUI_GENERICDISCRETECOLOURPALETTE_H */
