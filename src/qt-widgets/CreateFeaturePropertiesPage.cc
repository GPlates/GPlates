/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

#include <boost/optional.hpp>
#include <QColor>
#include <QDebug>
#include <QHeaderView>
#include <QMessageBox>
#include <QMetaType>
#include <QTableWidget>

#include "CreateFeaturePropertiesPage.h"

#include "CreateFeatureAddOrEditPropertyDialog.h"
#include "EditWidgetGroupBox.h"
#include "QtWidgetUtils.h"
#include "ResizeToContentsTextEdit.h"

#include "feature-visitors/ToQvariantConverter.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "model/FeatureHandle.h"
#include "model/Gpgim.h"
#include "model/GpgimFeatureClass.h"
#include "model/GpgimProperty.h"
#include "model/ModelUtils.h"


#include <boost/foreach.hpp>
namespace
{
	/**
	 * A non-null pointer that is default-constructible so it can be used with QVariant.
	 */
	template <class T, class H = GPlatesUtils::NullIntrusivePointerHandler>
	class DefaultConstructibleNonNullPtrType
	{
	public:

		DefaultConstructibleNonNullPtrType()
		{  }

		DefaultConstructibleNonNullPtrType(
				const GPlatesUtils::non_null_intrusive_ptr<T,H> &non_null_ptr) :
			d_non_null_ptr(non_null_ptr)
		{  }

		operator GPlatesUtils::non_null_intrusive_ptr<T,H>() const
		{
			return d_non_null_ptr.get();
		}

		bool
		operator==(
				const DefaultConstructibleNonNullPtrType &other)
		{
			return d_non_null_ptr == other.d_non_null_ptr;
		}

	private:

		boost::optional< GPlatesUtils::non_null_intrusive_ptr<T,H> > d_non_null_ptr;
	};


	/**
	 * Returns a simple string representation the specified top-level property.
	 */
	QString
	convert_top_level_property_to_display_string(
			const GPlatesModel::TopLevelProperty &top_level_property)
	{
		GPlatesFeatureVisitors::ToQvariantConverter qvariant_converter;
		top_level_property.accept_visitor(qvariant_converter);

		if (qvariant_converter.found_values_begin() == qvariant_converter.found_values_end())
		{
			// Return empty string if we don't know how to display the property.
			return QString();
		}

		return qvariant_converter.found_values_begin()->toString();
	}


	/**
	 * Returns true if any properties in @a feature_properties, or @a geometry_property_name, match
	 * @a property_name.
	 */
	bool
	feature_has_property_name(
			const GPlatesModel::PropertyName &property_name,
			const GPlatesModel::PropertyName &geometry_property_name,
			const GPlatesQtWidgets::CreateFeaturePropertiesPage::property_seq_type &feature_properties)
	{
		// Iterate over all feature properties.
		GPlatesQtWidgets::CreateFeaturePropertiesPage::property_seq_type::const_iterator iter =
				feature_properties.begin();
		GPlatesQtWidgets::CreateFeaturePropertiesPage::property_seq_type::const_iterator end =
				feature_properties.end();
		for ( ; iter != end; ++iter)
		{
			const GPlatesModel::PropertyName &feature_property_name = (*iter)->property_name();

			if (property_name == feature_property_name ||
				property_name == geometry_property_name)
			{
				// Found a matching property name.
				return true;
			}
		}

		return false;
	}
}


Q_DECLARE_METATYPE(DefaultConstructibleNonNullPtrType<GPlatesModel::TopLevelProperty>)
Q_DECLARE_METATYPE(DefaultConstructibleNonNullPtrType<const GPlatesModel::GpgimProperty>)


