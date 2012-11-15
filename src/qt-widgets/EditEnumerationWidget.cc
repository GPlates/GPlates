/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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

#include <QString>
#include <QStringList>

#include "EditEnumerationWidget.h"

#include "UninitialisedEditWidgetException.h"

#include "property-values/Enumeration.h"
#include "property-values/EnumerationContent.h"
#include "property-values/EnumerationType.h"

#include "model/Gpgim.h"
#include "model/GpgimEnumerationType.h"
#include "model/ModelUtils.h"

#include "utils/UnicodeStringUtils.h"

#include <boost/foreach.hpp>

namespace
{
	/**
	 * Query GPGIM to see if the specified property value type is a recognised enumeration type.
	 */
	bool
	is_property_value_type_handled(
			const GPlatesPropertyValues::StructuralType &property_value_type,
			const GPlatesModel::Gpgim &gpgim)
	{
		const GPlatesModel::Gpgim::property_enumeration_type_seq_type &gpgim_property_enumeration_types =
				gpgim.get_property_enumeration_types();

		// Compare the property value type (enumeration type) with those listed in the GPGIM.
		BOOST_FOREACH(
				const GPlatesModel::GpgimEnumerationType::non_null_ptr_to_const_type &gpgim_property_enumeration_type,
				gpgim_property_enumeration_types)
		{
			if (property_value_type == gpgim_property_enumeration_type->get_structural_type())
			{
				return true;
			}
		}

		return false;
	}

	/**
	 * Retrieve the list of allowed enumeration values for the specified property (enumeration) type.
	 */
	const QStringList
	get_enumeration_string_list(
			const GPlatesPropertyValues::StructuralType &property_value_type,
			const GPlatesModel::Gpgim &gpgim)
	{
		QStringList enum_value_list;

		// Get the GPGIM enumeration type.
		boost::optional<GPlatesModel::GpgimEnumerationType::non_null_ptr_to_const_type>
				gpgim_property_enumeration_type =
						gpgim.get_property_enumeration_type(property_value_type);
		if (gpgim_property_enumeration_type)
		{
			const GPlatesModel::GpgimEnumerationType::content_seq_type &enum_contents =
					gpgim_property_enumeration_type.get()->get_contents();

			// Add each allowed enumeration value to the list.
			BOOST_FOREACH(const GPlatesModel::GpgimEnumerationType::Content &enum_content, enum_contents)
			{
				enum_value_list.append(enum_content.value);
			}
		}

		return enum_value_list;
	}
}


GPlatesQtWidgets::EditEnumerationWidget::EditEnumerationWidget(
		const GPlatesModel::Gpgim &gpgim,
		QWidget *parent_):
	AbstractEditWidget(parent_),
	d_gpgim(gpgim)
{
	setupUi(this);
	reset_widget_to_default_values();
	
	QObject::connect(combobox_enumeration, SIGNAL(activated(int)),
			this, SLOT(handle_combobox_change()));
	
	label_value->setHidden(true);
	declare_default_label(label_value);
	setFocusProxy(combobox_enumeration);
}


void
GPlatesQtWidgets::EditEnumerationWidget::configure_for_property_value_type(
		const GPlatesPropertyValues::StructuralType &property_value_type)
{
	if (!is_property_value_type_handled(property_value_type, d_gpgim))
	{
		throw PropertyValueNotSupportedException(GPLATES_EXCEPTION_SOURCE);
	}

	d_property_value_type = property_value_type;
	combobox_enumeration->clear();
	combobox_enumeration->addItems(get_enumeration_string_list(d_property_value_type.get(), d_gpgim));
}


void
GPlatesQtWidgets::EditEnumerationWidget::reset_widget_to_default_values()
{
	d_enumeration_ptr = NULL;
	combobox_enumeration->clear();
	if (d_property_value_type)
	{
		combobox_enumeration->addItems(
				get_enumeration_string_list(d_property_value_type.get(), d_gpgim));
	}
	set_clean();
}


void
GPlatesQtWidgets::EditEnumerationWidget::update_widget_from_enumeration(
		GPlatesPropertyValues::Enumeration &enumeration)
{
	d_enumeration_ptr = &enumeration;
	// Get the type of Enumeration to use from the Enumeration property value.
	const GPlatesPropertyValues::StructuralType enum_type(enumeration.type());
	configure_for_property_value_type(enum_type);
	
	QString enum_value = GPlatesUtils::make_qstring_from_icu_string(
			enumeration.value().get());
	int enum_index = combobox_enumeration->findText(enum_value);
	if (enum_index != -1) {
		// Present the user with the current enumeration value.
		combobox_enumeration->setCurrentIndex(enum_index);
	} else {
		// Found a value we're not supposed to have for this enumeration type.
		// Add it because The User Is Always Right!
		combobox_enumeration->addItem(enum_value);
		combobox_enumeration->setCurrentIndex(combobox_enumeration->count() - 1);
	}
	set_clean();
}


GPlatesModel::PropertyValue::non_null_ptr_type
GPlatesQtWidgets::EditEnumerationWidget::create_property_value_from_widget() const
{
	if (!d_property_value_type ||
		!is_property_value_type_handled(d_property_value_type.get(), d_gpgim))
	{
		throw PropertyValueNotSupportedException(GPLATES_EXCEPTION_SOURCE);
	}

	const QString value = combobox_enumeration->currentText();

	GPlatesModel::PropertyValue::non_null_ptr_type property_value = 
			GPlatesPropertyValues::Enumeration::create(
					GPlatesPropertyValues::EnumerationType(d_property_value_type.get()),
					GPlatesUtils::make_icu_string_from_qstring(value));

	return property_value;
}


bool
GPlatesQtWidgets::EditEnumerationWidget::update_property_value_from_widget()
{
	// Remember that the property value pointer may be NULL!
	if (d_enumeration_ptr.get() != NULL) {
		if (is_dirty()) {
			const QString value = combobox_enumeration->currentText();
			d_enumeration_ptr->set_value(GPlatesPropertyValues::EnumerationContent(
					GPlatesUtils::make_icu_string_from_qstring(value)));
			set_clean();
			return true;
		} else {
			return false;
		}
	} else {
		throw UninitialisedEditWidgetException(GPLATES_EXCEPTION_SOURCE);
	}
}

