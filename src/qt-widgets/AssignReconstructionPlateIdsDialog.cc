/* $Id$ */
 
/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009, 2010 The University of Sydney, Australia
 *
 * This file is part of GPlates.
 *
 * GPlates is free software; you can redistribute file_iter and/or modify file_iter under
 * the terms of the GNU General Public License, version 2, as published by
 * the Free Software Foundation.
 *
 * GPlates is distributed in the hope that file_iter will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <numeric>
#include <vector>
#include <boost/cast.hpp>
#include <boost/foreach.hpp>
#include <QHeaderView>
#include <QMessageBox>
#include <QRadioButton>
#include <QString>

#include "AssignReconstructionPlateIdsDialog.h"

#include "ProgressDialog.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/AssignPlateIds.h"
#include "app-logic/Layer.h"
#include "app-logic/FeatureCollectionFileState.h"
#include "app-logic/Reconstruction.h"
#include "app-logic/ReconstructionTree.h"
#include "app-logic/TopologyUtils.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "gui/FeatureFocus.h"

#include "presentation/ViewState.h"
#include "presentation/VisualLayers.h"


namespace
{
	const QString HELP_PARTITIONING_LAYERS_DIALOG_TITLE = QObject::tr("Selecting partitioning layers");
	const QString HELP_PARTITIONING_LAYERS_DIALOG_TEXT = QObject::tr(
			"<html><body>\n"
			"<h3>Select either a 'Resolved Topological Closed Plate Boundaries' layer or "
			"a 'Reconstructed Geometries' layer that contains static polygon geometry.</h3>"
			"<p>These polygons will be intersected with features and a subset of the polygon's "
			"feature properties will be copied over.</p>"
			"</body></html>\n");
	const QString HELP_PARTITIONED_FILES_DIALOG_TITLE = QObject::tr("Selecting feature collections");
	const QString HELP_PARTITIONED_FILES_DIALOG_TEXT = QObject::tr(
			"<html><body>\n"
			"<h3>Select the feature collections that will be assigned reconstruction plate ids</h3>"
			"<p>All selected feature collections will have their features assigned a "
			"reconstruction plate id (if they already have one it will be overwritten).</p>"
			"<p>It is also possible to assign time of appearance and disappearance.</p>"
			"</body></html>\n");
	const QString HELP_RECONSTRUCTION_TIME_DIALOG_TITLE = QObject::tr("Selecting reconstruction time");
	const QString HELP_RECONSTRUCTION_TIME_DIALOG_TEXT = QObject::tr(
			"<html><body>\n"
			"<h3>Select the reconstruction time representing the geometry in the "
			"feature collections</h3>"
			"<p>The three options for reconstruction time are:</p>"
			"<ul>"
			"<li><b>Present day:</b> reconstruction time is 0Ma.</li>\n"
			"<li><b>Current reconstruction time:</b> the reconstruction time in the main window.</li>\n"
			"<li><b>Specify reconstruction time:</b> choose an arbitrary reconstruction time.</li>\n"
			"</ul>"
			"<p><em>Note: Present day should be selected when assigning plate ids to "
			"<b>VirtualGeomagneticPole</b> features.</em></p>"
			"<p>The polygons are reconstructed to the reconstruction time before "
			"testing for overlap/intersection with the features being partitioned.</p>"
			"<p>The geometry in partitioned features effectively represents a snapshot "
			"of the features at the reconstruction time. In other words the features to "
			"be partitioned effectively contain geometry at the reconstruction time "
			"regardless of whether they have a reconstruction plate id property or not.</p>"
			"</body></html>\n");
	const QString HELP_PARTITION_OPTIONS_DIALOG_TITLE = QObject::tr("Feature partition options");
	const QString HELP_PARTITION_OPTIONS_DIALOG_TEXT = QObject::tr(
			"<html><body>\n"
			"<h3>Specify how to partition features using the polygons</h3>"
			"These three options determine how features are partitioned."
			"<h4>Copy feature properties from the polygon that most overlaps a feature:</h4>\n"
			"<ul>\n"
			"<li>Assign, to each feature, a single plate id corresponding to the "
			"polygon that the feature's geometry overlaps the most.</li>\n"
			"<li>If a feature contains multiple sub-geometries then they are treated as "
			"one composite geometry for the purpose of this test.</li>\n"
			"</ul>\n"
			"<h4>Copy feature properties from the polygon that most overlaps each geometry in a feature:</h4>\n"
			"<ul>\n"
			"<li>Assign, to each sub-geometry of each feature, a single plate id "
			"corresponding to the polygon that the sub-geometry overlaps the most.</li>\n"
			"<li>This can create extra features, for example if a feature has two "
			"sub-geometries and one overlaps plate A the most and the other "
			"overlaps plate B the most then the original feature (with the two "
			"geometries) will then get split into two features - one feature will contain "
			"the first geometry (and have plate id A) and the other feature will "
			"contain the second geometry (and have plate id B).</li>\n"
			"</ul>\n"
			"<h4>Partition (cookie cut) feature geometry into polygons and copy feature properties:</h4>\n"
			"<ul>\n"
			"<li>Partition all geometries of each feature into the polygons "
			"containing them (intersecting them as needed).</li>\n"
			"<li>This can create extra features, for example if a feature has only one "
			"sub-geometry but it overlaps plate A and plate B then it is partitioned "
			"into geometry that is fully contained by plate A and likewise for plate B.  "
			"These two partitioned geometries will now be two features since they "
			"have different plate ids.</li>\n"
			"</ul>\n"
			"<p>If the polygons do not cover the entire surface of the globe then it is "
			"possible for some features (or partitioned geometries) to fall outside "
			"all polygons. In this situation the feature is not modified and will retain "
			"its original feature properties (such as reconstruction plate id).</p>"
			"<p><em><b>VirtualGeomagneticPole</b> features are treated differently - these "
			"features are assigned to the polygon whose boundary contains the feature's "
			"sample site point location. For these features the above options are ignored.</em></p>"
			"</body></html>\n");
	const QString HELP_PROPERTIES_TO_ASSIGN_DIALOG_TITLE = QObject::tr("Feature properties options");
	const QString HELP_PROPERTIES_TO_ASSIGN_DIALOG_TEXT = QObject::tr(
			"<html><body>\n"
			"<h3>Specify which feature properties to copy from a polygon</h3>"
			"<p>The two feature property options:</p>"
			"<ul>"
			"<li><b>Reconstruction plate id:</b> reconstruction time is 0Ma.</li>\n"
			"<li><b>Time of appearance and disappearance:</b> the time interval a feature exists.</li>\n"
			"</ul>"
			"<p>These options are not mutually exclusive.</p>"
			"<p>These properties are copied from the polygon feature to the feature being partitioned.</p>"
			"</body></html>\n");
}