GPlatesQtWidgets::CreateFeaturePropertiesPage::CreateFeaturePropertiesPage(
		const GPlatesModel::Gpgim &gpgim,
		GPlatesPresentation::ViewState &view_state,
		QWidget *parent_):
	QWidget(parent_),
	d_gpgim(gpgim),
	// Start off with the most basic feature type.
	// It's actually an 'abstract' feature but it'll get reset to a 'concrete' feature...
	d_feature_type(GPlatesModel::FeatureType::create_gml("AbstractFeature")),
	d_property_description_widget(new ResizeToContentsTextEdit(this)),
	d_add_or_edit_property_dialog(new CreateFeatureAddOrEditPropertyDialog(view_state, this))
{
	setupUi(this);

	// Set up the property description text edit widget.
	GPlatesQtWidgets::QtWidgetUtils::add_widget_to_placeholder(
			d_property_description_widget,
			property_description_placeholder_widget);
	d_property_description_widget->setReadOnly(true);
	d_property_description_widget->setSizePolicy(
			QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed/*Use sizeHint() since we've overridden it*/));
	d_property_description_widget->setLineWrapMode(QTextEdit::WidgetWidth);
	d_property_description_widget->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	// Limit the maximum height of the property description so it doesn't push the dialog off the screen.
	d_property_description_widget->setMaximumHeight(100);

	// Set some table widget properties not set in the Qt UI designer.
	// For 'available properties' we stretch the first column to ensure the property names
	// don't get clipped (ResizeToContents doesn't work on rows that are hidden due to a scrolling).
	// Whereas for the 'existing properties' we stretch the second column to ensure the
	// property value has the most room to be displayed.
	available_properties_table_widget->horizontalHeader()->setResizeMode(
			AVAILABLE_PROPERTIES_COLUMN_PROPERTY,
			QHeaderView::Stretch);
	available_properties_table_widget->horizontalHeader()->setResizeMode(
			AVAILABLE_PROPERTIES_COLUMN_MULTIPLICITY,
			QHeaderView::ResizeToContents);
	existing_properties_table_widget->horizontalHeader()->setResizeMode(
			EXISTING_PROPERTIES_COLUMN_PROPERTY,
			QHeaderView::ResizeToContents);
	existing_properties_table_widget->horizontalHeader()->setResizeMode(
			EXISTING_PROPERTIES_COLUMN_VALUE,
			QHeaderView::Stretch);

	// Set the initial button state.
	handle_available_properties_selection_changed();
	handle_existing_properties_selection_changed();

	// Connect button signals.
    QObject::connect(
			add_property_button, SIGNAL(clicked()),
			this, SLOT(handle_add_property_button_clicked()));
    QObject::connect(
			remove_property_button, SIGNAL(clicked()),
			this, SLOT(handle_remove_property_button_clicked()));
    QObject::connect(
			edit_property_button, SIGNAL(clicked()),
			this, SLOT(handle_edit_property_button_clicked()));

	// Connect available properties table widget signals.
	QObject::connect(
			available_properties_table_widget, SIGNAL(itemSelectionChanged()),
			this, SLOT(handle_available_properties_selection_changed()));

	// Connect existing properties table widget signals.
	QObject::connect(
			existing_properties_table_widget, SIGNAL(itemSelectionChanged()),
			this, SLOT(handle_existing_properties_selection_changed()));
}


void
GPlatesQtWidgets::CreateFeaturePropertiesPage::initialise(
		const GPlatesModel::FeatureType &feature_type,
		const GPlatesModel::PropertyName &geometry_property_name,
		const property_seq_type &feature_properties)
{
	d_feature_type = feature_type;
	d_geometry_property_name = geometry_property_name;

	//
	// Set the text labels for each table widget.
	//

	available_properties_label->setText(
			tr("Properties available to add to the '%1' feature:").arg(
					convert_qualified_xml_name_to_qstring(feature_type)));
	existing_properties_label->setText(
			tr("Properties added to the '%1' feature:").arg(
					convert_qualified_xml_name_to_qstring(feature_type)));

	// First initialise the existing properties table using the feature properties.
	initialise_existing_properties_table(feature_properties);

	// Then update the available properties based on the feature type, property multiplicity
	// and existing properties.
	update_available_properties_table();
}


