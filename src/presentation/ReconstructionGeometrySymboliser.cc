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

#include "ReconstructionGeometrySymboliser.h"

#include "PointSymbol.h"
#include "PolygonSymbol.h"
#include "PolylineSymbol.h"

#include "app-logic/GeometryUtils.h"
#include "app-logic/ReconstructedFeatureGeometry.h"
#include "app-logic/ReconstructionGeometryVisitor.h"


namespace GPlatesPresentation
{
	namespace
	{
		/**
		 * Visitor to symbolise derived @a ReconstructionGeometry type.
		 */
		class VisitReconstructionGeometryDerivedType :
				public GPlatesAppLogic::ConstReconstructionGeometryVisitor
		{
		public:

			explicit
			VisitReconstructionGeometryDerivedType(
					const ReconstructionGeometrySymboliser &reconstruction_geometry_symboliser) :
				d_reconstruction_geometry_symboliser(reconstruction_geometry_symboliser)
			{  }


			//! Return the symbol
			boost::optional<Symbol::non_null_ptr_type>
			get_symbol() const
			{
				return d_symbol;
			}


			// Bring base class visit methods into scope of current class.
			using GPlatesAppLogic::ConstReconstructionGeometryVisitor::visit;

			virtual
			void
			visit(
					const GPlatesUtils::non_null_intrusive_ptr<reconstructed_feature_geometry_type> &rfg)
			{
				d_symbol = d_reconstruction_geometry_symboliser.symbolise(*rfg);
			}

		private:
			const ReconstructionGeometrySymboliser &d_reconstruction_geometry_symboliser;
			boost::optional<Symbol::non_null_ptr_type> d_symbol;
		};
	}
}


GPlatesPresentation::ReconstructionGeometrySymboliser::ReconstructionGeometrySymboliser() :
	d_point_symboliser(PointSymboliser::create()),
	d_polyline_symboliser(PolylineSymboliser::create()),
	d_polygon_symboliser(PolygonSymboliser::create())
{
	// Start with just a simple points, lines and polygons for now.
	d_point_symboliser->add_layer(PointSymboliser::Layer(PointSymboliser::SimplePoint(4.0f)));
	d_polyline_symboliser->add_layer(PolylineSymboliser::Layer(PolylineSymboliser::SimpleLine(1.5f)));
	d_polygon_symboliser->add_layer(PolygonSymboliser::Layer(PolygonSymboliser::SimpleOutline(1.5f)));
}


boost::optional<GPlatesPresentation::Symbol::non_null_ptr_type>
GPlatesPresentation::ReconstructionGeometrySymboliser::symbolise(
		const GPlatesAppLogic::ReconstructionGeometry &reconstruction_geometry) const
{
	// Call symbolise() for derived reconstruction geometry type.
	VisitReconstructionGeometryDerivedType visitor(*this);
	reconstruction_geometry.accept_visitor(visitor);
	return visitor.get_symbol();
}


boost::optional<GPlatesPresentation::Symbol::non_null_ptr_type>
GPlatesPresentation::ReconstructionGeometrySymboliser::symbolise(
		const GPlatesAppLogic::ReconstructedFeatureGeometry &reconstruct_feature_geometry) const
{
	switch (GPlatesAppLogic::GeometryUtils::get_geometry_type(*reconstruct_feature_geometry.reconstructed_geometry()))
	{
	case GPlatesMaths::GeometryType::POINT:
	case GPlatesMaths::GeometryType::MULTIPOINT:
		return d_point_symboliser->symbolise(reconstruct_feature_geometry);

	case GPlatesMaths::GeometryType::POLYLINE:
		return d_polyline_symboliser->symbolise(reconstruct_feature_geometry);

	case GPlatesMaths::GeometryType::POLYGON:
		return d_polygon_symboliser->symbolise(reconstruct_feature_geometry);

	case GPlatesMaths::GeometryType::NONE:
	default:
		break;
	}

	return boost::none;
}


void
GPlatesPresentation::ReconstructionGeometrySymboliser::set_line_width(
		float line_width) const
{
	// HACK: Remove when symbolisers are properly created and modified via the GUI.
	const_cast<ReconstructionGeometrySymboliser *>(this)
			->d_polyline_symboliser->get_layers()[0].get_simple_line()->line_width = line_width;
	const_cast<ReconstructionGeometrySymboliser *>(this)
			->d_polygon_symboliser->get_layers()[0].get_simple_outline()->line_width = line_width;
}


void
GPlatesPresentation::ReconstructionGeometrySymboliser::set_point_size(
		float point_size) const
{
	// HACK: Remove when symbolisers are properly created and modified via the GUI.
	const_cast<ReconstructionGeometrySymboliser *>(this)
			->d_point_symboliser->get_layers()[0].get_simple_point()->point_size = point_size;
}
