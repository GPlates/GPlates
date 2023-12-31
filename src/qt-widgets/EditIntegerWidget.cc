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

#include "EditIntegerWidget.h"

#include "property-values/XsInteger.h"
#include "model/ModelUtils.h"
#include "UninitialisedEditWidgetException.h"


GPlatesQtWidgets::EditIntegerWidget::EditIntegerWidget(
		QWidget *parent_):
	AbstractEditWidget(parent_)
{
	setupUi(this);
	reset_widget_to_default_values();
	
	QObject::connect(spinbox_integer, SIGNAL(valueChanged(int)),
			this, SLOT(set_dirty()));

	label_value->setHidden(true);
	declare_default_label(label_value);
	setFocusProxy(spinbox_integer);
}


void
GPlatesQtWidgets::EditIntegerWidget::reset_widget_to_default_values()
{
	d_integer_ptr = NULL;
	spinbox_integer->setValue(0);
	set_clean();
}


void
GPlatesQtWidgets::EditIntegerWidget::update_widget_from_integer(
		GPlatesPropertyValues::XsInteger &xs_integer)
{
	d_integer_ptr = &xs_integer;
	spinbox_integer->setValue(xs_integer.value());
	set_clean();
}


GPlatesModel::PropertyValue::non_null_ptr_type
GPlatesQtWidgets::EditIntegerWidget::create_property_value_from_widget() const
{
	return GPlatesPropertyValues::XsInteger::create(
			spinbox_integer->value());
}


bool
GPlatesQtWidgets::EditIntegerWidget::update_property_value_from_widget()
{
	// Remember that the property value pointer may be NULL!
	if (d_integer_ptr.get() != NULL) {
		if (is_dirty()) {
			d_integer_ptr->set_value(spinbox_integer->value());
			set_clean();
			return true;
		} else {
			return false;
		}
	} else {
		throw UninitialisedEditWidgetException(GPLATES_EXCEPTION_SOURCE);
	}
}

