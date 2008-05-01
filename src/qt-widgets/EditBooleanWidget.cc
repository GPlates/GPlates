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

#include "EditBooleanWidget.h"

#include "property-values/XsBoolean.h"
#include "model/ModelUtils.h"


GPlatesQtWidgets::EditBooleanWidget::EditBooleanWidget(
		QWidget *parent_):
	AbstractEditWidget(parent_)
{
	setupUi(this);
	
	combobox_boolean->addItem(tr("True"));
	combobox_boolean->addItem(tr("False"));
	reset_widget_to_default_values();
	
	QObject::connect(combobox_boolean, SIGNAL(activated(int)),
			this, SLOT(handle_combobox_change()));
	
	setFocusPolicy(Qt::StrongFocus);
}


void
GPlatesQtWidgets::EditBooleanWidget::reset_widget_to_default_values()
{
	combobox_boolean->setCurrentIndex(0);
	set_clean();
}


void
GPlatesQtWidgets::EditBooleanWidget::update_widget_from_boolean(
		const GPlatesPropertyValues::XsBoolean &xs_boolean)
{
	if (xs_boolean.value() == true) {
		combobox_boolean->setCurrentIndex(0);
	} else {
		combobox_boolean->setCurrentIndex(1);
	}
	set_clean();
}


GPlatesModel::PropertyValue::non_null_ptr_type
GPlatesQtWidgets::EditBooleanWidget::create_property_value_from_widget() const
{
	if (combobox_boolean->currentIndex() == 0) {
		return GPlatesPropertyValues::XsBoolean::create(true);
	} else {
		return GPlatesPropertyValues::XsBoolean::create(false);
	}
}

