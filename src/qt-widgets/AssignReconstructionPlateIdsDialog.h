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
#include <boost/optional.hpp>
#include <boost/weak_ptr.hpp>
#include <QButtonGroup>
#include <QPushButton>

#include "AssignReconstructionPlateIdsDialogUi.h"

#include "InformationDialog.h"

#include "app-logic/AssignPlateIds.h"

#include "file-io/File.h"

#include "model/FeatureCollectionHandle.h"
#include "model/ModelInterface.h"

#include "presentation/VisualLayer.h"


namespace GPlatesAppLogic
{
	class ApplicationState;
	class AssignPlateIds;
	class FeatureCollectionFileState;
}

namespace GPlatesGui
{
	class FeatureFocus;
}

namespace GPlatesPresentation
{
	class ViewState;
	class VisualLayers;
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


		/**
		 * Opens a modal dialog allowing user to choose partitioning polygon files and
		 * the files contained features to be partitioned by those polygons.
		 */
		void
		exec_partition_features_dialog();

	public slots:
		void
		reject();

	private slots:
		void
		apply();

		void
		clear();

		void
		handle_prev();

		void
		handle_next();

		void
		handle_page_change(
				int page);

		void
		react_cell_changed_partitioned_files(
				int row,
				int column);

		void
		react_clear_all_partitioned_files();

		void
		react_select_all_partitioned_files();

		void
		react_reconstruction_time_radio_button(
				bool checked);

		void
		react_spin_box_reconstruction_time_changed(
				double reconstruction_time);

		void
		react_respect_feature_time_period_check_box_changed();

		void
		react_partition_options_radio_button(
				bool checked);

		void
		react_feature_properties_options_radio_button(
				bool checked);

	private:
		/**
		 * Keeps track of which files are enabled/disabled by the user.
		 */
		class FileState
		{
		public:
			//! Files are disabled by default - user will need to enable them.
			explicit
			FileState(
					const GPlatesFileIO::File::Reference &file_) :
				file(&file_),
				enabled(false)
			{  }

			const GPlatesFileIO::File::Reference *file;
			bool enabled;
		};
		typedef std::vector<FileState> file_state_seq_type;

		/**
		 * These should match the table columns set up in the UI designer.
		 */
		enum FileColumnName
		{
			FILENAME_COLUMN,
			ENABLE_FILE_COLUMN
		};

		struct FileStateCollection
		{
			FileStateCollection() :
				table_widget(NULL)
			{  }

			QTableWidget *table_widget; // Needs to be initialised after setupUi()
			file_state_seq_type file_state_seq;
		};

		//! Typedef for a sequence of file pointers.
		typedef std::vector<const GPlatesFileIO::File::Reference *> file_ptr_seq_type;

		//! Typedef for a sequence of feature collection weak refs.
		typedef std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref>
				feature_collection_seq_type;

		//! Typedef for a layer pointer.
		typedef boost::weak_ptr<GPlatesPresentation::VisualLayer> layer_ptr_type;

		/**
		 * Keeps track of which layers are enabled/disabled by the user.
		 */
		class LayerState
		{
		public:
			//! Layers are disabled by default - user will need to enable them.
			explicit
			LayerState(
					const layer_ptr_type &layer_) :
				layer(layer_)
			{  }

			layer_ptr_type layer;
		};
		typedef std::vector<LayerState> layer_state_seq_type;

		/**
		 * These should match the table columns set up in the UI designer.
		 */
		enum LayerColumnName
		{
			LAYER_NAME_COLUMN,
			ENABLE_LAYER_COLUMN
		};

		struct LayerStateCollection
		{
			LayerStateCollection() :
				table_widget(NULL),
				layer_selection_group(NULL)
			{  }

			QTableWidget *table_widget; // Needs to be initialised after setupUi()
			QButtonGroup *layer_selection_group; // Initialised with 'table_widget' as parent
			layer_state_seq_type layer_state_seq;
		};

		//! Typedef for a sequence of visual layers.
		typedef std::vector<layer_ptr_type> layer_ptr_seq_type;