namespace
{
	/**
	 * Finds the total number of features in a set of feature collections.
	 */
	GPlatesModel::container_size_type
	get_num_features(
			const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &feature_collections)
	{
		typedef std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> feature_collection_seq_type;

		GPlatesModel::container_size_type num_features = 0;

		feature_collection_seq_type::const_iterator feature_collection_iter = feature_collections.begin();
		feature_collection_seq_type::const_iterator feature_collection_end = feature_collections.end();
		for ( ; feature_collection_iter != feature_collection_end; ++feature_collection_iter)
		{
			num_features += (*feature_collection_iter)->size();
		}

		return num_features;
	}
}


GPlatesQtWidgets::AssignReconstructionPlateIdsDialog::AssignReconstructionPlateIdsDialog(
		GPlatesAppLogic::ApplicationState &application_state,
		GPlatesPresentation::ViewState &view_state,
		QWidget *parent_):
	QDialog(parent_, Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::MSWindowsFixedSizeDialogHint),
	d_help_partitioning_layers_dialog(
			new InformationDialog(
					HELP_PARTITIONING_LAYERS_DIALOG_TEXT,
					HELP_PARTITIONING_LAYERS_DIALOG_TITLE,
					this)),
	d_help_partitioned_files_dialog(
			new InformationDialog(
					HELP_PARTITIONED_FILES_DIALOG_TEXT,
					HELP_PARTITIONED_FILES_DIALOG_TITLE,
					this)),
	d_help_reconstruction_time_dialog(
			new InformationDialog(
					HELP_RECONSTRUCTION_TIME_DIALOG_TEXT,
					HELP_RECONSTRUCTION_TIME_DIALOG_TITLE,
					this)),
	d_help_partition_options_dialog(
			new InformationDialog(
					HELP_PARTITION_OPTIONS_DIALOG_TEXT,
					HELP_PARTITION_OPTIONS_DIALOG_TITLE,
					this)),
	d_help_properties_to_assign_dialog(
			new InformationDialog(
					HELP_PROPERTIES_TO_ASSIGN_DIALOG_TEXT,
					HELP_PROPERTIES_TO_ASSIGN_DIALOG_TITLE,
					this)),
	d_button_create(NULL),
	d_feature_collection_file_state(application_state.get_feature_collection_file_state()),
	d_application_state(view_state.get_application_state()),
	d_feature_focus(view_state.get_feature_focus()),
	d_visual_layers(view_state.get_visual_layers()),
	d_reconstruction_time_type(PRESENT_DAY_RECONSTRUCTION_TIME),
	d_spin_box_reconstruction_time(0),
	d_assign_plate_id_method(
			GPlatesAppLogic::AssignPlateIds::ASSIGN_FEATURE_TO_MOST_OVERLAPPING_PLATE),
	d_assign_plate_ids(true),
	d_assign_time_periods(false)
{
	setupUi(this);

	// NOTE: This needs to be done first thing after setupUi() is called.
	d_partitioning_layer_state_seq.table_widget = table_partitioning_layers;
	d_partitioning_layer_state_seq.layer_selection_group = new QButtonGroup(table_partitioning_layers);
	d_partitioned_file_state_seq.table_widget = table_partitioned_files;

	set_up_button_box();

	set_up_partitioning_layers_page();
	set_up_partitioned_files_page();
	set_up_general_options_page();

	// When the current page is changed, we need to enable and disable some buttons.
	QObject::connect(
			stack_widget, SIGNAL(currentChanged(int)),
			this, SLOT(handle_page_change(int)));

	// Send a fake page change event to ensure buttons are set up properly at start.
	handle_page_change(0);
}


