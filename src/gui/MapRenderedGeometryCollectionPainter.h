/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_MAPRENDEREDGEOMETRYCOLLECTIONPAINTER_H
#define GPLATES_GUI_MAPRENDEREDGEOMETRYCOLLECTIONPAINTER_H

#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>

#include "ColourScheme.h"
#include "LayerPainter.h"

#include "opengl/GLContext.h"
#include "opengl/GLVisualLayers.h"

#include "presentation/VisualLayers.h"

#include "view-operations/RenderedGeometryCollectionVisitor.h"


namespace GPlatesOpenGL
{
	class GLRenderer;
}

namespace GPlatesViewOperations
{
	class RenderedGeometryCollection;
	class RenderedGeometryLayer;
}

namespace GPlatesGui
{
	class MapProjection;

	/**
	 * Draws rendered geometries (in a @a RenderedGeometryCollection) onto a
	 * map view of the globe using OpenGL.
	 */
	class MapRenderedGeometryCollectionPainter :
			private GPlatesViewOperations::ConstRenderedGeometryCollectionVisitor<
				GPlatesPresentation::VisualLayers::rendered_geometry_layer_seq_type>,
			private boost::noncopyable
	{
	public:
		/**
		 * Typedef for an opaque object that caches a particular painting.
		 */
		typedef boost::shared_ptr<void> cache_handle_type;


		MapRenderedGeometryCollectionPainter(
				const MapProjection::non_null_ptr_to_const_type &map_projection,
				const GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
				const GPlatesOpenGL::GLVisualLayers::non_null_ptr_type &gl_visual_layers,
				const GPlatesPresentation::VisualLayers &visual_layers,
				ColourScheme::non_null_ptr_type colour_scheme,
				int device_pixel_ratio);

		/**
		 * Initialise objects requiring @a GLRenderer.
		 */
		void
		initialise(
				GPlatesOpenGL::GLRenderer &renderer);

		/**
		 * Draw the rendered geometries.
		 *
		 * @param viewport_zoom_factor is used for rendering view-dependent geometries.
		 */
		cache_handle_type
		paint(
				GPlatesOpenGL::GLRenderer &renderer,
				const double &viewport_zoom_factor,
				const double &device_independent_pixel_to_map_space_ratio);

		void
		set_scale(
				float scale);

		virtual
		boost::optional<GPlatesPresentation::VisualLayers::rendered_geometry_layer_seq_type>
		get_custom_child_layers_order(
				GPlatesViewOperations::RenderedGeometryCollection::MainLayerType parent_layer);

	private:

		virtual
		bool
		visit_main_rendered_layer(
				const GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
				GPlatesViewOperations::RenderedGeometryCollection::MainLayerType main_rendered_layer_type);

		virtual
		bool
		visit_rendered_geometry_layer(
				const GPlatesViewOperations::RenderedGeometryLayer &rendered_geometry_layer);


		//! Typedef for the base class.
		typedef GPlatesViewOperations::ConstRenderedGeometryCollectionVisitor<
				GPlatesPresentation::VisualLayers::rendered_geometry_layer_seq_type> base_type;


		/**
		 * Parameters that are only available when @a paint is called.
		 */
		struct PaintParams
		{
			PaintParams(
					GPlatesOpenGL::GLRenderer &renderer,
					const double &viewport_zoom_factor,
					const double &device_independent_pixel_to_map_space_ratio) :
				d_renderer(&renderer),
				d_inverse_viewport_zoom_factor(1.0 / viewport_zoom_factor),
				d_device_independent_pixel_to_map_space_ratio(device_independent_pixel_to_map_space_ratio),
				d_cache_handle(new std::vector<cache_handle_type>()),
				// Default to RECONSTRUCTION_LAYER (we set it before visiting each layer anyway) ...
				d_main_rendered_layer_type(GPlatesViewOperations::RenderedGeometryCollection::RECONSTRUCTION_LAYER)
			{  }

			GPlatesOpenGL::GLRenderer *d_renderer;
			double d_inverse_viewport_zoom_factor;
			double d_device_independent_pixel_to_map_space_ratio;

			// Cache of rendered geometry layers.
			boost::shared_ptr<std::vector<cache_handle_type> > d_cache_handle;

			// The layer type of the main rendered layer currently being rendered.
			GPlatesViewOperations::RenderedGeometryCollection::MainLayerType d_main_rendered_layer_type;
		};


		//! Parameters that are only available when @a paint is called.
		boost::optional<PaintParams> d_paint_params;

		//! Used to project vertices of rendered geometries to the map.
		MapProjection::non_null_ptr_to_const_type d_map_projection;

		const GPlatesViewOperations::RenderedGeometryCollection &d_rendered_geometry_collection;

		/**
		 * Keeps track of OpenGL-related objects that persist from one render to the next.
		 */
		GPlatesOpenGL::GLVisualLayers::non_null_ptr_type d_gl_visual_layers;

		const GPlatesPresentation::VisualLayers &d_visual_layers;

		//! Used to paint the layers.
		LayerPainter d_layer_painter;

		//! For assigning colours to RenderedGeometry
		ColourScheme::non_null_ptr_type d_colour_scheme;

		//! When rendering globes that are meant to be a scale copy of another
		float d_scale;
	};
}

#endif // GPLATES_GUI_MAPRENDEREDGEOMETRYCOLLECTIONPAINTER_H
