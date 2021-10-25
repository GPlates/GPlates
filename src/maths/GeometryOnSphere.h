/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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

#ifndef GPLATES_MATHS_GEOMETRYONSPHERE_H
#define GPLATES_MATHS_GEOMETRYONSPHERE_H

#include "ProximityHitDetail.h"
#include "utils/non_null_intrusive_ptr.h"
#include "utils/NullIntrusivePointerHandler.h"
#include "utils/ReferenceCount.h"


namespace GPlatesMaths
{
	class ProximityCriteria;
	class ConstGeometryOnSphereVisitor;


	/**
	 * This class is the abstract base of all geometries on the sphere.
	 *
	 * It provides pure virtual function declarations for cloning and accepting visitors.  It
	 * also provides the functions to be used by boost::intrusive_ptr for reference-counting.
	 */
	class GeometryOnSphere :
			public GPlatesUtils::ReferenceCount<GeometryOnSphere>
	{
	public:
		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<const GeometryOnSphere>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GeometryOnSphere> non_null_ptr_to_const_type;

		/**
		 * A convenience typedef for boost::intrusive_ptr<const GeometryOnSphere>.
		 */
		typedef boost::intrusive_ptr<const GeometryOnSphere> maybe_null_ptr_to_const_type;


		virtual
		~GeometryOnSphere()
		{  }

		/**
		 * Test for a proximity hit.
		 *
		 * If there is a hit, the pointer returned will be a pointer to extra information
		 * about the hit -- for example, the specific vertex (point) or segment (GCA) of a
		 * polyline which was hit.
		 *
		 * Otherwise (ie, if there was not a hit), the pointer returned will be null.
		 */
		virtual
		ProximityHitDetail::maybe_null_ptr_type
		test_proximity(
				const ProximityCriteria &criteria) const = 0;
				
		/**
		 * Test for a proximity hit, but only on the vertices of the geometry.                                                                     
		 */
		virtual
		ProximityHitDetail::maybe_null_ptr_type
		test_vertex_proximity(
				const ProximityCriteria &criteria) const = 0;

		/**
		 * Accept a ConstGeometryOnSphereVisitor instance.
		 *
		 * See the Visitor pattern (p.331) in Gamma95 for information on the purpose of
		 * this function.
		 */
		virtual
		void
		accept_visitor(
				ConstGeometryOnSphereVisitor &visitor) const = 0;

		/**
		 * Return this instance as a non-null pointer.
		 */
		const non_null_ptr_to_const_type
		get_non_null_pointer() const
		{
			return GPlatesUtils::get_non_null_pointer(this);
		}
	};
}

#endif  // GPLATES_MATHS_GEOMETRYONSPHERE_H
