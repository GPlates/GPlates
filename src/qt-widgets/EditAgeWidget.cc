/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2015 The University of Sydney, Australia
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

#include "EditAgeWidget.h"

#include "model/ModelUtils.h"
#include "gui/Completionist.h"
#include "UninitialisedEditWidgetException.h"


namespace
{
	/**
	 * These correspond to the three states of combobox_name_or_abs.
	 */
	enum ComboboxNameOrAbsValues {
		EDIT_AGE_ABSOLUTE, EDIT_AGE_NAMED, EDIT_AGE_BOTH
	};

	/**
	 * These correspond to the three states of combobox_uncertainty.
	 */
	enum ComboboxUncertaintyValues {
		UNCERTAINTY_NONE, UNCERTAINTY_PLUSMINUS, UNCERTAINTY_RANGE
	};
	
	
	boost::optional<double>
	gimme_double(
			const QLineEdit &lineedit)
	{
		bool conv_ok = false;
		// Note that this converts from the QLineEdit to a double assuming the system locale, falling back to C locale.
		// This is important if someone uses a locale where , is the decimal separator.
		double dbl = lineedit.locale().toDouble(lineedit.text(), &conv_ok);
		if (!conv_ok)
		{
			// QString::toDouble() only uses C locale.
			dbl = lineedit.text().toDouble(&conv_ok);
		}
		if (conv_ok) {
			return dbl;
		} else {
			return boost::none;
		}
	}


	QString
	gimme_qstring(
			const QLineEdit &lineedit)
	{
		QString str = lineedit.text().trimmed();
		if (str != "") {
			return str;
		} else {
			return QString();
		}
	}
	
	
	QString
	set_lineedit_contents(
			QLineEdit *lineedit,
			const boost::optional<GPlatesPropertyValues::TimescaleBand> &band)
	{
		QString stringified = "";
		if (band) {
			stringified = band.get().get().qstring();
		}
		if (lineedit) {
			lineedit->setText(stringified);
		}
		return stringified;
	}
	
		
	QString
	set_lineedit_contents(
			QLineEdit *lineedit,
			const boost::optional<GPlatesPropertyValues::TimescaleName> &name)
	{
		QString stringified = "";
		if (name) {
			stringified = name.get().get().qstring();
		}
		if (lineedit) {
			lineedit->setText(stringified);
		}
		return stringified;
	}

	
	QString
	set_lineedit_contents(
			QLineEdit *lineedit,
			const boost::optional<double> &dbl)
	{
		QString stringified = "";
		if (dbl) {
			// Note that this converts to the QLineEdit from a double assuming the system locale.
			// This is important if someone uses a locale where , is the decimal separator.
			stringified = QLocale::system().toString(dbl.get());
			// In other situations i.e. writing to file, it is more correct to use QLocale::c().toString(dbl.get());
		}
		if (lineedit) {
			lineedit->setText(stringified);
		}
		return stringified;
	}
	