void
GPlatesQtWidgets::CreateFeaturePropertiesPage::initialise_existing_properties_table(
		const property_seq_type &feature_properties)
{
	//
	// Populate the *existing* properties table widget.
	//

	// Clear the table.
	existing_properties_table_widget->clearContents(); // Do not clear the header items as well.
	existing_properties_table_widget->setRowCount(0);  // Remove the newly blanked rows.

	// Add the feature properties to the existing properties table.
	BOOST_FOREACH(
			const GPlatesModel::TopLevelProperty::non_null_ptr_type &feature_property,
			feature_properties)
	{
		add_to_existing_properties(feature_property);
	}
}


void
GPlatesQtWidgets::CreateFeaturePropertiesPage::add_to_existing_properties(
		const GPlatesModel::TopLevelProperty::non_null_ptr_type &feature_property)
{
	const int row = existing_properties_table_widget->rowCount();
	existing_properties_table_widget->insertRow(row);

	const QString property_name_string =
			convert_qualified_xml_name_to_qstring(feature_property->property_name());

	// Put the feature property in a QVariant so we can store it in the table widget row.
	QVariant feature_property_qvariant;
	feature_property_qvariant.setValue(
			DefaultConstructibleNonNullPtrType<GPlatesModel::TopLevelProperty>(feature_property));

	QTableWidgetItem *property_item = new QTableWidgetItem(property_name_string);
	property_item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
	property_item->setData(Qt::UserRole, feature_property_qvariant);
	existing_properties_table_widget->setItem(
			row,
			EXISTING_PROPERTIES_COLUMN_PROPERTY,
			property_item);

	QTableWidgetItem *value_item = new QTableWidgetItem();
	value_item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
	value_item->setText(convert_top_level_property_to_display_string(*feature_property));
	existing_properties_table_widget->setItem(
			row,
			EXISTING_PROPERTIES_COLUMN_VALUE,
			value_item);
}


