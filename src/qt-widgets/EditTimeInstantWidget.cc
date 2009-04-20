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

#include "EditTimeInstantWidget.h"

#include "property-values/GmlTimeInstant.h"
#include "model/ModelUtils.h"
#include "UninitialisedEditWidgetException.h"


namespace
{
	GPlatesPropertyValues::GeoTimeInstant
	create_geo_time_instant_from_widget(
			QDoubleSpinBox *spinbox)
	{
		return GPlatesPropertyValues::GeoTimeInstant(spinbox->value());
	}
}


GPlatesQtWidgets::EditTimeInstantWidget::EditTimeInstantWidget(
		QWidget *parent_):
	AbstractEditWidget(parent_)
{
	setupUi(this);
	reset_widget_to_default_values();
	
	QObject::connect(spinbox_time_position, SIGNAL(valueChanged(double)),
			this, SLOT(set_dirty()));

	label_time_position->setHidden(false);
	declare_default_label(label_time_position);
	setFocusProxy(spinbox_time_position);
}


void
GPlatesQtWidgets::EditTimeInstantWidget::reset_widget_to_default_values()
{
	d_time_instant_ptr = NULL;
	spinbox_time_position->setValue(0.0);
	set_clean();
}


void
GPlatesQtWidgets::EditTimeInstantWidget::update_widget_from_time_instant(
		GPlatesPropertyValues::GmlTimeInstant &gml_time_instant)
{
	d_time_instant_ptr = &gml_time_instant;
	const GPlatesPropertyValues::GeoTimeInstant time = gml_time_instant.time_position();
	spinbox_time_position->setValue(time.value());
	set_clean();
}


GPlatesModel::PropertyValue::non_null_ptr_type
GPlatesQtWidgets::EditTimeInstantWidget::create_property_value_from_widget() const
{
	GPlatesPropertyValues::GeoTimeInstant time = create_geo_time_instant_from_widget(
			spinbox_time_position);
			
	GPlatesPropertyValues::GmlTimeInstant::non_null_ptr_type gml_time_instant =
			GPlatesModel::ModelUtils::create_gml_time_instant(time);
	return gml_time_instant;
}


bool
GPlatesQtWidgets::EditTimeInstantWidget::update_property_value_from_widget()
{
	if (d_time_instant_ptr.get() != NULL) {
		if (is_dirty()) {
			GPlatesPropertyValues::GeoTimeInstant time = create_geo_time_instant_from_widget(
					spinbox_time_position);
			d_time_instant_ptr->set_time_position(time);
			set_clean();
			return true;
		} else {
			return false;
		}
	} else {
		throw UninitialisedEditWidgetException(GPLATES_EXCEPTION_SOURCE);
	}
}

