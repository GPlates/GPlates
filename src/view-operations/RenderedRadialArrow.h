/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
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

#ifndef GPLATES_VIEWOPERATIONS_RENDEREDRADIALARROW_H
#define GPLATES_VIEWOPERATIONS_RENDEREDRADIALARROW_H

#include "RenderedGeometryImpl.h"
#include "RenderedGeometryVisitor.h"

#include "maths/PointOnSphere.h"
#include "maths/Vector3D.h"

#include "gui/ColourProxy.h"


namespace GPlatesViewOperations
{
	/**
	 * An arrow that is radial, or normal, to the globe's surface.
	 *
	 * This is useful for rendering poles and light direction.
	 *
	 * In the 2D map views, this is not actually rendered as an arrow since an arrow would
	 * always be pointing directly out of the screen.
	 */
	class RenderedRadialArrow :
			public RenderedGeometryImpl
	{
	public:

		/**
		 * The types of (circularly symmetric) symbols used in map view and at base of arrow in globe view.
		 *
		 * They are circularly symmetric because they match the base of the arrow body in the globe
		 * view and the arrow is cylindrical. Although allowing a non-circularly symmetric is probably fine.
		 */
		enum SymbolType
		{
			SYMBOL_FILLED_CIRCLE,
			SYMBOL_CIRCLE,
			SYMBOL_CIRCLE_WITH_POINT,
			SYMBOL_CIRCLE_WITH_CROSS
		};

		/**
		 * @param position location on sphere/map
		 * @param arrow_projected_length zoom-dependent length of arrow (only in globe view).
		 * @param arrowhead_projected_size zoom-dependent size of arrow head (only in globe view).
		 * @param arrowline_projected_width zoom-dependent width of arrow body (only in globe view).
		 * @param arrow_colour colour of the arrow (body and head).
		 * @param symbol_type type of symbol to draw in map view and at base of arrow in globe view.
		 * @param symbol_size size of symbol in *scene* coordinates (only in map view).
		 *        In globe view the symbol size matches the size of the arrow (cylindrical) body.
		 * @param symbol_colour colour of the symbol.
		 */
		RenderedRadialArrow(
				const GPlatesMaths::PointOnSphere &position,
				float arrow_projected_length,
				float arrowhead_projected_size,
				float arrowline_projected_width,
				const GPlatesGui::ColourProxy &arrow_colour,
				SymbolType symbol_type,
				float symbol_size,
				const GPlatesGui::ColourProxy &symbol_colour) :
			d_position(position),
			d_arrow_projected_length(arrow_projected_length),
			d_arrowhead_projected_size(arrowhead_projected_size),
			d_arrowline_projected_width(arrowline_projected_width),
			d_arrow_colour(arrow_colour),
			d_symbol_type(symbol_type),
			d_symbol_size(symbol_size),
			d_symbol_colour(symbol_colour)
		{  }


		virtual
		void
		accept_visitor(
				ConstRenderedGeometryVisitor& visitor)
		{
			visitor.visit_rendered_radial_arrow(*this);
		}


		/**
		 * No hit detection performed because the arrow's geometry is *off* the globe and also
		 * is scaled by the viewport zoom and hence its geometry is not known until it is rendered.
		 *
		 * FIXME: Provide a way to test proximity on what the actual rendered geometry would be.
		 */
		virtual
		GPlatesMaths::ProximityHitDetail::maybe_null_ptr_type
		test_proximity(
			const GPlatesMaths::ProximityCriteria &criteria) const
		{
			// Always return the equivalent of false.
			return NULL;
		}


		/**
		 * Returns the location of the base of the arrow on the globe.
		 */
		const GPlatesMaths::PointOnSphere &
		get_position() const
		{
			return d_position;
		}


		/**
		 * Returns the length of the arrow projected onto the viewport window.
		 *
		 * The arrow length should appear to be a constant size when projected onto the
		 * viewport window regardless of the current zoom.
		 * The returned size is a proportion of the globe radius when the globe is fully zoomed out.
		 * For example, if @a get_arrow_projected_length returns 0.1 then the
		 * arrow should appear to be one tenth the globe radius when the globe is fully
		 * visible and should remain this projected size on screen as the view zooms in.
		 *
		 * This is only used for the 3D globe view because an arrow is not rendered in the 2D map views.
		 */
		float
		get_arrow_projected_length() const
		{
			return d_arrow_projected_length;
		}


		/**
		 * Returns the size of the arrowhead projected onto the viewport window.
		 *
		 * This should typically be smaller than @a get_arrow_projected_length otherwise the
		 * arrow body will not be visible.
		 *
		 * This is only used for the 3D globe view because an arrow is not rendered in the 2D map views.
		 */
		float
		get_arrowhead_projected_size() const
		{
			return d_arrowhead_projected_size;
		}


		/**
		 * Returns the width of the arrow body projected onto the viewport window.
		 *
		 * This should typically be smaller than @a get_arrowhead_projected_size otherwise
		 * it won't look like an arrow.
		 *
		 * This is only used for the 3D globe view because an arrow is not rendered in the 2D map views.
		 */
		float
		get_projected_arrowline_width() const
		{
			return d_arrowline_projected_width;
		}


		/**
		 * Returns the colour of the arrow (head and body).
		 *
		 * This is only used for the 3D globe view because an arrow is not rendered in the 2D map views.
		 */
		const GPlatesGui::ColourProxy &
		get_arrow_colour() const
		{
			return d_arrow_colour;
		}


		/**
		 * Returns the type of the symbol.
		 */
		SymbolType
		get_symbol_type() const
		{
			return d_symbol_type;
		}


		/**
		 * Returns the size of the symbol.
		 */
		float
		get_symbol_size() const
		{
			return d_symbol_size;
		}


		/**
		 * Returns the colour of the symbol.
		 */
		const GPlatesGui::ColourProxy &
		get_symbol_colour() const
		{
			return d_symbol_colour;
		}


	private:
		const GPlatesMaths::PointOnSphere d_position;
		const float d_arrow_projected_length;
		const float d_arrowhead_projected_size;
		const float d_arrowline_projected_width;
		const GPlatesGui::ColourProxy d_arrow_colour;
		SymbolType d_symbol_type;
		float d_symbol_size;
		const GPlatesGui::ColourProxy d_symbol_colour;
	};
}

#endif // GPLATES_VIEWOPERATIONS_RENDEREDRADIALARROW_H