void
GPlatesQtWidgets::CreateFeaturePropertiesPage::update_available_properties_table()
{
	//
	// Populate the *available* properties table widget.
	//

	// Clear the table.
	available_properties_table_widget->clearContents(); // Do not clear the header items as well.
	available_properties_table_widget->setRowCount(0);  // Remove the newly blanked rows.

	// Query the GPGIM for the feature class associated with the feature type.
	boost::optional<GPlatesModel::GpgimFeatureClass::non_null_ptr_to_const_type> gpgim_feature_class =
			d_gpgim.get_feature_class(d_feature_type);
	if (!gpgim_feature_class)
	{
		QMessageBox::warning(this,
				tr("Feature type not recognised by GPGIM"),
				tr("Internal error - the feature type '%1' was not recognised by the "
					"GPlates Geological Information Model (GPGIM). "
					"No properties will be available to add to the feature.")
						.arg(convert_qualified_xml_name_to_qstring(d_feature_type)),
				QMessageBox::Ok);
		return;
	}

	// Get the existing feature properties (from the 'existing properties' table widget).
	property_seq_type feature_properties;
	get_feature_properties(feature_properties);

	// Get allowed properties for the feature type.
	GPlatesModel::GpgimFeatureClass::gpgim_property_seq_type gpgim_feature_properties;
	gpgim_feature_class.get()->get_feature_properties(gpgim_feature_properties);

	// Iterate over the allowed properties for the feature type.
	BOOST_FOREACH(
			const GPlatesModel::GpgimProperty::non_null_ptr_to_const_type &gpgim_feature_property,
			gpgim_feature_properties)
	{
		// Only add property types supported by edit widgets, otherwise the user will be
		// left with the inability to actually add their selected property.
		if (!d_add_or_edit_property_dialog->is_property_supported(*gpgim_feature_property))
		{
			continue;
		}

		// If the current property is allowed to occur at most once per feature then only allow
		// the user to add the property if it doesn't already exist in the feature.
		if (gpgim_feature_property->get_multiplicity() == GPlatesModel::GpgimProperty::ZERO_OR_ONE ||
			gpgim_feature_property->get_multiplicity() == GPlatesModel::GpgimProperty::ONE)
		{
			// Should only get here from 'initialise()' which also sets the geometry property name.
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					d_geometry_property_name,
					GPLATES_ASSERTION_SOURCE);

			// Note that we also include the digitised geometry property name in the list of
			// feature properties because it's not yet present as a property in the list.
			// This is in case only one geometry property with that name is allowed by the GPGIM -
			// however if multiple properties are allowed then it's possible for the user to add
			// a second geometry via the Edit Geometry widget (the first geometry was created via a
			// Digitisation tool).
			if (feature_has_property_name(
				gpgim_feature_property->get_property_name(),
				d_geometry_property_name.get(),
				feature_properties))
			{
				continue;
			}
		}

		//
		// Passed all tests so we can add the current property.
		//

		// The current property is a required property if it has a minimum multiplicity of one.
		const bool required_property =
				gpgim_feature_property->get_multiplicity() == GPlatesModel::GpgimProperty::ONE ||
				gpgim_feature_property->get_multiplicity() == GPlatesModel::GpgimProperty::ONE_OR_MORE;

		// Add required properties to the front of the list, otherwise add to the end.
		const int row = required_property ? 0 : available_properties_table_widget->rowCount();
		available_properties_table_widget->insertRow(row);

		// Also colour the required rows differently to highlight them to the user.
		static const QColor REQUIRED_PROPERTY_ROW_COLOUR = Qt::lightGray;

		const QString property_name_string =
				convert_qualified_xml_name_to_qstring(gpgim_feature_property->get_property_name());

		// Put the GPGIM feature property in a QVariant so we can store it in the table widget row.
		QVariant gpgim_feature_property_qvariant;
		gpgim_feature_property_qvariant.setValue(
				DefaultConstructibleNonNullPtrType<const GPlatesModel::GpgimProperty>(
						gpgim_feature_property));

		QTableWidgetItem *property_item = new QTableWidgetItem(property_name_string);
		property_item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
		property_item->setData(Qt::UserRole, gpgim_feature_property_qvariant);
		if (required_property)
		{
			property_item->setData(Qt::BackgroundRole, REQUIRED_PROPERTY_ROW_COLOUR);
		}
		available_properties_table_widget->setItem(
				row,
				AVAILABLE_PROPERTIES_COLUMN_PROPERTY,
				property_item);

		QTableWidgetItem *multiplicity_item = new QTableWidgetItem();
		multiplicity_item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);
		if (required_property)
		{
			multiplicity_item->setData(Qt::BackgroundRole, REQUIRED_PROPERTY_ROW_COLOUR);
		}
		switch (gpgim_feature_property->get_multiplicity())
		{
		case GPlatesModel::GpgimProperty::ZERO_OR_ONE:
			multiplicity_item->setText("0..1");
			break;
		case GPlatesModel::GpgimProperty::ONE:
			multiplicity_item->setText("1");
			break;
		case GPlatesModel::GpgimProperty::ZERO_OR_MORE:
			multiplicity_item->setText("0..*");
			break;
		case GPlatesModel::GpgimProperty::ONE_OR_MORE:
			multiplicity_item->setText("1..*");
			break;
		}
		available_properties_table_widget->setItem(
				row,
				AVAILABLE_PROPERTIES_COLUMN_MULTIPLICITY,
				multiplicity_item);
	}

	update_focus();
}


boost::optional<GPlatesModel::GpgimProperty::non_null_ptr_to_const_type>
GPlatesQtWidgets::CreateFeaturePropertiesPage::get_available_property(
		int row) const
{
	if (row < 0 || row >= available_properties_table_widget->rowCount())
	{
		return boost::none;
	}

	const QTableWidgetItem *item =
			available_properties_table_widget->item(row, AVAILABLE_PROPERTIES_COLUMN_PROPERTY);

	// Get the GPGIM property stored in the table widget item.
	QVariant gpgim_feature_property_qvariant = item->data(Qt::UserRole);

	// This should always be convertible.
	if (!gpgim_feature_property_qvariant.canConvert<
		DefaultConstructibleNonNullPtrType<const GPlatesModel::GpgimProperty> >())
	{
		return boost::none;
	}

	const GPlatesModel::GpgimProperty::non_null_ptr_to_const_type gpgim_feature_property =
			gpgim_feature_property_qvariant.value<
					DefaultConstructibleNonNullPtrType<const GPlatesModel::GpgimProperty> >();

	return gpgim_feature_property;
}


