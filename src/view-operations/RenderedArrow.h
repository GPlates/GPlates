/**
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

#ifndef GPLATES_VIEWOPERATIONS_RENDEREDARROW_H
#define GPLATES_VIEWOPERATIONS_RENDEREDARROW_H

#include "RenderedGeometryImpl.h"
#include "RenderedGeometryVisitor.h"

#include "maths/PointOnSphere.h"
#include "maths/Vector3D.h"

#include "gui/Colour.h"


namespace GPlatesViewOperations
{
	/**
	 * An arrow with base on the surface of the Earth.
	 */
	class RenderedArrow :
			public RenderedGeometryImpl
	{
	public:

		RenderedArrow(
				const GPlatesMaths::PointOnSphere &start,
				const GPlatesMaths::Vector3D &vector,
				const GPlatesGui::Colour &colour,
				float arrowhead_size,
				float arrow_body_width) :
			d_start_position(start),
			d_vector(vector),
			d_colour(colour),
			d_arrowhead_size(arrowhead_size),
			d_arrow_body_width(arrow_body_width)
		{  }


		void
		accept_visitor(
				ConstRenderedGeometryVisitor &visitor) override
		{
			visitor.visit_rendered_arrow(*this);
		}


		/**
		 * No hit detection performed because a rendered arrow is not meant to be picked
		 * or selected by the user.
		 * So if the user wants to pick or select a velocity vector for example then
		 * they can select the point or multipoint geometry that this arrow is decorating.
		 */
		GPlatesMaths::ProximityHitDetail::maybe_null_ptr_type
		test_proximity(
				const GPlatesMaths::ProximityCriteria &criteria) const  override
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
		get_vector() const
		{
			return d_vector;
		}

		const GPlatesGui::Colour &
		get_colour() const
		{
			return d_colour;
		}

		float
		get_arrowhead_size() const
		{
			return d_arrowhead_size;
		}

		float
		get_arrow_body_width() const
		{
			return d_arrow_body_width;
		}


	private:
		const GPlatesMaths::PointOnSphere d_start_position;
		const GPlatesMaths::Vector3D d_vector;
		const GPlatesGui::Colour d_colour;
		const float d_arrowhead_size;
		const float d_arrow_body_width;
	};
}

#endif // GPLATES_VIEWOPERATIONS_RENDEREDARROW_H
