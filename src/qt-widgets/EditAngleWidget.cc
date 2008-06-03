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

#include <map>
#include "EditAngleWidget.h"

#include "property-values/GpmlMeasure.h"
#include "model/XmlAttributeName.h"
#include "model/XmlAttributeValue.h"
#include "UninitialisedEditWidgetException.h"


GPlatesQtWidgets::EditAngleWidget::EditAngleWidget(
		QWidget *parent_):
	AbstractEditWidget(parent_)
{
	setupUi(this);
	reset_widget_to_default_values();
	
	QObject::connect(spinbox_double, SIGNAL(valueChanged(double)),
			this, SLOT(set_dirty()));
	
	setFocusPolicy(Qt::StrongFocus);
}


void
GPlatesQtWidgets::EditAngleWidget::reset_widget_to_default_values()
{
	d_angle_ptr = NULL;
	// FIXME: Maybe we can infer which range to limit the input to by the property name.
	spinbox_double->setValue(0.0);
	set_clean();
}


void
GPlatesQtWidgets::EditAngleWidget::update_widget_from_angle(
		GPlatesPropertyValues::GpmlMeasure &gpml_measure)
{
	d_angle_ptr = &gpml_measure;
	spinbox_double->setValue(gpml_measure.quantity());
	set_clean();
}


GPlatesModel::PropertyValue::non_null_ptr_type
GPlatesQtWidgets::EditAngleWidget::create_property_value_from_widget() const
{
	GPlatesModel::XmlAttributeName attr_name = GPlatesModel::XmlAttributeName::create_gml("uom");
	GPlatesModel::XmlAttributeValue attr_value("urn:ogc:def:uom:OGC:1.0:degree");
	std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> uom;
	uom.insert(std::make_pair(attr_name, attr_value));
	
	return GPlatesPropertyValues::GpmlMeasure::create(
			spinbox_double->value(), uom);
}


bool
GPlatesQtWidgets::EditAngleWidget::update_property_value_from_widget()
{
	// Remember that the property value pointer may be NULL!
	if (d_angle_ptr.get() != NULL) {
		if (is_dirty()) {
			d_angle_ptr->set_quantity(spinbox_double->value());
			set_clean();
			return true;
		} else {
			return false;
		}
	} else {
		throw UninitialisedEditWidgetException();
	}
}

