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

#include "EditPlateIdWidget.h"

#include "property-values/GpmlPlateId.h"
#include "model/ModelUtils.h"
#include "UninitialisedEditWidgetException.h"


GPlatesQtWidgets::EditPlateIdWidget::EditPlateIdWidget(
		QWidget *parent_):
	AbstractEditWidget(parent_),
	d_null_value_permitted(false)
{
	setupUi(this);

	spinbox_plate_id->setMinimum(0);
	spinbox_plate_id->setMaximum(0x7fffffff); // Max plate ID is signed 32-bit integer.

	reset_widget_to_default_values();
	
	QObject::connect(spinbox_plate_id, SIGNAL(valueChanged(int)),
			this, SLOT(set_dirty()));
	QObject::connect(button_set_to_null, SIGNAL(clicked()),
			this, SLOT(nullify()));
	QObject::connect(spinbox_plate_id, SIGNAL(valueChanged(int)),
			this, SLOT(handle_value_changed()));

	label_plate_id->setHidden(false);
	declare_default_label(label_plate_id);
	setFocusProxy(spinbox_plate_id);
}


void
GPlatesQtWidgets::EditPlateIdWidget::reset_widget_to_default_values()
{
	d_plate_id_ptr = NULL;
	spinbox_plate_id->setValue( d_null_value_permitted? -1 : 0 );
	button_set_to_null->setVisible(d_null_value_permitted);
	set_clean();
}


void
GPlatesQtWidgets::EditPlateIdWidget::update_widget_from_plate_id(
		GPlatesPropertyValues::GpmlPlateId &gpml_plate_id)
{
	d_plate_id_ptr = &gpml_plate_id;
	spinbox_plate_id->setValue(gpml_plate_id.value());
	set_clean();
}


GPlatesModel::PropertyValue::non_null_ptr_type
GPlatesQtWidgets::EditPlateIdWidget::create_property_value_from_widget() const
{
	if (is_null()) {
		throw UninitialisedEditWidgetException(GPLATES_EXCEPTION_SOURCE);
	}

	return GPlatesPropertyValues::GpmlPlateId::create(
			spinbox_plate_id->value());
}


GPlatesModel::integer_plate_id_type
GPlatesQtWidgets::EditPlateIdWidget::create_integer_plate_id_from_widget() const
{
	if (is_null()) {
		throw UninitialisedEditWidgetException(GPLATES_EXCEPTION_SOURCE);
	}

	return static_cast<GPlatesModel::integer_plate_id_type>(spinbox_plate_id->value());
}


bool
GPlatesQtWidgets::EditPlateIdWidget::update_property_value_from_widget()
{
	if (is_null()) {
		throw UninitialisedEditWidgetException(GPLATES_EXCEPTION_SOURCE);
	}

	if (d_plate_id_ptr.get() != NULL) {
		if (is_dirty()) {
			d_plate_id_ptr->set_value(spinbox_plate_id->value());
			set_clean();
			return true;
		} else {
			return false;
		}
	} else {
		throw UninitialisedEditWidgetException(GPLATES_EXCEPTION_SOURCE);
	}
}


bool
GPlatesQtWidgets::EditPlateIdWidget::is_null() const
{
	return (spinbox_plate_id->value() == -1);
}

void
GPlatesQtWidgets::EditPlateIdWidget::set_null(
		bool should_nullify)
{
	spinbox_plate_id->setValue( should_nullify? -1 : 0 );
}

void
GPlatesQtWidgets::EditPlateIdWidget::handle_value_changed()
{
	Q_EMIT value_changed();
}

