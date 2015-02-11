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

#include <boost/foreach.hpp>
#include <QDebug>
#include <QtGlobal>

#include "Dialogs.h"

#include "app-logic/ApplicationState.h"

#include "global/GPlatesAssert.h"
#include "global/AssertionFailureException.h"

#include "gui/ViewportProjection.h"
#include "gui/TrinketArea.h"

#include "maths/InvalidLatLonException.h"

#include "presentation/ViewState.h"

#include "qt-widgets/GPlatesDialog.h"
#include "qt-widgets/QtWidgetUtils.h"
#include "qt-widgets/ReconstructionViewWidget.h"
#include "qt-widgets/SceneView.h"
#include "qt-widgets/ViewportWindow.h"

////////////////////////////////////////////////
// #includes for all dialogs managed by us.
////////////////////////////////////////////////
#include "qt-widgets/AboutDialog.h"
#include "qt-widgets/AnimateDialog.h"
#include "qt-widgets/AssignReconstructionPlateIdsDialog.h"
#include "qt-widgets/CalculateReconstructionPoleDialog.h"
#include "qt-widgets/FiniteRotationCalculatorDialog.h"
#include "qt-widgets/ChooseFeatureCollectionDialog.h"
#include "qt-widgets/ColouringDialog.h"
#include "qt-widgets/ConfigureCanvasToolGeometryRenderParametersDialog.h"
#include "qt-widgets/ConfigureGraticulesDialog.h"
#include "qt-widgets/ConfigureTextOverlayDialog.h"
#include "qt-widgets/ConnectWFSDialog.h"
#include "qt-widgets/CreateVGPDialog.h"
#include "qt-widgets/DrawStyleDialog.h"
#include "qt-widgets/ExportAnimationDialog.h"
#include "qt-widgets/GenerateVelocityDomainCitcomsDialog.h"
#include "qt-widgets/GenerateVelocityDomainLatLonDialog.h"
#include "qt-widgets/GenerateVelocityDomainTerraDialog.h"
#include "qt-widgets/FeaturePropertiesDialog.h"
#include "qt-widgets/KinematicGraphsDialog.h"
#include "qt-widgets/LicenseDialog.h"
#include "qt-widgets/LogDialog.h"
#include "qt-widgets/ManageFeatureCollectionsDialog.h"
#include "qt-widgets/PreferencesDialog.h"
#include "qt-widgets/ReadErrorAccumulationDialog.h"
#include "qt-widgets/SetCameraViewpointDialog.h"
#include "qt-widgets/SetProjectionDialog.h"
#include "qt-widgets/ShapefileAttributeViewerDialog.h"
#include "qt-widgets/SpecifyAnchoredPlateIdDialog.h"
#include "qt-widgets/SymbolManagerDialog.h"
#include "qt-widgets/TotalReconstructionPolesDialog.h"
#include "qt-widgets/TotalReconstructionSequencesDialog.h"
#include "qt-widgets/VisualLayersDialog.h"

GPlatesGui::Dialogs::Dialogs(
		GPlatesAppLogic::ApplicationState &_application_state,
		GPlatesPresentation::ViewState &_view_state,
		GPlatesQtWidgets::ViewportWindow &_viewport_window,
		QObject *_parent) :
	QObject(_parent),
	d_application_state_ptr(&_application_state),
	d_view_state_ptr(&_view_state),
	d_viewport_window_ptr(&_viewport_window),
	d_dialogs(NUM_DIALOGS) // Size the std::vector for the number of dialogs.
{
}


////////////////////////////////////////////////////////////////////////
// Here are all the accessors for dialogs managed by this class.
//
// Observe that they use a static member pointer to also hold the
// instances of those dialogs.
// However the static member pointer does not own the instance since
// each dialog is parented (eg, to the main window).
////////////////////////////////////////////////////////////////////////


GPlatesQtWidgets::AboutDialog &
GPlatesGui::Dialogs::about_dialog()
{
	// Putting this upfront reduces chance of error when copy'n'pasting for a new dialog function.
	const DialogType dialog_type = DIALOG_ABOUT;
	typedef GPlatesQtWidgets::AboutDialog dialog_typename;

	if (d_dialogs[dialog_type].isNull())
	{
		d_dialogs[dialog_type] = new dialog_typename(
				*this,
				application_state().get_gpgim(),
				&viewport_window());
	}

	return dynamic_cast<dialog_typename &>(*d_dialogs[dialog_type]);
}