void
GPlatesQtWidgets::AssignReconstructionPlateIdsDialog::exec_partition_features_dialog()
{
	const file_ptr_seq_type loaded_files = get_loaded_files();
	const layer_ptr_seq_type layers = get_possible_partitioning_layers();

	// Setup the partitioning layer list and the partitioned file list in the widget.
	initialise_layer_list(d_partitioning_layer_state_seq, layers);
	initialise_file_list(d_partitioned_file_state_seq, loaded_files);

	// Set the current reconstruction time label.
	label_current_reconstruction_time->setText(
			QString::number(d_application_state.get_current_reconstruction_time()));

	// Set the stack back to the first page.
	stack_widget->setCurrentIndex(0);

	// Get the user to confirm the list of files.
	// The assigning of plate ids will happen in 'accept()' if the user
	// pressed 'OK'.
	exec();
}


bool
GPlatesQtWidgets::AssignReconstructionPlateIdsDialog::partition_features()
{
	const boost::optional<GPlatesAppLogic::AssignPlateIds::non_null_ptr_type> plate_id_assigner = create_plate_id_assigner();
	if (!plate_id_assigner)
	{
		// A message box has already been popped up.
		return false;
	}

	// Determine if any partitioning polygons.
	if (!plate_id_assigner.get()->has_partitioning_polygons())
	{
		// Nothing to do if there are no partitioning polygons.
		pop_up_no_partitioning_polygons_found_message_box();
		return false;
	}

	if (!partition_features(*plate_id_assigner.get()))
	{
		return false;
	}

	return true;
}


boost::optional<GPlatesAppLogic::AssignPlateIds::non_null_ptr_type>
GPlatesQtWidgets::AssignReconstructionPlateIdsDialog::create_plate_id_assigner()
{
	// Get the partitioning polygon layer proxy.
	const boost::optional<layer_ptr_type> partitioning_layer = get_selected_layer(d_partitioning_layer_state_seq);
	if (!partitioning_layer)
	{
		// Either there are no partitioning layers to choose from or the user hasn't selected one.
		pop_up_no_partitioning_layers_found_or_selected_message_box();
		return boost::none;
	}

	boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_partitioning_layer = partitioning_layer->lock();
	const boost::optional<GPlatesAppLogic::LayerProxy::non_null_ptr_type> partitioning_layer_proxy_opt =
			locked_partitioning_layer->get_reconstruct_graph_layer().get_layer_output();
	if (!partitioning_layer_proxy_opt)
	{
		// NOTE: We should probably assert here since this shouldn't happen since the user is only
		// presented with *active* layers to choose from but let's get the user to try again in case
		// somehow the layer was deactivated while this dialog is active.
		pop_up_no_partitioning_layers_found_or_selected_message_box();
		return boost::none;
	}
	const GPlatesAppLogic::LayerProxy::non_null_ptr_type partitioning_layer_proxy = partitioning_layer_proxy_opt.get();

	double reconstruction_time = 0;

	// Determine which reconstruction time to use.
	switch (d_reconstruction_time_type)
	{
	case CURRENT_RECONSTRUCTION_TIME:
		// The user wants the current reconstruction time so just use the
		// current reconstruction.
		reconstruction_time = d_application_state.get_current_reconstruction_time();
		break;

	case USER_SPECIFIED_RECONSTRUCTION_TIME:
		// Use the reconstruction time specified by the user.
		reconstruction_time = d_spin_box_reconstruction_time;
		break;

	case PRESENT_DAY_RECONSTRUCTION_TIME:
	default:
		// Use the present day reconstruction time.
		reconstruction_time = 0;
		break;
	}

	// Determine which feature property types to copy from partitioning polygon.
	GPlatesAppLogic::AssignPlateIds::feature_property_flags_type feature_property_types_to_assign;
	if (d_assign_plate_ids)
	{
		feature_property_types_to_assign.set(GPlatesAppLogic::AssignPlateIds::RECONSTRUCTION_PLATE_ID);
	}
	if (d_assign_time_periods)
	{
		feature_property_types_to_assign.set(GPlatesAppLogic::AssignPlateIds::VALID_TIME);
	}

	return GPlatesAppLogic::AssignPlateIds::create(
			d_assign_plate_id_method,
			partitioning_layer_proxy,
			reconstruction_time,
			feature_property_types_to_assign);
}


