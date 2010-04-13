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

#include <vector>
#include <boost/cast.hpp>
#include <QHeaderView>
#include <QMessageBox>
#include <QString>

#include "AssignReconstructionPlateIdsDialog.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/AssignPlateIds.h"
#include "app-logic/FeatureCollectionFileState.h"
#include "app-logic/Reconstruct.h"
#include "app-logic/TopologyUtils.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "gui/FeatureFocus.h"

#include "presentation/ViewState.h"


namespace
{
	const QString HELP_FEATURE_COLLECTIONS_DIALOG_TITLE = QObject::tr("Selecting feature collections");
	const QString HELP_FEATURE_COLLECTIONS_DIALOG_TEXT = QObject::tr(
			"<html><body>\n"
			"<h3>Select the feature collections that will be assigned reconstruction plate ids</h3>"
			"<p>All selected feature collections will have their features assigned a "
			"reconstruction plate id (if they already have one it will be overwritten).</p>"
			"<p>The plate ids are assigned using the currently loaded and active plate boundary "
			"feature collections (these are the ones containing TopologicalClosedPlateBoundary "
			"features).</p>"
			"<p>Only the feature collections that are set as active in the "
			"Manage Feature Collections dialog will be available for selection in this dialog - "
			"if a feature collection appears to be missing try activating it in the "
			"Manage Feature Collections dialog first.</p>" 
			"<p><em>If this dialog was activated automatically when loading or reloading a "
			"feature collection then that feature collection contains one or more features "
			"that have no reconstruction plate id.</em></p>"
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
			"<p>The plate boundaries are rotated to the selected reconstruction time before "
			"testing for overlap/intersection with the feature collections.</p>"
			"<p>The geometry in the feature collections effectively represents a snapshot "
			"of the features at the reconstruction time.</p>"
			"<ul>"
			"<li>For features with an existing reconstruction plate id this will be "
			"the rotated geometry at the selected reconstruction time.</li>"
			"<li>For features with no existing reconstruction plate id this will be "
			"the unrotated geometry stored in the feature.</li>"
			"</ul>"
			"</body></html>\n");
	const QString HELP_ASSIGN_PLATE_ID_DIALOG_TITLE = QObject::tr("Assign plate id options");
	const QString HELP_ASSIGN_PLATE_ID_DIALOG_TEXT = QObject::tr(
			"<html><body>\n"
			"<h3>Specify how to assign plate ids to features</h3>"
			"These three options determine how plate ids are assigned and "
			"whether feature geometry will be subdivided or not."
			"<h4>Assign each feature to the plate it overlaps the most:</h4>\n"
			"<ul>\n"
			"<li>Assign, to each feature, a single plate id corresponding to the "
			"plate that the feature's geometry overlaps the most.</li>\n"
			"<li>If a feature contains multiple sub-geometries then they are treated as "
			"one composite geometry for the purpose of this test.</li>\n"
			"</ul>\n"
			"<h4>Assign each feature sub-geometry to the plate it overlaps the most:</h4>\n"
			"<ul>\n"
			"<li>Assign, to each sub-geometry of each feature, a single plate id "
			"corresponding to the plate that the sub-geometry overlaps the most.</li>\n"
			"<li>This can create extra features, for example if a feature has two "
			"sub-geometries and one overlaps plate A the most and the other "
			"overlaps plate B the most then the original feature (with the two "
			"geometries) will get split into two features - one feature will contain "
			"the first geometry (and have plate id A) and the other feature will "
			"contain the second geometry (and have plate id B).</li>\n"
			"</ul>\n"
			"<h4>Partition each feature into its containing plates:</h4>\n"
			"<ul>\n"
			"<li>Partition all geometries of each feature into the plates "
			"containing them (intersecting them as needed and assigning the "
			"resulting partitioned geometry to the appropriate plates).</li>\n"
			"<li>This can create extra features, for example if a feature has only one "
			"sub-geometry but it overlaps plate A and plate B then it is partitioned "
			"into geometry that is fully contained by plate A and likewise for plate B.  "
			"These two partitioned geometries will now be two features since they "
			"have different plate ids.</li>\n"
			"</ul>\n"
			"<p>If the plate boundaries do not cover the entire surface of the globe then it is "
			"possible for some features (or partitioned geometries) to fall outside "
			"all plate boundaries. In this situation the feature (or partitioned geometry) "
			"is not assigned a plate id and any existing plate id is removed. "
			"These features will all have the same colour and will not move when the "
			"reconstruction time is changed or animated.</p>"
			"<p><em><b>VirtualGeomagneticPole</b> features are treated differently - these "
			"features are assigned to the plate whose boundary contains the feature's "
			"sample site point location. For these features the above options are ignored.</em></p>"
			"</body></html>\n");
}


