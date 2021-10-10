/* $Id$ */

/** 
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008 Geological Survey of Norway
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

#include "ShapefileAttributeWidget.h"

#include "ShapefilePropertyMapper.h"

namespace
{
	void
	display_qmap(
		QMap< QString,QString > map)
	{
		QMap< QString, QString>::const_iterator it = map.begin();
		
		for ( ; it != map.end() ; ++it)
		{
			std::cerr << it.key().toStdString().c_str() << " " << it.value().toStdString().c_str() << std::endl;
		}
	}

	void
	display_field_names(
		QStringList names)
	{
		QStringList::const_iterator it = names.begin();
		 
		for ( ; it != names.end(); ++it)
		{
			std::cerr << (*it).toStdString().c_str() << std::endl;
		}
	}

	/**
	 * Fills the QStringList default_fields with field names from the list of default_attributes 
	 * defined in "ShapefilePropertyMapper.h"
	 */
	void
	fill_fields_from_default_list(
		QStringList &default_fields)
	{
		default_fields.insert(ShapefileAttributes::PLATEID,ShapefileAttributes::default_attributes[ShapefileAttributes::PLATEID]);
		default_fields.insert(ShapefileAttributes::FEATURE_TYPE,ShapefileAttributes::default_attributes[ShapefileAttributes::FEATURE_TYPE]);
		default_fields.insert(ShapefileAttributes::BEGIN,ShapefileAttributes::default_attributes[ShapefileAttributes::BEGIN]);
		default_fields.insert(ShapefileAttributes::END,ShapefileAttributes::default_attributes[ShapefileAttributes::END]);
		default_fields.insert(ShapefileAttributes::NAME,ShapefileAttributes::default_attributes[ShapefileAttributes::NAME]);
		default_fields.insert(ShapefileAttributes::DESCRIPTION,ShapefileAttributes::default_attributes[ShapefileAttributes::DESCRIPTION]);
	}

	void
	fill_fields_from_qmap(
		QStringList &default_fields,
		const QMap<QString,QString> &model_to_attribute_map,
		const QStringList &field_names,
		int field_index)
	{
		QMap<QString,QString>::const_iterator it = 
			model_to_attribute_map.constFind(ShapefileAttributes::model_properties[field_index]);

	//	display_field_names(field_names);	
	//	display_qmap(model_to_attribute_map);


		if (it == model_to_attribute_map.constEnd())
		{
			// We didn't find an entry for the current model property in the map, so use the default field name. 
			default_fields.insert(field_index,
					ShapefileAttributes::default_attributes[field_index]);
		}
		else{

			// We found an entry for the current model property in the map. 

			if (field_names.contains(it.value()))
			{
				// The map's attribute field was found in the shapefile attribute list. 
				default_fields.insert(field_index,it.value());
			}
			else
			{
				// The map's attribute field wasn't found amongst the shapefile's attributes, so
				// use the default field name. 
				default_fields.insert(field_index,
					ShapefileAttributes::default_attributes[field_index]);
			}
		}		
	}
}

GPlatesQtWidgets::ShapefileAttributeWidget::ShapefileAttributeWidget(
		QWidget *parent_,
		QString &filename,
		QStringList &field_names,
		QMap<QString,QString> &model_to_attribute_map,
		bool remapping
):
	QWidget(parent_),
	d_filename(filename),
	d_field_names(field_names),
	d_model_to_attribute_map(model_to_attribute_map)
{
	setupUi(this);

//	display_field_names(field_names);	
//	display_qmap(model_to_attribute_map);

	// The "Feature Type" property cannot be remapped. 
	combo_feature_type->setEnabled(!remapping);

	if (d_model_to_attribute_map.isEmpty())
	{
		// The map didn't provide is with any default fields for the combo boxes, so fill the "default_fields" QStringList with
		// fields from the default list defined in "ShapefilePropertyMapper.h"
		fill_fields_from_default_list(d_default_fields);
	}
	else
	{
		// The map does provide us with default fields for the combo boxes. Use these where we can. 
		fill_fields_from_qmap(d_default_fields,d_model_to_attribute_map,d_field_names,ShapefileAttributes::PLATEID);
		fill_fields_from_qmap(d_default_fields,d_model_to_attribute_map,d_field_names,ShapefileAttributes::FEATURE_TYPE);
		fill_fields_from_qmap(d_default_fields,d_model_to_attribute_map,d_field_names,ShapefileAttributes::BEGIN);
		fill_fields_from_qmap(d_default_fields,d_model_to_attribute_map,d_field_names,ShapefileAttributes::END);
		fill_fields_from_qmap(d_default_fields,d_model_to_attribute_map,d_field_names,ShapefileAttributes::NAME);
		fill_fields_from_qmap(d_default_fields,d_model_to_attribute_map,d_field_names,ShapefileAttributes::DESCRIPTION);
	}

	setup();
}

