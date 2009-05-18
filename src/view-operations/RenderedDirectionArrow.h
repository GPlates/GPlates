/* $Id$ */

/**
 * \file 
 * A @a RenderedGeometry derivation for a single line segment with an arrowhead indicating a direction.
 * $Revision$
 * $Date$
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

#ifndef GPLATES_VIEWOPERATIONS_RENDEREDDIRECTIONARROW_H
#define GPLATES_VIEWOPERATIONS_RENDEREDDIRECTIONARROW_H

#include "RenderedGeometryImpl.h"
#include "RenderedGeometryVisitor.h"
#include "maths/PointOnSphere.h"
#include "maths/Vector3D.h"
#include "gui/Colour.h"


namespace GPlatesViewOperations
{
	class RenderedDirectionArrow :
		public RenderedGeometryImpl
	{
	public:
		RenderedDirectionArrow(
				const GPlatesMaths::PointOnSphere &start,
				const GPlatesMaths::Vector3D &arrow_direction,
				const float ratio_size_arrowhead_to_arrowline,
				const GPlatesGui::Colour &colour,
				float arrowline_width_hint) :
			d_start_position(start),
			d_arrow_direction(arrow_direction),
			d_ratio_size_arrowhead_to_arrowline(ratio_size_arrowhead_to_arrowline),
			d_colour(colour),
			d_arrowline_width_hint(arrowline_width_hint)
		{  }


		virtual
		void
		accept_visitor(
				ConstRenderedGeometryVisitor& visitor)
		{
			visitor.visit_rendered_direction_arrow(*this);
		}


		/**
		 * No hit detection performed because a rendered arrow is not meant to be picked
		 * or selected by the user.
		 * So if the user wants to pick or select a velocity vector for example then
		 * they can select the point or multipoint geometry that this arrow is decorating.
		 */
		virtual
		GPlatesMaths::ProximityHitDetail::maybe_null_ptr_type
		test_proximity(
				const GPlatesMaths::ProximityCriteria &criteria) const
		{
			// Always return the equivalent of false.
			return NULL;
		}


		const GPlatesMaths::PointOnSphere &
		get_start_position() const
		{
			return d_start_position;
		}


		const GPlatesMaths::Vector3D &
		get_arrow_direction() const
		{
			return d_arrow_direction;
		}


		float
		get_ratio_size_arrowhead_to_arrowline() const
		{
			return d_ratio_size_arrowhead_to_arrowline;
		}


		const GPlatesGui::Colour &
		get_colour() const
		{
			return d_colour;
		}


		float
		get_arrowline_width_hint() const
		{
			return d_arrowline_width_hint;
		}


	private:
		const GPlatesMaths::PointOnSphere d_start_position;
		const GPlatesMaths::Vector3D d_arrow_direction;
		const float d_ratio_size_arrowhead_to_arrowline;
		const GPlatesGui::Colour d_colour;
		const float d_arrowline_width_hint;
	};
}

#endif // GPLATES_VIEWOPERATIONS_RENDEREDDIRECTIONARROW_H
