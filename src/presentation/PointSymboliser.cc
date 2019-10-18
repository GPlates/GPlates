/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2019 The University of Sydney, Australia
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

#include "PointSymbol.h"
#include "PointSymboliser.h"

#include "app-logic/ReconstructedFeatureGeometry.h"


GPlatesPresentation::Symbol::non_null_ptr_type
GPlatesPresentation::PointSymboliser::symbolise(
	const GPlatesAppLogic::ReconstructedFeatureGeometry &reconstructed_feature_geometry) const
{
	PointSymbol::non_null_ptr_type point_symbol = PointSymbol::create();

	for (unsigned int layer_index = 0; layer_index < d_layers.size(); ++layer_index)
	{
		const Layer &layer = d_layers[layer_index];
		if (boost::optional<const SimplePoint &> simple_point = layer.get_simple_point())
		{
			point_symbol->add_layer(PointSymbol::Layer(PointSymbol::SimplePoint(simple_point->point_size)));
		}
	}

	return point_symbol;
}