void
GPlatesGui::Dialogs::pop_up_about_dialog()
{
	about_dialog().exec();
}


GPlatesQtWidgets::AnimateDialog &
GPlatesGui::Dialogs::animate_dialog()
{
	// Putting this upfront reduces chance of error when copy'n'pasting for a new dialog function.
	const DialogType dialog_type = DIALOG_ANIMATE;
	typedef GPlatesQtWidgets::AnimateDialog dialog_typename;

	if (d_dialogs[dialog_type].isNull())
	{
		d_dialogs[dialog_type] = new dialog_typename(
				view_state().get_animation_controller(),
				&viewport_window());
	}

	return dynamic_cast<dialog_typename &>(*d_dialogs[dialog_type]);
}

void
GPlatesGui::Dialogs::pop_up_animate_dialog()
{
	animate_dialog().pop_up();
}


GPlatesQtWidgets::AssignReconstructionPlateIdsDialog &
GPlatesGui::Dialogs::assign_reconstruction_plate_ids_dialog()
{
	// Putting this upfront reduces chance of error when copy'n'pasting for a new dialog function.
	const DialogType dialog_type = DIALOG_ASSIGN_RECONSTRUCTION_PLATE_IDS;
	typedef GPlatesQtWidgets::AssignReconstructionPlateIdsDialog dialog_typename;

	if (d_dialogs[dialog_type].isNull())
	{
		d_dialogs[dialog_type] = new dialog_typename(application_state(), view_state(), &viewport_window());
	}

	return dynamic_cast<dialog_typename &>(*d_dialogs[dialog_type]);
}

void
GPlatesGui::Dialogs::pop_up_assign_reconstruction_plate_ids_dialog()
{
	assign_reconstruction_plate_ids_dialog().exec_partition_features_dialog();
}


GPlatesQtWidgets::CalculateReconstructionPoleDialog &
GPlatesGui::Dialogs::calculate_reconstruction_pole_dialog()
{
	// Putting this upfront reduces chance of error when copy'n'pasting for a new dialog function.
	const DialogType dialog_type = DIALOG_CALCULATE_RECONSTRUCTION_POLE;
	typedef GPlatesQtWidgets::CalculateReconstructionPoleDialog dialog_typename;

	if (d_dialogs[dialog_type].isNull())
	{
		d_dialogs[dialog_type] = new dialog_typename(view_state(), &viewport_window());
	}

	return dynamic_cast<dialog_typename &>(*d_dialogs[dialog_type]);
}

void
GPlatesGui::Dialogs::pop_up_calculate_reconstruction_pole_dialog()
{
	calculate_reconstruction_pole_dialog().pop_up();
}


GPlatesQtWidgets::ChooseFeatureCollectionDialog &
GPlatesGui::Dialogs::choose_feature_collection_dialog()
{
	// Putting this upfront reduces chance of error when copy'n'pasting for a new dialog function.
	const DialogType dialog_type = DIALOG_CHOOSE_FEATURE_COLLECTION;
	typedef GPlatesQtWidgets::ChooseFeatureCollectionDialog dialog_typename;

	if (d_dialogs[dialog_type].isNull())
	{
		d_dialogs[dialog_type] = new dialog_typename(
				application_state().get_reconstruct_method_registry(),
				application_state().get_feature_collection_file_state(),
				application_state().get_feature_collection_file_io(),
				&viewport_window());
	}

	return dynamic_cast<dialog_typename &>(*d_dialogs[dialog_type]);
}


GPlatesQtWidgets::ColouringDialog &
GPlatesGui::Dialogs::colouring_dialog()
{
	// Putting this upfront reduces chance of error when copy'n'pasting for a new dialog function.
	const DialogType dialog_type = DIALOG_COLOURING;
	typedef GPlatesQtWidgets::ColouringDialog dialog_typename;

	if (d_dialogs[dialog_type].isNull())
	{
		d_dialogs[dialog_type] = new dialog_typename(
				view_state(),
				viewport_window().reconstruction_view_widget().globe_and_map_widget(),
				read_error_accumulation_dialog(),
				&viewport_window());
	}

	return dynamic_cast<dialog_typename &>(*d_dialogs[dialog_type]);
}

