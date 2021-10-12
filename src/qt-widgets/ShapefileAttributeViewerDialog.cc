/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008, 2009 Geological Survey of Norway
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

#include <boost/foreach.hpp>

#include "ShapefileAttributeViewerDialog.h"

#include "app-logic/FeatureCollectionFileState.h"

#include "feature-visitors/KeyValueDictionaryFinder.h"
#include "feature-visitors/ToQvariantConverter.h"

#include "file-io/FileInfo.h"


namespace
{
	bool
	feature_collection_contains_shapefile_attributes(
			const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection)
	{

		if (!feature_collection.is_valid())
		{
			return false;
		}
			
		GPlatesModel::FeatureCollectionHandle::const_iterator
				iter = feature_collection->begin(),
				end = feature_collection->end();

		for (; iter != end ; ++iter)
		{
			static const GPlatesModel::PropertyName shapefile_attribute_property_name =
				GPlatesModel::PropertyName::create_gpml("shapefileAttributes");

			GPlatesFeatureVisitors::KeyValueDictionaryFinder finder(shapefile_attribute_property_name);
			finder.visit_feature(iter);
			if (finder.found_key_value_dictionaries_begin() != finder.found_key_value_dictionaries_end())
			{	
				return true;
			}			
		}

		return false;
	}

	bool
	file_contains_shapefile_attributes(
	const GPlatesFileIO::File::Reference &file)
	{
		GPlatesModel::FeatureCollectionHandle::const_weak_ref feature_collection =
				file.get_feature_collection();

		if (feature_collection_contains_shapefile_attributes(feature_collection))
		{
			return true;
		}

		return false;
	}


	bool
	is_file_shapefile(
		const GPlatesFileIO::FileInfo &file_info)
	{
		return  (!QString::compare(file_info.get_qfileinfo().suffix(),"shp",Qt::CaseInsensitive));
	}

	void
	fill_header_from_feature(
			const GPlatesModel::FeatureHandle::weak_ref &feature,
		QTableWidget *table_widget)
	{
		static const GPlatesModel::PropertyName shapefile_attribute_property_name =
			GPlatesModel::PropertyName::create_gpml("shapefileAttributes");

		GPlatesFeatureVisitors::KeyValueDictionaryFinder finder(shapefile_attribute_property_name);
		finder.visit_feature(feature);
		if (finder.found_key_value_dictionaries_begin() != finder.found_key_value_dictionaries_end())
		{
			// We got a set of shapefile attributes. Set the horizontal header fields from the key values.
			GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type dictionary =
					*(finder.found_key_value_dictionaries_begin());

			std::vector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement>::const_iterator 
				iter = dictionary->elements().begin(),
				end = dictionary->elements().end();

			QStringList header_list;

			for (; iter != end; ++iter)
			{
				header_list << GPlatesUtils::make_qstring_from_icu_string(iter->key()->value().get());
				//std::cerr << GPlatesUtils::make_qstring_from_icu_string(iter->key()->value().get()).toStdString().c_str() << std::endl;
			}
			
			table_widget->setColumnCount(header_list.size());
			table_widget->setHorizontalHeaderLabels(header_list);
		}
		else
		{
			// We didn't find any shapefileattributes...what do we do? 
		}
	}

	void
	fill_row_from_feature(
			const GPlatesModel::FeatureHandle::weak_ref &feature,
			QTableWidget* table_widget,
			int row)
	{
		static const GPlatesModel::PropertyName shapefile_attribute_property_name =
			GPlatesModel::PropertyName::create_gpml("shapefileAttributes");

		GPlatesFeatureVisitors::KeyValueDictionaryFinder finder(shapefile_attribute_property_name);
		finder.visit_feature(feature);
		if (finder.found_key_value_dictionaries_begin() != finder.found_key_value_dictionaries_end())
		{
			if (finder.number_of_found_dictionaries() > 1)
			{
			// We shouldn't really have more than one set of shapefileAttributes. 
			// Do something appropriate here, such as chiding the user.
			}
			GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type dictionary =
				*finder.found_key_value_dictionaries_begin();

			// Loop over the dictionary elements.
			std::vector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement>::const_iterator 
				iter = dictionary->elements().begin(),
				end = dictionary->elements().end();

			for (int column = 0; iter != end ; ++iter, ++ column)
			{
				GPlatesFeatureVisitors::ToQvariantConverter qvariant_finder;
				iter->value()->accept_visitor(qvariant_finder);
				QString text = qvariant_finder.found_values_begin()->toString();
				QTableWidgetItem *item = new QTableWidgetItem(text);

				// Make everything non-editable for now.
				item->setFlags(item->flags() & ~Qt::ItemIsEditable);
				table_widget->setItem(row,column,item);
			}

		}
		else
		{
			// We didn't find any shapefileattributes...what do we do? Leave an empty row I think. 
		}

	}

