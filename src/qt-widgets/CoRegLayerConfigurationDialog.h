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
 
#ifndef GPLATES_QT_WIDGETS_COREGLAYERCONFIGURATIONDIALOG_H
#define GPLATES_QT_WIDGETS_COREGLAYERCONFIGURATIONDIALOG_H

#include <boost/weak_ptr.hpp>
#include <boost/foreach.hpp>

#include <QListView>
#include <QComboBox>

#include "CoRegLayerConfigurationDialogUi.h"

#include "OpenDirectoryDialog.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/FeatureCollectionFileState.h"
#include "app-logic/ReconstructGraph.h"

#include "data-mining/CheckAttrTypeVisitor.h"
#include "data-mining/CoRegConfigurationTable.h"

#include "presentation/ViewState.h"

namespace GPlatesPresentation
{
	class ViewState;
	class VisualLayer;
}

namespace GPlatesQtWidgets
{
	using namespace GPlatesDataMining;

	enum ConfigurationTableColumnEnum
	{
		FeatureCollectionName = 0,
		AssociationType,
		AttributeName,
		Range,
		DataOperator,
		EndOFTheEnum
	};
	/**
	 * struct of QListWidgetItem so that we can display a list of FeatureCollection in
	 * the list widget using the filename as the label, while keeping track of which
	 * list item corresponds to which FeatureCollection.
	 */
	struct FeatureCollectionItem :
			public QListWidgetItem
	{
		FeatureCollectionItem(
				const GPlatesAppLogic::FeatureCollectionFileState::file_reference& file, 
				QString file_name):
			QListWidgetItem(file_name),
			file_ref(file),
			label(file_name)
			
			{ }

		const GPlatesAppLogic::FeatureCollectionFileState::file_reference file_ref;
		const QString label;
	};

	struct AttributeItem
	{
		AttributeItem(
				const GPlatesModel::PropertyName& name_,
				const AttributeType type_):  
			name(name_),
			attr_type(type_)
		{ }
		const GPlatesModel::PropertyName name;
		const AttributeType attr_type;
	};

	struct AttributeListItem :
		public QListWidgetItem,
		public AttributeItem
	{
		AttributeListItem(
				const QString &str_,
				const GPlatesModel::PropertyName& name_,
				const AttributeType type_):  
			QListWidgetItem(str_),
			AttributeItem(name_ ,type_)
		{ }
	};


	struct FeatureCollectionTableItem :
			public QTableWidgetItem
	{
		FeatureCollectionTableItem(
				const GPlatesAppLogic::FeatureCollectionFileState::file_reference& file, 
				QString file_name):
			QTableWidgetItem(file_name),
			file_ref(file),
			label(file_name)
			
			{ }

		const GPlatesAppLogic::FeatureCollectionFileState::file_reference file_ref;
		const QString label;
	};

	struct AttributeTableItem :
		public QTableWidgetItem,
		public AttributeItem
	{
		AttributeTableItem(
				const QString &str_,
				const GPlatesModel::PropertyName& name_,
				const AttributeType type_):  
			QTableWidgetItem(str_),
			AttributeItem(name_ ,type_)
		{ }
	};
	

