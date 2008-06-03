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

#include "EditOldPlatesHeaderWidget.h"

#include "property-values/GpmlOldPlatesHeader.h"
#include "utils/UnicodeStringUtils.h"
#include "UninitialisedEditWidgetException.h"


GPlatesQtWidgets::EditOldPlatesHeaderWidget::EditOldPlatesHeaderWidget(
		QWidget *parent_):
	AbstractEditWidget(parent_)
{
	setupUi(this);
	reset_widget_to_default_values();

	// Line 1
	QObject::connect(spinbox_region_number, SIGNAL(valueChanged(int)),
			this, SLOT(set_dirty()));
	QObject::connect(spinbox_reference_number, SIGNAL(valueChanged(int)),
			this, SLOT(set_dirty()));
	QObject::connect(spinbox_string_number, SIGNAL(valueChanged(int)),
			this, SLOT(set_dirty()));
	QObject::connect(lineedit_geographic_description, SIGNAL(textEdited(const QString &)),
			this, SLOT(set_dirty()));

	// Line 2
	QObject::connect(spinbox_plate_id_number, SIGNAL(valueChanged(int)),
			this, SLOT(set_dirty()));
	QObject::connect(doublespinbox_age_of_appearance, SIGNAL(valueChanged(double)),
			this, SLOT(set_dirty()));
	QObject::connect(doublespinbox_age_of_disappearance, SIGNAL(valueChanged(double)),
			this, SLOT(set_dirty()));
	QObject::connect(lineedit_data_type_code, SIGNAL(textEdited(const QString &)),
			this, SLOT(set_dirty()));
	QObject::connect(spinbox_data_type_code_number, SIGNAL(valueChanged(int)),
			this, SLOT(set_dirty()));
	QObject::connect(lineedit_data_type_code_number_additional, SIGNAL(textEdited(const QString &)),
			this, SLOT(set_dirty()));
	QObject::connect(spinbox_conjugate_plate_id_number, SIGNAL(valueChanged(int)),
			this, SLOT(set_dirty()));
	QObject::connect(spinbox_colour_code, SIGNAL(valueChanged(int)),
			this, SLOT(set_dirty()));

	setFocusPolicy(Qt::StrongFocus);
}


void
GPlatesQtWidgets::EditOldPlatesHeaderWidget::reset_widget_to_default_values()
{
	d_old_plates_header_ptr = NULL;
	// Line 1
	spinbox_region_number->setValue(0);
	spinbox_reference_number->setValue(0);
	spinbox_string_number->setValue(0);
	lineedit_geographic_description->setText("");
	
	// Line 2
	spinbox_plate_id_number->setValue(0);
	doublespinbox_age_of_appearance->setValue(0);
	doublespinbox_age_of_disappearance->setValue(0);
	lineedit_data_type_code->setText(0);
	spinbox_data_type_code_number->setValue(0);
	lineedit_data_type_code_number_additional->setText("");
	spinbox_conjugate_plate_id_number->setValue(0);
	spinbox_colour_code->setValue(0);
	label_number_of_points->setText(QString::number(0));
	
	set_clean();
}


void
GPlatesQtWidgets::EditOldPlatesHeaderWidget::update_widget_from_old_plates_header(
		GPlatesPropertyValues::GpmlOldPlatesHeader &header)
{
	d_old_plates_header_ptr = &header;
	// Line 1
	spinbox_region_number->setValue(header.region_number());
	spinbox_reference_number->setValue(header.reference_number());
	spinbox_string_number->setValue(header.string_number());
	lineedit_geographic_description->setText(
			GPlatesUtils::make_qstring_from_icu_string(header.geographic_description()));
	
	// Line 2
	spinbox_plate_id_number->setValue(header.plate_id_number());
	doublespinbox_age_of_appearance->setValue(header.age_of_appearance());
	doublespinbox_age_of_disappearance->setValue(header.age_of_disappearance());
	lineedit_data_type_code->setText(
			GPlatesUtils::make_qstring_from_icu_string(header.data_type_code()));
	spinbox_data_type_code_number->setValue(header.data_type_code_number());
	lineedit_data_type_code_number_additional->setText(
			GPlatesUtils::make_qstring_from_icu_string(header.data_type_code_number_additional()));
	spinbox_conjugate_plate_id_number->setValue(header.conjugate_plate_id_number());
	spinbox_colour_code->setValue(header.colour_code());
	label_number_of_points->setText(QString::number(header.number_of_points()));
	
	set_clean();
}


GPlatesModel::PropertyValue::non_null_ptr_type
GPlatesQtWidgets::EditOldPlatesHeaderWidget::create_property_value_from_widget() const
{
	GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type header =
			GPlatesPropertyValues::GpmlOldPlatesHeader::create(
					spinbox_region_number->value(),
					spinbox_reference_number->value(),
					spinbox_string_number->value(),
					GPlatesUtils::make_icu_string_from_qstring(lineedit_geographic_description->text()),
					spinbox_plate_id_number->value(),
					doublespinbox_age_of_appearance->value(),
					doublespinbox_age_of_disappearance->value(),
					GPlatesUtils::make_icu_string_from_qstring(lineedit_data_type_code->text()),
					spinbox_data_type_code_number->value(),
					GPlatesUtils::make_icu_string_from_qstring(lineedit_data_type_code_number_additional->text()),
					spinbox_conjugate_plate_id_number->value(),
					spinbox_colour_code->value(),
					label_number_of_points->text().toInt());
	return header;
}


bool
GPlatesQtWidgets::EditOldPlatesHeaderWidget::update_property_value_from_widget()
{
	if (d_old_plates_header_ptr.get() != NULL) {
		if (is_dirty()) {
			// FIXME: Some kind of transaction for all of this?
			// Line 1
			d_old_plates_header_ptr->set_region_number(spinbox_region_number->value());
			d_old_plates_header_ptr->set_reference_number(spinbox_reference_number->value());
			d_old_plates_header_ptr->set_string_number(spinbox_string_number->value());
			d_old_plates_header_ptr->set_geographic_description(
					GPlatesUtils::make_icu_string_from_qstring(lineedit_geographic_description->text()));
			
			// Line 2
			d_old_plates_header_ptr->set_plate_id_number(spinbox_plate_id_number->value());
			d_old_plates_header_ptr->set_age_of_appearance(doublespinbox_age_of_appearance->value());
			d_old_plates_header_ptr->set_age_of_disappearance(doublespinbox_age_of_disappearance->value());
			d_old_plates_header_ptr->set_data_type_code(
					GPlatesUtils::make_icu_string_from_qstring(lineedit_data_type_code->text()));
			d_old_plates_header_ptr->set_data_type_code_number(spinbox_data_type_code_number->value());
			d_old_plates_header_ptr->set_data_type_code_number_additional(
					GPlatesUtils::make_icu_string_from_qstring(lineedit_data_type_code_number_additional->text()));
			d_old_plates_header_ptr->set_conjugate_plate_id_number(spinbox_conjugate_plate_id_number->value());
			d_old_plates_header_ptr->set_colour_code(spinbox_colour_code->value());
			// Not strictly necessary because it cannot be edited:
			// d_old_plates_header_ptr->set_number_of_points(label_number_of_points->text().toInt());
			set_clean();
			return true;
		} else {
			return false;
		}
	} else {
		throw UninitialisedEditWidgetException();
	}
}

