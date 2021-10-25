/**
 * \file
 * $Revision: 8672 $
 * $Date: 2010-06-10 07:00:38 +0200 (to, 10 jun 2010) $
 *
 * Copyright (C) 2010 Geological Survey of Norway
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

#ifndef GPLATES_VIEWOPERATIONS_RENDEREDSQUARESYMBOL_H
#define GPLATES_VIEWOPERATIONS_RENDEREDSQUARESYMBOL_H
#include "RenderedGeometryImpl.h"
#include "RenderedGeometryVisitor.h"
#include "gui/ColourProxy.h"
#include "maths/PointOnSphere.h"


namespace GPlatesViewOperations
{

	/**
	 * First attempt at rendered square. North-south aligned, i.e. one of the edges aligned north-south.
	 */
	class RenderedSquareSymbol :
		public RenderedGeometryImpl
	{
	public:

		RenderedSquareSymbol(
			const GPlatesMaths::PointOnSphere &centre,
			const GPlatesGui::ColourProxy &colour,
			unsigned int size,
			bool filled,
			float line_width_hint) :
		    d_centre(centre),
		    d_colour(colour),
		    d_size(size),
		    d_is_filled(filled),
		    d_line_width_hint(line_width_hint)

		{  }


		virtual
		void
		accept_visitor(
				ConstRenderedGeometryVisitor& visitor)
		{
			visitor.visit_rendered_square_symbol(*this);
		}

		virtual
		GPlatesMaths::ProximityHitDetail::maybe_null_ptr_type
		test_proximity(
				const GPlatesMaths::ProximityCriteria &criteria) const
		{
			return d_centre.test_proximity(criteria);
		}

		virtual
		GPlatesMaths::ProximityHitDetail::maybe_null_ptr_type
		test_vertex_proximity(
				const GPlatesMaths::ProximityCriteria &criteria) const
		{
			return d_centre.test_vertex_proximity(criteria);
		}


		const GPlatesMaths::PointOnSphere &
		get_centre() const
		{
			return d_centre;
		}


		const GPlatesGui::ColourProxy &
		get_colour() const
		{
			return d_colour;
		}

		float
		get_line_width_hint() const
		{
			return d_line_width_hint;
		}

		bool
		get_is_filled() const
		{
		    return d_is_filled;
		}

		unsigned int
		get_size() const
		{
		     return d_size;
		}

	private:

		GPlatesMaths::PointOnSphere d_centre;

		GPlatesGui::ColourProxy d_colour;
		unsigned int d_size;
		bool d_is_filled;
		float d_line_width_hint;

	};
}


#endif // GPLATES_VIEWOPERATIONS_RENDEREDSQUARESYMBOL_H
