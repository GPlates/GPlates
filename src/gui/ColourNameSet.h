/* $Id$ */

/**
 * @file 
 * Contains the definition of the ColourNameSet class.
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

#ifndef GPLATES_GUI_COLOURNAMESET_H
#define GPLATES_GUI_COLOURNAMESET_H

#include "Colour.h"
#include <map>
#include <string>
#include <boost/optional.hpp>

namespace GPlatesGui
{
	/**
	 * ColourNameSet is a base class for classes that map colour names to Colours.
	 */
	class ColourNameSet
	{
	public:

		//! Destructor
		virtual
		~ColourNameSet()
		{
		}

		/**
		 * Retrieves a colour by @a name.
		 * @returns The corresponding colour, or boost::none if not found.
		 */
		boost::optional<Colour>
		get_colour(
				const std::string &name);

	protected:

		void
		insert_colour(
				const std::string &name,
				int r,
				int g,
				int b);

	private:

		std::map<std::string, Colour> colours;

	};

}

#endif  // GPLATES_GUI_COLOURNAMESET_H