	// It occurs to me that perhaps we could move to the 'ui *' style of doing Qt widgets,
	// and default-constructed property values which we can then set via a common method
	// like this.
	// Or modify how we do our Edit.*Widget classes so that the update_ and create_ methods
	// can share code more easily. --jclark 20150311
	void
	set_gpml_age_fields_from_widget(
			const Ui_EditAgeWidget &ui,
			GPlatesPropertyValues::GpmlAge::non_null_ptr_type age_ptr)
	{
		// Fetch and set the absolute and/or named age as appropriate. Which line-edits are valid
		// depends on the setting of the combobox.
		switch (ui.combobox_name_or_abs->currentIndex())
		{
		case EDIT_AGE_ABSOLUTE:
			age_ptr->set_age_absolute(gimme_double(*(ui.lineedit_abs_age)));
			age_ptr->set_age_named(boost::none);
			break;
	
		case EDIT_AGE_NAMED:
			age_ptr->set_age_absolute(boost::none);
			age_ptr->set_age_named(gimme_qstring(*(ui.lineedit_named_age)));
			break;
	
		case EDIT_AGE_BOTH:
			age_ptr->set_age_absolute(gimme_double(*(ui.lineedit_abs_age)));
			age_ptr->set_age_named(gimme_qstring(*(ui.lineedit_named_age)));
			break;
		}
		
		// The timescale selection can be from the combo box or from a line edit that is only shown
		// if the last option "Other:" is selected. The first entry is blank and indicates no timescale.
		if (ui.combobox_timescale->currentIndex() == 0) {
			age_ptr->set_timescale(boost::none);
			
		} else if (ui.combobox_timescale->currentIndex() == ui.combobox_timescale->count() - 1) {
			age_ptr->set_timescale(gimme_qstring(*(ui.lineedit_timescale_other)));
			
		} else {
			age_ptr->set_timescale(ui.combobox_timescale->currentText());
		}
		
		// The uncertainty data is also split into three possible representations via combobox.
		switch (ui.combobox_uncertainty->currentIndex())
		{
		case UNCERTAINTY_NONE:
			age_ptr->set_uncertainty_plusminus(boost::none);
			age_ptr->set_uncertainty_oldest_absolute(boost::none);
			age_ptr->set_uncertainty_oldest_named(boost::none);
			age_ptr->set_uncertainty_youngest_absolute(boost::none);
			age_ptr->set_uncertainty_youngest_named(boost::none);
			break;
	
		case UNCERTAINTY_PLUSMINUS:
			age_ptr->set_uncertainty_plusminus(gimme_double(*(ui.lineedit_uncertainty_plusminus)));
			age_ptr->set_uncertainty_oldest_absolute(boost::none);
			age_ptr->set_uncertainty_oldest_named(boost::none);
			age_ptr->set_uncertainty_youngest_absolute(boost::none);
			age_ptr->set_uncertainty_youngest_named(boost::none);
			break;
	
		case UNCERTAINTY_RANGE:
			age_ptr->set_uncertainty_plusminus(boost::none);
			// If the 'oldest' field is doubleish, we set it as such. Otherwise we assume it's a name.
			// It's all the same once it hits the XML anyway.
			boost::optional<double> oldest = gimme_double(*(ui.lineedit_uncertainty_oldest));
			if (oldest) {
				age_ptr->set_uncertainty_oldest_absolute(oldest);
				age_ptr->set_uncertainty_oldest_named(boost::none);
			} else {
				age_ptr->set_uncertainty_oldest_absolute(boost::none);
				age_ptr->set_uncertainty_oldest_named(gimme_qstring(*(ui.lineedit_uncertainty_oldest)));
			}
			// And the same for 'youngest'.
			boost::optional<double> youngest = gimme_double(*(ui.lineedit_uncertainty_youngest));
			if (youngest) {
				age_ptr->set_uncertainty_youngest_absolute(youngest);
				age_ptr->set_uncertainty_youngest_named(boost::none);
			} else {
				age_ptr->set_uncertainty_youngest_absolute(boost::none);
				age_ptr->set_uncertainty_youngest_named(gimme_qstring(*(ui.lineedit_uncertainty_youngest)));
			}
			break;
		}
	}
	
}