void
GPlatesGui::Dialogs::pop_up_colouring_dialog()
{
	colouring_dialog().pop_up();
}


GPlatesQtWidgets::ConfigureCanvasToolGeometryRenderParametersDialog &
GPlatesGui::Dialogs::configure_canvas_tool_geometry_render_parameters_dialog()
{
	// Putting this upfront reduces chance of error when copy'n'pasting for a new dialog function.
	const DialogType dialog_type = DIALOG_CONFIGURE_CANVAS_TOOL_GEOMETRY_RENDER_PARAMETERS;
	typedef GPlatesQtWidgets::ConfigureCanvasToolGeometryRenderParametersDialog dialog_typename;

	if (d_dialogs[dialog_type].isNull())
	{
		d_dialogs[dialog_type] = new dialog_typename(
				view_state().get_rendered_geometry_parameters(),
				&viewport_window());
	}

	return dynamic_cast<dialog_typename &>(*d_dialogs[dialog_type]);
}

void
GPlatesGui::Dialogs::pop_up_configure_canvas_tool_geometry_render_parameters_dialog()
{
	configure_canvas_tool_geometry_render_parameters_dialog().pop_up();
}


GPlatesQtWidgets::ConfigureGraticulesDialog &
GPlatesGui::Dialogs::configure_graticules_dialog()
{
	// Putting this upfront reduces chance of error when copy'n'pasting for a new dialog function.
	const DialogType dialog_type = DIALOG_CONFIGURE_GRATICULES;
	typedef GPlatesQtWidgets::ConfigureGraticulesDialog dialog_typename;

	if (d_dialogs[dialog_type].isNull())
	{
		d_dialogs[dialog_type] = new dialog_typename(&viewport_window());
	}

	return dynamic_cast<dialog_typename &>(*d_dialogs[dialog_type]);
}

void
GPlatesGui::Dialogs::pop_up_configure_graticules_dialog()
{
	if (configure_graticules_dialog().exec(view_state().get_graticule_settings()) == QDialog::Accepted)
	{
		viewport_window().reconstruction_view_widget().update();
	}
}


GPlatesQtWidgets::ConfigureTextOverlayDialog &
GPlatesGui::Dialogs::configure_text_overlay_dialog()
{
	// Putting this upfront reduces chance of error when copy'n'pasting for a new dialog function.
	const DialogType dialog_type = DIALOG_CONFIGURE_TEXT_OVERLAY;
	typedef GPlatesQtWidgets::ConfigureTextOverlayDialog dialog_typename;

	if (d_dialogs[dialog_type].isNull())
	{
		d_dialogs[dialog_type] = new dialog_typename(&viewport_window());
	}

	return dynamic_cast<dialog_typename &>(*d_dialogs[dialog_type]);
}

void
GPlatesGui::Dialogs::pop_up_configure_text_overlay_dialog()
{
	if (configure_text_overlay_dialog().exec(view_state().get_text_overlay_settings()) == QDialog::Accepted)
	{
		viewport_window().reconstruction_view_widget().update();
	}
}


GPlatesQtWidgets::ConnectWFSDialog &
GPlatesGui::Dialogs::connect_wfs_dialog()
{
	// Putting this upfront reduces chance of error when copy'n'pasting for a new dialog function.
	const DialogType dialog_type = DIALOG_CONNECT_WFS;
	typedef GPlatesQtWidgets::ConnectWFSDialog dialog_typename;

	if (d_dialogs[dialog_type].isNull())
	{
		d_dialogs[dialog_type] = new dialog_typename(application_state(), &viewport_window());
	}

	return dynamic_cast<dialog_typename &>(*d_dialogs[dialog_type]);
}

void
GPlatesGui::Dialogs::pop_up_connect_wfs_dialog()
{
	connect_wfs_dialog().pop_up();
}


GPlatesQtWidgets::CreateVGPDialog &
GPlatesGui::Dialogs::create_vgp_dialog()
{
	// Putting this upfront reduces chance of error when copy'n'pasting for a new dialog function.
	const DialogType dialog_type = DIALOG_CREATE_VGP;
	typedef GPlatesQtWidgets::CreateVGPDialog dialog_typename;

	if (d_dialogs[dialog_type].isNull())
	{
		d_dialogs[dialog_type] = new dialog_typename(view_state(), &viewport_window());
	}

	return dynamic_cast<dialog_typename &>(*d_dialogs[dialog_type]);
}

