/* $Id$ */

/**
 * @file 
 * Contains the definition of the ColourSchemeFactory class.
 *
 * Most recent change:
 *   $Date$
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

#ifndef GPLATES_GUI_COLOURSCHEMEFACTORY_H
#define GPLATES_GUI_COLOURSCHEMEFACTORY_H

#include <boost/shared_ptr.hpp>
#include <QColor>

namespace GPlatesAppLogic
{
	class Reconstruct;
}

namespace GPlatesGui
{
	class ColourScheme;

	/**
	 * Contains static helper functions to create ColourSchemes.
	 */
	class ColourSchemeFactory
	{
	public:

		static
		boost::shared_ptr<ColourScheme>
		create_single_colour_scheme(
				const QColor &qcolor);

		static
		boost::shared_ptr<ColourScheme>
		create_default_plate_id_colour_scheme();

		static
		boost::shared_ptr<ColourScheme>
		create_regional_plate_id_colour_scheme();

		static
		boost::shared_ptr<ColourScheme>
		create_default_age_colour_scheme(
				const GPlatesAppLogic::Reconstruct &reconstruct);

		static
		boost::shared_ptr<ColourScheme>
		create_default_feature_colour_scheme();
		
	};
}

#endif  /* GPLATES_GUI_COLOURSCHEMEFACTORY_H */
