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

#include <QMessageBox>

#include "EditTimePeriodWidget.h"

#include "property-values/GmlTimePeriod.h"
#include "property-values/GmlTimeInstant.h"
#include "model/ModelUtils.h"
#include "UninitialisedEditWidgetException.h"


namespace
{
	GPlatesPropertyValues::GeoTimeInstant
	create_geo_time_instant_from_widgets(
			QDoubleSpinBox *spinbox,
			QCheckBox *past,
			QCheckBox *future)
	{
		static const GPlatesPropertyValues::GeoTimeInstant distant_past = 
				GPlatesPropertyValues::GeoTimeInstant::create_distant_past();
		static const GPlatesPropertyValues::GeoTimeInstant distant_future = 
				GPlatesPropertyValues::GeoTimeInstant::create_distant_future();
		
		if (past->isChecked()) {
			return distant_past;
		} else if (future->isChecked()) {
			return distant_future;
		} else {
			return GPlatesPropertyValues::GeoTimeInstant(spinbox->value());
		}
	}
	
	
	void
	enable_or_disable_spinbox(
			QDoubleSpinBox *spinbox,
			QCheckBox *past,
			QCheckBox *future)
	{
		// If one of the Past or Future boxes are checked, the spinbox is invalid.
		if (past->isChecked() || future->isChecked()) {
			spinbox->setDisabled(true);
		} else {
			spinbox->setDisabled(false);
			spinbox->setFocus();
			spinbox->selectAll();
		}
	}
}


const QString GPlatesQtWidgets::EditTimePeriodWidget::s_help_dialog_text = QObject::tr(
		"<html><body>\n"
		"\n"
		"<h3>Time Period:</h3>\n"
		"<ul>\n"
		"<li> Times are specified in units of millions of years ago (Ma). The present day is <b>0 Ma</b>. </li>\n"
		"<li> The <b>Begin</b> time should be earlier than, or the same as, the <b>End</b> time. </li>\n"
		"</ul>\n"
		"<h3>Begin:</h3>\n"
		"<ul>\n"
		"<li> This specifies the time (in Ma) at which the feature appears, or is formed. </li>\n"
		"<li> If you don't know the time of appearance, select <b>Distant Past</b>. </li>\n"
		"</ul>\n"
		"<h3>End:</h3>\n"
		"<ul>\n"
		"<li> This specifies the time (in Ma) at which the feature disappears, or is destroyed. </li>\n"
		"<li> If you don't know the time of destruction, select <b>Distant Future</b>. </li>\n"
		"</ul>\n"
		"</body></html>\n");

const QString GPlatesQtWidgets::EditTimePeriodWidget::s_help_dialog_title = QObject::tr(
		"Specifying a time period");


GPlatesQtWidgets::EditTimePeriodWidget::EditTimePeriodWidget(
		QWidget *parent_):
	AbstractEditWidget(parent_),
	d_help_dialog(new InformationDialog(s_help_dialog_text, s_help_dialog_title, this))
{
	setupUi(this);
	reset_widget_to_default_values();
		
	QObject::connect(checkbox_appearance_is_distant_past, SIGNAL(clicked()),
			this, SLOT(handle_appearance_is_distant_past_check()));
	QObject::connect(checkbox_appearance_is_distant_future, SIGNAL(clicked()),
			this, SLOT(handle_appearance_is_distant_future_check()));
	QObject::connect(checkbox_disappearance_is_distant_past, SIGNAL(clicked()),
			this, SLOT(handle_disappearance_is_distant_past_check()));
	QObject::connect(checkbox_disappearance_is_distant_future, SIGNAL(clicked()),
			this, SLOT(handle_disappearance_is_distant_future_check()));
	
	QObject::connect(spinbox_time_of_appearance, SIGNAL(valueChanged(double)),
			this, SLOT(set_dirty()));
	QObject::connect(spinbox_time_of_disappearance, SIGNAL(valueChanged(double)),
			this, SLOT(set_dirty()));
	
	QObject::connect(button_help, SIGNAL(clicked()),
			d_help_dialog, SLOT(show()));
	
	// Since having both Distant Past and Distant Future available for both
	// Begin and End is confusing, the "less likely" choice of each is hidden.
	// I haven't removed them from the code entirely, since this would mean
	// a significant rewrite AND the widget would no longer match the
	// GmlTimePeriod model precisely.
	checkbox_appearance_is_distant_future->setVisible(false);
	checkbox_disappearance_is_distant_past->setVisible(false);

	setFocusProxy(spinbox_time_of_appearance);
}