void
GPlatesGui::Dialogs::pop_up_create_vgp_dialog()
{
	GPlatesQtWidgets::CreateVGPDialog &dialog = create_vgp_dialog();

	dialog.reset();
	dialog.exec();
}


GPlatesQtWidgets::DrawStyleDialog &
GPlatesGui::Dialogs::draw_style_dialog()
{
	// Putting this upfront reduces chance of error when copy'n'pasting for a new dialog function.
	const DialogType dialog_type = DIALOG_DRAW_STYLE;
	typedef GPlatesQtWidgets::DrawStyleDialog dialog_typename;

	if (d_dialogs[dialog_type].isNull())
	{
		d_dialogs[dialog_type] = new dialog_typename(view_state(), &viewport_window());
	}

	return dynamic_cast<dialog_typename &>(*d_dialogs[dialog_type]);
}

void
GPlatesGui::Dialogs::pop_up_draw_style_dialog()
{
	GPlatesQtWidgets::DrawStyleDialog &dialog = draw_style_dialog();

	dialog.init_category_table();
	dialog.pop_up();
}


GPlatesQtWidgets::ExportAnimationDialog &
GPlatesGui::Dialogs::export_animation_dialog()
{
	// Putting this upfront reduces chance of error when copy'n'pasting for a new dialog function.
	const DialogType dialog_type = DIALOG_EXPORT_ANIMATION;
	typedef GPlatesQtWidgets::ExportAnimationDialog dialog_typename;

	if (d_dialogs[dialog_type].isNull())
	{
		d_dialogs[dialog_type] = new dialog_typename(
				view_state(),
				viewport_window(),
				&viewport_window());
	}

	return dynamic_cast<dialog_typename &>(*d_dialogs[dialog_type]);
}

void
GPlatesGui::Dialogs::pop_up_export_animation_dialog()
{
	// FIXME: Should Export Animation be modal?
	export_animation_dialog().pop_up();
}


GPlatesQtWidgets::FeaturePropertiesDialog &
GPlatesGui::Dialogs::feature_properties_dialog()
{
	// Putting this upfront reduces chance of error when copy'n'pasting for a new dialog function.
	const DialogType dialog_type = DIALOG_FEATURE_PROPERTIES;
	typedef GPlatesQtWidgets::FeaturePropertiesDialog dialog_typename;

	if (d_dialogs[dialog_type].isNull())
	{
		d_dialogs[dialog_type] = new dialog_typename(view_state(), &viewport_window());
	}

	return dynamic_cast<dialog_typename &>(*d_dialogs[dialog_type]);
}

void
GPlatesGui::Dialogs::pop_up_feature_properties_dialog()
{
	feature_properties_dialog().pop_up();
}

GPlatesQtWidgets::FiniteRotationCalculatorDialog &
GPlatesGui::Dialogs::finite_rotation_calculator_dialog()
{
	// Putting this upfront reduces chance of error when copy'n'pasting for a new dialog function.
	const DialogType dialog_type = DIALOG_FINITE_ROTATION_CALCULATOR_DIALOG;
	typedef GPlatesQtWidgets::FiniteRotationCalculatorDialog dialog_typename;

	if (d_dialogs[dialog_type].isNull())
	{
		d_dialogs[dialog_type] = new dialog_typename(&viewport_window());
	}

	return dynamic_cast<dialog_typename &>(*d_dialogs[dialog_type]);
}

void
GPlatesGui::Dialogs::pop_up_finite_rotation_calculator_dialog()
{
	finite_rotation_calculator_dialog().pop_up();
}

GPlatesQtWidgets::KinematicGraphsDialog &
GPlatesGui::Dialogs::kinematics_tool_dialog()
{
	// Putting this upfront reduces chance of error when copy'n'pasting for a new dialog function.
	const DialogType dialog_type = DIALOG_KINEMATICS_TOOL;
	typedef GPlatesQtWidgets::KinematicGraphsDialog dialog_typename;

	if (d_dialogs[dialog_type].isNull())
	{
		d_dialogs[dialog_type] = new dialog_typename(view_state(),&viewport_window());
	}

	return dynamic_cast<dialog_typename &>(*d_dialogs[dialog_type]);
}

