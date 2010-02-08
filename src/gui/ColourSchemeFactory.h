/* $Id$ */

/**
 * @file 
 * Contains the definition of the ColourSchemeFactory class.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, 2009, 2010 The University of Sydney, Australia
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

#include "GenericColourScheme.h"
#include "PlateIdColourPalettes.h"
#include "model/types.h"
#include "app-logic/ReconstructionGeometryUtils.h"

#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>

namespace
{
	class PlateIdPropertyExtractor
	{
	public:
		
		typedef GPlatesModel::integer_plate_id_type return_type;

		const boost::optional<return_type>
		operator()(
				const GPlatesModel::ReconstructionGeometry &reconstruction_geometry) const
		{
			return GPlatesAppLogic::ReconstructionGeometryUtils::get_plate_id(
						&reconstruction_geometry);
		}
	};
}

namespace GPlatesGui
{
	typedef GenericColourScheme<PlateIdPropertyExtractor> PlateIdColourScheme;

	class ColourSchemeFactory
	{
	public:

		static
		boost::shared_ptr<ColourScheme>
		create_default_plate_id_colour_scheme()
		{
			return boost::shared_ptr<ColourScheme>(
					new PlateIdColourScheme(new DefaultPlateIdColourPalette()));
		}

		static
		boost::shared_ptr<ColourScheme>
		create_regional_plate_id_colour_scheme()
		{
			return boost::shared_ptr<ColourScheme>(
					new PlateIdColourScheme(new RegionalPlateIdColourPalette()));
		}
	};
}

#endif  /* GPLATES_GUI_COLOURSCHEMEFACTORY_H */
