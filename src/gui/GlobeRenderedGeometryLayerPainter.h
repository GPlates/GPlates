/* $Id$ */

/**
 * \file Draws rendered geometries in a specific @a RenderedGeometryLayer onto 3d orthographic
 * globe using OpenGL.
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009, 2010, 2011 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_GLOBERENDEREDGEOMETRYLAYERPAINTER_H
#define GPLATES_GUI_GLOBERENDEREDGEOMETRYLAYERPAINTER_H

#include <vector>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <QGLWidget>
#include <opengl/OpenGL.h>

#include "Colour.h"
#include "ColourScheme.h"
#include "GlobeVisibilityTester.h"
#include "LayerPainter.h"
#include "RenderSettings.h"

#include "maths/CubeQuadTreeLocation.h"
#include "maths/CubeQuadTreePartition.h"
#include "maths/types.h"
#include "maths/UnitVector3D.h"
#include "maths/Vector3D.h"

#include "opengl/GLCubeSubdivisionCache.h"
#include "opengl/GLFrustum.h"
#include "opengl/GLTexture.h"

#include "presentation/VisualLayers.h"

#include "view-operations/RenderedGeometry.h"
#include "view-operations/RenderedGeometryLayer.h"
#include "view-operations/RenderedGeometryVisitor.h"


namespace GPlatesOpenGL
{
	class GLRenderer;
}

namespace GPlatesViewOperations
{
	class RenderedGeometryLayer;
}

namespace GPlatesGui
{
	/**
	 * Handles drawing rendered geometries in a single rendered layer.
	 */
	class GlobeRenderedGeometryLayerPainter :
			public GPlatesViewOperations::ConstRenderedGeometryVisitor,
			private boost::noncopyable
	{
	public:
		/**
		 * Typedef for an opaque object that caches a particular painting.
		 */
		typedef boost::shared_ptr<void> cache_handle_type;


		//! Determines whether to paint the globe surface or sub-surface.
		enum PaintRegionType
		{
			PAINT_SURFACE,
			PAINT_SUB_SURFACE
		};


		/**
		 * @a paint_region specifies whether to draw surface or sub-surface rendered geometries in @a paint.
		 *
		 * @a surface_occlusion_texture is a viewport-size 2D texture containing the RGBA rendering
		 * of the surface geometries/rasters on the *front* of the globe.
		 * It is only used when rendering sub-surface geometries.
		 * @a improve_performance_reduce_quality_hint a hint to improve performance by presumably
		 * reducing quality - this is a temporary hint usually during globe rotation mouse drag.
		 */
		GlobeRenderedGeometryLayerPainter(
				const GPlatesViewOperations::RenderedGeometryLayer &rendered_geometry_layer,
				const double &inverse_viewport_zoom_factor,
				const RenderSettings &render_settings,
				const GlobeVisibilityTester &visibility_tester,
				ColourScheme::non_null_ptr_type colour_scheme,
				PaintRegionType paint_region,
				boost::optional<GPlatesOpenGL::GLTexture::shared_ptr_to_const_type>
						surface_occlusion_texture = boost::none,
				bool improve_performance_reduce_quality_hint = false);


		/**
		 * Draws rendered geometries on the globe surface or sub-surface depending on the
		 * PaintRegionType passed into constructor.
		 */
		cache_handle_type
		paint(
				GPlatesOpenGL::GLRenderer &renderer,
				LayerPainter &layer_painter);

		void
		set_scale(
				float scale)
		{
			d_scale = scale;
		}

	private:

		virtual
		void
		visit_rendered_arrowed_polyline(
			const GPlatesViewOperations::RenderedArrowedPolyline &rendered_arrowed_polyline);

		virtual
		void
		visit_rendered_strain_marker_symbol(
			const GPlatesViewOperations::RenderedStrainMarkerSymbol &);

		virtual
		void
		visit_rendered_cross_symbol(
			const GPlatesViewOperations::RenderedCrossSymbol &);

		virtual
		void
		visit_rendered_ellipse(
				const GPlatesViewOperations::RenderedEllipse &rendered_ellipse);
	
		virtual
		void
		visit_rendered_point_on_sphere(
				const GPlatesViewOperations::RenderedPointOnSphere &rendered_point_on_sphere);

		virtual
		void
		visit_rendered_multi_point_on_sphere(
				const GPlatesViewOperations::RenderedMultiPointOnSphere &rendered_multi_point_on_sphere);

		virtual
		void
		visit_rendered_polyline_on_sphere(
				const GPlatesViewOperations::RenderedPolylineOnSphere &rendered_polyline_on_sphere);

		virtual
		void
		visit_rendered_polygon_on_sphere(
				const GPlatesViewOperations::RenderedPolygonOnSphere &rendered_polygon_on_sphere);

		virtual
		void
		visit_rendered_coloured_edge_surface_mesh(
			const GPlatesViewOperations::RenderedColouredEdgeSurfaceMesh &rendered_coloured_edge_surface_mesh);

		virtual
		void
		visit_rendered_coloured_triangle_surface_mesh(
			const GPlatesViewOperations::RenderedColouredTriangleSurfaceMesh &rendered_coloured_triangle_surface_mesh);

		virtual
		void
		visit_rendered_resolved_raster(
				const GPlatesViewOperations::RenderedResolvedRaster &rendered_resolved_raster);

		virtual
		void
		visit_rendered_resolved_scalar_field_3d(
				const GPlatesViewOperations::RenderedResolvedScalarField3D &rendered_resolved_scalar_field);

		virtual
		void
		visit_rendered_direction_arrow(
				const GPlatesViewOperations::RenderedDirectionArrow &rendered_direction_arrow);
				
		virtual
		void
		visit_rendered_small_circle(
				const GPlatesViewOperations::RenderedSmallCircle &rendered_small_circle);
				
		virtual
		void
		visit_rendered_small_circle_arc(
				const GPlatesViewOperations::RenderedSmallCircleArc &rendered_small_circle_arc);

		virtual
		void
		visit_rendered_square_symbol(
				 const GPlatesViewOperations::RenderedSquareSymbol &rendered_square_symbol);

		virtual
		void
		visit_rendered_circle_symbol(
				 const GPlatesViewOperations::RenderedCircleSymbol &rendered_circle_symbol);

		virtual
		void
		visit_rendered_string(
				const GPlatesViewOperations::RenderedString &rendered_string);

		virtual
		void
		visit_rendered_triangle_symbol(
				 const GPlatesViewOperations::RenderedTriangleSymbol &rendered_triangle_symbol);

	private:

		//! Typedef for a vertex element (index).
		typedef LayerPainter::vertex_element_type vertex_element_type;

		//! Typedef for a coloured vertex.
		typedef LayerPainter::coloured_vertex_type coloured_vertex_type;

		//! Typedef for a sequence of coloured vertices.
		typedef LayerPainter::coloured_vertex_seq_type coloured_vertex_seq_type;

		//! Typedef for a sequence of vertex elements.
		typedef LayerPainter::vertex_element_seq_type vertex_element_seq_type;

		//! Typedef for a primitives stream containing coloured vertices.
		typedef LayerPainter::stream_primitives_type stream_primitives_type;


		//! Typedef for a rendered geometries spatial partition.
		typedef GPlatesViewOperations::RenderedGeometryLayer::rendered_geometries_spatial_partition_type
				rendered_geometries_spatial_partition_type;

		/**
		 * Typedef for a @a GLCubeSubvision cache that caches loose bounds.
		 */
		typedef GPlatesOpenGL::GLCubeSubdivisionCache<
				false/*CacheProjectionTransform*/, false/*CacheLooseProjectionTransform*/,
				false/*CacheFrusum*/, false/*CacheLooseFrustum*/,
				false/*CacheBoundingPolygon*/, false/*CacheLooseBoundingPolygon*/,
				false/*CacheBounds*/, true/*CacheLooseBounds*/>
						cube_subdivision_cache_type;


		/**
		 * Information associated with a rendered geometry.
		 */
		struct RenderedGeometryInfo
		{
			RenderedGeometryInfo(
					const GPlatesViewOperations::RenderedGeometry &rendered_geometry_,
					const rendered_geometries_spatial_partition_type::location_type &spatial_partition_location_ =
							rendered_geometries_spatial_partition_type::location_type()) :
				rendered_geometry(rendered_geometry_),
				spatial_partition_location(spatial_partition_location_)
			{  }

			GPlatesViewOperations::RenderedGeometry rendered_geometry;
			rendered_geometries_spatial_partition_type::location_type spatial_partition_location;
		};

		/**
		 * Helper structure to sort rendered geometries in their render order.
		 */
		struct RenderedGeometryOrder
		{
			RenderedGeometryOrder(
					unsigned int rendered_geometry_info_index_,
					GPlatesViewOperations::RenderedGeometryLayer::rendered_geometry_index_type render_order_) :
				rendered_geometry_info_index(rendered_geometry_info_index_),
				render_order(render_order_)
			{  }

			unsigned int rendered_geometry_info_index;
			GPlatesViewOperations::RenderedGeometryLayer::rendered_geometry_index_type render_order;

			//! Used to sort by render order.
			struct SortRenderOrder
			{
				bool
				operator()(
						const RenderedGeometryOrder &lhs,
						const RenderedGeometryOrder &rhs) const
				{
					return lhs.render_order < rhs.render_order;
				}
			};
		};


		const GPlatesViewOperations::RenderedGeometryLayer &d_rendered_geometry_layer;

		const double d_inverse_zoom_factor;

		//! Rendering flags for determining what gets shown
		const RenderSettings &d_render_settings;

		//! For determining whether a particular point on the globe is visible or not
		GlobeVisibilityTester d_visibility_tester;

		//! For assigning colours to RenderedGeometry
		ColourScheme::non_null_ptr_type d_colour_scheme;

		//! When rendering scaled globes that are meant to be a scaled version of another
		float d_scale;

		//! Whether to render the globe surface or sub-surface.
		PaintRegionType d_paint_region;

		/**
		 * Used to paint when the @a paint method is called.
		 *
		 * Is only valid during @a paint.
		 */
		boost::optional<LayerPainter &> d_layer_painter;

		/**
		 * Used for frustum culling when the @a paint method is called.
		 *
		 * Is only valid during @a paint.
		 */
		boost::optional<GPlatesOpenGL::GLFrustum> d_frustum_planes;

		/**
		 * A viewport-size 2D texture containing the RGBA rendering
		 * of the surface geometries/rasters on the *front* of the globe.
		 * It is only used when rendering sub-surface geometries.
		 */
		boost::optional<GPlatesOpenGL::GLTexture::shared_ptr_to_const_type> d_surface_occlusion_texture;

		/**
		 * A hint to improve performance presumably at the cost of quality.
		 *
		 * This is currently used to improve rendering performance of 3D scalar field iso-surfaces
		 * during globe rotation (mouse drag).
		 */
		bool d_improve_performance_reduce_quality_hint;

		/**
		 * Location in cube quad tree (spatial partition) when traversing a rendered geometries spatial partition.
		 *
		 * Is only valid during @a paint (and when a rendered geometry is visited).
		 */
		boost::optional<const rendered_geometries_spatial_partition_type::location_type &>
				d_current_spatial_partition_location;


		//! Multiplying factor to get point size of 1.0f to look like one screen-space pixel.
		static const float POINT_SIZE_ADJUSTMENT;

		//! Multiplying factor to get line width of 1.0f to look like one screen-space pixel.
		static const float LINE_WIDTH_ADJUSTMENT;
		static const float STRAIN_LINE_WIDTH_ADJUSTMENT;


		/**
		 * Visit each rendered geometry in our sequence (or spatial partition).
		 */
		void
		visit_rendered_geometries(
				GPlatesOpenGL::GLRenderer &renderer);

		void
		get_visible_rendered_geometries(
				GPlatesOpenGL::GLRenderer &renderer,
				std::vector<RenderedGeometryInfo> &rendered_geometry_infos,
				std::vector<RenderedGeometryOrder> &rendered_geometry_orders,
				const rendered_geometries_spatial_partition_type &rendered_geometries_spatial_partition);

		void
		get_visible_rendered_geometries_from_spatial_partition_quad_tree(
				std::vector<RenderedGeometryInfo> &rendered_geometry_infos,
				std::vector<RenderedGeometryOrder> &rendered_geometry_orders,
				const GPlatesMaths::CubeQuadTreeLocation &cube_quad_tree_node_location,
				rendered_geometries_spatial_partition_type::const_node_reference_type rendered_geometries_quad_tree_node,
				cube_subdivision_cache_type &cube_subdivision_cache,
				const cube_subdivision_cache_type::node_reference_type &cube_subdivision_cache_node,
				const GPlatesOpenGL::GLFrustum &frustum_planes,
				boost::uint32_t frustum_plane_mask);

		/**
		 * Determines the colour of a RenderedGeometry type using d_colour_scheme
		 */
		template <class T>
		inline
		boost::optional<Colour>
		get_colour_of_rendered_geometry(
				const T &geom);

		/**
		 * Paints great circle arcs of polylines and polygons.
		 */
		template <typename GreatCircleArcForwardIter>
		void
		paint_great_circle_arcs(
				GreatCircleArcForwardIter begin_arcs,
				GreatCircleArcForwardIter end_arcs,
				const Colour &colour,
				stream_primitives_type &lines_stream);

		/**
		 * Paints an ellipse.
		 */
		void
		paint_ellipse(
				const GPlatesViewOperations::RenderedEllipse &rendered_ellipse,
				const Colour &colour,
				stream_primitives_type &lines_stream);

		/**
		 * Paints a cone (for arrow head).
		 */
		void
		paint_cone(
				const GPlatesMaths::Vector3D &apex,
				const GPlatesMaths::UnitVector3D &cone_axis_unit_vector,
				const GPlatesMaths::real_t &cone_axis_mag,
				rgba8_t rgba8_color,
				stream_primitives_type &triangles_stream);
	};
}

#endif // GPLATES_GUI_GLOBERENDEREDGEOMETRYLAYERPAINTER_H