void
GPlatesGui::Dialogs::pop_up_kinematics_tool_dialog()
{
	kinematics_tool_dialog().pop_up();
}

GPlatesQtWidgets::LicenseDialog &
GPlatesGui::Dialogs::license_dialog()
{
	// Putting this upfront reduces chance of error when copy'n'pasting for a new dialog function.
	const DialogType dialog_type = DIALOG_LICENSE;
	typedef GPlatesQtWidgets::LicenseDialog dialog_typename;

	if (d_dialogs[dialog_type].isNull())
	{
		d_dialogs[dialog_type] = new dialog_typename(&viewport_window());
	}

	return dynamic_cast<dialog_typename &>(*d_dialogs[dialog_type]);
}

void
GPlatesGui::Dialogs::pop_up_license_dialog()
{
	license_dialog().pop_up();
}


GPlatesQtWidgets::LogDialog &
GPlatesGui::Dialogs::log_dialog()
{
	// Putting this upfront reduces chance of error when copy'n'pasting for a new dialog function.
	const DialogType dialog_type = DIALOG_LOG;
	typedef GPlatesQtWidgets::LogDialog dialog_typename;

	if (d_dialogs[dialog_type].isNull())
	{
		d_dialogs[dialog_type] = new dialog_typename(application_state(), &viewport_window());
	}

	return dynamic_cast<dialog_typename &>(*d_dialogs[dialog_type]);
}

void
GPlatesGui::Dialogs::pop_up_log_dialog()
{
	log_dialog().pop_up();
}


GPlatesQtWidgets::ManageFeatureCollectionsDialog &
GPlatesGui::Dialogs::manage_feature_collections_dialog()
{
	// Putting this upfront reduces chance of error when copy'n'pasting for a new dialog function.
	const DialogType dialog_type = DIALOG_MANAGE_FEATURE_COLLECTIONS;
	typedef GPlatesQtWidgets::ManageFeatureCollectionsDialog dialog_typename;

	if (d_dialogs[dialog_type].isNull())
	{
		d_dialogs[dialog_type] = new dialog_typename(
				application_state().get_feature_collection_file_state(),
				application_state().get_feature_collection_file_io(),
				viewport_window().file_io_feedback(),
				application_state().get_reconstruct_graph(),
				view_state(),
				&viewport_window());
	}

	return dynamic_cast<dialog_typename &>(*d_dialogs[dialog_type]);
}

void
GPlatesGui::Dialogs::pop_up_manage_feature_collections_dialog()
{
	manage_feature_collections_dialog().pop_up();
}


GPlatesQtWidgets::PreferencesDialog &
GPlatesGui::Dialogs::preferences_dialog()
{
	// Putting this upfront reduces chance of error when copy'n'pasting for a new dialog function.
	const DialogType dialog_type = DIALOG_PREFERENCES;
	typedef GPlatesQtWidgets::PreferencesDialog dialog_typename;

	if (d_dialogs[dialog_type].isNull())
	{
		d_dialogs[dialog_type] = new dialog_typename(application_state(), &viewport_window());
	}

	return dynamic_cast<dialog_typename &>(*d_dialogs[dialog_type]);
}

void
GPlatesGui::Dialogs::pop_up_preferences_dialog()
{
	preferences_dialog().pop_up();
}


GPlatesQtWidgets::ReadErrorAccumulationDialog &
GPlatesGui::Dialogs::read_error_accumulation_dialog()
{
	// Putting this upfront reduces chance of error when copy'n'pasting for a new dialog function.
	const DialogType dialog_type = DIALOG_READ_ERROR_ACCUMULATION;
	typedef GPlatesQtWidgets::ReadErrorAccumulationDialog dialog_typename;

	if (d_dialogs[dialog_type].isNull())
	{
		d_dialogs[dialog_type] = new dialog_typename(&viewport_window());
	}

	return dynamic_cast<dialog_typename &>(*d_dialogs[dialog_type]);
}

void
GPlatesGui::Dialogs::pop_up_read_error_accumulation_dialog()
{
	read_error_accumulation_dialog().pop_up();

	// Finally, if we're showing the Read Errors dialog, the user already knows about
	// the errors and doesn't need to see the reminder in the status bar.
	viewport_window().trinket_area().read_errors_trinket().setVisible(false);
}


