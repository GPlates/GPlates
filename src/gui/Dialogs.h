/* $Id$ */

/**
 * \file Responsible for managing instances of GPlatesQtWidgets::GPlatesDialog in the application.
 *
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
 
#ifndef GPLATES_GUI_DIALOGS_H
#define GPLATES_GUI_DIALOGS_H

#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/weak_ptr.hpp>
#include <QPointer>


namespace GPlatesAppLogic
{
	class ApplicationState;
}

namespace GPlatesPresentation
{
	class ViewState;
	class VisualLayer;
}

namespace GPlatesQtWidgets
{
	class GPlatesDialog;
	class ViewportWindow;
	
	////////////////////////////////////////////////
	// Forward declarations for our dialogs.
	////////////////////////////////////////////////
	class AboutDialog;
	class AnimateDialog;
	class AssignReconstructionPlateIdsDialog;
	class CalculateReconstructionPoleDialog;
	class ChooseFeatureCollectionDialog;
	class ColouringDialog;
	class ConfigureGraticulesDialog;
	class ConfigureTextOverlayDialog;
	class ConnectWFSDialog;
	class CreateVGPDialog;
	class DrawStyleDialog;
	class ExportAnimationDialog;
	class FeaturePropertiesDialog;
	class LicenseDialog;
	class LogDialog;
	class ManageFeatureCollectionsDialog;
	class GenerateVelocityDomainCitcomsDialog;
	class PreferencesDialog;
	class ReadErrorAccumulationDialog;
	class SetCameraViewpointDialog;
	class SetProjectionDialog;
	class ShapefileAttributeViewerDialog;
	class SpecifyAnchoredPlateIdDialog;
	class SymbolManagerDialog;
	class TotalReconstructionPolesDialog;
	class TotalReconstructionSequencesDialog;
	class VisualLayersDialog;
}

namespace GPlatesGui
{
	/**
	 * Class responsible for managing instances of GPlatesDialog in the application.
	 * 
	 * Major dialogs and handy floating window-like things that typically hang off of ViewportWindow
	 * are to be managed here to avoid further cluttering up the ViewportWindow class itself.
	 *
	 * All such instances are of our QDialog subclass, GPlatesDialog, so that we have a few helper
	 * methods available. All are parented to ViewportWindow itself, so that Qt knows how the hierarchy
	 * of windows and dialogs should be logically arranged. However, methods for accessing those
	 * dialogs should be kept here.
	 *
	 * IMPORTANT NOTE: The exact flow of this class isn't quite finalised yet, so don't refactor
	 *                 everything in ViewportWindow just yet. Specifically, I want a neat way to
	 *                 ensure that menu action connections can be done without having to #include
	 *                 the entire dialog in ViewportWindow.cc.
	 */
	class Dialogs :
			public QObject,
			private boost::noncopyable
	{
		Q_OBJECT
	public:
	
		/**
		 * Much like the ApplicationState-members, GPlatesGui::Dialogs should be instantiated and
		 * kept somewhere nice. It is a QObject so that we can use signals and slots, and will be
		 * noncopyable.
		 */
		explicit
		Dialogs(
				GPlatesAppLogic::ApplicationState &_application_state,
				GPlatesPresentation::ViewState &_view_state,
				GPlatesQtWidgets::ViewportWindow &_viewport_window,
				QObject *_parent);
		
		virtual
		~Dialogs()
		{  }

		////////////////////////////////////////////////////////////////////////
		// Here are all the accessors for dialogs managed by this class.
		////////////////////////////////////////////////////////////////////////

		GPlatesQtWidgets::AboutDialog &
		about_dialog();

		GPlatesQtWidgets::AnimateDialog &
		animate_dialog();

		GPlatesQtWidgets::AssignReconstructionPlateIdsDialog &
		assign_reconstruction_plate_ids_dialog();

		GPlatesQtWidgets::CalculateReconstructionPoleDialog &
		calculate_reconstruction_pole_dialog();

		GPlatesQtWidgets::ChooseFeatureCollectionDialog &
		choose_feature_collection_dialog();

		GPlatesQtWidgets::ColouringDialog &
		colouring_dialog();

		GPlatesQtWidgets::ConfigureGraticulesDialog &
		configure_graticules_dialog();

		GPlatesQtWidgets::ConfigureTextOverlayDialog &
		configure_text_overlay_dialog();

		GPlatesQtWidgets::ConnectWFSDialog &
		connect_wfs_dialog();

		GPlatesQtWidgets::CreateVGPDialog &
		create_vgp_dialog();

		GPlatesQtWidgets::DrawStyleDialog &
		draw_style_dialog();

		GPlatesQtWidgets::ExportAnimationDialog &
		export_animation_dialog();

		GPlatesQtWidgets::FeaturePropertiesDialog &
		feature_properties_dialog();

		GPlatesQtWidgets::LicenseDialog &
		license_dialog();

		GPlatesQtWidgets::LogDialog &
		log_dialog();

		GPlatesQtWidgets::ManageFeatureCollectionsDialog &
		manage_feature_collections_dialog();

		GPlatesQtWidgets::GenerateVelocityDomainCitcomsDialog &
		velocity_domain_citcoms_dialog();

		GPlatesQtWidgets::PreferencesDialog &
		preferences_dialog();

		GPlatesQtWidgets::ReadErrorAccumulationDialog &
		read_error_accumulation_dialog();

		GPlatesQtWidgets::SetCameraViewpointDialog &
		set_camera_viewpoint_dialog();

		GPlatesQtWidgets::SetProjectionDialog &
		set_projection_dialog();

		GPlatesQtWidgets::ShapefileAttributeViewerDialog &
		shapefile_attribute_viewer_dialog();

		GPlatesQtWidgets::SpecifyAnchoredPlateIdDialog &
		specify_anchored_plate_id_dialog();

		GPlatesQtWidgets::SymbolManagerDialog &
		symbol_manager_dialog();

		GPlatesQtWidgets::TotalReconstructionPolesDialog &
		total_reconstruction_poles_dialog();

		GPlatesQtWidgets::TotalReconstructionSequencesDialog &
		total_reconstruction_sequences_dialog();

		GPlatesQtWidgets::VisualLayersDialog &
		visual_layers_dialog();

		////////////////////////////////////////////////////////////////////////

	public Q_SLOTS:

		////////////////////////////////////////////////////////////////////////
		// And here are wrappers around various_dialogs().pop_up() so that
		// those dialogs which support it can be lazy-loaded after the user
		// triggers their appropriate menu item.
		//
		// NOTE: Can also include extra code besides 'pop_up()'.
		// These pop up methods have been moved over from @a ViewportWindow.
		//
		// Not recommended for anything that has tight integration with the
		// rest of GPlates, as it may cause Heisenbugs that only occur before
		// or after certain dialogs are shown for the first time - such as
		// one involving the Configure Animation Dialog's slider.
		//
		// The reason these methods exist (eg, log_dialog().pop_up() is not being used directly by user)
		// is to allow the dialogs to be lazy-loaded not just at app startup, but *even later* when the
		// user first triggers it. Not all dialogs will be able to cope with this as they may have
		// quite tight integration with the rest of the app.
		////////////////////////////////////////////////////////////////////////

		void
		pop_up_about_dialog();

		void
		pop_up_animate_dialog();

		void
		pop_up_assign_reconstruction_plate_ids_dialog();

		void
		pop_up_calculate_reconstruction_pole_dialog();

		void
		pop_up_colouring_dialog();

		void
		pop_up_configure_graticules_dialog();

		void
		pop_up_configure_text_overlay_dialog();

		void
		pop_up_connect_wfs_dialog();

		void
		pop_up_create_vgp_dialog();

		void
		pop_up_draw_style_dialog();

		void
		pop_up_export_animation_dialog();

		void
		pop_up_feature_properties_dialog();

		void
		pop_up_license_dialog();

		void
		pop_up_log_dialog();

		void
		pop_up_manage_feature_collections_dialog();

		void
		pop_up_velocity_domain_citcoms_dialog();

		void
		pop_up_preferences_dialog();

		void
		pop_up_read_error_accumulation_dialog();

		void
		pop_up_set_camera_viewpoint_dialog();

		void
		pop_up_set_projection_dialog();

		void
		pop_up_shapefile_attribute_viewer_dialog();

		void
		pop_up_specify_anchored_plate_id_dialog();

		void
		pop_up_symbol_manager_dialog();

		void
		pop_up_total_reconstruction_poles_dialog();

		void
		pop_up_total_reconstruction_poles_dialog(
				boost::weak_ptr<GPlatesPresentation::VisualLayer> visual_layer);

		void
		pop_up_total_reconstruction_sequences_dialog();

		void
		pop_up_visual_layers_dialog();


		////////////////////////////////////////////////////////////////////////

		/**
		 * Closes any QDialog instances parented to ViewportWindow.
		 */
		void
		close_all_dialogs();

	private:

		/**
		 * The different dialog types.
		 */
		enum DialogType
		{
			DIALOG_ABOUT,
			DIALOG_ANIMATE,
			DIALOG_ASSIGN_RECONSTRUCTION_PLATE_IDS,
			DIALOG_CALCULATE_RECONSTRUCTION_POLE,
			DIALOG_CHOOSE_FEATURE_COLLECTION,
			DIALOG_COLOURING,
			DIALOG_CONFIGURE_GRATICULES,
			DIALOG_CONFIGURE_TEXT_OVERLAY,
			DIALOG_CONNECT_WFS,
			DIALOG_CREATE_VGP,
			DIALOG_DRAW_STYLE,
			DIALOG_EXPORT_ANIMATION,
			DIALOG_FEATURE_PROPERTIES,
			DIALOG_LICENSE,
			DIALOG_LOG,
			DIALOG_MANAGE_FEATURE_COLLECTIONS,
			DIALOG_VELOCITY_DOMAIN_CITCOMS,
			DIALOG_PREFERENCES,
			DIALOG_READ_ERROR_ACCUMULATION,
			DIALOG_SET_CAMERA_VIEWPOINT,
			DIALOG_SET_PROJECTION,
			DIALOG_SHAPEFILE_ATTRIBUTE_VIEWER,
			DIALOG_SPECIFY_ANCHORED_PLATE_ID,
			DIALOG_SYMBOL_MANAGER,
			DIALOG_TOTAL_RECONSTRUCTION_POLES,
			DIALOG_TOTAL_RECONSTRUCTION_SEQUENCES,
			DIALOG_VISUAL_LAYERS_DIALOG,

			NUM_DIALOGS // NOTE: This must be last.
		};


		/**
		 * Convenience method to get at ApplicationState.
		 */
		GPlatesAppLogic::ApplicationState &
		application_state() const;

		/**
		 * Convenience method to get at ViewState.
		 */
		GPlatesPresentation::ViewState &
		view_state() const;
		
		/**
		 * Convenience method to get at ViewportWindow.
		 */
		GPlatesQtWidgets::ViewportWindow &
		viewport_window() const;


		/**
		 * We keep guarded pointers to major GPlates classes to help with dialog construction.
		 */
		QPointer<GPlatesAppLogic::ApplicationState> d_application_state_ptr;
		QPointer<GPlatesPresentation::ViewState> d_view_state_ptr;
		QPointer<GPlatesQtWidgets::ViewportWindow> d_viewport_window_ptr;


		/**
		 * List of all dialogs managed by this class.
		 */
		std::vector< QPointer<GPlatesQtWidgets::GPlatesDialog> > d_dialogs;
	};
}


#endif //GPLATES_GUI_DIALOGS_H