void
GPlatesQtWidgets::CreateFeaturePropertiesPage::get_available_properties(
		gpgim_property_seq_type &gpgim_feature_properties) const
{
	// Iterate over the 'available properties' table widget.
	for (int row = 0; row < available_properties_table_widget->rowCount(); ++row)
	{
		boost::optional<GPlatesModel::GpgimProperty::non_null_ptr_to_const_type>
				gpgim_feature_property = get_available_property(row);
		if (gpgim_feature_property)
		{
			gpgim_feature_properties.push_back(gpgim_feature_property.get());
		}
	}
}


boost::optional<GPlatesModel::TopLevelProperty::non_null_ptr_type>
GPlatesQtWidgets::CreateFeaturePropertiesPage::get_existing_property(
		int row) const
{
	if (row < 0 || row >= existing_properties_table_widget->rowCount())
	{
		return boost::none;
	}

	const QTableWidgetItem *item =
			existing_properties_table_widget->item(row, EXISTING_PROPERTIES_COLUMN_PROPERTY);

	// Get the top-level property stored in the table widget item.
	QVariant feature_property_qvariant = item->data(Qt::UserRole);

	// This should always be convertible.
	if (!feature_property_qvariant.canConvert<
		DefaultConstructibleNonNullPtrType<GPlatesModel::TopLevelProperty> >())
	{
		return boost::none;
	}

	const GPlatesModel::TopLevelProperty::non_null_ptr_type feature_property =
			feature_property_qvariant.value<
					DefaultConstructibleNonNullPtrType<GPlatesModel::TopLevelProperty> >();

	return feature_property;
}


bool
GPlatesQtWidgets::CreateFeaturePropertiesPage::is_finished() const
{
	// If 'initialise()' has not yet been called then we're not finished (haven't started).
	if (!d_geometry_property_name)
	{
		return false;
	}

	// Get the existing feature properties (from the 'existing properties' table widget).
	property_seq_type existing_properties;
	get_feature_properties(existing_properties);

	// Get the remaining allowed feature properties (from the 'available properties' table widget).
	gpgim_property_seq_type available_properties;
	get_available_properties(available_properties);

	// Iterate over the available properties.
	BOOST_FOREACH(
			const GPlatesModel::GpgimProperty::non_null_ptr_to_const_type &available_property,
			available_properties)
	{
		// If the current property has a minimum multiplicity of one (ie, is required) then
		// make sure that property is in the 'existing' properties list.
		if (available_property->get_multiplicity() == GPlatesModel::GpgimProperty::ONE ||
			available_property->get_multiplicity() == GPlatesModel::GpgimProperty::ONE_OR_MORE)
		{
			if (!feature_has_property_name(
				available_property->get_property_name(),
				d_geometry_property_name.get(),
				existing_properties))
			{
				return false;
			}
		}
	}

	return true;
}


void
GPlatesQtWidgets::CreateFeaturePropertiesPage::get_feature_properties(
		property_seq_type &feature_properties) const
{
	// Iterate over the 'existing properties' table widget.
	for (int row = 0; row < existing_properties_table_widget->rowCount(); ++row)
	{
		boost::optional<GPlatesModel::TopLevelProperty::non_null_ptr_type> feature_property =
				get_existing_property(row);
		if (feature_property)
		{
			feature_properties.push_back(feature_property.get());
		}
	}
}


