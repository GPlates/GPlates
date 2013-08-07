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

#include "EditDoubleWidget.h"

#include "property-values/XsDouble.h"
#include "UninitialisedEditWidgetException.h"


GPlatesQtWidgets::EditDoubleWidget::EditDoubleWidget(
		QWidget *parent_):
	AbstractEditWidget(parent_)
{
	setupUi(this);
	reset_widget_to_default_values();
	
	QObject::connect(spinbox_double, SIGNAL(valueChanged(double)),
			this, SLOT(set_dirty()));
	
	label_value->setHidden(true);	
	declare_default_label(label_value);
	setFocusProxy(spinbox_double);
}


void
GPlatesQtWidgets::EditDoubleWidget::reset_widget_to_default_values()
{
	d_double_ptr = NULL;
	spinbox_double->setValue(0.0);
	set_clean();
}


void
GPlatesQtWidgets::EditDoubleWidget::update_widget_from_double(
		GPlatesPropertyValues::XsDouble &xs_double)
{
	d_double_ptr = &xs_double;
	spinbox_double->setValue(xs_double.get_value());
	set_clean();
}


GPlatesModel::PropertyValue::non_null_ptr_type
GPlatesQtWidgets::EditDoubleWidget::create_property_value_from_widget() const
{
	return GPlatesPropertyValues::XsDouble::create(
			spinbox_double->value());
}


bool
GPlatesQtWidgets::EditDoubleWidget::update_property_value_from_widget()
{
	// Remember that the property value pointer may be NULL!
	if (d_double_ptr.get() != NULL) {
		if (is_dirty()) {
			d_double_ptr->set_value(spinbox_double->value());
			set_clean();
			return true;
		} else {
			return false;
		}
	} else {
		throw UninitialisedEditWidgetException(GPLATES_EXCEPTION_SOURCE);
	}
}