GPlatesQtWidgets::AssignReconstructionPlateIdsDialog::AssignReconstructionPlateIdsDialog(
		GPlatesAppLogic::ApplicationState &application_state,
		GPlatesPresentation::ViewState &view_state,
		QWidget *parent_):
	QDialog(parent_, Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::MSWindowsFixedSizeDialogHint),
	d_help_feature_collections_dialog(
			new InformationDialog(
					HELP_FEATURE_COLLECTIONS_DIALOG_TEXT,
					HELP_FEATURE_COLLECTIONS_DIALOG_TITLE,
					this)),
	d_help_reconstruction_time_dialog(
			new InformationDialog(
					HELP_RECONSTRUCTION_TIME_DIALOG_TEXT,
					HELP_RECONSTRUCTION_TIME_DIALOG_TITLE,
					this)),
	d_help_assign_plate_id_options_dialog(
			new InformationDialog(
					HELP_ASSIGN_PLATE_ID_DIALOG_TEXT,
					HELP_ASSIGN_PLATE_ID_DIALOG_TITLE,
					this)),
	d_model(application_state.get_model_interface()),
	d_feature_collection_file_state(application_state.get_feature_collection_file_state()),
	d_reconstruct(view_state.get_reconstruct()),
	d_feature_focus(view_state.get_feature_focus()),
	d_only_assign_to_features_with_no_reconstruction_plate_id(false),
	d_reconstruction_time_type(PRESENT_DAY_RECONSTRUCTION_TIME),
	d_spin_box_reconstruction_time(0),
	d_assign_plate_id_method(
			GPlatesAppLogic::AssignPlateIds::ASSIGN_FEATURE_TO_MOST_OVERLAPPING_PLATE)
{
	setupUi(this);

	// Connect the help dialogs.
	QObject::connect(push_button_help_feature_collections, SIGNAL(clicked()),
			d_help_feature_collections_dialog, SLOT(show()));
	QObject::connect(push_button_help_reconstruction_time, SIGNAL(clicked()),
			d_help_reconstruction_time_dialog, SLOT(show()));
	QObject::connect(push_button_help_assign_plate_id_options, SIGNAL(clicked()),
			d_help_assign_plate_id_options_dialog, SLOT(show()));

	// Connect the 'OK' button to the apply slot.
	QObject::connect(button_apply, SIGNAL(clicked()),
			this, SLOT(apply()));

	// Listen for changes to the checkbox that enables/disables files.
	QObject::connect(table_feature_collections, SIGNAL(cellChanged(int, int)),
			this, SLOT(react_cell_changed(int, int)));
	QObject::connect(button_clear_all, SIGNAL(clicked()),
			this, SLOT(react_clear_all()));
	QObject::connect(button_select_all, SIGNAL(clicked()),
			this, SLOT(react_select_all()));

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

	// Listen for assign plate id options radio button selections.
	QObject::connect(radio_button_assign_features, SIGNAL(toggled(bool)),
			this, SLOT(react_assign_plate_id_options_radio_button(bool)));
	QObject::connect(radio_button_assign_feature_sub_geometries, SIGNAL(toggled(bool)),
			this, SLOT(react_assign_plate_id_options_radio_button(bool)));
	QObject::connect(radio_button_partition_features, SIGNAL(toggled(bool)),
			this, SLOT(react_assign_plate_id_options_radio_button(bool)));

	// Try to adjust column widths.
	QHeaderView *header = table_feature_collections->horizontalHeader();
	header->setResizeMode(FILENAME_COLUMN, QHeaderView::Stretch);
	header->setResizeMode(ENABLE_FILE_COLUMN, QHeaderView::ResizeToContents);

	// Set the initial reconstruction time for the double spin box.
	double_spin_box_reconstruction_time->setValue(d_spin_box_reconstruction_time);

	// Set the default radio button to be present day reconstruction time.
	// This will also disable the reconstruction time spin box.
	radio_button_present_day->setChecked(true);

	// Set the default radio button to assign each feature to the plate id
	// it overlaps the most.
	radio_button_assign_features->setChecked(true);
}


