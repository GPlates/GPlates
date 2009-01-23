/* $Id$ */

/**
 * \file 
 * Partially abstract interface for creating @a RenderedGeometry types.
 * $Revision$
 * $Date$
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

#include "RenderedGeometryFactory.h"
#include "RenderedMultiPointOnSphere.h"
#include "RenderedPointOnSphere.h"
#include "RenderedPolygonOnSphere.h"
#include "RenderedPolylineOnSphere.h"
#include "maths/ConstGeometryOnSphereVisitor.h"

namespace GPlatesViewOperations
{
	namespace
	{
		/**
		 * Determines derived type of a @a GeometryOnSphere and creates
		 * a @a RenderedGeometry from it.
		 */
		class CreateRenderedGeometryFromGeometryOnSphere :
			private GPlatesMaths::ConstGeometryOnSphereVisitor
		{
		public:
			CreateRenderedGeometryFromGeometryOnSphere(
				RenderedGeometryFactory *rendered_geom_factory,
				GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geom_on_sphere,
				const GPlatesGui::Colour &colour,
				float point_size_hint,
				float line_width_hint) :
			d_rendered_geom_factory(rendered_geom_factory),
			d_geom_on_sphere(geom_on_sphere),
			d_colour(colour),
			d_point_size_hint(point_size_hint),
			d_line_width_hint(line_width_hint)
			{  }

			/**
			 * Creates a @a RenderedGeometryImpl from @a GeometryOnSphere passed into constructor.
			 */
			RenderedGeometry
			create_rendered_geometry()
			{
				d_geom_on_sphere->accept_visitor(*this);

				// Transfer created rendered geometry to caller.
				const RenderedGeometry rendered_geom = d_rendered_geom;
				d_rendered_geom = RenderedGeometry();

				return rendered_geom;
			}

		private:
			virtual
			void
			visit_multi_point_on_sphere(
					GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere)
			{
				d_rendered_geom = d_rendered_geom_factory->create_rendered_multi_point_on_sphere(
						multi_point_on_sphere, d_colour, d_point_size_hint);
			}

			virtual
			void
			visit_point_on_sphere(
					GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point_on_sphere)
			{
				d_rendered_geom = d_rendered_geom_factory->create_rendered_point_on_sphere(
						*point_on_sphere, d_colour, d_point_size_hint);
			}

			virtual
			void
			visit_polygon_on_sphere(
					GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere)
			{
				d_rendered_geom = d_rendered_geom_factory->create_rendered_polygon_on_sphere(
						polygon_on_sphere, d_colour, d_line_width_hint);
			}

			virtual
			void
			visit_polyline_on_sphere(
					GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere)
			{
				d_rendered_geom = d_rendered_geom_factory->create_rendered_polyline_on_sphere(
						polyline_on_sphere, d_colour, d_line_width_hint);
			}

			RenderedGeometryFactory *d_rendered_geom_factory;
			GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type d_geom_on_sphere;
			const GPlatesGui::Colour &d_colour;
			float d_point_size_hint;
			float d_line_width_hint;
			RenderedGeometry d_rendered_geom;
		};
	}
}



GPlatesViewOperations::RenderedGeometry
GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geom_on_sphere,
		const GPlatesGui::Colour &colour,
		float point_size_hint,
		float line_width_hint)
{
	// This is used to determine the derived type of 'geom_on_sphere'
	// and create a RenderedGeometryImpl for it.
	CreateRenderedGeometryFromGeometryOnSphere create_rendered_geom(
			this, geom_on_sphere, colour, point_size_hint, line_width_hint);

	return create_rendered_geom.create_rendered_geometry();
}

GPlatesViewOperations::RenderedGeometry
GPlatesViewOperations::RenderedGeometryFactory::create_rendered_point_on_sphere(
		GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point_on_sphere,
		const GPlatesGui::Colour &colour,
		float point_size_hint)
{
	RenderedGeometry::impl_ptr_type rendered_geom_impl(new RenderedPointOnSphere(
			point_on_sphere, colour, point_size_hint));

	return RenderedGeometry(rendered_geom_impl);
}

GPlatesViewOperations::RenderedGeometry
GPlatesViewOperations::RenderedGeometryFactory::create_rendered_multi_point_on_sphere(
		GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere,
		const GPlatesGui::Colour &colour,
		float point_size_hint)
{
	RenderedGeometry::impl_ptr_type rendered_geom_impl(new RenderedMultiPointOnSphere(
			multi_point_on_sphere, colour, point_size_hint));

	return RenderedGeometry(rendered_geom_impl);
}

GPlatesViewOperations::RenderedGeometry
GPlatesViewOperations::RenderedGeometryFactory::create_rendered_polyline_on_sphere(
		GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere,
		const GPlatesGui::Colour &colour,
		float line_width_hint)
{
	RenderedGeometry::impl_ptr_type rendered_geom_impl(new RenderedPolylineOnSphere(
			polyline_on_sphere, colour, line_width_hint));

	return RenderedGeometry(rendered_geom_impl);
}

GPlatesViewOperations::RenderedGeometry
GPlatesViewOperations::RenderedGeometryFactory::create_rendered_polygon_on_sphere(
		GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere,
		const GPlatesGui::Colour &colour,
		float line_width_hint)
{
	RenderedGeometry::impl_ptr_type rendered_geom_impl(new RenderedPolygonOnSphere(
			polygon_on_sphere, colour, line_width_hint));

	return RenderedGeometry(rendered_geom_impl);
}
