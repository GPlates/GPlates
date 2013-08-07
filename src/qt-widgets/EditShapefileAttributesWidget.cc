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

#include <QTableWidgetItem>
#include <QVariant>

#include "feature-visitors/ToQvariantConverter.h"
#include "property-values/GpmlKeyValueDictionary.h"
#include "property-values/GpmlKeyValueDictionaryElement.h"
#include "property-values/XsDouble.h"
#include "property-values/XsInteger.h"
#include "property-values/XsString.h"

#include "EditShapefileAttributesWidget.h"
#include "UninitialisedEditWidgetException.h"

namespace {

	enum KeyValueColumnLayout{
		COLUMN_KEY, COLUMN_TYPE, COLUMN_VALUE
	};

	QVariant
	get_qvariant_from_element(
		const GPlatesPropertyValues::GpmlKeyValueDictionaryElement &element)
	{
		GPlatesFeatureVisitors::ToQvariantConverter converter;

		element.value()->accept_visitor(converter);

		if (converter.found_values_begin() != converter.found_values_end())
		{
			return *converter.found_values_begin();
		}
		else
		{
			return QVariant();
		}
	}

	QString
	get_type_qstring_from_qvariant(
		QVariant &variant)
	{
		switch (variant.type())
		{
		case QVariant::Int:
			return QString("integer");
			break;
		case QVariant::Double:
			return QString("double");
			break;
		case QVariant::String:
			return QString("string");
			break;
		default:
			return QString();
		}
	}

}

GPlatesQtWidgets::EditShapefileAttributesWidget::EditShapefileAttributesWidget(
	QWidget *parent_):
	AbstractEditWidget(parent_)
{
	setupUi(this);

	QObject::connect(table_elements,SIGNAL(cellChanged(int,int)),
			this,SLOT(handle_cell_changed(int,int)));
}

void
GPlatesQtWidgets::EditShapefileAttributesWidget::reset_widget_to_default_values()
{

}

GPlatesModel::PropertyValue::non_null_ptr_type
GPlatesQtWidgets::EditShapefileAttributesWidget::create_property_value_from_widget() const
{
	return GPlatesPropertyValues::GpmlKeyValueDictionary::create();			
}

