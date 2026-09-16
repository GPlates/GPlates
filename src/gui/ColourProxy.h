/* $Id$ */

/**
 * @file 
 * Contains the definition of the ColourProxy class.
 * 
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_COLOURPROXY_H
#define GPLATES_GUI_COLOURPROXY_H

#include <boost/optional.hpp>

#include "Colour.h"


namespace GPlatesGui
{
	/**
	 * A colour that a RenderedGeometry carries, which may be absent.
	 *
	 * Both constructors are deliberately not explicit, so a Colour or an optional
	 * Colour converts to one of these.
	 */
	class ColourProxy
	{
	public:

		ColourProxy(
				const Colour &colour) :
			d_colour(colour)
		{  }

		ColourProxy(
				boost::optional<Colour> colour) :
			d_colour(colour)
		{  }

		/**
		 * The colour, if there is one. Always check the result before dereferencing.
		 */
		const boost::optional<Colour> &
		get_colour() const
		{
			return d_colour;
		}

	private:

		boost::optional<Colour> d_colour;
	};
}

#endif  // GPLATES_GUI_COLOURPROXY_H
