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

#include <map>
#include <vector>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <QGLWidget>

#include "Colour.h"
#include "ColourScheme.h"
#include "GlobeVisibilityTester.h"
#include "PersistentOpenGLObjects.h"
#include "TextRenderer.h"
#include "RenderSettings.h"

#include "maths/CubeQuadTreePartition.h"
#include "maths/types.h"
#include "maths/Vector3D.h"

#include "opengl/GLCubeSubdivisionCache.h"
#include "opengl/GLStreamPrimitives.h"
#include "opengl/GLTransformState.h"
#include "opengl/GLUNurbsRenderer.h"
#include "opengl/GLVertex.h"

#include "view-operations/RenderedGeometry.h"
#include "view-operations/RenderedGeometryVisitor.h"


namespace GPlatesOpenGL
{
	class GLMultiResolutionFilledPolygons;
	class GLRenderer;
}

namespace GPlatesViewOperations
{
	class RenderedGeometryLayer;
}

namespace GPlatesGui
{
	/**
	 * Handles drawing rendered geometries in a single layer by drawing the
	 * opaque primitives first followed by the transparent primitives.
	 */
	class GlobeRenderedGeometryLayerPainter :
			public GPlatesViewOperations::ConstRenderedGeometryVisitor,
			private boost::noncopyable
	{
	public:
		GlobeRenderedGeometryLayerPainter(
				const GPlatesViewOperations::RenderedGeometryLayer &rendered_geometry_layer,
				const PersistentOpenGLObjects::non_null_ptr_type &persistent_opengl_objects,
				const double &inverse_viewport_zoom_factor,
				const GPlatesOpenGL::GLUNurbsRenderer::non_null_ptr_type &nurbs_renderer,
				RenderSettings &render_settings,
				const TextRenderer::non_null_ptr_to_const_type &text_renderer_ptr,
				const GlobeVisibilityTester &visibility_tester,
				ColourScheme::non_null_ptr_type colour_scheme) :
			d_rendered_geometry_layer(rendered_geometry_layer),
			d_persistent_opengl_objects(persistent_opengl_objects),
			d_nurbs_renderer(nurbs_renderer),
			d_inverse_zoom_factor(inverse_viewport_zoom_factor),
			d_render_settings(render_settings),
			d_text_renderer_ptr(text_renderer_ptr),
			d_visibility_tester(visibility_tester),
			d_colour_scheme(colour_scheme),
			d_scale(1.0f)
		{  }


		/**
		 * Draws the sequence of rendered geometries passed into constructor.
		 */
		void
		paint(
				GPlatesOpenGL::GLRenderer &renderer);

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
		visit_resolved_raster(
				const GPlatesViewOperations::RenderedResolvedRaster &rendered_resolved_raster);

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
		visit_rendered_string(
				const GPlatesViewOperations::RenderedString &rendered_string);

		virtual
		void
		visit_rendered_triangle_symbol(
				 const GPlatesViewOperations::RenderedTriangleSymbol &rendered_triangle_symbol);

		//! Typedef for a rendered geometries spatial partition.
		typedef GPlatesMaths::CubeQuadTreePartition<GPlatesViewOperations::RenderedGeometry>
				rendered_geometries_spatial_partition_type;

		//! Typedef for filled polygon that will be rendered as a mask filled geometry outline.
		typedef GPlatesOpenGL::GLMultiResolutionFilledPolygons::filled_polygon_type filled_polygon_type;

		//! Typedef for a filled polygon spatial partition.
		typedef GPlatesOpenGL::GLMultiResolutionFilledPolygons::filled_polygons_spatial_partition_type
				filled_polygons_spatial_partition_type;

		//! Typedef for a coloured vertex.
		typedef GPlatesOpenGL::GLColouredVertex coloured_vertex_type;
	
		//! Typedef for a sequence of drawables.
		typedef std::vector<GPlatesOpenGL::GLDrawable::non_null_ptr_to_const_type> drawable_seq_type;

		/**
		 * Lines can be rendered using either regular vertex line primitives or nurbs curves.
		 */
		struct LineDrawables
		{
			LineDrawables(
					const GPlatesOpenGL::GLStreamPrimitives<coloured_vertex_type>::non_null_ptr_type &stream_);

			GPlatesOpenGL::GLStreamPrimitives<coloured_vertex_type>::non_null_ptr_type stream;
			drawable_seq_type nurbs_drawables;
		};


		/**
		 * Drawables for points, lines and polygons (triangles and quads).
		 */
		class PointLinePolygonDrawables
		{
		public:
			//! Constructor.
			PointLinePolygonDrawables();

			/**
			 * Returns the stream for points of size @a point_size.
			 */
			GPlatesOpenGL::GLStreamPrimitives<coloured_vertex_type> &
			get_point_drawables(
					float point_size);

			/**
			 * Returns the stream and nurbs drawable list for lines of width @a line_width.
			 */
			LineDrawables &
			get_line_drawables(
					float line_width);

			/**
			 * Returns the stream for the triangle mesh drawables.
			 *
			 * There's no point size of line width equivalent for polygons
			 * so they all get lumped into a single stream.
			 */
			GPlatesOpenGL::GLStreamPrimitives<coloured_vertex_type> &
			get_triangle_drawables()
			{
				return *d_triangle_drawables;
			}

			/**
			 * Returns the stream for the quad mesh drawables.
			 */
			GPlatesOpenGL::GLStreamPrimitives<coloured_vertex_type> &
			get_quad_drawables()
			{
				return *d_quad_drawables;
			}

			
			/**
			 * Paints all drawables for all primitives types.
			 */
			void
			paint_drawables(
					GPlatesOpenGL::GLRenderer &renderer);

		private:
			/**
			 * Typedef for mapping point size to drawable.
			 * The stream generates the drawables.
			 * All points of the same size will get grouped together.
			 */
			typedef std::map<
					GPlatesMaths::real_t,
					GPlatesOpenGL::GLStreamPrimitives<coloured_vertex_type>::non_null_ptr_type >
							point_size_to_drawables_map_type;

			/**
			 * Typedef for mapping line width to drawable.
			 * All lines of the same width will get grouped together.
			 */
			typedef std::map<
					GPlatesMaths::real_t,
					LineDrawables> line_width_to_drawables_map_type;


			point_size_to_drawables_map_type d_point_drawables_map;
			line_width_to_drawables_map_type d_line_drawables_map;

			/*
			 * There's no point size or line width equivalent for polygons (triangles and quads)
			 * so they can be lumped into a single streams.
			 *
			 * Although we keep a separate stream for triangles and quads since they are
			 * different primitive types and streaming is more efficient that way.
			 */
			GPlatesOpenGL::GLStreamPrimitives<coloured_vertex_type>::non_null_ptr_type d_triangle_drawables;
			GPlatesOpenGL::GLStreamPrimitives<coloured_vertex_type>::non_null_ptr_type d_quad_drawables;

			void
			paint_points_drawable(
					GPlatesOpenGL::GLRenderer &renderer,
					float point_size,
					const GPlatesOpenGL::GLDrawable::non_null_ptr_to_const_type &points_drawable);

			boost::optional<GPlatesOpenGL::GLDrawable::non_null_ptr_to_const_type>
			get_lines_drawable(
					LineDrawables &line_drawables);

			void
			paint_lines_drawable(
					GPlatesOpenGL::GLRenderer &renderer,
					float line_width,
					const GPlatesOpenGL::GLDrawable::non_null_ptr_to_const_type &lines_drawable);
		};


		/**
		 * Drawables that get filled in their interior.
		 *
		 * For 'filled' to make any sense these drawables should have a sequence of points that
		 * defines some kind of outline (the outline can be concave or convex).
		 */
		struct FilledDrawables
		{
			FilledDrawables() :
				spatial_partition(
						// The max depth does not matter because we are mirroring the
						// rendered geometries spatial partition instead of adding geometries...
						filled_polygons_spatial_partition_type::create(0/*max_quad_tree_depth*/))
			{  }
			
			/**
			 * Paints the filled drawables.
			 */
			void
			paint_drawables(
					GPlatesOpenGL::GLRenderer &renderer,
					PersistentOpenGLObjects &persistent_opengl_objects);

			filled_polygons_spatial_partition_type::non_null_ptr_type spatial_partition;

			/**
			 * Optional destination in the filled polygons spatial partition to add filled drawables to.
			 *
			 * If it's boost::none then filled polygons are added to the root of the spatial partition.
			 * This is the default if the rendered geometries being visited are not in a spatial partition.
			 */
			boost::optional<filled_polygons_spatial_partition_type::node_reference_type> current_node;
		};


		/**
		 * Parameters that are only available when @a paint is called.
		 */
		class PaintParams
		{
		public:
			explicit
			PaintParams(
					GPlatesOpenGL::GLRenderer &renderer_) :
				renderer(&renderer_)
			{  }


			void
			paint_text_off_the_sphere();


			GPlatesOpenGL::GLRenderer *renderer;

			FilledDrawables filled_drawables_on_the_sphere;
			PointLinePolygonDrawables drawables_off_the_sphere;
			PointLinePolygonDrawables opaque_drawables_on_the_sphere;
			PointLinePolygonDrawables translucent_drawables_on_the_sphere;

			drawable_seq_type text_off_the_sphere;
		};


		//! Parameters that are only available when @a paint is called.
		boost::optional<PaintParams> d_paint_params;

		const GPlatesViewOperations::RenderedGeometryLayer &d_rendered_geometry_layer;

		/**
		 * Keeps track of OpenGL-related objects that persist from one render to the next.
		 */
		PersistentOpenGLObjects::non_null_ptr_type d_persistent_opengl_objects;

		GPlatesOpenGL::GLUNurbsRenderer::non_null_ptr_type d_nurbs_renderer;
		const double d_inverse_zoom_factor;

		//! Rendering flags for determining what gets shown
		RenderSettings &d_render_settings;

		//! For rendering text
		TextRenderer::non_null_ptr_to_const_type d_text_renderer_ptr;

		//! For determining whether a particular point on the globe is visible or not
		GlobeVisibilityTester d_visibility_tester;

		//! For assigning colours to RenderedGeometry
		ColourScheme::non_null_ptr_type d_colour_scheme;

		//! When rendering scaled globes that are meant to be a scaled version of another
		float d_scale;

		//! Multiplying factor to get point size of 1.0f to look like one screen-space pixel.
		static const float POINT_SIZE_ADJUSTMENT;

		//! Multiplying factor to get line width of 1.0f to look like one screen-space pixel.
		static const float LINE_WIDTH_ADJUSTMENT;

		/**
		 * Visit each rendered geometry in our sequence (or spatial partition).
		 */
		void
		visit_rendered_geometries(
				GPlatesOpenGL::GLRenderer &renderer);

		void
		render_spatial_partition(
				GPlatesOpenGL::GLRenderer &renderer,
				const rendered_geometries_spatial_partition_type &rendered_geometries_spatial_partition);

		void
		render_spatial_partition_quad_tree(
				const rendered_geometries_spatial_partition_type &rendered_geometries_spatial_partition,
				rendered_geometries_spatial_partition_type::const_node_reference_type rendered_geometries_quad_tree_node,
				filled_polygons_spatial_partition_type::node_reference_type filled_polygons_quad_tree_node,
				PersistentOpenGLObjects::cube_subdivision_loose_bounds_cache_type &cube_subdivision_loose_bounds,
				const PersistentOpenGLObjects::cube_subdivision_loose_bounds_cache_type::node_reference_type &loose_bounds_node,
				const GPlatesOpenGL::GLFrustum &frustum_planes,
				boost::uint32_t frustum_plane_mask);

		GPlatesOpenGL::GLStateSet::non_null_ptr_to_const_type
		get_state_for_primitives_off_the_sphere();

		GPlatesOpenGL::GLStateSet::non_null_ptr_to_const_type
		get_state_for_non_raster_primitives_on_the_sphere();

		GPlatesOpenGL::GLStateSet::non_null_ptr_to_const_type
		get_state_for_raster_primitives_on_the_sphere() const;

		GPlatesOpenGL::GLStateSet::non_null_ptr_to_const_type
		get_state_for_filled_polygons_on_the_sphere() const;

		GPlatesOpenGL::GLStateSet::non_null_ptr_to_const_type
		get_state_for_text_off_the_sphere();

		/**
		 * Sets up alpha-blending and point/line anti-aliasing state.
		 */
		GPlatesOpenGL::GLStateSet::non_null_ptr_to_const_type
		get_translucent_state();

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
				const GreatCircleArcForwardIter &begin_arcs,
				const GreatCircleArcForwardIter &end_arcs,
				const Colour &colour,
				LineDrawables &line_drawables,
				GPlatesOpenGL::GLUNurbsRenderer &nurbs_renderer);

		/**
		 * Paints an ellipse.
		 */
		void
		paint_ellipse(
				const GPlatesViewOperations::RenderedEllipse &rendered_ellipse,
				const Colour &colour,
				LineDrawables &line_drawables);

		/**
		 * Paints a cone (for arrow head).
		 */
		void
		paint_cone(
				const GPlatesMaths::Vector3D &apex,
				const GPlatesMaths::Vector3D &cone_axis,
				rgba8_t rgba8_color,
				GPlatesOpenGL::GLStreamPrimitives<coloured_vertex_type> &stream);
	};
}

#endif // GPLATES_GUI_GLOBERENDEREDGEOMETRYLAYERPAINTER_H
