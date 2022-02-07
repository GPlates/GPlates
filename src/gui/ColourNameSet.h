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

#include <map>
#include <string>
#include <vector>
#include <boost/optional.hpp>
#include <boost/noncopyable.hpp>

#include "Colour.h"

namespace GPlatesGui
{
	/**
	 * ColourNameSet is a base class for classes that map colour names to Colours.
	 */
	class ColourNameSet :
			public boost::noncopyable
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
				const std::string &name) const;

		const std::map<std::string, std::vector<int> >&
		get_name_map() const
		{
			return d_color_name_table;
		}

	protected:

		void
		insert_colour(
				const std::string &name,
				int r,
				int g,
				int b);

	private:

		std::map<std::string, Colour> colours;
		std::map<std::string, std::vector<int> > d_color_name_table;

	};

}

#endif  // GPLATES_GUI_COLOURNAMESET_H