GPlatesQtWidgets::EditAgeWidget::EditAgeWidget(
		QWidget *parent_):
	AbstractEditWidget(parent_)
{
	setupUi(this);
	reset_widget_to_default_values();

	// Mark ourselves as dirty if anything gets fiddled with by the user (not programatically).
	connect(combobox_name_or_abs, SIGNAL(activated(int)), this, SLOT(set_dirty()));
	connect(combobox_timescale, SIGNAL(activated(int)), this, SLOT(set_dirty()));
	connect(combobox_uncertainty, SIGNAL(activated(int)), this, SLOT(set_dirty()));
	connect(lineedit_abs_age, SIGNAL(textEdited(const QString &)), this, SLOT(set_dirty()));
	connect(lineedit_named_age, SIGNAL(textEdited(const QString &)), this, SLOT(set_dirty()));
	connect(lineedit_timescale_other, SIGNAL(textEdited(const QString &)), this, SLOT(set_dirty()));
	connect(lineedit_uncertainty_plusminus, SIGNAL(textEdited(const QString &)), this, SLOT(set_dirty()));
	connect(lineedit_uncertainty_youngest, SIGNAL(textEdited(const QString &)), this, SLOT(set_dirty()));
	connect(lineedit_uncertainty_oldest, SIGNAL(textEdited(const QString &)), this, SLOT(set_dirty()));

	// Comboboxes are used to reconfigure which data entry widgets are shown. These signals trigger regardless of how the widget was modified.
	connect(combobox_name_or_abs, SIGNAL(currentIndexChanged(int)), this, SLOT(handle_name_or_abs_changed(int)));
	connect(combobox_timescale, SIGNAL(currentIndexChanged(int)), this, SLOT(handle_timescale_changed(int)));
	handle_name_or_abs_changed(combobox_name_or_abs->currentIndex());
	handle_timescale_changed(combobox_timescale->currentIndex());
	// The Uncertainty combobox is just done using the Signals/Slots GUI in Qt Designer and a Stacked Widget.
	
	// Here's the fun one: Get a dictionary of suitable Timescale Age Band names and add them as completable entries to the
	// appropriate QLineEdits.
	// Note: Obviously in this case we are just installing the default completion dictionary that's built into
	// GPlates, the :gpgim/timescales/ICC2012.xml file. In an ideal world, we'd be able to swap out for a
	// different completer using a different timescale, if the user has selected some other timescale that GPlates
	// is aware of - but for now it's quite reasonable to assume that there is One True Timescale that GPlates uses
	// for everything name-related.
	GPlatesGui::Completionist &completionist = GPlatesGui::Completionist::instance();
	completionist.install_completer(*lineedit_named_age);
	completionist.install_completer(*lineedit_uncertainty_youngest);
	completionist.install_completer(*lineedit_uncertainty_oldest);
	
	setFocusProxy(combobox_name_or_abs);
}


void
GPlatesQtWidgets::EditAgeWidget::handle_name_or_abs_changed(
		int slot)
{
	switch (slot)
	{
	case EDIT_AGE_ABSOLUTE:
		label_abs->hide();
		lineedit_abs_age->show();
		label_name->hide();
		lineedit_named_age->hide();
		break;

	case EDIT_AGE_NAMED:
		label_abs->hide();
		lineedit_abs_age->hide();
		label_name->hide();
		lineedit_named_age->show();
		break;

	case EDIT_AGE_BOTH:
		label_abs->show();
		lineedit_abs_age->show();
		label_name->show();
		lineedit_named_age->show();
		break;
	}
}


void
GPlatesQtWidgets::EditAgeWidget::handle_timescale_changed(
		int slot)
{
	// I'm just gonna assume that the last entry is the 'Other:' entry for now.
	// Obviously we can't just check the string because it is a user-facing presumably-translated string.
	if (slot == combobox_timescale->count() - 1) {
		lineedit_timescale_other->show();
	} else {
		lineedit_timescale_other->hide();
	}
}


void
GPlatesQtWidgets::EditAgeWidget::reset_widget_to_default_values()
{
	d_age_ptr = NULL;
	combobox_name_or_abs->setCurrentIndex(0);
	combobox_timescale->setCurrentIndex(0);
	combobox_uncertainty->setCurrentIndex(0);
	lineedit_abs_age->clear();
	lineedit_named_age->clear();
	lineedit_timescale_other->clear();
	lineedit_uncertainty_plusminus->clear();
	lineedit_uncertainty_youngest->clear();
	lineedit_uncertainty_oldest->clear();
	set_clean();
}