GPlatesQtWidgets::SetCameraViewpointDialog &
GPlatesGui::Dialogs::set_camera_viewpoint_dialog()
{
	// Putting this upfront reduces chance of error when copy'n'pasting for a new dialog function.
	const DialogType dialog_type = DIALOG_SET_CAMERA_VIEWPOINT;
	typedef GPlatesQtWidgets::SetCameraViewpointDialog dialog_typename;

	if (d_dialogs[dialog_type].isNull())
	{
		d_dialogs[dialog_type] = new dialog_typename(viewport_window(), &viewport_window());
	}

	return dynamic_cast<dialog_typename &>(*d_dialogs[dialog_type]);
}

void
GPlatesGui::Dialogs::pop_up_set_camera_viewpoint_dialog()
{
	GPlatesQtWidgets::SetCameraViewpointDialog &dialog = set_camera_viewpoint_dialog();

	boost::optional<GPlatesMaths::LatLonPoint> cur_llp_opt =
			viewport_window().reconstruction_view_widget().camera_llp();
	GPlatesMaths::LatLonPoint cur_llp = cur_llp_opt ? *cur_llp_opt : GPlatesMaths::LatLonPoint(0.0, 0.0);

	dialog.set_lat_lon(cur_llp.latitude(), cur_llp.longitude());

	if (dialog.exec())
	{
		try
		{
			GPlatesMaths::LatLonPoint desired_centre(dialog.latitude(), dialog.longitude());

			viewport_window().reconstruction_view_widget().active_view().set_camera_viewpoint(desired_centre);
		}
		catch (GPlatesMaths::InvalidLatLonException &)
		{
			// The argument name in the above expression was removed to
			// prevent "unreferenced local variable" compiler warnings under MSVC.

			// User somehow managed to specify an invalid lat,lon. Pretend it didn't happen.
		}
	}
}


GPlatesQtWidgets::SetProjectionDialog &
GPlatesGui::Dialogs::set_projection_dialog()
{
	// Putting this upfront reduces chance of error when copy'n'pasting for a new dialog function.
	const DialogType dialog_type = DIALOG_SET_PROJECTION;
	typedef GPlatesQtWidgets::SetProjectionDialog dialog_typename;

	if (d_dialogs[dialog_type].isNull())
	{
		d_dialogs[dialog_type] = new dialog_typename(viewport_window(), &viewport_window());
	}

	return dynamic_cast<dialog_typename &>(*d_dialogs[dialog_type]);
}

void
GPlatesGui::Dialogs::pop_up_set_projection_dialog()
{
	GPlatesQtWidgets::SetProjectionDialog &dialog = set_projection_dialog();

	dialog.setup();

	if (dialog.exec())
	{
		try
		{
			// Notify the view state of the projection change.
			// It will handle the rest.
			GPlatesGui::ViewportProjection &viewport_projection = view_state().get_viewport_projection();
			viewport_projection.set_projection_type(dialog.get_projection_type());
			viewport_projection.set_central_meridian(dialog.central_meridian());
		}
		catch(GPlatesGui::ProjectionException &e)
		{
			qWarning() << e;
		}
	}
}


GPlatesQtWidgets::ShapefileAttributeViewerDialog &
GPlatesGui::Dialogs::shapefile_attribute_viewer_dialog()
{
	// Putting this upfront reduces chance of error when copy'n'pasting for a new dialog function.
	const DialogType dialog_type = DIALOG_SHAPEFILE_ATTRIBUTE_VIEWER;
	typedef GPlatesQtWidgets::ShapefileAttributeViewerDialog dialog_typename;

	if (d_dialogs[dialog_type].isNull())
	{
		d_dialogs[dialog_type] = new dialog_typename(
				application_state().get_feature_collection_file_state(),
				&viewport_window());
	}

	return dynamic_cast<dialog_typename &>(*d_dialogs[dialog_type]);
}

void
GPlatesGui::Dialogs::pop_up_shapefile_attribute_viewer_dialog()
{
	GPlatesQtWidgets::ShapefileAttributeViewerDialog &dialog = shapefile_attribute_viewer_dialog();

	dialog.pop_up();
	dialog.update(application_state().get_feature_collection_file_state());
}