bool
GPlatesQtWidgets::AssignReconstructionPlateIdsDialog::partition_features(
		GPlatesAppLogic::AssignPlateIds &plate_id_assigner)
{
	const feature_collection_seq_type selected_partitioned_feature_collections =
			get_selected_feature_collections(d_partitioned_file_state_seq);

	if (selected_partitioned_feature_collections.empty())
	{
		// No files were selected so notify the user and return without closing this dialog.
		pop_up_no_partitioned_files_selected_message_box();
		return false;
	}

	// Determine the number of features we are going to partition.
	const GPlatesModel::container_size_type num_features_to_partition =
			get_num_features(selected_partitioned_feature_collections);

	// Setup a progress dialog.
	ProgressDialog *partition_progress_dialog = new ProgressDialog(this);
	const QString progress_dialog_text("Partitioning features...");
	GPlatesModel::container_size_type num_features_partitioned = 0;
	// Make progress dialog modal so cannot interact with assign plate ids dialog
	// until processing finished or cancel button pressed.
	partition_progress_dialog->setWindowModality(Qt::WindowModal);
	partition_progress_dialog->setRange(0, num_features_to_partition);
	partition_progress_dialog->setValue(0);
	partition_progress_dialog->show();

	// Iterate through the partitioned feature collections accepted by the user.
	feature_collection_seq_type::const_iterator feature_collection_iter =
			selected_partitioned_feature_collections.begin();
	feature_collection_seq_type::const_iterator feature_collection_end =
			selected_partitioned_feature_collections.end();
	for ( ; feature_collection_iter != feature_collection_end; ++feature_collection_iter)
	{
		const GPlatesModel::FeatureCollectionHandle::weak_ref feature_collection_ref =
				*feature_collection_iter;

		// Iterate over the features in the current feature collection.
		GPlatesModel::FeatureCollectionHandle::iterator feature_iter = feature_collection_ref->begin();
		const GPlatesModel::FeatureCollectionHandle::iterator feature_end = feature_collection_ref->end();
		for ( ; feature_iter != feature_end; ++feature_iter)
		{
			const GPlatesModel::FeatureHandle::weak_ref &feature_ref = (*feature_iter)->reference();

			partition_progress_dialog->update_progress(
					num_features_partitioned,
					progress_dialog_text);

			// Partition the feature.
			plate_id_assigner.assign_reconstruction_plate_id(
					feature_ref,
					feature_collection_ref);

			++num_features_partitioned;

			// See if feature is the focused feature.
			if ((*feature_iter).get() == d_feature_focus.focused_feature().handle_ptr())
			{
				d_feature_focus.announce_modification_of_focused_feature();
			}

			if (partition_progress_dialog->canceled())
			{
				partition_progress_dialog->close();

				// Reconstruct since we most likely modified a few features before
				// the user pressed "Cancel".
				d_application_state.reconstruct();

				// Return without closing this dialog (the assign plate id dialog).
				return false;
			}
		}
	}

	partition_progress_dialog->close();

	// If any plate ids were assigned then we need to do another reconstruction.
	// Note we'll do one anyway since the feature may have been modified even if
	// a plate id was not assigned (such as removing an existing plate id).
	d_application_state.reconstruct();

	// Let the caller know it can close this dialog since files were selected.
	return true;
}


void
GPlatesQtWidgets::AssignReconstructionPlateIdsDialog::pop_up_no_partitioning_layers_found_or_selected_message_box()
{
	const QString message = tr(
			"There were no partitioning polygon layers or none were selected.");
	QMessageBox::warning(
			this,
			tr("No partitioning polygon layers found or selected"),
			message,
			QMessageBox::Ok,
			QMessageBox::Ok);
}


void
GPlatesQtWidgets::AssignReconstructionPlateIdsDialog::pop_up_no_partitioning_polygons_found_message_box()
{
	const QString message = tr(
			"Did not find any polygons (static or topological) in the selected partitioning layer.\n\n"
			"Please select a layer that generates static or topological polygons.");
	QMessageBox::warning(
			this,
			tr("No partitioning polygons found"),
			message,
			QMessageBox::Ok,
			QMessageBox::Ok);
}


void
GPlatesQtWidgets::AssignReconstructionPlateIdsDialog::pop_up_no_partitioned_files_selected_message_box()
{
	const QString message = tr("Please select one or more feature collections to be partitioned.");
	QMessageBox::information(this, tr("No features for partitioning"), message,
			QMessageBox::Ok, QMessageBox::Ok);
}


GPlatesQtWidgets::AssignReconstructionPlateIdsDialog::file_ptr_seq_type
GPlatesQtWidgets::AssignReconstructionPlateIdsDialog::get_loaded_files()
{
	//
	// Get a list of all loaded files.
	//
	file_ptr_seq_type loaded_files;

	const std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference> loaded_file_refs =
			d_feature_collection_file_state.get_loaded_files();
	BOOST_FOREACH(
			const GPlatesAppLogic::FeatureCollectionFileState::file_reference &loaded_file_ref,
			loaded_file_refs)
	{
		const GPlatesFileIO::File::Reference &loaded_file = loaded_file_ref.get_file();
		loaded_files.push_back(&loaded_file);
	}

	return loaded_files;
}


