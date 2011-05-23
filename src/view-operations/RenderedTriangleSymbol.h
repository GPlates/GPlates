/* $Id: RenderedSmallCircle.h 9129 2010-08-06 13:17:39Z jcannon $ */

/**
 * \file 
 * A @a RenderedGeometry derivation for @a PointOnSphere.
 * $Revision: 9129 $
 * $Date: 2010-08-06 23:17:39 +1000 (Fri, 06 Aug 2010) $
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

#ifndef GPLATES_VIEWOPERATIONS_RENDEREDTRIANGLESYMBOL_H
#define GPLATES_VIEWOPERATIONS_RENDEREDTRIANGLESYMBOL_H

#include "RenderedGeometryImpl.h"
#include "RenderedGeometryVisitor.h"
#include "gui/ColourProxy.h"


namespace GPlatesViewOperations
{
        /**
         * First attempt at rendered equilateral triangle. North-south aligned, i.e. an altitude is aligned north-south.
         * May want some kind of "max triangle size" parameter, similar to arrowhead-style geometries. No control over
         * size yet; not queryable.
         */
	class RenderedTriangleSymbol :
		public RenderedGeometryImpl
	{
	public:

		RenderedTriangleSymbol(
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
			visitor.visit_rendered_triangle_symbol(*this);
		}

		virtual
		GPlatesMaths::ProximityHitDetail::maybe_null_ptr_type
		test_proximity(
				const GPlatesMaths::ProximityCriteria &criteria) const
		{
			return d_centre.test_proximity(criteria);
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

#endif // GPLATES_VIEWOPERATIONS_RENDEREDTRIANGLESYMBOL_H