void
GPlatesQtWidgets::AssignReconstructionPlateIdsDialog::assign_plate_ids_to_newly_loaded_feature_collections(
		const file_seq_type &newly_loaded_feature_collection_files,
		bool pop_up_message_box_if_no_plate_boundaries)
{
	if (!check_for_plate_boundaries(pop_up_message_box_if_no_plate_boundaries))
	{
		return;
	}

	d_only_assign_to_features_with_no_reconstruction_plate_id = true;

	file_ptr_seq_type files_to_assign_plate_ids;
	get_files_to_assign_plate_ids(
			files_to_assign_plate_ids,
			newly_loaded_feature_collection_files);

	query_user_and_assign_plate_ids(files_to_assign_plate_ids);
}


void
GPlatesQtWidgets::AssignReconstructionPlateIdsDialog::reassign_reconstruction_plate_ids(
		bool pop_up_message_box_if_no_plate_boundaries)
{
	if (!check_for_plate_boundaries(pop_up_message_box_if_no_plate_boundaries))
	{
		return;
	}

	d_only_assign_to_features_with_no_reconstruction_plate_id = false;

	file_ptr_seq_type files_to_assign_plate_ids;
	get_files_to_reassign_plate_ids(files_to_assign_plate_ids);

	query_user_and_assign_plate_ids(files_to_assign_plate_ids);
}


bool
GPlatesQtWidgets::AssignReconstructionPlateIdsDialog::check_for_plate_boundaries(
			bool pop_up_message_box_if_no_plate_boundaries)
{
	// Determine if any resolved topological boundaries.
	if (!GPlatesAppLogic::TopologyUtils::has_resolved_topological_boundaries(
			d_reconstruct.get_current_reconstruction()))
	{
		// Nothing to do if there are no resolved topological boundaries because
		// cannot test geometry against plate boundaries.
		if (pop_up_message_box_if_no_plate_boundaries)
		{
			pop_up_no_plate_boundaries_message_box();
		}
		return false;
	}

	return true;
}


void
GPlatesQtWidgets::AssignReconstructionPlateIdsDialog::pop_up_no_plate_boundaries_message_box()
{
	const QString message = tr("GPlates cannot assign plate ids without plate boundary features.\n\n"
			"Please load feature collection(s) containing "
			"TopologicalClosedPlateBoundary features.");
	QMessageBox::warning(this, tr("No plate boundary features loaded"), message,
			QMessageBox::Ok, QMessageBox::Ok);
}


void
GPlatesQtWidgets::AssignReconstructionPlateIdsDialog::pop_up_no_files_selected_message_box()
{
	const QString message = tr("Please select one or more feature collections.");
	QMessageBox::information(this, tr("No feature collections selected"), message,
			QMessageBox::Ok, QMessageBox::Ok);
}