void
GPlatesQtWidgets::CreateFeaturePropertiesPage::handle_available_properties_selection_changed()
{
	// Enable or disable the Add property button.
	add_property_button->setDisabled(available_properties_table_widget->selectedItems().isEmpty());

	// Get the property description text (if a property is selected).
	QString property_description_string;
	if (available_properties_table_widget->selectedItems().count() > 0)
	{
		// Get the GPGIM property from the currently selected row in the 'available properties' table widget.
		boost::optional<GPlatesModel::GpgimProperty::non_null_ptr_to_const_type> gpgim_feature_property =
				get_available_property(available_properties_table_widget->currentRow());
		if (gpgim_feature_property)
		{
			property_description_string = gpgim_feature_property.get()->get_property_description();
		}
	}

	// Set or clear the property description QTextEdit.
	if (!property_description_string.isEmpty())
	{
		property_description_widget->show();
		d_property_description_widget->setPlainText(property_description_string);
	}
	else
	{
		property_description_widget->hide();
		d_property_description_widget->clear();
	}
}


void
GPlatesQtWidgets::CreateFeaturePropertiesPage::handle_existing_properties_selection_changed()
{
	remove_property_button->setDisabled(existing_properties_table_widget->selectedItems().isEmpty());
	edit_property_button->setDisabled(existing_properties_table_widget->selectedItems().isEmpty());
}


void
GPlatesQtWidgets::CreateFeaturePropertiesPage::handle_add_property_button_clicked()
{
	// Get the GPGIM property from the currently selected row in the 'available properties' table widget.
	boost::optional<GPlatesModel::GpgimProperty::non_null_ptr_to_const_type> gpgim_feature_property =
			get_available_property(available_properties_table_widget->currentRow());
	if (!gpgim_feature_property)
	{
		return;
	}

	// Popup dialog so user can add a property.
	boost::optional<GPlatesModel::TopLevelProperty::non_null_ptr_type> feature_property =
			d_add_or_edit_property_dialog->add_property(*gpgim_feature_property.get());

	// If the user canceled the add then return early.
	if (!feature_property)
	{
		return;
	}

	// Add the property to our 'existing properties' table widget.
	add_to_existing_properties(feature_property.get());

	// Update available properties based on feature type, property multiplicity and existing properties.
	//
	// We just added to the existing properties and this can change the properties 'available' to add
	// based on GPGIM property multiplicity.
	update_available_properties_table();
}


void
GPlatesQtWidgets::CreateFeaturePropertiesPage::handle_remove_property_button_clicked()
{
	existing_properties_table_widget->removeRow(existing_properties_table_widget->currentRow());

	// Update available properties based on feature type, property multiplicity and existing properties.
	//
	// We just removed an existing property and this can change the properties 'available' to add
	// based on GPGIM property multiplicity.
	update_available_properties_table();
}


void
GPlatesQtWidgets::CreateFeaturePropertiesPage::handle_edit_property_button_clicked()
{
	// Get the existing feature property from the currently selected row in the 'existing properties' table widget.
	boost::optional<GPlatesModel::TopLevelProperty::non_null_ptr_type> feature_property =
			get_existing_property(existing_properties_table_widget->currentRow());
	if (!feature_property)
	{
		return;
	}

	// Popup dialog so user can edit the property.
	d_add_or_edit_property_dialog->edit_property(feature_property.get());

	// Update the corresponding row in the 'existing properties' table widget.
	QTableWidgetItem *item = existing_properties_table_widget->item(
			existing_properties_table_widget->currentRow(),
			EXISTING_PROPERTIES_COLUMN_VALUE);
	item->setText(convert_top_level_property_to_display_string(*feature_property.get()));
}


void
GPlatesQtWidgets::CreateFeaturePropertiesPage::update_focus()
{
	// If there are required feature properties that the user must add then select the first
	// required property and set the focus to the "Add" button.
	if (is_finished())
	{
		emit finished();
	}
	else
	{
		// All required properties are in the first table widget rows.
		available_properties_table_widget->selectRow(0);
		add_property_button->setFocus();
	}
}


void
GPlatesQtWidgets::CreateFeaturePropertiesPage::focusInEvent(
		QFocusEvent *focus_event)
{
	update_focus();
}
