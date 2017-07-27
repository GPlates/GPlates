/* $Id$ */

/**
 * \file 
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

#ifndef GPLATES_PRESENTATION_VIEWSTATE_H
#define GPLATES_PRESENTATION_VIEWSTATE_H

#include <map>
#include <utility>
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <QObject>
#include <QString>

#include "global/PointerTraits.h"
#include "gui/Symbol.h"
#include "utils/VirtualProxy.h"

////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
// NOTE: Please use forward declarations (and boost::scoped_ptr) instead of including headers
// where possible.
// This header gets included in a lot of other files and we want to reduce compile times.
////////////////////////////////////////////////////////////////////////////////////////////////



#include "property-values/GeoTimeInstant.h"
////////////////////////////////////////////////////////////////////////////////////////////////
// FIXME remove this header
#include "view-operations/RenderedGeometryCollection.h"

namespace GPlatesAppLogic
{
	class ApplicationState;
	class FeatureCollectionFileIO;
	class Reconstruction;
}

namespace GPlatesGui
{
	class AnimationController;
	class Colour;
	class ColourScheme;
	class ColourSchemeContainer;
	class ColourSchemeDelegator;
	class ExportAnimationRegistry;
	class FeatureTableModel;
	class FeatureFocus;
	class FileIODirectoryConfigurations;
	class GraticuleSettings;
	class MapTransform;
	class PythonManager;
	class RenderSettings;
	class SceneLightingParameters;
	class TextOverlaySettings;
	class TopologySectionsContainer;
	class ViewportProjection;
	class ViewportZoom;

}

namespace GPlatesQtWidgets
{
	class ViewportWindow;
}

namespace GPlatesViewOperations
{
	class FocusedFeatureGeometryManipulator;
	class GeometryBuilder;
	class RenderedGeometryCollection;
	class RenderedGeometryParameters;
}

namespace GPlatesPresentation
{
	class SessionManagement;
	class VisualLayerRegistry;
	class VisualLayers;

	class ViewState :
			public QObject,
			private boost::noncopyable
	{
		Q_OBJECT
		
	public:

		explicit
		ViewState(
				GPlatesAppLogic::ApplicationState &application_state);

		~ViewState();


		GPlatesAppLogic::ApplicationState &
		get_application_state();

		const GPlatesAppLogic::ApplicationState &
		get_application_state() const;


		///////////////////////////////////
		// FIXME: temporary horrible hack - remove this method when all state
		// accessed by this method has been moved in this class.
		/**
		 * Only use this method if you are accessing state that will ultimately
		 * be moved into this class (@a ViewState) - ie, stuff that does not involve
		 * Qt widgets.
		 * If you need to access Qt widget related stuff then pass a reference to
		 * ViewportWindow into your class/function - the idea is that when all
		 * relevant state has been moved over then this method will be removed.
		 */
		GPlatesQtWidgets::ViewportWindow &
		get_other_view_state()
		{
			return *d_other_view_state;
		}

		void
		set_other_view_state(
				GPlatesQtWidgets::ViewportWindow &viewport_window)
		{
			d_other_view_state = &viewport_window;
		}
		//
		///////////////////////////////////


		GPlatesGui::AnimationController &
		get_animation_controller();

		const GPlatesGui::AnimationController &
		get_animation_controller() const;

		/**
		 * Stores/Loads loaded file information to and from persistent storage.
		 */
		SessionManagement &
		get_session_management();

		const SessionManagement &
		get_session_management() const;


		GPlatesViewOperations::RenderedGeometryCollection &
		get_rendered_geometry_collection();

		const GPlatesViewOperations::RenderedGeometryCollection &
		get_rendered_geometry_collection() const;


		GPlatesGui::FeatureFocus &
		get_feature_focus();

		const GPlatesGui::FeatureFocus &
		get_feature_focus() const;


		GPlatesGui::FeatureTableModel &
		get_feature_table_model();

		const GPlatesGui::FeatureTableModel &
		get_feature_table_model() const;


		GPlatesGui::ViewportZoom &
		get_viewport_zoom();

		const GPlatesGui::ViewportZoom &
		get_viewport_zoom() const;


		GPlatesGui::ViewportProjection &
		get_viewport_projection();

		const GPlatesGui::ViewportProjection &
		get_viewport_projection() const;


		GPlatesViewOperations::GeometryBuilder &
		get_digitise_geometry_builder();

		const GPlatesViewOperations::GeometryBuilder &
		get_digitise_geometry_builder() const;


		GPlatesViewOperations::GeometryBuilder &
		get_focused_feature_geometry_builder();

		const GPlatesViewOperations::GeometryBuilder &
		get_focused_feature_geometry_builder() const;


		//! Returns all loaded colour schemes, sorted by category.
		GPlatesGui::ColourSchemeContainer &
		get_colour_scheme_container();

		const GPlatesGui::ColourSchemeContainer &
		get_colour_scheme_container() const;


		/**
		 * Returns the current colour scheme.
		 *
		 * When performing colour lookup with the returned colour scheme
		 * it will always refer to the latest colour scheme selected.
		 * This is because the returned colour scheme delegates colour lookup to an
		 * actual colour scheme implementation which itself can be switched inside the
		 * delegate.
		 */
		GPlatesGlobal::PointerTraits<GPlatesGui::ColourScheme>::non_null_ptr_type
		get_colour_scheme();

		GPlatesGlobal::PointerTraits<const GPlatesGui::ColourScheme>::non_null_ptr_type
		get_colour_scheme() const;


		GPlatesGlobal::PointerTraits<GPlatesGui::ColourSchemeDelegator>::non_null_ptr_type
		get_colour_scheme_delegator();

		GPlatesGlobal::PointerTraits<const GPlatesGui::ColourSchemeDelegator>::non_null_ptr_type
		get_colour_scheme_delegator() const;


		GPlatesGui::RenderSettings &
		get_render_settings();

		const GPlatesGui::RenderSettings &
		get_render_settings() const;


		GPlatesViewOperations::RenderedGeometryParameters &
		get_rendered_geometry_parameters();

		const GPlatesViewOperations::RenderedGeometryParameters &
		get_rendered_geometry_parameters() const;


		GPlatesGui::SceneLightingParameters &
		get_scene_lighting_parameters();

		const GPlatesGui::SceneLightingParameters &
		get_scene_lighting_parameters() const;


		VisualLayers &
		get_visual_layers();

		const VisualLayers &
		get_visual_layers() const;


		VisualLayerRegistry &
		get_visual_layer_registry();

		const VisualLayerRegistry &
		get_visual_layer_registry() const;


		GPlatesGui::MapTransform &
		get_map_transform();

		const GPlatesGui::MapTransform &
		get_map_transform() const;


		const std::pair<int, int> &
		get_main_viewport_dimensions() const;


		void
		set_main_viewport_dimensions(
				const std::pair<int, int> &dimensions);


		int
		get_main_viewport_min_dimension() const;

		
		int
		get_main_viewport_max_dimension() const;

		// TODO: the get_last_open_directory methods should be obsolete now, but retain
		// until the FileIODirectoryConfiguration stuff has been tested further.
		QString &
		get_last_open_directory();

		const QString &
		get_last_open_directory() const;

		bool
		get_show_stars() const;


		void
		set_show_stars(
				bool show_stars = false);


		GPlatesGui::symbol_map_type &
		get_feature_type_symbol_map();

		const GPlatesGui::symbol_map_type &
		get_feature_type_symbol_map() const;


		const GPlatesGui::Colour &
		get_background_colour() const;


		void
		set_background_colour(
				const GPlatesGui::Colour &colour);


		GPlatesGui::GraticuleSettings &
		get_graticule_settings();


		const GPlatesGui::GraticuleSettings &
		get_graticule_settings() const;


		GPlatesGui::TextOverlaySettings &
		get_text_overlay_settings();
		
		const GPlatesGui::TextOverlaySettings &
		get_text_overlay_settings() const;


		GPlatesGui::ExportAnimationRegistry &
		get_export_animation_registry();

		const GPlatesGui::ExportAnimationRegistry &
		get_export_animation_registry() const;


		GPlatesGui::TopologySectionsContainer &
		get_topology_boundary_sections_container();

		const GPlatesGui::TopologySectionsContainer &
		get_topology_boundary_sections_container() const;


		GPlatesGui::TopologySectionsContainer &
		get_topology_interior_sections_container();

		const GPlatesGui::TopologySectionsContainer &
		get_topology_interior_sections_container() const;


		GPlatesGui::FileIODirectoryConfigurations&
		get_file_io_directory_configurations();

		const GPlatesGui::FileIODirectoryConfigurations&
		get_file_io_directory_configurations() const;


		GPlatesGui::PythonManager&
		get_python_manager()
		{
			return *d_python_manager_ptr;
		}


	private Q_SLOTS:


		void
		handle_zoom_change();


	private:

		void
		connect_to_viewport_zoom();

		void
		connect_to_feature_focus();

		void
		setup_rendered_geometry_collection();
		
		/**
		 * Overrides some ViewState settings' defaults based on UserPreferences.
		 */
		void
		initialise_from_user_preferences();

		//
		// NOTE: Most of these are boost::scoped_ptr's to avoid having to include header files.
		//

		GPlatesAppLogic::ApplicationState &d_application_state;

		// FIXME: remove this when refactored
		GPlatesQtWidgets::ViewportWindow *d_other_view_state;

		//! Handles logic for animating the reconstruction time (for time slider and export).
		boost::scoped_ptr<GPlatesGui::AnimationController> d_animation_controller;

		//! Manages saving and restoring sessions.
		boost::scoped_ptr<SessionManagement> d_session_management_ptr;

		//! Contains all rendered geometries for this view state.
		boost::scoped_ptr<GPlatesViewOperations::RenderedGeometryCollection> d_rendered_geometry_collection;

		/**
		 * Tracks the currently focused feature (if any).
		 *
		 * Depends on @a d_rendered_geometry_collection.
		 */
		boost::scoped_ptr<GPlatesGui::FeatureFocus> d_feature_focus;

		//! The 'Clicked' table. Depends on FeatureFocus. NOTE: Not parented by Qt.	
		boost::scoped_ptr<GPlatesGui::FeatureTableModel> d_feature_table_model_ptr;

		//! Holds all loaded colour schemes, sorted by category.
		boost::scoped_ptr<GPlatesGui::ColourSchemeContainer> d_colour_scheme_container;

		//! Keeps track of the currently selected colour scheme.
		GPlatesGlobal::PointerTraits<GPlatesGui::ColourSchemeDelegator>::non_null_ptr_type d_colour_scheme;

		//! The viewport zoom state.
		boost::scoped_ptr<GPlatesGui::ViewportZoom> d_viewport_zoom;

		//! The viewport projection state.
		boost::scoped_ptr<GPlatesGui::ViewportProjection> d_viewport_projection;

		//! Builds geometry for digitised geometry.
		boost::scoped_ptr<GPlatesViewOperations::GeometryBuilder> d_digitise_geometry_builder;

		//! Builds geometry for the focused feature.
		boost::scoped_ptr<GPlatesViewOperations::GeometryBuilder> d_focused_feature_geometry_builder;

		//! Depends on @a d_focused_feature_geometry_builder and @a d_feature_focus.
		boost::scoped_ptr<GPlatesViewOperations::FocusedFeatureGeometryManipulator> d_focused_feature_geom_manipulator;

		/**
		 * Holds map of feature type to symbol.
		 */
		GPlatesGui::symbol_map_type d_feature_type_symbol_map;

		//! What geometry types get rendered and what don't.
		boost::scoped_ptr<GPlatesGui::RenderSettings> d_render_settings;

		/**
		 * Render parameters (point/line sizes, colours, etc) of geometries (mostly in canvas tools).
		 *
		 * NOTE: This needs to be declared before 'd_visual_layers'.
		 */
		boost::scoped_ptr<GPlatesViewOperations::RenderedGeometryParameters> d_rendered_geometry_parameters;

		//! Parameters used when lighting the scene during OpenGL rendering.
		boost::scoped_ptr<GPlatesGui::SceneLightingParameters> d_scene_lighting_parameters;

		/**
		 * Manages the various layers (usually corresponding to each loaded feature collection)
		 * whose output results are drawn into child layers of the RECONSTRUCTION main
		 * rendered geometry layer.
		 */
		boost::scoped_ptr<VisualLayers> d_visual_layers;

		/**
		 * Stores information about the available visual layer types.
		 */
		boost::scoped_ptr<VisualLayerRegistry> d_visual_layer_registry;

		//! Sends signals to transform maps
		boost::scoped_ptr<GPlatesGui::MapTransform> d_map_transform;

		/**
		 * The dimensions (in pixels) of the main globe or map attached.
		 * Used for scaling additional globes and maps.
		 */
		std::pair<int, int> d_main_viewport_dimensions;

		/**
		 * Stores the directory containing the files last opened, or the last opened
		 * directory.
		 *
		 * This should be obsolete now that FileIODirectoryConfigurations are being
		 * used, but will retain for the time being.
		 */
		QString d_last_open_directory;

		/**
		 * Stores last-used directories, preferred behaviour etc for various file types
		 */
		boost::scoped_ptr<GPlatesGui::FileIODirectoryConfigurations> d_file_io_directory_configurations;

		/**
		 * Whether to draw stars behind the 3D globe.
		 */
		bool d_show_stars;

		/**
		 * The colour of the background sphere or plane in 3D globe or map view respectively.
		 */
		boost::scoped_ptr<GPlatesGui::Colour> d_background_colour;

		/**
		 * Settings related to the graticules displayed on the map and the globe.
		 */
		boost::scoped_ptr<GPlatesGui::GraticuleSettings> d_graticule_settings;

		/**
		 * Settings related to the overlay of text on top the map and the globe.
		 */
		boost::scoped_ptr<GPlatesGui::TextOverlaySettings> d_text_overlay_settings;

		/**
		 * Stores information about the export animation types.
		 */
		boost::scoped_ptr<GPlatesGui::ExportAnimationRegistry> d_export_animation_registry;

		//! The data behind the Topology Sections table (containing boundary sections).
		boost::scoped_ptr<GPlatesGui::TopologySectionsContainer> d_topology_boundary_sections_container_ptr;

		//! The data behind the Topology Sections table (containing interior sections).
		boost::scoped_ptr<GPlatesGui::TopologySectionsContainer> d_topology_interior_sections_container_ptr;

		GPlatesGui::PythonManager* d_python_manager_ptr;
	};
}

#endif // GPLATES_PRESENTATION_VIEWSTATE_H