void
GPlatesQtWidgets::AssignReconstructionPlateIdsDialog::get_files_to_assign_plate_ids(
		file_ptr_seq_type &files_to_assign_plate_ids,
		const file_seq_type &newly_loaded_feature_collection_files)
{
	//
	// Iterate through the feature collections and find any that contains features that
	// do not have a 'reconstructionPlateId' property.
	// Exclude reconstruction features (used for generating rotations).
	//

	file_seq_type::const_iterator file_seq_iter = newly_loaded_feature_collection_files.begin();
	file_seq_type::const_iterator file_seq_end = newly_loaded_feature_collection_files.end();
	for ( ; file_seq_iter != file_seq_end; ++file_seq_iter)
	{
		const GPlatesFileIO::File::shared_ref &file_ref = *file_seq_iter;

		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection_ref =
				file_ref->get_feature_collection();

		// Find any features in the current feature collection that are not
		// reconstructable features (have no 'reconstructionPlateId' plate id) and
		// are also not reconstruction features.
		std::vector<GPlatesModel::FeatureHandle::weak_ref> features_needing_plate_id;
		if (GPlatesAppLogic::AssignPlateIds::find_reconstructable_features_without_reconstruction_plate_id(
			features_needing_plate_id, feature_collection_ref))
		{
			// We found features in the current feature collection that need plate id assigning.
			files_to_assign_plate_ids.push_back(file_ref.get());
		}
	}
}


void
GPlatesQtWidgets::AssignReconstructionPlateIdsDialog::get_files_to_reassign_plate_ids(
		file_ptr_seq_type &files_to_assign_plate_ids)
{
	//
	// Get a list of all active files excluding reconstruction files.
	// This is the set of feature collections that can possibly make use
	// of a 'gpml:reconstructionPlateId'. We're effectively removing any
	// active files that the user will not want to assign a reconstruction plate id to.
	//

	typedef std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_iterator>
			file_iterator_seq_type;
	file_iterator_seq_type active_files;
	d_feature_collection_file_state.get_active_files(
			active_files,
			d_feature_collection_file_state.get_registered_workflow_tags(),
			true/*include_reconstructable_workflow*/);

	// Convert sequence of file iterators to a sequence of file pointers.
	files_to_assign_plate_ids.reserve(active_files.size());
	file_iterator_seq_type::const_iterator active_files_iter = active_files.begin();
	file_iterator_seq_type::const_iterator active_files_end = active_files.end();
	for ( ; active_files_iter != active_files_end; ++active_files_iter)
	{
		const GPlatesAppLogic::FeatureCollectionFileState::file_iterator &file_iter =
				*active_files_iter;
		GPlatesFileIO::File *active_file = &*file_iter;

		files_to_assign_plate_ids.push_back(active_file);
	}
}


void
GPlatesQtWidgets::AssignReconstructionPlateIdsDialog::query_user_and_assign_plate_ids(
		const file_ptr_seq_type &files_to_assign_plate_ids)
{
	// If there are no files then return early.
	if (files_to_assign_plate_ids.empty())
	{
		return;
	}

	// Setup the file list in the widget.
	initialise_file_list(files_to_assign_plate_ids);

	// Set the current reconstruction time label.
	label_current_reconstruction_time->setText(
			QString::number(d_reconstruct.get_current_reconstruction_time()));

	// Get the user to confirm the list of files.
	// The assigning of plate ids will happen in 'accept()' if the user
	// pressed 'OK'.
	exec();
}


void
GPlatesQtWidgets::AssignReconstructionPlateIdsDialog::initialise_file_list(
		const file_ptr_seq_type &file_list)
{
	clear_rows();
	for (file_ptr_seq_type::const_iterator file_iter = file_list.begin();
		file_iter != file_list.end();
		++file_iter)
	{
		add_row(*file_iter);
	}
}


void
GPlatesQtWidgets::AssignReconstructionPlateIdsDialog::clear_rows()
{
	table_feature_collections->clearContents();	// Do not clear the header items as well.
	table_feature_collections->setRowCount(0);	// Do remove the newly blanked rows.
}


