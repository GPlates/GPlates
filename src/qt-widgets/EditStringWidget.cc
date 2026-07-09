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

#include "EditStringWidget.h"

#include "property-values/XsString.h"
#include "property-values/TextContent.h"
#include "model/ModelUtils.h"
#include "utils/UnicodeStringUtils.h"
#include "UninitialisedEditWidgetException.h"


GPlatesQtWidgets::EditStringWidget::EditStringWidget(
		QWidget *parent_):
	AbstractEditWidget(parent_)
{
	setupUi(this);
	reset_widget_to_default_values();
	
	QObject::connect(combobox_code_space, SIGNAL(activated(int)),
			this, SLOT(set_dirty()));
	QObject::connect(line_edit, SIGNAL(textEdited(const QString &)),
			this, SLOT(set_dirty()));

	label_value->setHidden(true);
	declare_default_label(label_value);
	setFocusProxy(line_edit);
}


void
GPlatesQtWidgets::EditStringWidget::reset_widget_to_default_values()
{
	d_string_ptr = NULL;
	label_code_space->hide();
	combobox_code_space->hide();
	line_edit->clear();
	set_clean();
}


void
GPlatesQtWidgets::EditStringWidget::update_widget_from_string(
		GPlatesPropertyValues::XsString &xs_string)
{
	d_string_ptr = &xs_string;
	// FIXME: Support codespaces!
	label_code_space->hide();
	combobox_code_space->hide();
	line_edit->setText(GPlatesUtils::make_qstring_from_icu_string(
			xs_string.get_value().get()));
	set_clean();
}


GPlatesModel::PropertyValue::non_null_ptr_type
GPlatesQtWidgets::EditStringWidget::create_property_value_from_widget() const
{
	QString value = line_edit->text();
	GPlatesModel::PropertyValue::non_null_ptr_type property_value = 
			GPlatesPropertyValues::XsString::create(
					GPlatesUtils::make_icu_string_from_qstring(value));
	return property_value;
}


bool
GPlatesQtWidgets::EditStringWidget::update_property_value_from_widget()
{
	if (d_string_ptr.get() != NULL) {
		if (is_dirty()) {
			QString value = line_edit->text();
			d_string_ptr->set_value(GPlatesPropertyValues::TextContent(
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