		/**
		 * The user's choice of reconstruction time.
		 */
		enum ReconstructionTimeType
		{
			PRESENT_DAY_RECONSTRUCTION_TIME,
			CURRENT_RECONSTRUCTION_TIME,
			USER_SPECIFIED_RECONSTRUCTION_TIME
		};


		GPlatesQtWidgets::InformationDialog *d_help_partitioning_layers_dialog;
		GPlatesQtWidgets::InformationDialog *d_help_partitioned_files_dialog;
		GPlatesQtWidgets::InformationDialog *d_help_reconstruction_time_dialog;
		GPlatesQtWidgets::InformationDialog *d_help_partition_options_dialog;
		GPlatesQtWidgets::InformationDialog *d_help_properties_to_assign_dialog;
		GPlatesQtWidgets::InformationDialog *d_help_respect_feature_time_period;
		
		/**
		 * Button added to buttonbox for 'Apply' button that partitions the features.
		 */
		QPushButton *d_button_create;

		GPlatesAppLogic::FeatureCollectionFileState &d_feature_collection_file_state;
		GPlatesAppLogic::ApplicationState &d_application_state;
		GPlatesGui::FeatureFocus &d_feature_focus;

		/**
		 * The user selects a layer to be the polygon partitioning layer.
		 */
		GPlatesPresentation::VisualLayers &d_visual_layers;

		/**
		 * Keeps track of which partitioning layers are enabled by the user in the GUI.
		 */
		LayerStateCollection d_partitioning_layer_state_seq;

		/**
		 * Keeps track of which partitioned files are enabled by the user in the GUI.
		 */
		FileStateCollection d_partitioned_file_state_seq;

		/**
		 * Which reconstruction time the user has chosen.
		 */
		ReconstructionTimeType d_reconstruction_time_type;

		/**
		 * The reconstruction time set by the double spin box.
		 */
		double d_spin_box_reconstruction_time;

		/**
		 * Determines if features are only partitioned if the reconstruction time is within
		 * the time period over which the features are defined.
		 *
		 * This may not apply to some feature types (eg, virtual geomagnetic poles).
		 */
		bool d_respect_feature_time_period;

		/**
		 * How to assign plate ids to features.
		 */
		GPlatesAppLogic::AssignPlateIds::AssignPlateIdMethodType d_assign_plate_id_method;

		//! Whether to copy plate ids from the partitioning polygons or not.
		bool d_assign_plate_ids;

		//! Whether to copy time periods from the partitioning polygons or not.
		bool d_assign_time_periods;


		void
		set_up_button_box();

		void
		set_up_partitioning_layers_page();

		void
		set_up_partitioned_files_page();

		void
		set_up_general_options_page();

		void
		pop_up_no_partitioning_layers_found_or_selected_message_box();

		void
		pop_up_no_partitioning_polygons_found_message_box();

		void
		pop_up_no_partitioned_files_selected_message_box();

		void
		initialise_file_list(
				FileStateCollection &file_state_collection,
				const file_ptr_seq_type &files);

		void
		clear_file_rows(
				FileStateCollection &file_state_collection);

		void
		add_file_row(
				FileStateCollection &file_state_collection,
				const GPlatesFileIO::File::Reference &file);

		void
		initialise_layer_list(
				LayerStateCollection &layer_state_collection,
				const layer_ptr_seq_type &layers);

		void
		clear_layer_rows(
				LayerStateCollection &layer_state_collection);

		void
		add_layer_row(
				LayerStateCollection &layer_state_collection,
				const layer_ptr_type &visual_layer);

		file_ptr_seq_type
		get_loaded_files();

		layer_ptr_seq_type
		get_possible_partitioning_layers();

		feature_collection_seq_type
		get_selected_feature_collections(
				FileStateCollection &file_state_collection);

		boost::optional<layer_ptr_type>
		get_selected_layer(
				LayerStateCollection &layer_state_collection);

		bool
		partition_features();

		boost::optional<GPlatesAppLogic::AssignPlateIds::non_null_ptr_type>
		create_plate_id_assigner();

		bool
		partition_features(
				GPlatesAppLogic::AssignPlateIds &plate_id_assigner);
	};
}

#endif // GPLATES_QT_WIDGETS_ASSIGNRECONSTRUCTIONPLATEIDSDIALOG_H