void
GPlatesQtWidgets::AssignReconstructionPlateIdsDialog::add_row(
		GPlatesFileIO::File *file)
{
	const GPlatesFileIO::FileInfo &file_info = file->get_file_info();

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
	const int row = table_feature_collections->rowCount();
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			row == boost::numeric_cast<int>(d_file_state_seq.size()),
			GPLATES_ASSERTION_SOURCE);

	// Add a row.
	table_feature_collections->insertRow(row);
	d_file_state_seq.push_back(FileState(file));
	const FileState &row_file_state = d_file_state_seq.back();
	
	// Add filename item.
	QTableWidgetItem *filename_item = new QTableWidgetItem(display_name);
	filename_item->setToolTip(tr("Location: %1").arg(filepath_str));
	filename_item->setFlags(Qt::ItemIsEnabled);
	table_feature_collections->setItem(row, FILENAME_COLUMN, filename_item);

	// Add checkbox item to enable/disable the file.
	QTableWidgetItem *file_enabled_item = new QTableWidgetItem();
	file_enabled_item->setToolTip(tr("Select to enable file for plate id assignment"));
	file_enabled_item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
	file_enabled_item->setCheckState(row_file_state.enabled ? Qt::Checked : Qt::Unchecked);
	table_feature_collections->setItem(row, ENABLE_FILE_COLUMN, file_enabled_item);
}


void
GPlatesQtWidgets::AssignReconstructionPlateIdsDialog::clear()
{
	clear_rows();
	d_file_state_seq.clear();
}


void
GPlatesQtWidgets::AssignReconstructionPlateIdsDialog::react_cell_changed(
		int row,
		int column)
{
	if (row < 0 ||
			boost::numeric_cast<file_state_seq_type::size_type>(row) > d_file_state_seq.size())
	{
		return;
	}

	// It should be the enable file checkbox column as that's the only
	// cell that's editable.
	if (column != ENABLE_FILE_COLUMN)
	{
		return;
	}

	// Set the enable flag in our internal file sequence.
	d_file_state_seq[row].enabled =
			table_feature_collections->item(row, column)->checkState() == Qt::Checked;
}


void
GPlatesQtWidgets::AssignReconstructionPlateIdsDialog::react_clear_all()
{
	for (int row = 0; row < table_feature_collections->rowCount(); ++row)
	{
		table_feature_collections->item(row, ENABLE_FILE_COLUMN)->setCheckState(Qt::Unchecked);
	}
}


