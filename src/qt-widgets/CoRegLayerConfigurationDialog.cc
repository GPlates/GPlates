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

#include <QDoubleSpinBox>
#include <QFileDialog>

#include "CoRegLayerConfigurationDialog.h"

#include "data-mining/PopulateShapeFileAttributesVisitor.h"
#include "data-mining/CoRegConfigurationTable.h"

#include "global/CompilerWarnings.h"

#include "presentation/VisualLayer.h"

GPlatesDataMining::CoRegConfigurationTable GPlatesQtWidgets::CoRegLayerConfigurationDialog::CoRegCfgTable;

void 
GPlatesQtWidgets::CoRegLayerConfigurationDialog::pop_up()
{
	RelationalRadioButton->setChecked(true);
	FeatureCollectionListWidget->clear();
	populate_feature_collection_list();
	check_integrity();

	show();
	// In most cases, 'show()' is sufficient. However, selecting the menu entry
	// a second time, when the dialog is still open, should make the dialog 'active'
	// and return keyboard focus to it.
	activateWindow();
	// On platforms which do not keep dialogs on top of their parent, a call to
	// raise() may also be necessary to properly 're-pop-up' the dialog.
	raise();
}


std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference>
GPlatesQtWidgets::CoRegLayerConfigurationDialog::get_input_seed_files() const
{
	// TODO: create a const for the channel name.
	return get_input_files("CoRegistration seed Channel");
}


std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference>
GPlatesQtWidgets::CoRegLayerConfigurationDialog::get_input_target_files() const
{
	// TODO: create a const for the channel name.
	return get_input_files("CoRegistration input Channel");
}


// The BOOST_FOREACH macro in versions of boost before 1.37 uses the same local
// variable name in each instantiation. Nested BOOST_FOREACH macros therefore
// cause GCC to warn about shadowed declarations.
DISABLE_GCC_WARNING("-Wshadow")

std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference>
GPlatesQtWidgets::CoRegLayerConfigurationDialog::get_input_files(
		const QString& channel_name) const
{
	std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference> files;
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
		d_visual_layer.lock())
	{
		GPlatesAppLogic::Layer layer = locked_visual_layer->get_reconstruct_graph_layer();

		std::vector<GPlatesAppLogic::Layer::InputConnection> input_connections =
			layer.get_channel_inputs(channel_name);

		BOOST_FOREACH(const GPlatesAppLogic::Layer::InputConnection& connection, input_connections)
		{
			//The inputs of co-registration layer are the output of other layers.
			//We can only get associated files from upper layers.
			boost::optional<GPlatesAppLogic::Layer> input_layer = connection.get_input_layer();
			if(!input_layer)
			{
				continue;
			}
			QString main_input_chanel = 
			input_layer->get_main_input_feature_collection_channel();

			std::vector<GPlatesAppLogic::Layer::InputConnection> main_input =
				input_layer->get_channel_inputs(main_input_chanel);

			//loop over all input connection to get files info.
			BOOST_FOREACH(const GPlatesAppLogic::Layer::InputConnection& input_connection, main_input)
			{
				boost::optional<GPlatesAppLogic::Layer::InputFile> input_file = 
					input_connection.get_input_file();
				if(!input_file)
				{
					continue;
				}
				files.push_back(input_file->get_file());
			}
		}
	}
	return files;
}

// See above
ENABLE_GCC_WARNING("-Wshadow")


void
GPlatesQtWidgets::CoRegLayerConfigurationDialog::populate_feature_collection_list() const
{
	std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference> files =
		get_input_target_files();
	BOOST_FOREACH(GPlatesAppLogic::FeatureCollectionFileState::file_reference& file, files)
	{
		QString file_name;
		if (GPlatesFileIO::file_exists(file.get_file().get_file_info()))
		{
			file_name = file.get_file().get_file_info().get_display_name(false);
		}
		else
		{
			// The file doesn't exist so give it a filename to indicate this.
			file_name = "New Feature Collection";
		}

		FeatureCollectionItem* item = 
			new  FeatureCollectionItem(file,file_name);
		FeatureCollectionListWidget->addItem(item); 
	}
}


void
GPlatesQtWidgets::CoRegLayerConfigurationDialog::react_feature_collection_changed()
{
	AttributesListWidget->clear();
	
	FeatureCollectionItem* current_item = 
		dynamic_cast< FeatureCollectionItem* > (FeatureCollectionListWidget->currentItem());
	if(!current_item)
	{
		qDebug() << "The current feature collection item is null.";
		return;
	}

	if(RelationalRadioButton->isChecked()) 
	{
		populate_relational_attributes();
	}
	else 
	{
		populate_coregistration_attributes();
	}
}