void
GPlatesQtWidgets::ShapefileAttributeWidget::setup()
{
// Insert the filename.
	line_filename->setText(d_filename);

// Fill the drop down boxes with the attribute names from the shapefile. 
	
	//The first field of each combo box will be "<none>".
	combo_plateID->addItem(QString(QObject::tr("<none>")));
	combo_feature_type->addItem(QString(QObject::tr("<none>")));
	combo_from_age->addItem(QString(QObject::tr("<none>")));
	combo_to_age->addItem(QString(QObject::tr("<none>")));
	combo_name->addItem(QString(QObject::tr("<none>")));
	combo_description->addItem(QString(QObject::tr("<none>")));

	// Fill the remaining fields from the QStringList d_field_names.
	combo_plateID->addItems(d_field_names);
	combo_feature_type->addItems(d_field_names);
	combo_from_age->addItems(d_field_names);
	combo_to_age->addItems(d_field_names);
	combo_name->addItems(d_field_names);
	combo_description->addItems(d_field_names);

	// Check for any of the default field names. If we find any, set the combo box to the default.
	// 1 is added to the index to account for the <none> item, which is 0th in each combo box. 
	int index; 

	if ( (index = d_field_names.indexOf(d_default_fields[ShapefileAttributes::PLATEID])) != -1) {	
		combo_plateID->setCurrentIndex(index + 1);
	}

	if ( (index = d_field_names.indexOf(d_default_fields[ShapefileAttributes::FEATURE_TYPE])) != -1) {	
		combo_feature_type->setCurrentIndex(index + 1);
	}

	if ( (index = d_field_names.indexOf(d_default_fields[ShapefileAttributes::BEGIN])) != -1) {	
		combo_from_age->setCurrentIndex(index + 1);
	}

	if ( (index = d_field_names.indexOf(d_default_fields[ShapefileAttributes::END])) != -1) {	
		combo_to_age->setCurrentIndex(index + 1);
	}

	if ( (index = d_field_names.indexOf(d_default_fields[ShapefileAttributes::NAME])) != -1) {	
		combo_name->setCurrentIndex(index + 1);
	}

	if ( (index = d_field_names.indexOf(d_default_fields[ShapefileAttributes::DESCRIPTION])) != -1) {	
		combo_description->setCurrentIndex(index + 1);
	}


// save the state of the combo boxes so that we can reset them.
	d_combo_reset_map.clear();
	d_combo_reset_map.push_back(combo_plateID->currentIndex());
	d_combo_reset_map.push_back(combo_feature_type->currentIndex());
	d_combo_reset_map.push_back(combo_from_age->currentIndex());
	d_combo_reset_map.push_back(combo_to_age->currentIndex());
	d_combo_reset_map.push_back(combo_name->currentIndex());
	d_combo_reset_map.push_back(combo_description->currentIndex());

}

void
GPlatesQtWidgets::ShapefileAttributeWidget::reset_fields()
{
// reset all the fields to their default values

	combo_plateID->setCurrentIndex(d_combo_reset_map[ShapefileAttributes::PLATEID]);
	combo_feature_type->setCurrentIndex(d_combo_reset_map[ShapefileAttributes::FEATURE_TYPE]);
	combo_from_age->setCurrentIndex(d_combo_reset_map[ShapefileAttributes::BEGIN]);
	combo_to_age->setCurrentIndex(d_combo_reset_map[ShapefileAttributes::END]);
	combo_name->setCurrentIndex(d_combo_reset_map[ShapefileAttributes::NAME]);
	combo_description->setCurrentIndex(d_combo_reset_map[ShapefileAttributes::DESCRIPTION]);


}

void
GPlatesQtWidgets::ShapefileAttributeWidget::accept_fields()
{

// Use the state of the combo boxes to fill the model_to_attribute_map.
	
		d_model_to_attribute_map.clear();

		// A zero in the current index corresponds to <none>, in which case we don't save this in the map.
		// If we have a non-zero index, subtract 1 to correct for the <none> field. 
		if (combo_plateID->currentIndex() != 0){
			d_model_to_attribute_map.insert(
				ShapefileAttributes::model_properties[ShapefileAttributes::PLATEID],
						d_field_names[combo_plateID->currentIndex()-1]);
		}

		if (combo_feature_type->currentIndex() != 0){
			d_model_to_attribute_map.insert(
				ShapefileAttributes::model_properties[ShapefileAttributes::FEATURE_TYPE],
						d_field_names[combo_feature_type->currentIndex()-1]);
		}

		if (combo_from_age->currentIndex() != 0){
			d_model_to_attribute_map.insert(
				ShapefileAttributes::model_properties[ShapefileAttributes::BEGIN],
						d_field_names[combo_from_age->currentIndex()-1]);
		}

		if (combo_to_age->currentIndex() != 0){
			d_model_to_attribute_map.insert(
				ShapefileAttributes::model_properties[ShapefileAttributes::END],
						d_field_names[combo_to_age->currentIndex()-1]);
		}

		if (combo_name->currentIndex() != 0){
			d_model_to_attribute_map.insert(
				ShapefileAttributes::model_properties[ShapefileAttributes::NAME],
						d_field_names[combo_name->currentIndex()-1]);
		}

		if (combo_description->currentIndex() != 0){
			d_model_to_attribute_map.insert(
				ShapefileAttributes::model_properties[ShapefileAttributes::DESCRIPTION],
						d_field_names[combo_description->currentIndex()-1]);
		}

}