void
GPlatesQtWidgets::EditTimePeriodWidget::reset_widget_to_default_values()
{
	d_time_period_ptr = NULL;
	// NOTE: We do NOT setFocus() on the spinbox here, as reset_widget is (inexplicably)
	// called several times, such as on feature focus change, despite this widget not
	// being visible yet. Normally, this isn't a problem; on Linux and win32, the focus
	// does not move if the widget is invisible. However, it appears that there is some
	// odd behaviour on OS X, where the spinbox is able to steal keyboard focus even
	// while completely invisible. This caused issues that were most noticable while F11
	// was being developed, where clicking a (F)eature and then toggling to (P)ole
	// manipulation was failing, since the spinbox was grabbing the keyboard.
	// This widget is due for a redesign anyway.
	spinbox_time_of_appearance->selectAll();
	spinbox_time_of_appearance->setValue(0);
	spinbox_time_of_disappearance->setValue(0);
	spinbox_time_of_appearance->setDisabled(false);
	spinbox_time_of_disappearance->setDisabled(false);
	checkbox_appearance_is_distant_past->setChecked(false);
	checkbox_appearance_is_distant_future->setChecked(false);
	checkbox_disappearance_is_distant_past->setChecked(false);
	checkbox_disappearance_is_distant_future->setChecked(false);
	set_clean();
}


void
GPlatesQtWidgets::EditTimePeriodWidget::update_widget_from_time_period(
		GPlatesPropertyValues::GmlTimePeriod &gml_time_period)
{
	d_time_period_ptr = &gml_time_period;
	const GPlatesPropertyValues::GeoTimeInstant begin = gml_time_period.begin()->time_position();
	const GPlatesPropertyValues::GeoTimeInstant end = gml_time_period.end()->time_position();

	if (begin.is_real()) {
		spinbox_time_of_appearance->setValue(begin.value());
		spinbox_time_of_appearance->setFocus();
		spinbox_time_of_appearance->selectAll();
	} else {
		spinbox_time_of_appearance->setDisabled(true);
	}
	checkbox_appearance_is_distant_past->setChecked(begin.is_distant_past());
	checkbox_appearance_is_distant_future->setChecked(begin.is_distant_future());

	if (end.is_real()) {
		spinbox_time_of_disappearance->setValue(end.value());
		spinbox_time_of_disappearance->setFocus();
		spinbox_time_of_disappearance->selectAll();
	} else {
		spinbox_time_of_disappearance->setDisabled(true);
	}
	checkbox_disappearance_is_distant_past->setChecked(end.is_distant_past());
	checkbox_disappearance_is_distant_future->setChecked(end.is_distant_future());
	set_clean();
}


GPlatesModel::PropertyValue::non_null_ptr_type
GPlatesQtWidgets::EditTimePeriodWidget::create_property_value_from_widget() const
{
	
	GPlatesPropertyValues::GeoTimeInstant begin = create_geo_time_instant_from_widgets(
			spinbox_time_of_appearance,
			checkbox_appearance_is_distant_past,
			checkbox_appearance_is_distant_future);
	GPlatesPropertyValues::GeoTimeInstant end = create_geo_time_instant_from_widgets(
			spinbox_time_of_disappearance,
			checkbox_disappearance_is_distant_past,
			checkbox_disappearance_is_distant_future);

	GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_type gml_time_period =
			GPlatesModel::ModelUtils::create_gml_time_period(begin, end);
	return gml_time_period;
}


