/* $Id$ */


/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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
 
#ifndef GPLATES_GUI_PERSISTENTOPENGLOBJECTS_H
#define GPLATES_GUI_PERSISTENTOPENGLOBJECTS_H

#include <map>
#include <boost/optional.hpp>

#include "RasterColourScheme.h"

#include "app-logic/Layer.h"
#include "app-logic/ReconstructGraph.h"
#include "app-logic/ReconstructRasterPolygons.h"

#include "opengl/GLContext.h"
#include "opengl/GLMultiResolutionCubeRaster.h"
#include "opengl/GLMultiResolutionRaster.h"
#include "opengl/GLMultiResolutionReconstructedRaster.h"
#include "opengl/GLRenderGraphNode.h"

#include "property-values/Georeferencing.h"
#include "property-values/RawRaster.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"

namespace GPlatesAppLogic
{
	class ApplicationState;
}


namespace GPlatesGui
{
	/**
	 * Keeps track of any OpenGL-related objects that persistent beyond one rendering frame.
	 *
	 * Until now there have been no such objects, but rasters are now persistent otherwise
	 * it would be far too expensive to rebuild them each time they need to be rendered.
	 *
	 * Each OpenGL context that does not share list objects such as textures and display lists
	 * will require a separate instance of this class.
	 *
	 * This class will 
	 */
	class PersistentOpenGLObjects :
			public QObject,
			public GPlatesUtils::ReferenceCount<PersistentOpenGLObjects>
	{
		Q_OBJECT

	public:
		//! A convenience typedef for a shared pointer to a non-const @a PersistentOpenGLObjects.
		typedef GPlatesUtils::non_null_intrusive_ptr<PersistentOpenGLObjects> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a PersistentOpenGLObjects.
		typedef GPlatesUtils::non_null_intrusive_ptr<const PersistentOpenGLObjects> non_null_ptr_to_const_type;


		/**
		 * Any objects that use textures, display lists, vertex buffer objects, etc
		 * should go here, otherwise use @a NonListObjects.
		 */
		class ListObjects
		{
		public:
			explicit
			ListObjects(
					const boost::shared_ptr<GPlatesOpenGL::GLContext::SharedState> &opengl_shared_state) :
				d_opengl_shared_state(opengl_shared_state)
			{  }


			/**
			 * Returns the OpenGL shared state (textures, etc).
			 */
			const boost::shared_ptr<GPlatesOpenGL::GLContext::SharedState> &
			get_opengl_shared_state()
			{
				return d_opengl_shared_state;
			}


			/**
			 * Returns a render graph node representing the possibly reconstructed
			 * multi-resolution raster.
			 *
			 * This method will try to reuse an existing multi-resolution raster as
			 * best it can if some of the parameters are common.
			 *
			 * Returns false if no multi-resolution raster could be created.
			 */
			boost::optional<GPlatesOpenGL::GLRenderGraphNode::non_null_ptr_type>
			get_or_create_raster_render_graph_node(
					const GPlatesAppLogic::Layer &layer,
					const GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type &georeferencing,
					const GPlatesPropertyValues::RawRaster::non_null_ptr_type &source_raster,
					const boost::optional<GPlatesGui::RasterColourScheme::non_null_ptr_type> &raster_colour_scheme,
					const boost::optional<GPlatesAppLogic::ReconstructRasterPolygons::non_null_ptr_to_const_type> &
							reconstruct_raster_polygons);

		private:
			/**
			 * Keeps track of bits and pieces that make up a raster for each layer to
			 * avoid expensive rebuilds.
			 */
			struct RasterBuilder
			{
				struct Raster
				{
					struct Input
					{
						boost::optional<GPlatesPropertyValues::Georeferencing::non_null_ptr_to_const_type> georeferencing;
						boost::optional<GPlatesPropertyValues::RawRaster::non_null_ptr_type> source_raster;
						boost::optional<GPlatesGui::RasterColourScheme::non_null_ptr_type> raster_colour_scheme;
						boost::optional<GPlatesAppLogic::ReconstructRasterPolygons::non_null_ptr_to_const_type>
								reconstruct_raster_polygons;
					};

					struct Output
					{
						boost::optional<GPlatesOpenGL::GLMultiResolutionRaster::non_null_ptr_type>
								source_multi_resolution_raster;
						boost::optional<GPlatesOpenGL::GLMultiResolutionCubeRaster::non_null_ptr_type>
								source_multi_resolution_cube_raster;
						boost::optional<GPlatesOpenGL::GLMultiResolutionReconstructedRaster::non_null_ptr_type>
								source_multi_resolution_reconstructed_raster;
					};

					Input input;
					Output output;
				};

				typedef std::map<GPlatesAppLogic::Layer, Raster> layer_to_raster_map_type;
				layer_to_raster_map_type layer_to_raster_map;
			};


			/**
			 * Shared textures, etc.
			 */
			boost::shared_ptr<GPlatesOpenGL::GLContext::SharedState> d_opengl_shared_state;

			RasterBuilder d_raster_builder;


			bool
			update_source_multi_resolution_raster(
					const RasterBuilder::Raster &old_raster,
					RasterBuilder::Raster &new_raster);
		};


		/**
		 * Any objects that do *not* use textures, display lists, vertex buffer objects, etc
		 * can go here, otherwise use @a ListObjects.
		 *
		 * These objects will be shared even if two OpenGL contexts don't share list objects.
		 *
		 * Objects that might go here are things like vertex arrays (which reside in system memory)
		 * that are large and hence sharing would reduce memory usage. Because they reside in
		 * system memory (ie, not dedicated RAM on the graphics card) it doesn't matter which
		 * OpenGL context is active when we draw them.
		 */
		class NonListObjects
		{
		};


		/**
		 * Creates a new @a PersistentOpenGLObjects object.
		 *
		 * Currently listens for removed layers to determine when to flush
		 */
		static
		non_null_ptr_type
		create(
				const GPlatesOpenGL::GLContext::non_null_ptr_type &opengl_context,
				GPlatesAppLogic::ApplicationState &application_state)
		{
			return non_null_ptr_type(new PersistentOpenGLObjects(opengl_context, application_state));
		}

		/**
		 * Creates a @a PersistentOpenGLObjects object and that always shares the non-list objects
		 * and only shares the list objects if @a objects_from_another_context uses a context
		 * that shares the same shared state as @a opengl_context.
		 *
		 * This basically allows objects that use textures and display lists to be shared
		 * across QGLWidgets objects (or whatever objects have different OpenGL contexts).
		 * The sharing depends on whether the two OpenGL contexts allow shared textures/display-lists.
		 */
		static
		non_null_ptr_type
		create(
				const GPlatesOpenGL::GLContext::non_null_ptr_type &opengl_context,
				const PersistentOpenGLObjects::non_null_ptr_type &objects_from_another_context,
				GPlatesAppLogic::ApplicationState &application_state)
		{
			return non_null_ptr_type(
					new PersistentOpenGLObjects(opengl_context, objects_from_another_context, application_state));
		}


		ListObjects &
		get_list_objects()
		{
			return *d_list_objects;
		}


		NonListObjects &
		get_non_list_objects()
		{
			return *d_non_list_objects;
		}

	public slots:
		// NOTE: all signals/slots should use namespace scope for all arguments
		//       otherwise differences between signals and slots will cause Qt
		//       to not be able to connect them at runtime.

		/**
		 * Called when an existing layer is about to be removed.
		 */
		void
		handle_layer_about_to_be_removed(
				GPlatesAppLogic::ReconstructGraph &reconstruct_graph,
				GPlatesAppLogic::Layer layer);

	private:
		boost::shared_ptr<ListObjects> d_list_objects;
		boost::shared_ptr<NonListObjects> d_non_list_objects;


		//! Constructor.
		explicit
		PersistentOpenGLObjects(
				const GPlatesOpenGL::GLContext::non_null_ptr_type &opengl_context,
				GPlatesAppLogic::ApplicationState &application_state);

		//! Constructor.
		PersistentOpenGLObjects(
				const GPlatesOpenGL::GLContext::non_null_ptr_type &opengl_context,
				const PersistentOpenGLObjects::non_null_ptr_type &objects_from_another_context,
				GPlatesAppLogic::ApplicationState &application_state);


		void
		make_signal_slot_connections(
				GPlatesAppLogic::ReconstructGraph &reconstruct_graph);
	};
}

#endif // GPLATES_GUI_PERSISTENTOPENGLOBJECTS_H
