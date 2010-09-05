/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <QObject>
#include <QString>

#include "global/PointerTraits.h"

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
	class VGPRenderSettings;
}

namespace GPlatesGui
{
	class Colour;
	class ColourScheme;
	class ColourSchemeContainer;
	class ColourSchemeDelegator;
	class GeometryFocusHighlight;
	class FeatureFocus;
	class MapTransform;
	class RasterColourSchemeMap;
	class RenderSettings;
	class ViewportProjection;
	class ViewportZoom;
}

namespace GPlatesQtWidgets
{
	class ViewportWindow;
}

namespace GPlatesViewOperations
{
	class ReconstructView;
	class RenderedGeometryCollection;
}

namespace GPlatesPresentation
{
	class VisualLayerRegistry;
	class VisualLayers;

	class ViewState :
			public QObject,
			private boost::noncopyable
	{
		Q_OBJECT
		
	public:
	
		ViewState(
				GPlatesAppLogic::ApplicationState &application_state);

		~ViewState();


		GPlatesAppLogic::ApplicationState &
		get_application_state();

		inline
		const GPlatesAppLogic::ApplicationState &
		get_application_state() const
		{
			return d_application_state;
		}


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


		GPlatesViewOperations::RenderedGeometryCollection &
		get_rendered_geometry_collection();


		GPlatesGui::FeatureFocus &
		get_feature_focus();


		GPlatesGui::ViewportZoom &
		get_viewport_zoom();


		GPlatesGui::ViewportProjection &
		get_viewport_projection();


		//! Returns all loaded colour schemes, sorted by category.
		GPlatesGui::ColourSchemeContainer &
		get_colour_scheme_container();


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


		GPlatesGlobal::PointerTraits<GPlatesGui::ColourSchemeDelegator>::non_null_ptr_type
		get_colour_scheme_delegator();


		VisualLayers &
		get_visual_layers();


		VisualLayerRegistry &
		get_visual_layer_registry();


		GPlatesGui::RenderSettings &
		get_render_settings();


		GPlatesGui::MapTransform &
		get_map_transform();


		int
		get_main_viewport_min_dimension();


		void
		set_main_viewport_min_dimension(
				int min_dimension);


		inline
		const GPlatesAppLogic::VGPRenderSettings &
		get_vgp_render_settings() const
		{
			return *d_vgp_render_settings;
		}
	
		GPlatesAppLogic::VGPRenderSettings &
		get_vgp_render_settings();


		GPlatesGui::RasterColourSchemeMap &
		get_raster_colour_scheme_map();


		const GPlatesGui::RasterColourSchemeMap &
		get_raster_colour_scheme_map() const;

		QString &
		get_open_file_path();

		const QString &
		get_open_file_path() const;


	private slots:


		void
		handle_zoom_change();


	private:
		//
		// NOTE: Most of these are boost::scoped_ptr's to avoid having to include header files.
		//

		GPlatesAppLogic::ApplicationState &d_application_state;

		// FIXME: remove this when refactored
		GPlatesQtWidgets::ViewportWindow *d_other_view_state;

		//! Contains all rendered geometries for this view state.
		boost::scoped_ptr<GPlatesViewOperations::RenderedGeometryCollection> d_rendered_geometry_collection;

		//! Holds all loaded colour schemes, sorted by category.
		boost::scoped_ptr<GPlatesGui::ColourSchemeContainer> d_colour_scheme_container;

		//! Keeps track of the currently selected colour scheme.
		GPlatesGlobal::PointerTraits<GPlatesGui::ColourSchemeDelegator>::non_null_ptr_type d_colour_scheme;

		//! The viewport zoom state.
		boost::scoped_ptr<GPlatesGui::ViewportZoom> d_viewport_zoom;

		//! The viewport projection state.
		boost::scoped_ptr<GPlatesGui::ViewportProjection> d_viewport_projection;

		/**
		 * Renders the focused geometry highlighted.
		 *
		 * Depends on d_rendered_geometry_collection.
		 */
		boost::scoped_ptr<GPlatesGui::GeometryFocusHighlight> d_geometry_focus_highlight;

		/**
		 * Tracks the currently focused feature (if any).
		 *
		 * Depends on d_reconstruct.
		 */
		boost::scoped_ptr<GPlatesGui::FeatureFocus> d_feature_focus;

		// FIXME: remove these 
		GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type
			d_comp_mesh_point_layer;
		GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type
			d_comp_mesh_arrow_layer;
			
		GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type
			d_paleomag_layer;

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

		//! What gets rendered and what doesn't
		boost::scoped_ptr<GPlatesGui::RenderSettings> d_render_settings;

		//! Sends signals to transform maps
		boost::scoped_ptr<GPlatesGui::MapTransform> d_map_transform;

		/**
		 * The smaller of width or height of the main globe or map attached.
		 * Used for scaling additional globes and maps.
		 */
		int d_main_viewport_min_dimension;

		/**
		 * Stores render settings for VirtualGeomagneticPole features.                                                                     
		 */
		boost::scoped_ptr<GPlatesAppLogic::VGPRenderSettings> d_vgp_render_settings;

		/**
		 * Stores the mapping of Layer to RasterColourSchemes.
		 */
		boost::scoped_ptr<GPlatesGui::RasterColourSchemeMap> d_raster_colour_scheme_map;

		/**
		 * Stores the file path of the last opened file, or the directory of the last
		 * opened directory.
		 *
		 * NOTE: This is currently only used for rasters.
		 */
		QString d_open_file_path;

		void
		connect_to_viewport_zoom();

		void
		connect_to_feature_focus();

		void
		connect_to_raster_colour_scheme_map();

		void
		setup_rendered_geometry_collection();
	};
}

#endif // GPLATES_PRESENTATION_VIEWSTATE_H