bool
GPlatesQtWidgets::EditTimePeriodWidget::update_property_value_from_widget()
{
	// Remember that the property value pointer may be NULL!
	if (d_time_period_ptr.get() != NULL) {
		if (is_dirty()) {
			// If the start-end time are not valid, do not update property value.
			if (!valid())
			{
				QMessageBox::warning(this, tr("Time Period Invalid"),
					tr("The begin-end time is not valid - time period was not updated."),
					QMessageBox::Ok);
				set_clean();
				return false;
			}

			GPlatesPropertyValues::GeoTimeInstant begin = create_geo_time_instant_from_widgets(
					spinbox_time_of_appearance,
					checkbox_appearance_is_distant_past,
					checkbox_appearance_is_distant_future);
			GPlatesPropertyValues::GeoTimeInstant end = create_geo_time_instant_from_widgets(
					spinbox_time_of_disappearance,
					checkbox_disappearance_is_distant_past,
					checkbox_disappearance_is_distant_future);
			
			GPlatesPropertyValues::GmlTimeInstant::non_null_ptr_type begin_ti =
					GPlatesModel::ModelUtils::create_gml_time_instant(begin);
			GPlatesPropertyValues::GmlTimeInstant::non_null_ptr_type end_ti =
					GPlatesModel::ModelUtils::create_gml_time_instant(end);
			
			d_time_period_ptr->set_begin(begin_ti);
			d_time_period_ptr->set_end(end_ti);
			set_clean();
			return true;
		} else {
			return false;
		}
	} else {
		throw UninitialisedEditWidgetException(GPLATES_EXCEPTION_SOURCE);
	}
}



void
GPlatesQtWidgets::EditTimePeriodWidget::handle_appearance_is_distant_past_check()
{
	set_dirty();
	bool is_checked = checkbox_appearance_is_distant_past->isChecked();
	
	// Time of Appearance cannot be both Past and Future.
	if (is_checked && checkbox_appearance_is_distant_future->isChecked()) {
		checkbox_appearance_is_distant_future->setChecked(false);
	}
	
	enable_or_disable_spinbox(spinbox_time_of_appearance,
			checkbox_appearance_is_distant_past,
			checkbox_appearance_is_distant_future);

	// Checking one of these checkboxes can cause an update.
	if (is_checked) {
		Q_EMIT commit_me();
	}
}

void
GPlatesQtWidgets::EditTimePeriodWidget::handle_appearance_is_distant_future_check()
{
	set_dirty();
	bool is_checked = checkbox_appearance_is_distant_future->isChecked();

	// Time of Appearance cannot be both Past and Future.
	if (is_checked && checkbox_appearance_is_distant_past->isChecked()) {
		checkbox_appearance_is_distant_past->setChecked(false);
	}
	
	enable_or_disable_spinbox(spinbox_time_of_appearance,
			checkbox_appearance_is_distant_past,
			checkbox_appearance_is_distant_future);

	// Checking one of these checkboxes can cause an update.
	if (is_checked) {
		Q_EMIT commit_me();
	}
}

void
GPlatesQtWidgets::EditTimePeriodWidget::handle_disappearance_is_distant_past_check()
{
	set_dirty();
	bool is_checked = checkbox_disappearance_is_distant_past->isChecked();

	// Time of Appearance cannot be both Past and Future.
	if (is_checked && checkbox_disappearance_is_distant_future->isChecked()) {
		checkbox_disappearance_is_distant_future->setChecked(false);
	}
	
	enable_or_disable_spinbox(spinbox_time_of_disappearance,
			checkbox_disappearance_is_distant_past,
			checkbox_disappearance_is_distant_future);

	// Checking one of these checkboxes can cause an update.
	if (is_checked) {
		Q_EMIT commit_me();
	}
}

void
GPlatesQtWidgets::EditTimePeriodWidget::handle_disappearance_is_distant_future_check()
{
	set_dirty();
	bool is_checked = checkbox_disappearance_is_distant_future->isChecked();

	// Time of Appearance cannot be both Past and Future.
	if (is_checked && checkbox_disappearance_is_distant_past->isChecked()) {
		checkbox_disappearance_is_distant_past->setChecked(false);
	}
	
	enable_or_disable_spinbox(spinbox_time_of_disappearance,
			checkbox_disappearance_is_distant_past,
			checkbox_disappearance_is_distant_future);

	// Checking one of these checkboxes can cause an update.
	if (is_checked) {
		Q_EMIT commit_me();
	}
}


bool
GPlatesQtWidgets::EditTimePeriodWidget::valid()
{
	if(spinbox_time_of_appearance->isEnabled() && 
		spinbox_time_of_disappearance->isEnabled())
	{
		if(spinbox_time_of_appearance->value() < spinbox_time_of_disappearance->value())
		{
			spinbox_time_of_disappearance->setValue(0);
			spinbox_time_of_disappearance->setFocus();
			return false;
		}
	}
	return true;
};