bool
GPlatesQtWidgets::EditShapefileAttributesWidget::update_property_value_from_widget()
{
	// FIXME: consider if we want the mapped model property corresponding
	// to the shapefile attribute, if such a mapped property exists, to be updated automatically.
	//
	// The user can always do this manually by re-mapping via the ManageFeatureCollections dialog,
	// and perhaps it's reasonable to leave it at that. 
	//
	// Bear in mind that this key_value_dictionary may have been read from a gpml file, and
	// so no mapping information would exist anyway.

	if (d_key_value_dictionary_ptr.get() != NULL) {
		if (is_dirty()) {
			
			const int row = table_elements->currentRow();
			const int column = table_elements->currentColumn();

			std::vector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement> dictionary_elements =
					d_key_value_dictionary_ptr->get_elements();
			GPlatesPropertyValues::GpmlKeyValueDictionaryElement &dictionary_element = 
				dictionary_elements.at(row);


			static const GPlatesPropertyValues::StructuralType string_type =
				GPlatesPropertyValues::StructuralType::create_xsi("string");
			static const GPlatesPropertyValues::StructuralType integer_type =
				GPlatesPropertyValues::StructuralType::create_xsi("integer");
			static const GPlatesPropertyValues::StructuralType double_type =
				GPlatesPropertyValues::StructuralType::create_xsi("double");

			// Get the existing key and type. 
			GPlatesPropertyValues::XsString::non_null_ptr_type key = dictionary_element.key();
			GPlatesPropertyValues::StructuralType type = dictionary_element.value_type();

			QString item_string = table_elements->item(row,column)->text();	

			bool field_is_valid = true;

			// Check what type we have, and create the appropriate element. 
			//
			// FIXME: There are possibly better ways to do this, by adding suitable functionality
			// to the KeyValueDictionaryElement class to allow setting members for example...?
			if (type == string_type)
			{
				GPlatesPropertyValues::XsString::non_null_ptr_type value = 
					GPlatesPropertyValues::XsString::create(
							GPlatesUtils::make_icu_string_from_qstring(item_string));
				GPlatesPropertyValues::GpmlKeyValueDictionaryElement new_element(
					key,
					value,
					type);
				dictionary_element = new_element;
			}
			else if (type == integer_type)
			{
				int new_value = item_string.toInt(&field_is_valid);
				if (field_is_valid){
					GPlatesPropertyValues::XsInteger::non_null_ptr_type value = 
						GPlatesPropertyValues::XsInteger::create(new_value);
					GPlatesPropertyValues::GpmlKeyValueDictionaryElement new_element(
						key,
						value,
						type);
					dictionary_element = new_element;
				}
			}
			else if (type == double_type)
			{
				double new_value = item_string.toDouble(&field_is_valid);
				if (field_is_valid)
				{
					GPlatesPropertyValues::XsDouble::non_null_ptr_type value = 
						GPlatesPropertyValues::XsDouble::create(new_value);
					GPlatesPropertyValues::GpmlKeyValueDictionaryElement new_element(
						key,
						value,
						type);
					dictionary_element = new_element;
				}
			}

			d_key_value_dictionary_ptr->set_elements(dictionary_elements);
		
			if (!field_is_valid){
				// An invalid field was entered, so reset the cell to the value in the dictionary element.
				QString value_string = get_qvariant_from_element(dictionary_element).toString();
				QTableWidgetItem *value_item = new QTableWidgetItem(value_string);
				value_item->setFlags(value_item->flags() | Qt::ItemIsEditable);
				table_elements->setItem(row,COLUMN_VALUE,value_item); 	
			}

			set_clean();
	
			return true;
		} else {
			return false;
		}
	} else {
		throw UninitialisedEditWidgetException(GPLATES_EXCEPTION_SOURCE);
	}
}

void
GPlatesQtWidgets::EditShapefileAttributesWidget::update_widget_from_key_value_dictionary(
		GPlatesPropertyValues::GpmlKeyValueDictionary &gpml_key_value_dictionary)
{
	d_key_value_dictionary_ptr = &gpml_key_value_dictionary;

	table_elements->clearContents();
	table_elements->setRowCount(gpml_key_value_dictionary.get_elements().size());

	std::vector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement>::const_iterator
		 it = gpml_key_value_dictionary.get_elements().begin(),
		 end = gpml_key_value_dictionary.get_elements().end();

	for (int row = 0 ; it != end ; ++it, ++row)
	{

		QString key_string = GPlatesUtils::make_qstring_from_icu_string(it->key()->get_value().get());
		QVariant value_variant = get_qvariant_from_element(*it);

		// Key field.
		QTableWidgetItem *key_string_item = new QTableWidgetItem(key_string);

		// Make this field non-editable. 
		key_string_item->setFlags(key_string_item->flags() & ~Qt::ItemIsEditable);

		table_elements->setItem(row,COLUMN_KEY, key_string_item);	

		// Value-type field.
		QString type_string = get_type_qstring_from_qvariant(value_variant);
		QTableWidgetItem *type_item = new QTableWidgetItem(type_string);

		// Make this field non-editable.
		type_item->setFlags(type_item->flags() & ~Qt::ItemIsEditable);

		table_elements->setItem(row,COLUMN_TYPE, type_item);

		// Value field.
		QTableWidgetItem *value_item = new QTableWidgetItem(value_variant.toString());

		// Make sure this field is editable.
		value_item->setFlags(value_item->flags() | Qt::ItemIsEditable);
		table_elements->setItem(row,COLUMN_VALUE,value_item);

	}


	set_clean();
}

void
GPlatesQtWidgets::EditShapefileAttributesWidget::handle_cell_changed(
	int row, int column)
{
	// We are only interested in the value field, and indeed this should be the only field that
	// is editable. 
	if (column != COLUMN_VALUE){
		return;
	}
 
	set_dirty();
	Q_EMIT commit_me();
}