void
GPlatesQtWidgets::CoRegLayerConfigurationDialog::get_unique_attribute_names(
		const GPlatesModel::FeatureCollectionHandle::const_weak_ref feature_collection_ref,
		std::set< GPlatesModel::PropertyName > &names)
{
	using namespace GPlatesDataMining;
	GPlatesModel::FeatureCollectionHandle::const_iterator it_begin = 
		feature_collection_ref->begin();
	GPlatesModel::FeatureCollectionHandle::const_iterator it_end = 
		feature_collection_ref->end();

	for( ; it_begin != it_end; it_begin++)
	{
		GPlatesModel::FeatureHandle::const_iterator fh_it_begin = 
			(*it_begin)->begin();
		GPlatesModel::FeatureHandle::const_iterator fh_it_end = 
			(*it_begin)->end();

		for( ; fh_it_begin != fh_it_end; fh_it_begin++)
		{
			GPlatesModel::PropertyName name = (*fh_it_begin)->property_name();
			names.insert( name );

			CheckAttrTypeVisitor visitor;
			(*fh_it_begin)->accept_visitor(visitor);
			//hacking code for shape file.
			using namespace GPlatesPropertyValues;
			if(GPlatesUtils::make_qstring_from_icu_string(name.get_name()) == "shapefileAttributes")
			{
				d_attr_name_type_map.insert(
						visitor.shape_map().begin(),
						visitor.shape_map().end());
				
			}
			else
			{
				QString q_name = GPlatesUtils::make_qstring_from_icu_string(name.get_name());
				d_attr_name_type_map.insert(
					std::pair< QString, GPlatesDataMining::AttributeTypeEnum >(
								q_name,visitor.type() ) );
			}
		}
	}
}


void
GPlatesQtWidgets::CoRegLayerConfigurationDialog::add_shape_file_attrs( 
		const GPlatesModel::FeatureCollectionHandle::const_weak_ref feature_collection,
		const GPlatesModel::PropertyName& _property_name,
		QListWidget *_listWidget_attributes)
{
	GPlatesModel::FeatureCollectionHandle::const_iterator it = feature_collection->begin();
	GPlatesModel::FeatureCollectionHandle::const_iterator it_end = feature_collection->end();
	std::set< QString > shape_attr_set;
	for(; it != it_end; it++)
	{
		GPlatesDataMining::PopulateShapeFileAttributesVisitor visitor;
		visitor.visit_feature((*it)->reference());
		std::vector< QString > attr_names =
				visitor.get_shape_file_attr_names();
		std::vector< QString >::const_iterator inner_it = attr_names.begin();
		std::vector< QString >::const_iterator inner_it_end = attr_names.end();
		for(; inner_it != inner_it_end; inner_it++)
		{
			shape_attr_set.insert(*inner_it);
		}
	}

	std::set< QString >::const_iterator set_it = shape_attr_set.begin();
	std::set< QString >::const_iterator set_it_end = shape_attr_set.end();
	for(; set_it != set_it_end; set_it++)
	{
		QListWidgetItem *item = 
				new AttributeListItem(
						*set_it,
						_property_name,
						SHAPE_FILE_ATTRIBUTE);
		_listWidget_attributes->addItem(item);
	}
}


void
GPlatesQtWidgets::CoRegLayerConfigurationDialog::populate_relational_attributes()
{
	AttributesListWidget->clear();
	FeatureCollectionItem* current_item = 
		dynamic_cast< FeatureCollectionItem* > (FeatureCollectionListWidget->currentItem());
	if(!current_item)
	{
		return;
	}
	const char* DISTANCE = QT_TR_NOOP("Distance");
	const char* PRESENCE = QT_TR_NOOP("Presence");
	const char* NUM_ROI  = QT_TR_NOOP("Number in Region");

	QListWidgetItem *item = 
		new AttributeListItem(
				QApplication::tr(DISTANCE), 
				GPlatesModel::PropertyName::create_gpml(DISTANCE),
				DISTANCE_ATTRIBUTE);
	AttributesListWidget->addItem(item);

	item = 
		new AttributeListItem(
				QApplication::tr(PRESENCE), 
				GPlatesModel::PropertyName::create_gpml(PRESENCE),
				PRESENCE_ATTRIBUTE);
	AttributesListWidget->addItem(item);

	item = 
		new AttributeListItem(
				QApplication::tr(NUM_ROI), 
				GPlatesModel::PropertyName::create_gpml(NUM_ROI),
				NUMBER_OF_PRESENCE_ATTRIBUTE);
	AttributesListWidget->addItem(item);
}


