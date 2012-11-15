/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009, 2011 The University of Sydney, Australia
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

#include <string>
#include <boost/bind.hpp>
#include <boost/utility/in_place_factory.hpp>
#include <QDebug>

#include "Application.h"

#include "file-io/ShapefileReader.h"

#include "gui/AnimationController.h"
#include "gui/Dialogs.h"
#include "gui/FeatureFocus.h"

#include "qt-widgets/CreateFeatureDialog.h"
#include "qt-widgets/DigitisationWidget.h"
#include "qt-widgets/ManageFeatureCollectionsEditConfigurations.h"
#include "qt-widgets/SearchResultsDockWidget.h"
#include "qt-widgets/ShapefileAttributeViewerDialog.h"
#include "qt-widgets/ShapefilePropertyMapper.h"
#include "qt-widgets/SpecifyAnchoredPlateIdDialog.h"
#include "qt-widgets/TaskPanel.h"

#include <boost/foreach.hpp>

GPlatesPresentation::Application::Application() :
	d_view_state(d_application_state),
	d_main_window(d_application_state, d_view_state)
{
	initialise();
}


void
GPlatesPresentation::Application::initialise()
{
	// Register the default edit configurations for those file formats that have configurations.
	GPlatesQtWidgets::ManageFeatureCollections::register_default_edit_configurations(
			d_main_window.dialogs().manage_feature_collections_dialog(),
			d_application_state.get_model_interface());

	// Initialise the Shapefile property mapper before we start reading.
	// FIXME: Not sure where this should go since it involves qt widgets (logical place is
	// in FeatureCollectionFileIO but that is application state and shouldn't know about
	// qt widgets).
	boost::shared_ptr<GPlatesQtWidgets::ShapefilePropertyMapper> shapefile_property_mapper(
			new GPlatesQtWidgets::ShapefilePropertyMapper(&d_main_window));
	GPlatesFileIO::ShapefileReader::set_property_mapper(shapefile_property_mapper);

	// If the focus is changed programatically, from e.g. Clone Feature, ensure the Clicked
	// Table still displays it.
	QObject::connect(
			&d_view_state.get_feature_focus(),
			SIGNAL(focused_feature_modified(GPlatesGui::FeatureFocus &)),
			&d_main_window.search_results_dock_widget(),
			SLOT(highlight_focused_feature_in_table(GPlatesGui::FeatureFocus &)));

	// If the focused feature is modified, we may need to update the ShapefileAttributeViewerDialog.
	QObject::connect(
			&d_view_state.get_feature_focus(),
			SIGNAL(focused_feature_modified(GPlatesGui::FeatureFocus &)),
			&d_main_window.dialogs().shapefile_attribute_viewer_dialog(),
			SLOT(update()));

	// If the Specify Anchored Plate ID dialog changes the anchored plate id then perform a reconstruction.
	QObject::connect(
			&d_main_window.dialogs().specify_anchored_plate_id_dialog(),
			SIGNAL(value_changed(GPlatesModel::integer_plate_id_type)),
			&d_application_state,
			SLOT(set_anchored_plate_id(GPlatesModel::integer_plate_id_type)));

	// If the user creates a new feature with the DigitisationWidget, we need to reconstruct to
	// make sure everything is displayed properly.
	QObject::connect(
			&d_main_window.task_panel_ptr()->digitisation_widget().get_create_feature_dialog(),
			SIGNAL(feature_created(GPlatesModel::FeatureHandle::weak_ref)),
			&d_application_state,
			SLOT(reconstruct()));

	// Render everything on the screen in present-day positions.
	d_application_state.reconstruct();

	// Initialise the default range of the animation slider based on UserPreferences.
	// FIXME: For some reason this comes *after* reconstructing - not sure if that should be the case.
	d_view_state.get_animation_controller().init_default_time_range();
}


void
GPlatesPresentation::Application::enable_syncing_with_external_applications(
		bool gplates_is_master)
{
	if (!d_external_sync_controller)
	{
		// Pass ExternalSyncController constructor parameters to boost::optional to
		// construct a new object directly in-place.
		d_external_sync_controller = boost::in_place(
				gplates_is_master,
				&d_main_window,
				&d_view_state);
	}

	d_external_sync_controller->enable_external_syncing();
}


void
GPlatesPresentation::Application::set_reconstruction_time(
		const double &reconstruction_time)
{
	d_view_state.get_animation_controller().set_view_time(reconstruction_time);
}
