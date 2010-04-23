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

#include "global/PointerTraits.h"

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
	class FeatureCollectionWorkflow;
	class PaleomagWorkflow;
	class PlateVelocityWorkflow;
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
	class RenderSettings;
	class ViewportProjection;
	class ViewportZoom;
}

namespace GPlatesModel
{
	class Reconstruction;
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
	class ViewState :
			public QObject,
			private boost::noncopyable
	{
		Q_OBJECT
		
	public:
	
		static const double INITIAL_VGP_DELTA_T;

		/**
		 *  Stores render settings for VirtualGeomagneticPole features.                                                                     
		 */
		class VGPRenderSettings
		{



		public:
			enum VGPVisibilitySetting{
				ALWAYS_VISIBLE,
				TIME_WINDOW,
				DELTA_T_AROUND_AGE
			};		

			VGPRenderSettings():
				d_vgp_visibility_setting(DELTA_T_AROUND_AGE),
				d_vgp_delta_t(INITIAL_VGP_DELTA_T),
				d_vgp_earliest_time(GPlatesPropertyValues::GeoTimeInstant::create_distant_past()),
				d_vgp_latest_time(GPlatesPropertyValues::GeoTimeInstant::create_distant_future()),
				d_should_draw_circular_error(true)
				{ }


				VGPVisibilitySetting 
				get_vgp_visibility_setting() const
				{	
					return d_vgp_visibility_setting;
				}

				void
				set_vgp_visibility_setting(
				VGPVisibilitySetting setting)
				{
					d_vgp_visibility_setting = setting;
				}

				double
				get_vgp_delta_t() const
				{
					return d_vgp_delta_t;
				}

				void
				set_vgp_delta_t(
				const double &vgp_delta_t)
				{	
					d_vgp_delta_t = vgp_delta_t;
				}

				const 
				GPlatesPropertyValues::GeoTimeInstant &
				get_vgp_earliest_time() 
				{
					return d_vgp_earliest_time;	
				};

				const 
				GPlatesPropertyValues::GeoTimeInstant &
				get_vgp_latest_time() 
				{
					return d_vgp_latest_time;	
				};

				void
				set_vgp_earliest_time(
					const GPlatesPropertyValues::GeoTimeInstant &earliest_time)
				{	
					d_vgp_earliest_time = GPlatesPropertyValues::GeoTimeInstant(earliest_time);
				}

				void
				set_vgp_latest_time(
					const GPlatesPropertyValues::GeoTimeInstant &latest_time)
				{	
					d_vgp_latest_time = GPlatesPropertyValues::GeoTimeInstant(latest_time);
				};		

				bool
				should_draw_circular_error()
				{
					return d_should_draw_circular_error;
				}

				void
				set_should_draw_circular_error(
					bool should_draw_circular_error_)
				{
					d_should_draw_circular_error = should_draw_circular_error_;
				}

		private:

			/**
			 *  enum indicating what sort of VGP visibility we have, one of:
			 *		ALWAYS_VISIBLE		- all vgps are displayed at all times
			 *		TIME_WINDOW			- all vgps are displayed between a specified time interval
			 *		DELTA_T_AROUND_AGE  - vgps are displayed if the reconstruction time is within a time window 
			 *							  around the VGP's age.                                                                   
			 */
			VGPVisibilitySetting d_vgp_visibility_setting;

			/**
			 *  Delta used for time window around VGP age.                                                                     
			 */
			double d_vgp_delta_t;

			/**
			 *  Begin time used when the TIME_WINDOW VGPVisibilitySetting is selected.                                                                    
			 */
			GPlatesPropertyValues::GeoTimeInstant d_vgp_earliest_time;

			/**
			 *  End time used when the TIME_WINDOW VGPVisibilitySetting is selected.                                                                    
			 */
			GPlatesPropertyValues::GeoTimeInstant d_vgp_latest_time;

			/**
			 * Whether or not we should draw pole errors as circles around the pole location.
			 * 
			 * If true, we draw circles (circle size defined by the A95 property).
			 * If false, we draw ellipses. (ellipse size defined by yet-to-be-calculated properties).                                                                     
			 */
			bool d_should_draw_circular_error;
		};	
	
	
	
		ViewState(
				GPlatesAppLogic::ApplicationState &application_state);

		~ViewState();


		GPlatesAppLogic::ApplicationState &
		get_application_state();


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


		const GPlatesAppLogic::PlateVelocityWorkflow &
		get_plate_velocity_workflow() const;


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


		GPlatesGui::RenderSettings &
		get_render_settings();


		GPlatesGui::MapTransform &
		get_map_transform();


		int
		get_main_viewport_min_dimension();


		void
		set_main_viewport_min_dimension(
				int min_dimension);

		VGPRenderSettings &
		get_vgp_render_settings()
		{
			return d_vgp_render_settings;
		}

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

		//! Feature collection workflow for calculating plate velocities - pointer owns workflow.
		boost::scoped_ptr<GPlatesAppLogic::PlateVelocityWorkflow> d_plate_velocity_workflow;

		//! This shared pointer only unregisters the plate velocity workflow - doesn't own it.
		boost::shared_ptr<GPlatesAppLogic::FeatureCollectionWorkflow> d_plate_velocity_unregister;

		boost::shared_ptr<GPlatesAppLogic::PaleomagWorkflow> d_paleomag_workflow;
		boost::shared_ptr<GPlatesAppLogic::FeatureCollectionWorkflow> d_paleomag_unregister;

		/**
		 * Performs tasks each time a reconstruction is generated.
		 *
		 * Depends on d_plate_velocity_workflow, d_rendered_geometry_collection.
		 */
		boost::scoped_ptr<GPlatesViewOperations::ReconstructView> d_reconstruct_view;

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
		VGPRenderSettings d_vgp_render_settings;

		void
		connect_to_viewport_zoom();

		void
		connect_to_feature_focus();

		void
		setup_rendered_geometry_collection();
	};
}

#endif // GPLATES_PRESENTATION_VIEWSTATE_H