void
GPlatesQtWidgets::CoRegLayerConfigurationDialog::populate_coregistration_attributes()
{
	AttributesListWidget->clear();
	FeatureCollectionItem* current_item = 
		dynamic_cast< FeatureCollectionItem* > (FeatureCollectionListWidget->currentItem());
	if(!current_item)
	{
		return;
	}
	GPlatesModel::FeatureCollectionHandle::const_weak_ref feature_collection_ref = 
			current_item->file_ref.get_file().get_feature_collection();

	std::set<GPlatesModel::PropertyName> attr_names;
	get_unique_attribute_names(
			feature_collection_ref, 
			attr_names);
	
	std::set< GPlatesModel::PropertyName >::const_iterator it = attr_names.begin();
	std::set< GPlatesModel::PropertyName >::const_iterator it_end = attr_names.end();

	for(; it != it_end; it++)
	{
		QListWidgetItem *item = 
			new AttributeListItem(
					GPlatesUtils::make_qstring_from_icu_string((*it).get_name()),
					*it,
					CO_REGISTRATION_ATTRIBUTE);

		if(GPlatesUtils::make_qstring_from_icu_string((*it).get_name()) == "shapefileAttributes")
		{
			add_shape_file_attrs(
					feature_collection_ref,
					*it,
					AttributesListWidget);
		}
		else
		{
			AttributesListWidget->addItem(item);
		}
	}
}


void
GPlatesQtWidgets::CoRegLayerConfigurationDialog::react_add_button_clicked()
{
	//feature collection and attribute must have been selected.
	if( !(FeatureCollectionListWidget->currentItem() && AttributesListWidget->currentItem()))
	{
		return;
	}

	int row_num=CoRegCfgTableWidget->rowCount();
	CoRegCfgTableWidget->insertRow(row_num);

	//Attribute Name column 
	AttributeListItem* attr_item = 
			dynamic_cast< AttributeListItem* >(AttributesListWidget->currentItem());
	if(attr_item)
	{
		CoRegCfgTableWidget->setItem(
				row_num, 
				AttributeName, 
				new AttributeTableItem(
						attr_item->text(),
						attr_item->name,
						attr_item->attr_type));
	}

	//Data Operator column
	QComboBox* combo = new QComboBox();       
    CoRegCfgTableWidget->setCellWidget(
			row_num, 
			DataOperator, 
			combo);

	setup_data_operator_combobox(
			attr_item->text(),
			combo);

	//Feature Collection Name column
	FeatureCollectionItem* fc_item = 
		dynamic_cast<FeatureCollectionItem*>(FeatureCollectionListWidget->currentItem());
	CoRegCfgTableWidget->setItem(
			row_num,
			FeatureCollectionName,
			new FeatureCollectionTableItem(
					fc_item->file_ref,
					fc_item->label));

	//Association Type column
	//TODO: To be finished...
	QComboBox* association_combo = new QComboBox();       
    CoRegCfgTableWidget->setCellWidget(
			row_num, 
			AssociationType, 
			association_combo);
	
	setup_association_type_combobox(association_combo);

	//Range column
	QDoubleSpinBox* ROI_range_spinbox = new QDoubleSpinBox(); 
	QObject::connect(
			ROI_range_spinbox, SIGNAL(valueChanged(double)),
			this, SLOT(handle_range_changed(double)));
	ROI_range_spinbox->setRange(0,25000);
	ROI_range_spinbox->setValue(0);
    CoRegCfgTableWidget->setCellWidget(
			row_num, 
			Range, 
			ROI_range_spinbox);
}


void
GPlatesQtWidgets::CoRegLayerConfigurationDialog::setup_data_operator_combobox(
		const QString& attribute_name,	
		QComboBox* combo)
{
	GPlatesDataMining::AttributeTypeEnum a_type = GPlatesDataMining::Unknown_Type;
	if(d_attr_name_type_map.find(attribute_name) != d_attr_name_type_map.end())
	{
		a_type = d_attr_name_type_map.find(attribute_name)->second;
	}

	if(GPlatesDataMining::String_Attribute == a_type || GPlatesDataMining::Unknown_Type == a_type)
	{
		combo->addItem(
				QApplication::tr("Lookup"), 
				GPlatesDataMining::DATA_OPERATOR_LOOKUP);
		combo->addItem(
				QApplication::tr("Vote"),
				GPlatesDataMining::DATA_OPERATOR_VOTE);
	}

	if(GPlatesDataMining::Number_Attribute == a_type || GPlatesDataMining::Unknown_Type == a_type)
	{
		combo->addItem(
				QApplication::tr("Min"),
				GPlatesDataMining::DATA_OPERATOR_MIN);
		combo->addItem(
				QApplication::tr("Max"),
				GPlatesDataMining::DATA_OPERATOR_MAX);
		combo->addItem(
				QApplication::tr("Mean"),
				GPlatesDataMining::DATA_OPERATOR_MEAN);
		combo->addItem(
				QApplication::tr("Median"),
				GPlatesDataMining::DATA_OPERATOR_MEDIAN);
	}
	return;
}