void
GPlatesQtWidgets::AssignReconstructionPlateIdsDialog::react_select_all()
{
	for (int row = 0; row < table_feature_collections->rowCount(); ++row)
	{
		table_feature_collections->item(row, ENABLE_FILE_COLUMN)->setCheckState(Qt::Checked);
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
GPlatesQtWidgets::AssignReconstructionPlateIdsDialog::react_assign_plate_id_options_radio_button(
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
GPlatesQtWidgets::AssignReconstructionPlateIdsDialog::apply()
{
	GPlatesAppLogic::AssignPlateIds::non_null_ptr_type plate_id_assigner =
			create_plate_id_assigner();

	if (!assign_plate_ids(*plate_id_assigner))
	{
		// No files were selected so notify the user and return
		// without closing this dialog.
		pop_up_no_files_selected_message_box();
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


GPlatesAppLogic::AssignPlateIds::non_null_ptr_type
GPlatesQtWidgets::AssignReconstructionPlateIdsDialog::create_plate_id_assigner()
{
	GPlatesAppLogic::AssignPlateIds::non_null_ptr_type plate_id_assigner;

	// Determine which reconstruction time to use.
	switch (d_reconstruction_time_type)
	{
	case PRESENT_DAY_RECONSTRUCTION_TIME:
	default:
		// Use the present day reconstruction time.
		plate_id_assigner =
				GPlatesAppLogic::AssignPlateIds::create_at_new_reconstruction_time(
						d_assign_plate_id_method,
						d_model,
						d_feature_collection_file_state,
						0/*present day*/,
						d_reconstruct.get_current_anchored_plate_id());
		break;

	case CURRENT_RECONSTRUCTION_TIME:
		// The user wants the current reconstruction time so just use the
		// current reconstruction.
		plate_id_assigner =
				GPlatesAppLogic::AssignPlateIds::create_at_current_reconstruction_time(
						d_assign_plate_id_method, d_model, d_reconstruct);
		break;

	case USER_SPECIFIED_RECONSTRUCTION_TIME:
		// Use the reconstruction time specified by the user.
		plate_id_assigner =
				GPlatesAppLogic::AssignPlateIds::create_at_new_reconstruction_time(
						d_assign_plate_id_method,
						d_model,
						d_feature_collection_file_state,
						d_spin_box_reconstruction_time,
						d_reconstruct.get_current_anchored_plate_id());
		break;
	}

	return plate_id_assigner;
}


bool
GPlatesQtWidgets::AssignReconstructionPlateIdsDialog::assign_plate_ids(
		GPlatesAppLogic::AssignPlateIds &plate_id_assigner)
{
	bool assigned_any_plate_ids = false;
	bool any_files_enabled = false;

	// Iterate through the files accepted by the user.
	file_state_seq_type::const_iterator file_state_iter = d_file_state_seq.begin();
	file_state_seq_type::const_iterator file_state_end = d_file_state_seq.end();
	for ( ; file_state_iter != file_state_end; ++file_state_iter)
	{
		const FileState &file_state = *file_state_iter;

		// If user has disabled then continue to the next file.
		if (!file_state.enabled)
		{
			continue;
		}
		any_files_enabled = true;

		const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection_ref =
				file_state.file->get_feature_collection();

		if (d_only_assign_to_features_with_no_reconstruction_plate_id)
		{
			// Find any features in the current feature collection that are not
			// reconstructable features (have no 'reconstructionPlateId' plate id) and
			// are also not reconstruction features.
			std::vector<GPlatesModel::FeatureHandle::weak_ref> features_needing_plate_id;
			if (GPlatesAppLogic::AssignPlateIds::find_reconstructable_features_without_reconstruction_plate_id(
				features_needing_plate_id, feature_collection_ref))
			{
				// We found features in the current feature collection that need plate id assigning.
				const bool assigned_plate_id =
						plate_id_assigner.assign_reconstruction_plate_ids(
								features_needing_plate_id, feature_collection_ref);

				assigned_any_plate_ids |= assigned_plate_id;

				// See if any of the features are the focused feature.
				std::vector<GPlatesModel::FeatureHandle::weak_ref>::const_iterator feature_ref_iter
						= features_needing_plate_id.begin();
				std::vector<GPlatesModel::FeatureHandle::weak_ref>::const_iterator feature_ref_end
						= features_needing_plate_id.end();
				for ( ; feature_ref_iter != feature_ref_end; ++feature_ref_iter)
				{
					if (feature_ref_iter->is_valid() &&
						feature_ref_iter->handle_ptr() ==
								d_feature_focus.focused_feature().handle_ptr())
					{
						d_feature_focus.announce_modification_of_focused_feature();
						break;
					}
				}
			}
		}
		else
		{
			const bool assigned_plate_id =
					plate_id_assigner.assign_reconstruction_plate_ids(feature_collection_ref);

			assigned_any_plate_ids |= assigned_plate_id;

			// See if any of the features are the focused feature.
			GPlatesModel::FeatureCollectionHandle::iterator feature_iter;
			for (feature_iter = feature_collection_ref->begin();
				feature_iter != feature_collection_ref->end();
				++feature_iter)
			{
				if ((*feature_iter).get() == d_feature_focus.focused_feature().handle_ptr())
				{
					d_feature_focus.announce_modification_of_focused_feature();
					break;
				}
			}
		}
	}

	// If no files were selected.
	if (!any_files_enabled)
	{
		return false;
	}

	// If any plate ids were assigned then we need to do another reconstruction.
	// Note we'll do one anyway since the feature may have been modified even if
	// a plate id was not assigned (such as removing an existing plate id).
	d_reconstruct.reconstruct();

	// Let the caller know it can close this dialog since files were selected.
	return true;
}