GPlatesQtWidgets::AssignReconstructionPlateIdsDialog::layer_ptr_seq_type
GPlatesQtWidgets::AssignReconstructionPlateIdsDialog::get_possible_partitioning_layers()
{
	//
	// Get a list of all layers that can be used to partition.
	//
	layer_ptr_seq_type possible_partitioning_layers;

	// Iterate over the visual layers.
	for (size_t i = d_visual_layers.size(); i != 0; --i)
	{
		boost::weak_ptr<GPlatesPresentation::VisualLayer> visual_layer = d_visual_layers.visual_layer_at(i - 1);
		if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = visual_layer.lock())
		{
			// Get the app-logic layer.
			const GPlatesAppLogic::Layer &layer = locked_visual_layer->get_reconstruct_graph_layer();

			// We only want active layers.
			if (!layer.is_active())
			{
				continue;
			}

			// If the layer type can generate topological plate polygons or reconstruction geometries
			// (the latter may contain static polygons) then add it to the list of possible layers.
			if (layer.get_type() == GPlatesAppLogic::LayerTaskType::TOPOLOGY_BOUNDARY_RESOLVER ||
				layer.get_type() == GPlatesAppLogic::LayerTaskType::RECONSTRUCT)
			{
				possible_partitioning_layers.push_back(visual_layer);
			}
		}
	}

	return possible_partitioning_layers;
}


GPlatesQtWidgets::AssignReconstructionPlateIdsDialog::feature_collection_seq_type
GPlatesQtWidgets::AssignReconstructionPlateIdsDialog::get_selected_feature_collections(
		FileStateCollection &file_state_collection)
{
	feature_collection_seq_type selected_feature_collections;

	// Iterate through the files accepted by the user.
	file_state_seq_type::const_iterator file_state_iter =
			file_state_collection.file_state_seq.begin();
	file_state_seq_type::const_iterator file_state_end =
			file_state_collection.file_state_seq.end();
	for ( ; file_state_iter != file_state_end; ++file_state_iter)
	{
		const FileState &file_state = *file_state_iter;

		if (file_state.enabled)
		{
			selected_feature_collections.push_back(
					file_state.file->get_feature_collection());
		}
	}

	return selected_feature_collections;
}


boost::optional<GPlatesQtWidgets::AssignReconstructionPlateIdsDialog::layer_ptr_type>
GPlatesQtWidgets::AssignReconstructionPlateIdsDialog::get_selected_layer(
		LayerStateCollection &layer_state_collection)
{
	const int checked_button_id = d_partitioning_layer_state_seq.layer_selection_group->checkedId();
	if (checked_button_id < 0) // No button is selected...
	{
		// Either there are no possible partitioning layers or the user has not yet selected any layer.
		return boost::none;
	}
	const unsigned int checked_button_row = checked_button_id;

	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			checked_button_row < d_partitioning_layer_state_seq.layer_state_seq.size(),
			GPLATES_ASSERTION_SOURCE);

	// Return the currently selected partitioning layer.
	return d_partitioning_layer_state_seq.layer_state_seq[checked_button_row].layer;
}


void
GPlatesQtWidgets::AssignReconstructionPlateIdsDialog::set_up_button_box()
{
	// Default 'OK' button should read 'Apply'.
	d_button_create = buttonbox->addButton(tr("Apply"), QDialogButtonBox::AcceptRole);
	d_button_create->setDefault(true);
	
	QObject::connect(buttonbox, SIGNAL(accepted()),
			this, SLOT(apply()));
	QObject::connect(buttonbox, SIGNAL(rejected()),
			this, SLOT(reject()));

	// Extra buttons for switching between the pages.
	QObject::connect(button_prev, SIGNAL(clicked()),
			this, SLOT(handle_prev()));
	QObject::connect(button_next, SIGNAL(clicked()),
			this, SLOT(handle_next()));
}


void
GPlatesQtWidgets::AssignReconstructionPlateIdsDialog::set_up_partitioning_layers_page()
{
	// Connect the help dialogs.
	QObject::connect(push_button_help_partitioning_layers, SIGNAL(clicked()),
			d_help_partitioning_layers_dialog, SLOT(show()));

	// Try to adjust column widths.
	QHeaderView *header = table_partitioning_layers->horizontalHeader();
	header->setResizeMode(LAYER_NAME_COLUMN, QHeaderView::Stretch);
	header->setResizeMode(ENABLE_LAYER_COLUMN, QHeaderView::ResizeToContents);
}