GPlatesQtWidgets::SpecifyAnchoredPlateIdDialog &
GPlatesGui::Dialogs::specify_anchored_plate_id_dialog()
{
	// Putting this upfront reduces chance of error when copy'n'pasting for a new dialog function.
	const DialogType dialog_type = DIALOG_SPECIFY_ANCHORED_PLATE_ID;
	typedef GPlatesQtWidgets::SpecifyAnchoredPlateIdDialog dialog_typename;

	if (d_dialogs[dialog_type].isNull())
	{
		d_dialogs[dialog_type] = new dialog_typename(&viewport_window());
	}

	return dynamic_cast<dialog_typename &>(*d_dialogs[dialog_type]);
}

void
GPlatesGui::Dialogs::pop_up_specify_anchored_plate_id_dialog()
{
	GPlatesQtWidgets::SpecifyAnchoredPlateIdDialog &dialog = specify_anchored_plate_id_dialog();

	dialog.populate(
			application_state().get_current_anchored_plate_id(),
			view_state().get_feature_focus().focused_feature());
	dialog.pop_up();
}


GPlatesQtWidgets::SymbolManagerDialog &
GPlatesGui::Dialogs::symbol_manager_dialog()
{
	// Putting this upfront reduces chance of error when copy'n'pasting for a new dialog function.
	const DialogType dialog_type = DIALOG_SYMBOL_MANAGER;
	typedef GPlatesQtWidgets::SymbolManagerDialog dialog_typename;

	if (d_dialogs[dialog_type].isNull())
	{
		d_dialogs[dialog_type] = new dialog_typename(&viewport_window());
	}

	return dynamic_cast<dialog_typename &>(*d_dialogs[dialog_type]);
}

void
GPlatesGui::Dialogs::pop_up_symbol_manager_dialog()
{
	symbol_manager_dialog().pop_up();
}


GPlatesQtWidgets::TotalReconstructionPolesDialog &
GPlatesGui::Dialogs::total_reconstruction_poles_dialog()
{
	// Putting this upfront reduces chance of error when copy'n'pasting for a new dialog function.
	const DialogType dialog_type = DIALOG_TOTAL_RECONSTRUCTION_POLES;
	typedef GPlatesQtWidgets::TotalReconstructionPolesDialog dialog_typename;

	if (d_dialogs[dialog_type].isNull())
	{
		d_dialogs[dialog_type] = new dialog_typename(view_state(), &viewport_window());
	}

	return dynamic_cast<dialog_typename &>(*d_dialogs[dialog_type]);
}

void
GPlatesGui::Dialogs::pop_up_total_reconstruction_poles_dialog()
{
	GPlatesQtWidgets::TotalReconstructionPolesDialog &dialog = total_reconstruction_poles_dialog();

	dialog.pop_up();
	dialog.update();
}

void
GPlatesGui::Dialogs::pop_up_total_reconstruction_poles_dialog(
		boost::weak_ptr<GPlatesPresentation::VisualLayer> visual_layer)
{
	GPlatesQtWidgets::TotalReconstructionPolesDialog &dialog = total_reconstruction_poles_dialog();

	dialog.pop_up();
	dialog.update(visual_layer);
}


GPlatesQtWidgets::TotalReconstructionSequencesDialog &
GPlatesGui::Dialogs::total_reconstruction_sequences_dialog()
{
	// Putting this upfront reduces chance of error when copy'n'pasting for a new dialog function.
	const DialogType dialog_type = DIALOG_TOTAL_RECONSTRUCTION_SEQUENCES;
	typedef GPlatesQtWidgets::TotalReconstructionSequencesDialog dialog_typename;

	if (d_dialogs[dialog_type].isNull())
	{
		d_dialogs[dialog_type] = new dialog_typename(
				application_state().get_feature_collection_file_state(),
				view_state(),
				&viewport_window());
	}

	return dynamic_cast<dialog_typename &>(*d_dialogs[dialog_type]);
}

void
GPlatesGui::Dialogs::pop_up_total_reconstruction_sequences_dialog()
{
	GPlatesQtWidgets::TotalReconstructionSequencesDialog &dialog = total_reconstruction_sequences_dialog();
	
	dialog.pop_up();
	dialog.update();
}