	/**
	 * The configuration dialog for Co-registration layer.
	 */
	class CoRegLayerConfigurationDialog :
			public QDialog, 
			protected Ui_CoRegLayerConfigurationDialog
	{
		Q_OBJECT

	public:

		CoRegLayerConfigurationDialog(
				GPlatesPresentation::ViewState &view_state,
				boost::weak_ptr<GPlatesPresentation::VisualLayer> layer) :
			d_visual_layer(layer),
			d_open_directory_dialog(
					this,
					tr("Select Path"),
					view_state)
		{
			setupUi(this);
			QObject::connect(FeatureCollectionListWidget, SIGNAL(itemSelectionChanged()),
				this, SLOT(react_feature_collection_changed()));

			QObject::connect(FeatureCollectionListWidget, SIGNAL(itemClicked(QListWidgetItem *)),
				this, SLOT(react_feature_collection_changed()));

			QObject::connect(AddPushButton, SIGNAL(clicked()),
				this, SLOT(react_add_button_clicked()));

			QObject::connect(buttonBox, SIGNAL(clicked(QAbstractButton*)), 
				this, SLOT(apply(QAbstractButton*)));

			QObject::connect(ChooseExportPathPushButton, SIGNAL(clicked()), 
				this, SLOT(react_choose_export_path()));

			QObject::connect(RelationalRadioButton, SIGNAL(clicked()), 
				this, SLOT(populate_relational_attributes()));

			QObject::connect(CoregRadioButton, SIGNAL(clicked()), 
				this, SLOT(populate_coregistration_attributes()));

			QObject::connect(
				&view_state.get_application_state().get_feature_collection_file_state(),
				SIGNAL(file_state_file_about_to_be_removed(
						GPlatesAppLogic::FeatureCollectionFileState &,
						GPlatesAppLogic::FeatureCollectionFileState::file_reference)),
				this,
				SLOT(handle_file_state_file_about_to_be_removed(
						GPlatesAppLogic::FeatureCollectionFileState &,
						GPlatesAppLogic::FeatureCollectionFileState::file_reference)));

			QObject::connect(
				&view_state.get_application_state().get_reconstruct_graph(),
				SIGNAL(layer_removed_input_connection(
						GPlatesAppLogic::ReconstructGraph &,
						GPlatesAppLogic::Layer)),
				this,
				SLOT(handle_layer_removed_input_connection(
						GPlatesAppLogic::ReconstructGraph &,
						GPlatesAppLogic::Layer )));
		}

		void
		pop_up();

		inline
		GPlatesDataMining::CoRegConfigurationTable&
		cfg_table()
		{
			return d_cfg_table;
		}

	protected:
		void
		populate_feature_collection_list() const;

		void
		get_unique_attribute_names(
				const GPlatesModel::FeatureCollectionHandle::const_weak_ref feature_collection_ref,
				std::set< GPlatesModel::PropertyName > &names);

		void
		add_shape_file_attrs( 
				const GPlatesModel::FeatureCollectionHandle::const_weak_ref feature_collection,
				const GPlatesModel::PropertyName& _property_name,
				QListWidget *_listWidget_attributes);
			
		void
		setup_data_operator_combobox(
				const QString& attribute_name,	
				QComboBox* combo);

		void
		setup_association_type_combobox(
				QComboBox* combo);

	public slots:
		void
		reject()
		{
			done(QDialog::Rejected);
		}
		
		void
		apply(
				QAbstractButton*);

	private slots:

		void
		react_feature_collection_changed();

		void
		react_add_button_clicked();

		void
		react_choose_export_path();

		void
		populate_relational_attributes();

		void
		populate_coregistration_attributes();

		void
		handle_file_state_file_about_to_be_removed(
				GPlatesAppLogic::FeatureCollectionFileState &file_state,
				GPlatesAppLogic::FeatureCollectionFileState::file_reference file);

		void
		handle_layer_removed_input_connection(
				GPlatesAppLogic::ReconstructGraph &,
				GPlatesAppLogic::Layer);

	private:
		void
		check_integrity();
	
		std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference>
		get_input_target_files() const;

		std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference>
		get_input_seed_files() const;

		std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference>
		get_input_files(
				const QString&) const;

		void
		update_export_path(
				const QString&);

		GPlatesDataMining::CoRegConfigurationTable d_cfg_table;

		std::multimap< QString, GPlatesDataMining::AttributeTypeEnum >  d_attr_name_type_map;

		boost::weak_ptr<GPlatesPresentation::VisualLayer> d_visual_layer;
		
		OpenDirectoryDialog d_open_directory_dialog;
	};

}
#endif // GPLATES_QT_WIDGETS_COREGLAYERCONFIGURATIONDIALOG_H