void
GPlatesQtWidgets::AssignReconstructionPlateIdsDialog::set_up_partitioned_files_page()
{
	// Connect the help dialogs.
	QObject::connect(push_button_help_partitioned_files, SIGNAL(clicked()),
			d_help_partitioned_files_dialog, SLOT(show()));

	// Listen for changes to the checkbox that enables/disables files.
	QObject::connect(table_partitioned_files, SIGNAL(cellChanged(int, int)),
			this, SLOT(react_cell_changed_partitioned_files(int, int)));
	QObject::connect(button_clear_all_partitioned_files, SIGNAL(clicked()),
			this, SLOT(react_clear_all_partitioned_files()));
	QObject::connect(button_select_all_partitioned_files, SIGNAL(clicked()),
			this, SLOT(react_select_all_partitioned_files()));

	// Try to adjust column widths.
	QHeaderView *header = table_partitioned_files->horizontalHeader();
	header->setResizeMode(FILENAME_COLUMN, QHeaderView::Stretch);
	header->setResizeMode(ENABLE_FILE_COLUMN, QHeaderView::ResizeToContents);
}


void
GPlatesQtWidgets::AssignReconstructionPlateIdsDialog::set_up_general_options_page()
{
	// Connect the help dialogs.
	QObject::connect(push_button_help_reconstruction_time, SIGNAL(clicked()),
			d_help_reconstruction_time_dialog, SLOT(show()));
	QObject::connect(push_button_help_partitions_options, SIGNAL(clicked()),
			d_help_partition_options_dialog, SLOT(show()));
	QObject::connect(push_button_help_properties_to_assign, SIGNAL(clicked()),
			d_help_properties_to_assign_dialog, SLOT(show()));

	// Listen for reconstruction time radio button selections.
	QObject::connect(radio_button_present_day, SIGNAL(toggled(bool)),
			this, SLOT(react_reconstruction_time_radio_button(bool)));
	QObject::connect(radio_button_current_recon_time, SIGNAL(toggled(bool)),
			this, SLOT(react_reconstruction_time_radio_button(bool)));
	QObject::connect(radio_button_specify_recon_time, SIGNAL(toggled(bool)),
			this, SLOT(react_reconstruction_time_radio_button(bool)));

	// Listen for reconstruction time changes in the double spin box.
	QObject::connect(double_spin_box_reconstruction_time, SIGNAL(valueChanged(double)),
			this, SLOT(react_spin_box_reconstruction_time_changed(double)));

	// Listen for partition options radio button selections.
	QObject::connect(radio_button_assign_features, SIGNAL(toggled(bool)),
			this, SLOT(react_partition_options_radio_button(bool)));
	QObject::connect(radio_button_assign_feature_sub_geometries, SIGNAL(toggled(bool)),
			this, SLOT(react_partition_options_radio_button(bool)));
	QObject::connect(radio_button_partition_features, SIGNAL(toggled(bool)),
			this, SLOT(react_partition_options_radio_button(bool)));

	// Listen for feature properties radio button selections.
	QObject::connect(check_box_assign_plate_id, SIGNAL(toggled(bool)),
			this, SLOT(react_feature_properties_options_radio_button(bool)));
	QObject::connect(check_box_assign_time_period, SIGNAL(toggled(bool)),
			this, SLOT(react_feature_properties_options_radio_button(bool)));

	// Set the initial reconstruction time for the double spin box.
	double_spin_box_reconstruction_time->setValue(d_spin_box_reconstruction_time);

	// Set the default radio button to be present day reconstruction time.
	// This will also disable the reconstruction time spin box.
	radio_button_present_day->setChecked(true);

	// Set the default radio button to assign each feature to the plate id
	// it overlaps the most.
	radio_button_assign_features->setChecked(true);

	// Copy plate ids from partitioning polygon?
	check_box_assign_plate_id->setChecked(d_assign_plate_ids);
	// Copy time periods from partitioning polygon?
	check_box_assign_time_period->setChecked(d_assign_time_periods);
}


void
GPlatesQtWidgets::AssignReconstructionPlateIdsDialog::handle_prev()
{
	const int prev_index = stack_widget->currentIndex() - 1;
	if (prev_index >= 0)
	{
		stack_widget->setCurrentIndex(prev_index);
	}
}


void
GPlatesQtWidgets::AssignReconstructionPlateIdsDialog::handle_next()
{
	const int next_index = stack_widget->currentIndex() + 1;
	if (next_index < stack_widget->count())
	{
		stack_widget->setCurrentIndex(next_index);
	}
}


void
GPlatesQtWidgets::AssignReconstructionPlateIdsDialog::handle_page_change(
		int page)
{
	// Enable all buttons and then disable buttons appropriately.
	button_prev->setEnabled(true);
	button_next->setEnabled(true);
	d_button_create->setEnabled(true);
	
	// Disable buttons which are not valid for the page,
	// and focus the first widget.
	switch (page)
	{
	case 0:
			partitioning_files->setFocus();
			button_prev->setEnabled(false);
			d_button_create->setEnabled(false);
			break;

	case 1:
			partitioned_files->setFocus();
			d_button_create->setEnabled(false);
			break;

	case 2:
			general_options->setFocus();
			button_next->setEnabled(false);
			break;

	default:
		break;
	}
}