GPlatesQtWidgets::GenerateVelocityDomainCitcomsDialog &
GPlatesGui::Dialogs::velocity_domain_citcoms_dialog()
{
	// Putting this upfront reduces chance of error when copy'n'pasting for a new dialog function.
	const DialogType dialog_type = DIALOG_VELOCITY_DOMAIN_CITCOMS;
	typedef GPlatesQtWidgets::GenerateVelocityDomainCitcomsDialog dialog_typename;

	if (d_dialogs[dialog_type].isNull())
	{
		d_dialogs[dialog_type] = new dialog_typename(viewport_window(), &viewport_window());
	}

	return dynamic_cast<dialog_typename &>(*d_dialogs[dialog_type]);
}

void
GPlatesGui::Dialogs::pop_up_velocity_domain_citcoms_dialog()
{
	velocity_domain_citcoms_dialog().pop_up();
}


GPlatesQtWidgets::GenerateVelocityDomainLatLonDialog &
GPlatesGui::Dialogs::velocity_domain_lat_lon_dialog()
{
	// Putting this upfront reduces chance of error when copy'n'pasting for a new dialog function.
	const DialogType dialog_type = DIALOG_VELOCITY_DOMAIN_LAT_LON;
	typedef GPlatesQtWidgets::GenerateVelocityDomainLatLonDialog dialog_typename;

	if (d_dialogs[dialog_type].isNull())
	{
		d_dialogs[dialog_type] = new dialog_typename(viewport_window(), &viewport_window());
	}

	return dynamic_cast<dialog_typename &>(*d_dialogs[dialog_type]);
}

void
GPlatesGui::Dialogs::pop_up_velocity_domain_lat_lon_dialog()
{
	velocity_domain_lat_lon_dialog().pop_up();
}


GPlatesQtWidgets::GenerateVelocityDomainTerraDialog &
GPlatesGui::Dialogs::velocity_domain_terra_dialog()
{
	// Putting this upfront reduces chance of error when copy'n'pasting for a new dialog function.
	const DialogType dialog_type = DIALOG_VELOCITY_DOMAIN_TERRA;
	typedef GPlatesQtWidgets::GenerateVelocityDomainTerraDialog dialog_typename;

	if (d_dialogs[dialog_type].isNull())
	{
		d_dialogs[dialog_type] = new dialog_typename(viewport_window(), &viewport_window());
	}

	return dynamic_cast<dialog_typename &>(*d_dialogs[dialog_type]);
}

void
GPlatesGui::Dialogs::pop_up_velocity_domain_terra_dialog()
{
	velocity_domain_terra_dialog().pop_up();
}


GPlatesQtWidgets::VisualLayersDialog &
GPlatesGui::Dialogs::visual_layers_dialog()
{
	// Putting this upfront reduces chance of error when copy'n'pasting for a new dialog function.
	const DialogType dialog_type = DIALOG_VISUAL_LAYERS_DIALOG;
	typedef GPlatesQtWidgets::VisualLayersDialog dialog_typename;

	if (d_dialogs[dialog_type].isNull())
	{
		GPlatesQtWidgets::VisualLayersDialog *dialog = new dialog_typename(
				view_state().get_visual_layers(),
				application_state(),
				view_state(),
				&viewport_window(),
				&viewport_window());

		d_dialogs[dialog_type] = dialog;
	}

	return dynamic_cast<dialog_typename &>(*d_dialogs[dialog_type]);
}

void
GPlatesGui::Dialogs::pop_up_visual_layers_dialog()
{
	visual_layers_dialog().pop_up();
}


////////////////////////////////////////////////////////////////////////

void
GPlatesGui::Dialogs::close_all_dialogs()
{
	BOOST_FOREACH(QPointer<GPlatesQtWidgets::GPlatesDialog> &dialog, d_dialogs)
	{
		if (!dialog.isNull())
		{
			dialog->reject();
		}
	}
}


GPlatesAppLogic::ApplicationState &
GPlatesGui::Dialogs::application_state() const
{
	return *d_application_state_ptr;
}

GPlatesPresentation::ViewState &
GPlatesGui::Dialogs::view_state() const
{
	return *d_view_state_ptr;
}

GPlatesQtWidgets::ViewportWindow &
GPlatesGui::Dialogs::viewport_window() const
{
	return *d_viewport_window_ptr;
}




