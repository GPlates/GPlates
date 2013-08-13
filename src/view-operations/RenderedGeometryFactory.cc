/* $Id$ */

/**
 * \file 
 * Object factory for creating @a RenderedGeometry types.
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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


#include "RenderedGeometryFactory.h"
#include "RenderedArrowedPolyline.h"
#include "RenderedColouredEdgeSurfaceMesh.h"
#include "RenderedColouredTriangleSurfaceMesh.h"
#include "RenderedCrossSymbol.h"
#include "RenderedDirectionArrow.h"
#include "RenderedEllipse.h"
#include "RenderedMultiPointOnSphere.h"
#include "RenderedPointOnSphere.h"
#include "RenderedPolygonOnSphere.h"
#include "RenderedPolylineOnSphere.h"
#include "RenderedResolvedRaster.h"
#include "RenderedResolvedScalarField3D.h"
#include "RenderedReconstructionGeometry.h"
#include "RenderedSmallCircle.h"
#include "RenderedSmallCircleArc.h"
#include "RenderedSquareSymbol.h"
#include "RenderedStrainMarkerSymbol.h"
#include "RenderedCircleSymbol.h"
#include "RenderedString.h"
#include "RenderedTriangleSymbol.h"
#include "gui/Symbol.h"
#include "maths/ConstGeometryOnSphereVisitor.h"


namespace GPlatesViewOperations
{
	namespace RenderedGeometryFactory
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
					GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geom_on_sphere,
					const GPlatesGui::ColourProxy &colour,
					float point_size_hint,
					float line_width_hint,
					bool fill_polygon,
					bool fill_polyline,
					const GPlatesGui::Colour &fill_modulate_colour,
					const boost::optional<GPlatesGui::Symbol> &symbol_ = boost::none) :
				d_geom_on_sphere(geom_on_sphere),
				d_colour(colour),
				d_point_size_hint(point_size_hint),
				d_line_width_hint(line_width_hint),
				d_fill_polygon(fill_polygon),
				d_fill_polyline(fill_polyline),
				d_fill_modulate_colour(fill_modulate_colour),
				d_symbol(symbol_)
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
					d_rendered_geom = create_rendered_multi_point_on_sphere(
							multi_point_on_sphere, d_colour, d_point_size_hint);
				}

				virtual
				void
				visit_point_on_sphere(
						GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point_on_sphere)
				{
					if (d_symbol)
					{
						switch(d_symbol->d_symbol_type){
						case GPlatesGui::Symbol::TRIANGLE:
						d_rendered_geom = create_rendered_triangle_symbol(
							*point_on_sphere, d_colour, d_symbol->d_size, d_symbol->d_filled, d_line_width_hint);
						break;

						case GPlatesGui::Symbol::SQUARE:
						d_rendered_geom = create_rendered_square_symbol(
							*point_on_sphere, d_colour, d_symbol->d_size, d_symbol->d_filled, d_line_width_hint);
						break;

						case GPlatesGui::Symbol::CIRCLE:
						d_rendered_geom = create_rendered_circle_symbol(
							*point_on_sphere, d_colour, d_symbol->d_size, d_symbol->d_filled, d_line_width_hint);
						break;

						case GPlatesGui::Symbol::CROSS:
						d_rendered_geom = create_rendered_cross_symbol(
							*point_on_sphere, d_colour, d_symbol->d_size, d_line_width_hint);
						break;

						case GPlatesGui::Symbol::STRAIN_MARKER:
						d_rendered_geom = create_rendered_strain_marker_symbol(
							*point_on_sphere, d_colour, d_symbol->d_size, d_line_width_hint, 
							d_symbol->d_scale_x.get(), d_symbol->d_scale_y.get(), d_symbol->d_angle.get());
						break;

						default:
						d_rendered_geom = create_rendered_point_on_sphere(
								*point_on_sphere, d_colour, d_point_size_hint);
						break;
						}

					}
					else
					{
						d_rendered_geom = create_rendered_point_on_sphere(
								*point_on_sphere, d_colour, d_point_size_hint);
					}

				}

				virtual
				void
				visit_polygon_on_sphere(
						GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere)
				{
					d_rendered_geom = create_rendered_polygon_on_sphere(
							polygon_on_sphere,
							d_colour,
							d_line_width_hint,
							d_fill_polygon,
							d_fill_modulate_colour);
				}

				virtual
				void
				visit_polyline_on_sphere(
						GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere)
				{
					d_rendered_geom = create_rendered_polyline_on_sphere(
							polyline_on_sphere,
							d_colour,
							d_line_width_hint,
							d_fill_polyline,
							d_fill_modulate_colour);
				}

				GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type d_geom_on_sphere;
				const GPlatesGui::ColourProxy &d_colour;
				float d_point_size_hint;
				float d_line_width_hint;
				bool d_fill_polygon;
				bool d_fill_polyline;
				GPlatesGui::Colour d_fill_modulate_colour;
				const boost::optional<GPlatesGui::Symbol> &d_symbol;
				RenderedGeometry d_rendered_geom;
			};
		}
	}
}


GPlatesViewOperations::RenderedGeometry
GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geom_on_sphere,
		const GPlatesGui::ColourProxy &colour,
		float point_size_hint,
		float line_width_hint,
		bool fill_polygon,
		bool fill_polyline,
		const GPlatesGui::Colour &fill_modulate_colour,
		const boost::optional<GPlatesGui::Symbol> &symbol)
{
	// This is used to determine the derived type of 'geom_on_sphere'
	// and create a RenderedGeometryImpl for it.
	CreateRenderedGeometryFromGeometryOnSphere create_rendered_geom(
			geom_on_sphere,
			colour,
			point_size_hint,
			line_width_hint,
			fill_polygon,
			fill_polyline,
			fill_modulate_colour,
			symbol);

	return create_rendered_geom.create_rendered_geometry();
}

GPlatesViewOperations::RenderedGeometry
GPlatesViewOperations::RenderedGeometryFactory::create_rendered_point_on_sphere(
		GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point_on_sphere,
		const GPlatesGui::ColourProxy &colour,
		float point_size_hint)
{
	RenderedGeometry::impl_ptr_type rendered_geom_impl(new RenderedPointOnSphere(
			point_on_sphere, colour, point_size_hint));

	return RenderedGeometry(rendered_geom_impl);
}

GPlatesViewOperations::RenderedGeometry
GPlatesViewOperations::RenderedGeometryFactory::create_rendered_multi_point_on_sphere(
		GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere,
		const GPlatesGui::ColourProxy &colour,
		float point_size_hint)
{
	RenderedGeometry::impl_ptr_type rendered_geom_impl(new RenderedMultiPointOnSphere(
			multi_point_on_sphere, colour, point_size_hint));

	return RenderedGeometry(rendered_geom_impl);
}

GPlatesViewOperations::RenderedGeometry
GPlatesViewOperations::RenderedGeometryFactory::create_rendered_polyline_on_sphere(
		GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere,
		const GPlatesGui::ColourProxy &colour,
		float line_width_hint,
		bool filled,
		const GPlatesGui::Colour &fill_modulate_colour)
{
	RenderedGeometry::impl_ptr_type rendered_geom_impl(
			new RenderedPolylineOnSphere(
					polyline_on_sphere,
					colour,
					line_width_hint,
					filled,
					fill_modulate_colour));

	return RenderedGeometry(rendered_geom_impl);
}

GPlatesViewOperations::RenderedGeometry
GPlatesViewOperations::RenderedGeometryFactory::create_rendered_polygon_on_sphere(
		GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere,
		const GPlatesGui::ColourProxy &colour,
		float line_width_hint,
		bool filled,
		const GPlatesGui::Colour &fill_modulate_colour)
{
	RenderedGeometry::impl_ptr_type rendered_geom_impl(
			new RenderedPolygonOnSphere(
					polygon_on_sphere,
					colour,
					line_width_hint,
					filled,
					fill_modulate_colour));

	return RenderedGeometry(rendered_geom_impl);
}


GPlatesViewOperations::RenderedGeometry
GPlatesViewOperations::RenderedGeometryFactory::create_rendered_coloured_edge_surface_mesh(
		const RenderedColouredEdgeSurfaceMesh::edge_seq_type &mesh_edges,
		const RenderedColouredEdgeSurfaceMesh::vertex_seq_type &mesh_vertices,
		float line_width_hint)
{
	RenderedGeometry::impl_ptr_type rendered_geom_impl(
			new RenderedColouredEdgeSurfaceMesh(
					mesh_edges.begin(), mesh_edges.end(),
					mesh_vertices.begin(), mesh_vertices.end(),
					line_width_hint));

	return RenderedGeometry(rendered_geom_impl);
}


GPlatesViewOperations::RenderedGeometry
GPlatesViewOperations::RenderedGeometryFactory::create_rendered_coloured_triangle_surface_mesh(
		const RenderedColouredTriangleSurfaceMesh::triangle_seq_type &mesh_triangles,
		const RenderedColouredTriangleSurfaceMesh::vertex_seq_type &mesh_vertices)
{
	RenderedGeometry::impl_ptr_type rendered_geom_impl(
			new RenderedColouredTriangleSurfaceMesh(
					mesh_triangles.begin(), mesh_triangles.end(),
					mesh_vertices.begin(), mesh_vertices.end()));

	return RenderedGeometry(rendered_geom_impl);
}


GPlatesViewOperations::RenderedGeometry
GPlatesViewOperations::RenderedGeometryFactory::create_rendered_resolved_raster(
		const GPlatesAppLogic::resolved_raster_non_null_ptr_to_const_type &resolved_raster,
		const GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type &raster_colour_palette,
		const GPlatesGui::Colour &raster_modulate_colour,
		float normal_map_height_field_scale_factor)
{
	RenderedGeometry::impl_ptr_type rendered_geom_impl(
			new RenderedResolvedRaster(
					resolved_raster,
					raster_colour_palette,
					raster_modulate_colour,
					normal_map_height_field_scale_factor));

	return RenderedGeometry(rendered_geom_impl);
}


GPlatesViewOperations::RenderedGeometry
GPlatesViewOperations::RenderedGeometryFactory::create_rendered_resolved_scalar_field_3d(
		const GPlatesAppLogic::resolved_scalar_field_3d_non_null_ptr_to_const_type &resolved_scalar_field,
		const ScalarField3DRenderParameters &scalar_field_render_parameters)
{
	RenderedGeometry::impl_ptr_type rendered_geom_impl(
			new RenderedResolvedScalarField3D(
					resolved_scalar_field,
					scalar_field_render_parameters));

	return RenderedGeometry(rendered_geom_impl);
}


GPlatesViewOperations::RenderedGeometry
GPlatesViewOperations::RenderedGeometryFactory::create_rendered_direction_arrow(
		const GPlatesMaths::PointOnSphere &start,
		const GPlatesMaths::Vector3D &arrow_direction,
		const float ratio_unit_vector_direction_to_globe_radius,
		const GPlatesGui::ColourProxy &colour,
		const float ratio_arrowhead_size_to_globe_radius,
		const float arrowline_width_hint)
{
	const GPlatesMaths::Vector3D scaled_direction =
			ratio_unit_vector_direction_to_globe_radius * arrow_direction;

	// The arrowhead size should scale with length of arrow only up to a certain
	// length otherwise long arrows will have arrowheads that are too big.
	// When arrow length reaches a limit make the arrowhead projected size remain constant.
	const float MIN_RATIO_ARROWHEAD_TO_ARROWLINE = 0.5f;

	RenderedGeometry::impl_ptr_type rendered_geom_impl(new RenderedDirectionArrow(
			start,
			scaled_direction,
			ratio_arrowhead_size_to_globe_radius,
			MIN_RATIO_ARROWHEAD_TO_ARROWLINE,
			colour,
			arrowline_width_hint));

	return RenderedGeometry(rendered_geom_impl);
}

GPlatesViewOperations::RenderedGeometry
GPlatesViewOperations::RenderedGeometryFactory::create_rendered_reconstruction_geometry(
		GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type reconstruction_geom,
		RenderedGeometry rendered_geom)
{
	RenderedGeometry::impl_ptr_type rendered_geom_impl(new RenderedReconstructionGeometry(
			reconstruction_geom, rendered_geom));

	return RenderedGeometry(rendered_geom_impl);
}

GPlatesViewOperations::RenderedGeometry
GPlatesViewOperations::RenderedGeometryFactory::create_rendered_small_circle(
		const GPlatesMaths::SmallCircle &small_circle,
		const GPlatesGui::ColourProxy &colour,
		const float line_width_hint)
{
	RenderedGeometry::impl_ptr_type rendered_geom_impl(
			new RenderedSmallCircle(small_circle, colour, line_width_hint));

	return RenderedGeometry(rendered_geom_impl);
}

GPlatesViewOperations::RenderedGeometry
GPlatesViewOperations::RenderedGeometryFactory::create_rendered_small_circle_arc(
		const GPlatesMaths::SmallCircleArc &small_circle_arc,
		const GPlatesGui::ColourProxy &colour,
		const float line_width_hint)
{
	RenderedGeometry::impl_ptr_type rendered_geom_impl(
			new RenderedSmallCircleArc(small_circle_arc, colour, line_width_hint));

	return RenderedGeometry(rendered_geom_impl);
}

GPlatesViewOperations::RenderedGeometry
GPlatesViewOperations::RenderedGeometryFactory::create_rendered_ellipse(
		const GPlatesMaths::PointOnSphere &centre,
		const GPlatesMaths::Real &semi_major_axis_radians,
		const GPlatesMaths::Real &semi_minor_axis_radians,
		const GPlatesMaths::GreatCircle &axis,
		const GPlatesGui::ColourProxy &colour,
		float line_width_hint)
{
	RenderedGeometry::impl_ptr_type rendered_geom_impl(new RenderedEllipse(
		centre,semi_major_axis_radians,semi_minor_axis_radians,axis,colour,line_width_hint));
		
	return RenderedGeometry(rendered_geom_impl);
}

GPlatesViewOperations::RenderedGeometry
GPlatesViewOperations::RenderedGeometryFactory::create_rendered_string(
		GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point_on_sphere,
		const QString &string,
		const GPlatesGui::ColourProxy &colour,
		const GPlatesGui::ColourProxy &shadow_colour,
		int x_offset,
		int y_offset,
		const QFont &font)
{
	RenderedGeometry::impl_ptr_type rendered_geom_impl(new RenderedString(
				point_on_sphere,
				string,
				colour,
				shadow_colour,
				x_offset,
				y_offset,
				font));

	return RenderedGeometry(rendered_geom_impl);
}

GPlatesViewOperations::RenderedGeometry
GPlatesViewOperations::RenderedGeometryFactory::create_rendered_arrowed_polyline(
	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type points,
	const GPlatesGui::ColourProxy &colour,
	const float ratio_arrowhead_size_to_globe_radius,
	const float arrowline_width_hint)
{

	// This could also be passed in the arguments to this "create..." function...
	const float MAX_ARROWHEAD_SIZE = 0.005f;

	RenderedGeometry::impl_ptr_type rendered_geom_impl(new RenderedArrowedPolyline(
		points, colour,ratio_arrowhead_size_to_globe_radius,
		MAX_ARROWHEAD_SIZE,arrowline_width_hint));

	return RenderedGeometry(rendered_geom_impl);		

}

GPlatesViewOperations::RenderedGeometry
GPlatesViewOperations::RenderedGeometryFactory::create_rendered_triangle_symbol(
	const GPlatesMaths::PointOnSphere &centre,
        const GPlatesGui::ColourProxy &colour,
	const unsigned int size,
        bool filled,
        const float line_width_hint)
{
    RenderedGeometry::impl_ptr_type rendered_geom_impl(new RenderedTriangleSymbol(
	    centre,colour,size,filled,line_width_hint));

    return RenderedGeometry(rendered_geom_impl);

}

GPlatesViewOperations::RenderedGeometry
GPlatesViewOperations::RenderedGeometryFactory::create_rendered_square_symbol(
	const GPlatesMaths::PointOnSphere &centre,
	const GPlatesGui::ColourProxy &colour,
	const unsigned int size,
	bool filled,
	const float line_width_hint)
{
    RenderedGeometry::impl_ptr_type rendered_geom_impl(new RenderedSquareSymbol(
	    centre,colour,size,filled,line_width_hint));

    return RenderedGeometry(rendered_geom_impl);
}

GPlatesViewOperations::RenderedGeometry
GPlatesViewOperations::RenderedGeometryFactory::create_rendered_circle_symbol(
	const GPlatesMaths::PointOnSphere &centre,
	const GPlatesGui::ColourProxy &colour,
	const unsigned int size,
	bool filled,
	const float line_width_hint)
{
    RenderedGeometry::impl_ptr_type rendered_geom_impl(new RenderedCircleSymbol(
	    centre,colour,size,filled,line_width_hint));

    return RenderedGeometry(rendered_geom_impl);
}

GPlatesViewOperations::RenderedGeometry
GPlatesViewOperations::RenderedGeometryFactory::create_rendered_cross_symbol(
	const GPlatesMaths::PointOnSphere &centre,
	const GPlatesGui::ColourProxy &colour,
	const unsigned int size,
	const float line_width_hint)
{
    RenderedGeometry::impl_ptr_type rendered_geom_impl(new RenderedCrossSymbol(
	    centre,colour,size,line_width_hint));

    return RenderedGeometry(rendered_geom_impl);
}

GPlatesViewOperations::RenderedGeometry
GPlatesViewOperations::RenderedGeometryFactory::create_rendered_strain_marker_symbol(
	const GPlatesMaths::PointOnSphere &centre,
	const GPlatesGui::ColourProxy &colour,
	const unsigned int size,
	const float line_width_hint,
	const double scale_x,
	const double scale_y,
	const double angle)
{
    RenderedGeometry::impl_ptr_type rendered_geom_impl(
		new RenderedStrainMarkerSymbol(
	    	centre,colour,size,line_width_hint,
			scale_x, scale_y, angle));

    return RenderedGeometry(rendered_geom_impl);
}