void
GPlatesQtWidgets::AssignReconstructionPlateIdsDialog::initialise_file_list(
		FileStateCollection &file_state_collection,
		const file_ptr_seq_type &files)
{
	clear_file_rows(file_state_collection);

	for (file_ptr_seq_type::const_iterator file_iter = files.begin();
		file_iter != files.end();
		++file_iter)
	{
		add_file_row(file_state_collection, **file_iter);
	}
}


void
GPlatesQtWidgets::AssignReconstructionPlateIdsDialog::clear_file_rows(
		FileStateCollection &file_state_collection)
{
	file_state_collection.table_widget->clearContents(); // Do not clear the header items as well.
	file_state_collection.table_widget->setRowCount(0);  // Do remove the newly blanked rows.
}


void
GPlatesQtWidgets::AssignReconstructionPlateIdsDialog::add_file_row(
		FileStateCollection &file_state_collection,
		const GPlatesFileIO::File::Reference &file)
{
	const GPlatesFileIO::FileInfo &file_info = file.get_file_info();

	// Obtain information from the FileInfo
	const QFileInfo &qfileinfo = file_info.get_qfileinfo();

	// Some files might not actually exist yet if the user created a new
	// feature collection internally and hasn't saved it to file yet.
	QString display_name;
	if (GPlatesFileIO::file_exists(file_info))
	{
		display_name = file_info.get_display_name(false);
	}
	else
	{
		// The file doesn't exist so give it a filename to indicate this.
		display_name = "New Feature Collection";
	}
	
	const QString filepath_str = qfileinfo.path();
	
	// The rows in the QTableWidget and our internal file sequence should be in sync.
	const int row = file_state_collection.table_widget->rowCount();
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			row == boost::numeric_cast<int>(file_state_collection.file_state_seq.size()),
			GPLATES_ASSERTION_SOURCE);

	// Add a row.
	file_state_collection.table_widget->insertRow(row);
	file_state_collection.file_state_seq.push_back(FileState(file));
	const FileState &row_file_state = file_state_collection.file_state_seq.back();
	
	// Add filename item.
	QTableWidgetItem *filename_item = new QTableWidgetItem(display_name);
	filename_item->setToolTip(tr("Location: %1").arg(filepath_str));
	filename_item->setFlags(Qt::ItemIsEnabled);
	file_state_collection.table_widget->setItem(row, FILENAME_COLUMN, filename_item);

	// Add checkbox item to enable/disable the layer.
	QTableWidgetItem *file_enabled_item = new QTableWidgetItem();
	file_enabled_item->setToolTip(tr("Select to enable layer for partitioning"));
	file_enabled_item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
	file_enabled_item->setCheckState(row_file_state.enabled ? Qt::Checked : Qt::Unchecked);
	file_state_collection.table_widget->setItem(row, ENABLE_FILE_COLUMN, file_enabled_item);
}


void
GPlatesQtWidgets::AssignReconstructionPlateIdsDialog::initialise_layer_list(
		LayerStateCollection &layer_state_collection,
		const layer_ptr_seq_type &layers)
{
	clear_layer_rows(layer_state_collection);

	for (layer_ptr_seq_type::const_iterator layer_iter = layers.begin();
		layer_iter != layers.end();
		++layer_iter)
	{
		add_layer_row(layer_state_collection, *layer_iter);
	}
}


void
GPlatesQtWidgets::AssignReconstructionPlateIdsDialog::clear_layer_rows(
		LayerStateCollection &layer_state_collection)
{
	layer_state_collection.table_widget->clearContents(); // Do not clear the header items as well.
	layer_state_collection.table_widget->setRowCount(0);  // Do remove the newly blanked rows.
}


void
GPlatesQtWidgets::AssignReconstructionPlateIdsDialog::add_layer_row(
		LayerStateCollection &layer_state_collection,
		const layer_ptr_type &visual_layer)
{
	boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = visual_layer.lock();
	if (!locked_visual_layer)
	{
		return;
	}

	QString layer_name = locked_visual_layer->get_name();
	
	// The rows in the QTableWidget and our internal layer sequence should be in sync.
	const int row = layer_state_collection.table_widget->rowCount();
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			row == boost::numeric_cast<int>(layer_state_collection.layer_state_seq.size()),
			GPLATES_ASSERTION_SOURCE);

	// Add a row.
	layer_state_collection.table_widget->insertRow(row);
	layer_state_collection.layer_state_seq.push_back(LayerState(visual_layer));
	const LayerState &row_layer_state = layer_state_collection.layer_state_seq.back();
	
	// Add layer name item.
	QTableWidgetItem *layer_name_item = new QTableWidgetItem(layer_name);
	layer_name_item->setFlags(Qt::ItemIsEnabled);
	layer_state_collection.table_widget->setItem(row, LAYER_NAME_COLUMN, layer_name_item);

	// Add radio button to select the layer.
	QRadioButton* layer_selection_button = new QRadioButton(layer_state_collection.table_widget);
	layer_selection_button->setToolTip(tr("Select the layer for partitioning"));
	// Add to button group so we can identify which row it's in when user selects it.
	d_partitioning_layer_state_seq.layer_selection_group->addButton(layer_selection_button);
	d_partitioning_layer_state_seq.layer_selection_group->setId(layer_selection_button, row);
	// Set radio button in appropriate row/column of table widget.
	layer_state_collection.table_widget->setCellWidget(
			row,
			ENABLE_LAYER_COLUMN,
			layer_selection_button);
}