	void
	fill_table_from_feature_collection(
			const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection,
			QTableWidget *table_widget)
	{

		if (feature_collection.is_valid())
		{
			GPlatesModel::FeatureCollectionHandle::iterator
				iter = feature_collection->begin(),
				end = feature_collection->end();

			// Run over the feature collection to find the number of features, and hence 
			// the number of rows required. (Assuming that every feature will have shapefile attributes). 

			int num_rows; 
			for (num_rows = 0; iter != end ; ++iter, ++num_rows)
			{}

			table_widget->setRowCount(num_rows);

			iter = feature_collection->begin();

			fill_header_from_feature((*iter)->reference(),table_widget);

			for (int row = 0; iter != end ; ++iter, ++row)
			{
				fill_row_from_feature((*iter)->reference(),table_widget,row);
			}

		}
		
	}
}

GPlatesQtWidgets::ShapefileAttributeViewerDialog::ShapefileAttributeViewerDialog(
		GPlatesAppLogic::FeatureCollectionFileState &file_state,
		QWidget *parent_):
	QDialog(parent_, Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint)
{
	setupUi(this);

	connect_feature_collection_file_state_signals(file_state);

	QObject::connect(combo_feature_collections,SIGNAL(currentIndexChanged(int)),
					this,SLOT(handle_feature_collection_changed(int)));

}

void
GPlatesQtWidgets::ShapefileAttributeViewerDialog::update(
		GPlatesAppLogic::FeatureCollectionFileState &file_state)
{
	if (!isVisible())
	{
		return;
	}

	// Update the combo box with currently loaded shapefile feature collections, and
	// update the table if necessary. 
	
	combo_feature_collections->clear();
	d_file_vector.clear();

	const std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference> loaded_files =
			file_state.get_loaded_files();
	BOOST_FOREACH(
			const GPlatesAppLogic::FeatureCollectionFileState::file_reference &file_ref,
			loaded_files)
	{
#if 0
		// Get the file name, check if it's a shapefile; if so, add it to the combobox. 
		if (is_file_shapefile(*it))
		{
			combo_feature_collections->addItem(file_ref.get_file().get_file_info().get_qfileinfo().fileName());
			d_fileinfo_vector.push_back(*it);
		}
#endif
		// Get the file name, check if its feature collection contains shapefile attributes ; 
		// if so, add it to the combobox. 
		if (file_contains_shapefile_attributes(file_ref.get_file()))
		{
			combo_feature_collections->addItem(
					file_ref.get_file().get_file_info().get_qfileinfo().fileName());

			d_file_vector.push_back(&file_ref.get_file());
		}


	}

	update_table();
}

void
GPlatesQtWidgets::ShapefileAttributeViewerDialog::update_table()
{
	// Check the active file-info
	int index = combo_feature_collections->currentIndex();

	table_attributes->clear();
	table_attributes->setRowCount(0);

	if (index >= 0 && index < static_cast<int>(d_file_vector.size()))
	{
		const GPlatesFileIO::File::Reference *file = d_file_vector.at(index);

		//std::cerr << index << " " << file.get_file_info().get_qfileinfo().fileName().toStdString().c_str() << std::endl;

		GPlatesModel::FeatureCollectionHandle::weak_ref feature_collection =
				file->get_feature_collection();
		fill_table_from_feature_collection(feature_collection,table_attributes);

	}
}

void
GPlatesQtWidgets::ShapefileAttributeViewerDialog::handle_feature_collection_changed(
	int index)
{
	update_table();
}


void	
GPlatesQtWidgets::ShapefileAttributeViewerDialog::connect_feature_collection_file_state_signals(
		GPlatesAppLogic::FeatureCollectionFileState &file_state)
{
	// Update when any changes to the file state occur.
	QObject::connect(
			&file_state,
			SIGNAL(file_state_changed(
					GPlatesAppLogic::FeatureCollectionFileState &)),
			this,
			SLOT(update(
					GPlatesAppLogic::FeatureCollectionFileState &)));
}