void
GPlatesQtWidgets::CoRegLayerConfigurationDialog::setup_association_type_combobox(
			QComboBox* combo)
{
	combo->addItem(
			QApplication::tr("Region of Interest"),
			GPlatesDataMining::REGION_OF_INTEREST);
	combo->addItem(
			QApplication::tr("TODO..."),
			GPlatesDataMining::REGION_OF_INTEREST);
	return;
}


void
GPlatesQtWidgets::CoRegLayerConfigurationDialog::apply(
		QAbstractButton* button)
{
	CoRegCfgTable.clear(); //Clean up the table each time applied.
	
	if(buttonBox->buttonRole(button) != QDialogButtonBox::ApplyRole)
	{
		return;
	}

	int row_num = CoRegCfgTableWidget->rowCount();

	for(int i = 0; i < row_num; i++)
	{
		FeatureCollectionTableItem* feature_collection_item = 
			dynamic_cast< FeatureCollectionTableItem* > (
					CoRegCfgTableWidget->item(i, FeatureCollectionName) );
		AttributeTableItem* attr_item = 
			dynamic_cast< AttributeTableItem* > (
					CoRegCfgTableWidget->item(i, AttributeName) );
		QComboBox* data_operator = 
			dynamic_cast< QComboBox* >(
					CoRegCfgTableWidget->cellWidget(i, DataOperator) );
		QDoubleSpinBox* spinbox_ROI_range = 
			dynamic_cast< QDoubleSpinBox* >(
					CoRegCfgTableWidget->cellWidget(i,Range) );
		
		if( !(	feature_collection_item &&
				attr_item				&&
				data_operator			&&
				spinbox_ROI_range) )
		{
			qWarning() << "Invalid input table item found! Skip this iteration";
			continue;
		}
		
		QString operator_name = data_operator->currentText();
		GPlatesDataMining::DataOperatorType op = 
			static_cast<GPlatesDataMining::DataOperatorType>(
					data_operator->itemData(data_operator->currentIndex()).toUInt());

		GPlatesDataMining::ConfigurationTableRow row;

		row.target_feature_collection_handle = 
				feature_collection_item->file_ref.get_file().get_feature_collection();
 
		//TODO: TO BE IMPLEMENTED
		row.association_operator_type = GPlatesDataMining::REGION_OF_INTEREST;

		row.association_parameters.d_ROI_range = spinbox_ROI_range->value();
		row.attribute_name = attr_item->text();
		row.attr_type = attr_item->attr_type;
		row.data_operator_type = op;

		CoRegCfgTable.push_back(row);	

		CoRegCfgTable.export_path() = ExportPathlineEdit->text();
		//TODO:
		//Validate the path here.
		if(CoRegCfgTable.export_path().isEmpty())
		{
			qWarning() << "The export path is invalid";
			return;
		}
	}
	CoRegCfgTable.set_seeds_file(get_input_seed_files()); 
	done(QDialog::Accepted);
}


void
GPlatesQtWidgets::CoRegLayerConfigurationDialog::react_choose_export_path()
{
	QString path = d_open_directory_dialog.get_existing_directory();
	
	if(!path.isEmpty())
	{
		update_export_path(path);
	}
}


void
GPlatesQtWidgets::CoRegLayerConfigurationDialog::update_export_path(
		const QString& path)
{
	//Check the directory info.
	QFileInfo export_dir_info(path);
	
	if (export_dir_info.exists() && 
		export_dir_info.isDir() && 
		export_dir_info.isWritable()) 
	{		 		
		ExportPathlineEdit->setText(path);
	}
	else
	{
		qWarning() << "The export path is invalid.";
	}
	return;
}


void
GPlatesQtWidgets::CoRegLayerConfigurationDialog::check_integrity()
{
	bool flag = false;

	for(int i = 0; i < CoRegCfgTableWidget->rowCount(); )
	{
		FeatureCollectionTableItem* feature_collection_item = 
			dynamic_cast< FeatureCollectionTableItem* > (
					CoRegCfgTableWidget->item(i, FeatureCollectionName) );
		int list_size = FeatureCollectionListWidget->count();
		for(int j = 0; j < list_size; j++ ) // make sure the items in cfg table are still valid.
		{
			FeatureCollectionItem* item = 
				dynamic_cast< FeatureCollectionItem* > (FeatureCollectionListWidget->item(j));
			if(item->file_ref == feature_collection_item->file_ref)
			{
				flag = true;
				break;
			}
		}
		if(!flag)
		{
			CoRegCfgTableWidget->removeRow(i);
		}
		else
		{
			flag = false;
			i++;
		}
	}
	return;
}

// Suppress warning with boost::variant with Boost 1.34 and g++ 4.2.
// This is here at the end of the file because the problem resides in a template
// being instantiated at the end of the compilation unit.
DISABLE_GCC_WARNING("-Wshadow")