void
GPlatesQtWidgets::AssignReconstructionPlateIdsDialog::clear()
{
	clear_layer_rows(d_partitioning_layer_state_seq);
	clear_file_rows(d_partitioned_file_state_seq);

	d_partitioning_layer_state_seq.layer_state_seq.clear();
	d_partitioned_file_state_seq.file_state_seq.clear();
}


void
GPlatesQtWidgets::AssignReconstructionPlateIdsDialog::react_cell_changed_partitioned_files(
		int row,
		int column)
{
	if (row < 0 ||
			boost::numeric_cast<file_state_seq_type::size_type>(row) >
					d_partitioned_file_state_seq.file_state_seq.size())
	{
		return;
	}

	// It should be the enable layer checkbox column as that's the only
	// cell that's editable.
	if (column != ENABLE_FILE_COLUMN)
	{
		return;
	}

	// Set the enable flag in our internal layer sequence.
	d_partitioned_file_state_seq.file_state_seq[row].enabled =
			d_partitioned_file_state_seq.table_widget->item(row, column)->checkState() == Qt::Checked;
}


void
GPlatesQtWidgets::AssignReconstructionPlateIdsDialog::react_clear_all_partitioned_files()
{
	for (int row = 0; row < d_partitioned_file_state_seq.table_widget->rowCount(); ++row)
	{
		d_partitioned_file_state_seq.table_widget->item(row, ENABLE_FILE_COLUMN)
				->setCheckState(Qt::Unchecked);
	}
}


void
GPlatesQtWidgets::AssignReconstructionPlateIdsDialog::react_select_all_partitioned_files()
{
	for (int row = 0; row < d_partitioned_file_state_seq.table_widget->rowCount(); ++row)
	{
		d_partitioned_file_state_seq.table_widget->item(row, ENABLE_FILE_COLUMN)
				->setCheckState(Qt::Checked);
	}
}


void
GPlatesQtWidgets::AssignReconstructionPlateIdsDialog::react_reconstruction_time_radio_button(
		bool checked)
{
	if (radio_button_present_day->isChecked())
	{
		d_reconstruction_time_type = PRESENT_DAY_RECONSTRUCTION_TIME;
	}

	if (radio_button_current_recon_time->isChecked())
	{
		d_reconstruction_time_type = CURRENT_RECONSTRUCTION_TIME;
	}

	if (radio_button_specify_recon_time->isChecked())
	{
		d_reconstruction_time_type = USER_SPECIFIED_RECONSTRUCTION_TIME;
		double_spin_box_reconstruction_time->setEnabled(true);
	}
	else
	{
		// Disable spin box.
		double_spin_box_reconstruction_time->setEnabled(false);
	}
}


void
GPlatesQtWidgets::AssignReconstructionPlateIdsDialog::react_spin_box_reconstruction_time_changed(
		double reconstruction_time)
{
	d_spin_box_reconstruction_time = reconstruction_time;
}


void
GPlatesQtWidgets::AssignReconstructionPlateIdsDialog::react_partition_options_radio_button(
		bool checked)
{
	if (!checked)
	{
		return;
	}

	if (radio_button_assign_features->isChecked())
	{
		d_assign_plate_id_method =
				GPlatesAppLogic::AssignPlateIds::ASSIGN_FEATURE_TO_MOST_OVERLAPPING_PLATE;
	}

	if (radio_button_assign_feature_sub_geometries->isChecked())
	{
		d_assign_plate_id_method =
				GPlatesAppLogic::AssignPlateIds::ASSIGN_FEATURE_SUB_GEOMETRY_TO_MOST_OVERLAPPING_PLATE;
	}

	if (radio_button_partition_features->isChecked())
	{
		d_assign_plate_id_method =
				GPlatesAppLogic::AssignPlateIds::PARTITION_FEATURE;
	}
}


void
GPlatesQtWidgets::AssignReconstructionPlateIdsDialog::react_feature_properties_options_radio_button(
		bool checked)
{
	d_assign_plate_ids = check_box_assign_plate_id->isChecked();
	d_assign_time_periods = check_box_assign_time_period->isChecked();
}


void
GPlatesQtWidgets::AssignReconstructionPlateIdsDialog::apply()
{
	if (!partition_features())
	{
		// Return early and don't close dialog.
		// This allows user to correct a mistake.
		// Use still has option of pressing "Cancel".
		return;
	}

	clear();

	done(QDialog::Accepted);
}


void
GPlatesQtWidgets::AssignReconstructionPlateIdsDialog::reject()
{
	clear();

	done(QDialog::Rejected);
}
