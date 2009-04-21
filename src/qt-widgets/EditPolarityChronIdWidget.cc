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

#include "EditPolarityChronIdWidget.h"

#include "model/ModelUtils.h"
#include "UninitialisedEditWidgetException.h"


GPlatesQtWidgets::EditPolarityChronIdWidget::EditPolarityChronIdWidget(
		QWidget *parent_):
	AbstractEditWidget(parent_)
{
	setupUi(this);
	reset_widget_to_default_values();
	
	QObject::connect(combobox_era, SIGNAL(activated(int)),
			this, SLOT(set_dirty()));
	QObject::connect(spinbox_major, SIGNAL(valueChanged(int)),
			this, SLOT(set_dirty()));
	QObject::connect(lineedit_minor, SIGNAL(textEdited(const QString &)),
			this, SLOT(set_dirty()));

	setFocusProxy(combobox_era);
}


void
GPlatesQtWidgets::EditPolarityChronIdWidget::reset_widget_to_default_values()
{
	d_polarity_chron_id_ptr = NULL;
	combobox_era->setCurrentIndex(0);
	spinbox_major->setValue(0);
	lineedit_minor->clear();
	set_clean();
}


void
GPlatesQtWidgets::EditPolarityChronIdWidget::update_widget_from_polarity_chron_id(
		GPlatesPropertyValues::GpmlPolarityChronId &polarity_chron_id)
{
	reset_widget_to_default_values();
	d_polarity_chron_id_ptr = &polarity_chron_id;
	
	const boost::optional<QString> &era = polarity_chron_id.get_era();
	const boost::optional<unsigned int> &major = polarity_chron_id.get_major_region();
	const boost::optional<QString> &minor = polarity_chron_id.get_minor_region();
	
	if (era) {
		int era_index = combobox_era->findText(*era);
		if (era_index != -1) {
			// Present the user with the current era value.
			combobox_era->setCurrentIndex(era_index);
		} else {
			// Found a value we're not supposed to have.
			// Add it because The User Is Always Right!
			combobox_era->addItem(*era);
			combobox_era->setCurrentIndex(combobox_era->count() - 1);
		}
	}
	
	if (major) {
		spinbox_major->setValue(*major);
	}
	
	if (minor) {
		lineedit_minor->setText(*minor);
	}
	
	set_clean();
}


GPlatesModel::PropertyValue::non_null_ptr_type
GPlatesQtWidgets::EditPolarityChronIdWidget::create_property_value_from_widget() const
{
	return GPlatesPropertyValues::GpmlPolarityChronId::create(
			combobox_era->currentText(),
			spinbox_major->value(),
			lineedit_minor->text());
}


bool
GPlatesQtWidgets::EditPolarityChronIdWidget::update_property_value_from_widget()
{
	if (d_polarity_chron_id_ptr.get() != NULL) {
		if (is_dirty()) {
			d_polarity_chron_id_ptr->set_era(combobox_era->currentText());
			d_polarity_chron_id_ptr->set_major_region(spinbox_major->value());
			d_polarity_chron_id_ptr->set_minor_region(lineedit_minor->text());
			set_clean();
			return true;
		} else {
			return false;
		}
	} else {
		throw UninitialisedEditWidgetException(GPLATES_EXCEPTION_SOURCE);
	}
}

