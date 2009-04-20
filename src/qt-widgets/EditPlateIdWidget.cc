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
	AbstractEditWidget(parent_)
{
	setupUi(this);
	reset_widget_to_default_values();
	
	QObject::connect(spinbox_plate_id, SIGNAL(valueChanged(int)),
			this, SLOT(set_dirty()));

	label_plate_id->setHidden(false);
	declare_default_label(label_plate_id);
	setFocusProxy(spinbox_plate_id);
}


void
GPlatesQtWidgets::EditPlateIdWidget::reset_widget_to_default_values()
{
	d_plate_id_ptr = NULL;
	spinbox_plate_id->setValue(0);
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
	return GPlatesPropertyValues::GpmlPlateId::create(
			spinbox_plate_id->value());
}


GPlatesModel::integer_plate_id_type
GPlatesQtWidgets::EditPlateIdWidget::create_integer_plate_id_from_widget() const
{
	return static_cast<GPlatesModel::integer_plate_id_type>(spinbox_plate_id->value());
}


bool
GPlatesQtWidgets::EditPlateIdWidget::update_property_value_from_widget()
{
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

