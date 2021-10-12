/* $Id$ */


/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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
 
#ifndef GPLATES_QT_WIDGETS_ASSIGNRECONSTRUCTIONPLATEIDSDIALOG_H
#define GPLATES_QT_WIDGETS_ASSIGNRECONSTRUCTIONPLATEIDSDIALOG_H

#include <vector>
#include <boost/cast.hpp>

#include "AssignReconstructionPlateIdsDialogUi.h"

#include "InformationDialog.h"

#include "app-logic/AssignPlateIds.h"

#include "file-io/File.h"

#include "model/ModelInterface.h"


namespace GPlatesAppLogic
{
	class ApplicationState;
	class AssignPlateIds;
	class FeatureCollectionFileState;
	class Reconstruct;
}

namespace GPlatesGui
{
	class FeatureFocus;
}

namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesQtWidgets
{
	class AssignReconstructionPlateIdsDialog:
			public QDialog, 
			protected Ui_AssignReconstructionPlateIdsDialog
	{
		Q_OBJECT

	public:
		AssignReconstructionPlateIdsDialog(
				GPlatesAppLogic::ApplicationState &application_state,
				GPlatesPresentation::ViewState &view_state,
				QWidget *parent_ = NULL);


		//! Typedef for a sequence of file shared refs.
		typedef std::vector<GPlatesFileIO::File::shared_ref> file_seq_type;


		/**
		 * Opens a modal dialog listing the files in @a newly_loaded_feature_collection_files and
		 * allows user to choose which feature collection files get assigned plate ids.
		 *
		 * Only those features in the files that have no 'gpml:reconstructionPlateId'
		 * property will have plate ids assigned.
		 *
		 * @pre It is assumed that features of type TopologicalClosedPlateBoundary have
		 * already been loaded (as they are required to cookie cut the input features) -
		 * if none are loaded then no plate ids will get assigned.
		 */
		void
		assign_plate_ids_to_newly_loaded_feature_collections(
				const file_seq_type &newly_loaded_feature_collection_files,
				bool pop_up_message_box_if_no_plate_boundaries);

		/**
		 * Opens a modal dialog listing all active feature collection files (excluding
		 * reconstruction files - used for rotations) and allows user to choose
		 * which files get reassigned plate ids.
		 *
		 * Any features that already have a 'gpml:reconstructionPlateId' property
		 * will have plate ids reassigned.
		 *
		 * @pre It is assumed that features of type TopologicalClosedPlateBoundary have
		 * already been loaded (as they are required to cookie cut the input features) -
		 * if none are loaded then no plate ids will get reassigned.
		 */
		void
		reassign_reconstruction_plate_ids(
				bool pop_up_message_box_if_no_plate_boundaries);

	public slots:
		void
		reject();

	private slots:
		void
		apply();

		void
		clear();

		void
		react_cell_changed(
				int row,
				int column);

		void
		react_clear_all();

		void
		react_select_all();

		void
		react_reconstruction_time_radio_button(
				bool checked);

		void
		react_spin_box_reconstruction_time_changed(
				double reconstruction_time);

		void
		react_assign_plate_id_options_radio_button(
				bool checked);

	private:
		/**
		 * Keeps track of which files are enabled/disabled by the user.
		 */
		class FileState
		{
		public:
			//! Files are disabled by default - user will need to enable them.
			FileState(
					GPlatesFileIO::File *file_) :
				file(file_),
				enabled(false)
			{  }

			GPlatesFileIO::File *file;
			bool enabled;
		};
		typedef std::vector<FileState> file_state_seq_type;

		//! Typedef for a sequence of file pointers.
		typedef std::vector<GPlatesFileIO::File *> file_ptr_seq_type;

		/**
		 * These should match the table columns set up in the UI designer.
		 */
		enum ColumnName
		{
			FILENAME_COLUMN,
			ENABLE_FILE_COLUMN
		};

		/**
		 * The user's choice of reconstruction time.
		 */
		enum ReconstructionTimeType
		{
			PRESENT_DAY_RECONSTRUCTION_TIME,
			CURRENT_RECONSTRUCTION_TIME,
			USER_SPECIFIED_RECONSTRUCTION_TIME
		};

		GPlatesQtWidgets::InformationDialog *d_help_feature_collections_dialog;
		GPlatesQtWidgets::InformationDialog *d_help_reconstruction_time_dialog;
		GPlatesQtWidgets::InformationDialog *d_help_assign_plate_id_options_dialog;

		GPlatesModel::ModelInterface d_model;
		GPlatesAppLogic::FeatureCollectionFileState &d_feature_collection_file_state;
		GPlatesAppLogic::Reconstruct &d_reconstruct;
		GPlatesGui::FeatureFocus &d_feature_focus;

		/**
		 * Keeps track of which files are enabled by the user in the GUI.
		 */
		file_state_seq_type d_file_state_seq;

		bool d_only_assign_to_features_with_no_reconstruction_plate_id;

		/**
		 * Which reconstruction time the user has chosen.
		 */
		ReconstructionTimeType d_reconstruction_time_type;

		/**
		 * The reconstruction time set by the double spin box.
		 */
		double d_spin_box_reconstruction_time;

		/**
		 * How to assign plate ids to features.
		 */
		GPlatesAppLogic::AssignPlateIds::AssignPlateIdMethodType d_assign_plate_id_method;


		bool
		check_for_plate_boundaries(
				bool pop_up_message_box_if_no_plate_boundaries);

		void
		pop_up_no_plate_boundaries_message_box();

		void
		pop_up_no_files_selected_message_box();

		void
		get_files_to_assign_plate_ids(
				file_ptr_seq_type &files_to_assign_plate_ids,
				const file_seq_type &newly_loaded_feature_collection_files);

		void
		get_files_to_reassign_plate_ids(
				file_ptr_seq_type &files_to_assign_plate_ids);

		void
		query_user_and_assign_plate_ids(
				const file_ptr_seq_type &files_to_assign_plate_ids);

		void
		initialise_file_list(
				const file_ptr_seq_type &file_list);

		void
		clear_rows();

		void
		add_row(
				GPlatesFileIO::File *file);

		GPlatesAppLogic::AssignPlateIds::non_null_ptr_type
		create_plate_id_assigner();

		bool
		assign_plate_ids(
				GPlatesAppLogic::AssignPlateIds &plate_id_assigner);
	};
}

#endif // GPLATES_QT_WIDGETS_ASSIGNRECONSTRUCTIONPLATEIDSDIALOG_H