void
GPlatesQtWidgets::EditAgeWidget::update_widget_from_age(
		GPlatesPropertyValues::GpmlAge &age)
{
	reset_widget_to_default_values();
	d_age_ptr = &age;
	
	// The meat of the data: The age, as absolute numeric value in Ma or a named age band from some timscale.
	switch (age.age_type())
	{
	default:
	case GPlatesPropertyValues::GpmlAge::AgeDefinition::AGE_NONE :
	case GPlatesPropertyValues::GpmlAge::AgeDefinition::AGE_ABSOLUTE :
		combobox_name_or_abs->setCurrentIndex(EDIT_AGE_ABSOLUTE);
		break;
	case GPlatesPropertyValues::GpmlAge::AgeDefinition::AGE_NAMED :
		combobox_name_or_abs->setCurrentIndex(EDIT_AGE_NAMED);
		break;
	case GPlatesPropertyValues::GpmlAge::AgeDefinition::AGE_BOTH :
		combobox_name_or_abs->setCurrentIndex(EDIT_AGE_BOTH);
		break;
	}
	set_lineedit_contents(lineedit_abs_age, age.get_age_absolute());
	set_lineedit_contents(lineedit_named_age, age.get_age_named());

	// The selected timescale in use.
	combobox_timescale->setCurrentIndex(0);	// The zeroeth index is the blank undefined one. This also hides the lineedit.
	if (age.get_timescale()) {
		QString timescale_name = set_lineedit_contents(lineedit_timescale_other, age.get_timescale());
		// If it's one of our predefined ones, select it in the combobox, don't use the lineedit.
		int timescale_index = combobox_timescale->findText(timescale_name);
		if (timescale_index != -1) {
			// It's fine, we know about this one.
			combobox_timescale->setCurrentIndex(timescale_index);
			lineedit_timescale_other->clear();
		} else {
			// Not a predefined timescale. Set the combobox to 'Other:' (which must be the last entry), revealing the lineedit.
			combobox_timescale->setCurrentIndex(combobox_timescale->count() - 1);
		}
	}
	
	// Uncertainty information about the chosen age, if any.
	switch (age.uncertainty_type())
	{
	default:
	case GPlatesPropertyValues::GpmlAge::UncertaintyDefinition::UNC_NONE :
		combobox_uncertainty->setCurrentIndex(UNCERTAINTY_NONE);
		break;
	case GPlatesPropertyValues::GpmlAge::UncertaintyDefinition::UNC_PLUS_OR_MINUS :
		combobox_uncertainty->setCurrentIndex(UNCERTAINTY_PLUSMINUS);
		break;
	case GPlatesPropertyValues::GpmlAge::UncertaintyDefinition::UNC_RANGE :
		combobox_uncertainty->setCurrentIndex(UNCERTAINTY_RANGE);
		break;
	}
	set_lineedit_contents(lineedit_uncertainty_plusminus, age.get_uncertainty_plusminus());
	if (age.get_uncertainty_oldest_absolute()) {
		set_lineedit_contents(lineedit_uncertainty_oldest, age.get_uncertainty_oldest_absolute());
	} else {
		set_lineedit_contents(lineedit_uncertainty_oldest, age.get_uncertainty_oldest_named());
	}
	if (age.get_uncertainty_youngest_absolute()) {
		set_lineedit_contents(lineedit_uncertainty_youngest, age.get_uncertainty_youngest_absolute());
	} else {
		set_lineedit_contents(lineedit_uncertainty_youngest, age.get_uncertainty_youngest_named());
	}

	set_clean();
}


GPlatesModel::PropertyValue::non_null_ptr_type
GPlatesQtWidgets::EditAgeWidget::create_property_value_from_widget() const
{
	// With so many optionals and a second near identical workflow for update_property_value_from_widget,
	// it's easiest to default-construct a blank GpmlAge and then fill in its fields.
	GPlatesPropertyValues::GpmlAge::non_null_ptr_type age_ptr = GPlatesPropertyValues::GpmlAge::create();
	set_gpml_age_fields_from_widget(*this, age_ptr);
	return age_ptr;
}


bool
GPlatesQtWidgets::EditAgeWidget::update_property_value_from_widget()
{
	if (d_age_ptr.get() == NULL) {
		throw UninitialisedEditWidgetException(GPLATES_EXCEPTION_SOURCE);
	}
	if ( ! is_dirty()) {
		// Should be in sync with the property value already, nothing to do.
		return false;
	}
	
	set_gpml_age_fields_from_widget(*this, d_age_ptr.get());
	
	set_clean();
	return true;
}

