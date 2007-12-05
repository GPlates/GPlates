/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2007 Geological Survey of Norway
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
#include <iostream>

#include <QString>

#include "model/FeatureCollectionHandle.h"
#include "model/PropertyContainer.h"
#include "model/XmlAttributeName.h"
#include "model/XmlAttributeValue.h"
#include "utils/UnicodeStringUtils.h"

#include "ShapefileAttributeMapperDialog.h"


GPlatesQtWidgets::ShapefileAttributeMapperDialog::ShapefileAttributeMapperDialog(
		QWidget *parent_):
	QDialog(parent_)
{
	setupUi(this);

	connect(shapefile_combo_box,SIGNAL(currentIndexChanged(int)),this,SLOT(shapefileChanged(int)));

}

void
GPlatesQtWidgets::ShapefileAttributeMapperDialog::setup(std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref>& collections)
{
	d_collections = collections;
// fill the combo boxes with 0 index, and the attributes of the 0th shapefile. 
	shapefile_combo_box->clear();
	attribute_combo_box->clear();

	int n = d_collections.size();
	for(int count = 0; count < n; count++)
	{
		QString str;
		str.setNum(count);
		shapefile_combo_box->insertItem(count,str);
	}

#if 0
	// get the first feature collection for the intial display
	std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref>::iterator iter = d_collections.begin();
	GPlatesModel::FeatureCollectionHandle::weak_ref feature_collection = *iter;

	// get the first feature in that collection
	GPlatesModel::FeatureCollectionHandle::features_iterator feature_iter = feature_collection->features_begin();

	// iterate over the properties, and find which, if any, have an XmlAttribute identifying the 
	// attribute as one of shapefile-origin. 
	GPlatesModel::FeatureHandle::properties_iterator p_iter = (*feature_iter)->properties_begin();
	GPlatesModel::FeatureHandle::properties_iterator p_end = (*feature_iter)->properties_end();
	GPlatesModel::XmlAttributeName targetName("source");
	GPlatesModel::XmlAttributeValue targetValue("shapefile");
	for(p_iter; p_iter != p_end; p_iter++)
	{

		std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> xml_attributes = (*p_iter)->xml_attributes();
		std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue>::iterator it = xml_attributes.find(targetName);
		if(it != xml_attributes.end())
		{
			GPlatesModel::XmlAttributeValue value = it->second;
			if(value == targetValue) // we've found a shapefile attribute, so stick it in the box.
			{
				std::cerr << "found shapefile attribute" << std::endl;
				GPlatesModel::PropertyName name = (*p_iter)->property_name();
				QString string = GPlatesUtils::make_qstring_from_icu_string(name.get());
				attribute_combo_box->addItem(string);
			}
		}
	
	}
#endif

}

void
GPlatesQtWidgets::ShapefileAttributeMapperDialog::shapefileChanged(int file_number)
{
	attribute_combo_box->clear();

	if(d_collections.empty()) return;
	if(file_number > d_collections.size()) return;

	GPlatesModel::FeatureCollectionHandle::weak_ref feature_collection = d_collections[file_number];

	// get the first feature in that collection
	GPlatesModel::FeatureCollectionHandle::features_iterator feature_iter = feature_collection->features_begin();

	// iterate over the properties, and find which, if any, have an XmlAttribute identifying the 
	// attribute as one of shapefile-origin. 
	GPlatesModel::FeatureHandle::properties_iterator p_iter = (*feature_iter)->properties_begin();
	GPlatesModel::FeatureHandle::properties_iterator p_end = (*feature_iter)->properties_end();
	GPlatesModel::XmlAttributeName targetName("source");
	GPlatesModel::XmlAttributeValue targetValue("shapefile");
	for(p_iter; p_iter != p_end; p_iter++)
	{

		std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> xml_attributes = (*p_iter)->xml_attributes();
		std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue>::iterator it = xml_attributes.find(targetName);
		if(it != xml_attributes.end())
		{
			GPlatesModel::XmlAttributeValue value = it->second;
			if(value == targetValue) // we've found a shapefile attribute, so stick it in the box.
			{
				std::cerr << "found shapefile attribute" << std::endl;
				GPlatesModel::PropertyName name = (*p_iter)->property_name();
				QString string = GPlatesUtils::make_qstring_from_icu_string(name.get());
				attribute_combo_box->addItem(string);
			}
		}
	
	}
}

void
GPlatesQtWidgets::ShapefileAttributeMapperDialog::accept()
{
	//std::cout << "in accept..." << std::endl;
	QString selected_attribute_string = attribute_combo_box->currentText();
}

