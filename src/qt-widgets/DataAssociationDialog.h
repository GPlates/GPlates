/* $Id$ */


/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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
 
#ifndef GPLATES_QT_WIDGETS_DATAASSOCIATIONDIALOG_H
#define GPLATES_QT_WIDGETS_DATAASSOCIATIONDIALOG_H

#include <vector>
#include <boost/cast.hpp>
#include <QPushButton>

#include "DataAssociationDialogUi.h"
#include "InformationDialog.h"
#include "ProgressDialog.h"
#include "ResultTableDialog.h"

#include "app-logic/AssignPlateIds.h"

#include "data-mining/DataSelector.h"
#include "data-mining/CheckAttrTypeVisitor.h"
#include "data-mining/AssociationOperator.h"

#include "file-io/File.h"

#include "model/FeatureCollectionHandle.h"
#include "model/ModelInterface.h"

#include "app-logic/ApplicationState.h"
#include "presentation/ViewState.h"


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
}

typedef std::set< QString > DefaultAttributeList;

namespace GPlatesQtWidgets
{
	class DataAssociationDialog:
			public QDialog, 
			protected Ui_DataAssociationDialog
	{
		Q_OBJECT

	public:
		enum InputTableColumnEnum
		{
			FeatureCollectionName = 0,
			AssociationType,
			AttributesFunctions,
			RegionOfInterestRange,
			DataOperatorCombo
		};

		enum AssociationTypeEnum
		{
			Relational,
			Coregistration
		};

		static CoRegConfigurationTable CoRegCfgTable;

		DataAssociationDialog(
				GPlatesAppLogic::ApplicationState &application_state,
				GPlatesPresentation::ViewState &view_state,
				QWidget *parent_ = NULL);

		~DataAssociationDialog()
		{
			if(d_progress_dialog != NULL)
			{
				delete d_progress_dialog;
			}
		}
		
		static const DefaultAttributeList d_default_attribute_list;

		static const QString DISTANCE;
		static const QString PRESENCE;
		static const QString NUM_ROI;

		void
		update_progress_bar(
				time_t);

		void
		pop_up_dialog();
	
	private:
		ProgressDialog* d_progress_dialog;
		unsigned d_progress_dialog_counter;
		unsigned d_progress_dialog_range;

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
		react_cell_changed_partitioning_files(
				int row,
				int column);

		void
		react_feature_collection_changed();

		void
		react_add_button_clicked();

		void
		react_association_operator_changed(
				int idx);

		void
		react_combox_association_type_current_index_changed(
				int idx);
		inline
		void
		handle_default_ROI_range_changed(
				double value_)
		{
			d_default_ROI_range = value_;
		}

		void
		create_data_association_feature();

		void
		apply_layer_configuration();

	private:

		/*
		*
		*/
		/*
		*
		*/
		void
		set_progress_bar_range();
		/*
		*
		*/
		void
		initialise_progress_dialog();
		/*
		*
		*/
		void
		destroy_progress_dialog();

		/*
		*
		*/
		void
		setup_operator_combobox(
				const QString& attribute_name,	
				QComboBox* combo);

		/*
		*
		*/
		void
		add_default_attributes();
		
		/*
		*
		*/
		void
		init_target_collection_list_widget();

		/*
		*
		*/
		const GPlatesFileIO::File::Reference *
		get_file_by_name(
				QString& name);
		/*
		*
		*/
		boost::optional< GPlatesModel::FeatureCollectionHandle::const_weak_ref >
		get_feature_collection_by_name(
				QString&);

		typedef std::multimap<
				GPlatesModel::PropertyName,
				GPlatesModel::FeatureCollectionHandle::const_weak_ref
							> PropertyNameFeatureCollectionMap;

		PropertyNameFeatureCollectionMap d_property_map;

		/*
		*
		*/
		void
		populate_input_table(
				GPlatesDataMining::CoRegConfigurationTable&);
		/*
		*
		*/
		void
		sort_input_table(
		GPlatesDataMining::CoRegConfigurationTable& input_table);

		/**
		 * Keeps track of which files are enabled/disabled by the user.
		 */

		void
		get_unique_attribute_names(
				const GPlatesModel::FeatureCollectionHandle::const_weak_ref feature_collection_ref,
				std::set< GPlatesModel::PropertyName > &names);

		class FileState
		{
			public:
				FileState(
					const GPlatesFileIO::File::Reference &file_) :
				file(&file_),
				enabled(false)
				{  }

			const GPlatesFileIO::File::Reference *file;
			bool enabled;
		};
		typedef std::vector<FileState> file_state_seq_type;

		//! Typedef for a sequence of file pointers.
		typedef std::vector<const GPlatesFileIO::File::Reference *> file_ptr_seq_type;

		//! Typedef for a sequence of feature collection weak refs.
		typedef std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref>
				feature_collection_seq_type;


		/**
		 * These should match the table columns set up in the UI designer.
		 */
		enum ColumnName
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

		GPlatesFileIO::File* d_selected_file;

		
		/**
		 * Button added to buttonbox for 'Apply' button that partitions the features.
		 */
		QPushButton *d_button_create;

		GPlatesAppLogic::FeatureCollectionFileState &d_feature_collection_file_state;
		GPlatesAppLogic::ApplicationState &d_application_state;
		GPlatesGui::FeatureFocus &d_feature_focus;

		/**
		 * Keeps track of which seed files are enabled by the user in the GUI.
		 */
		FileStateCollection d_seed_file_state_seq;

		/**
		 * Keeps track of which partitioned files are enabled by the user in the GUI.
		 */
		FileStateCollection d_selected_file_state_seq;

		GPlatesDataMining::AssociationOperatorType d_association_operator;
		int d_default_ROI_range;
		double d_start_time, d_end_time, d_time_inc;

		void
		set_up_button_box();

		void
		set_up_seed_files_page();

		void
		set_up_select_attr_page();

		void
		set_up_general_options_page();

		void
		initialise_file_list(
				FileStateCollection &file_state_collection,
				const file_ptr_seq_type &files);

		void
		clear_rows(
				FileStateCollection &file_state_collection);

		void
		add_row(
				FileStateCollection &file_state_collection,
				const GPlatesFileIO::File::Reference &file);

		void
		react_cell_changed(
				FileStateCollection &file_state_collection,
				int row,
				int column);

		void
		react_select_all(
				FileStateCollection &file_state_collection);


		void
		react_clear_all(
				FileStateCollection &file_state_collection);


		file_ptr_seq_type
		get_loaded_files();


		feature_collection_seq_type
		get_selected_feature_collections(
				FileStateCollection &file_state_collection);


		void
		add_shape_file_attrs( 
				const GPlatesModel::FeatureCollectionHandle::const_weak_ref feature_collection,
				const GPlatesModel::PropertyName& property_name,
				QListWidget *listWidget_attributes);


		typedef
			std::multimap< QString, GPlatesDataMining::AttributeTypeEnum > AttrNameTypeMap;
		
		AttrNameTypeMap d_attr_name_type_map;
		boost::scoped_ptr< ResultTableDialog > d_result_dialog;
	};
}
#endif // GPLATES_QT_WIDGETS_DATAASSOCIATIONDIALOG_H





