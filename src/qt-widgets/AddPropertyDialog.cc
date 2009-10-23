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

#include <QStringList>
#include <QMessageBox>
#include <boost/optional.hpp>
#include <boost/none.hpp>

#include "AddPropertyDialog.h"
#include "EditFeaturePropertiesWidget.h"
#include "InvalidPropertyValueException.h"

#include "model/PropertyName.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/TemplateTypeParameterType.h"


#define NUM_ELEMS(a) (sizeof(a) / sizeof((a)[0]))


namespace
{
	struct PropertyNameInfo
	{
		const QString name;
		const QString suggested_type;
		bool is_time_dependent;
	};
	
	/**
	 * Sourced from the GPGIM_1.6.juuml document.
	 * Grep script used to hunt down unique property names stored in
	 * /gpgim/trunk/tools/grep_for_properties.pl
	 *
	 * Note that the properties listed here should be properties of Features only -
	 * no properties of structural types.
	 *
	 * Unimplemented properties (i.e. unimplemented property types) are commented out.
	 *
	 * Properties which are not intended to be user-editable (e.g. gpml:identity) are
	 * not included in this list.
	 *
	 * Properties relating to the Topological stuff are similarly not included.
	 *
	 * Contour's properties aren't included because Contour is a geometry type.
	 *
	 * Oh and naturally, when you're editing an InstantaneousFeature, all time-dependent
	 * stuff needs to be stripped out.
	 */
	static const PropertyNameInfo property_name_info_table[] = {
		{ "gpml:angle", "gpml:angle", false },
		{ "gpml:boundary", "gml:Polygon", true }, // TimeDependentPropertyValue<>
		{ "gpml:centerLineOf", "gml:LineString", true }, // TimeDependentPropertyValue<> _Geometry
		{ "gpml:conjugatePlateId", "gpml:plateId", false },	// half-way support for Isochron.
		{ "gpml:continentalSide", "gpml:ContinentalBoundarySideEnumeration", false }, // Enumeration
		{ "gml:description", "xs:string", false },
		{ "gpml:dipAngle", "gpml:angle", false },
		{ "gpml:dipSide", "gpml:DipSideEnumeration", false }, // Enumeration
		{ "gpml:dipSlip", "gpml:DipSlipEnumeration", false }, // Enumeration
		{ "gpml:edge", "gpml:ContinentalBoundaryEdgeEnumeration", false }, // Enumeration
		{ "gpml:errorBounds", "gml:Polygon", true }, // TimeDependentPropertyValue<_Geometry>
	//	{ "gpml:fixedReferenceFrame", "gpml:plateId", false }, // For TotalReconstructionSequence
	//	{ "gpml:movingReferenceFrame", "gpml:plateId", false }, // For TotalReconstructionSequence
		{ "gpml:foldAnnotation", "gpml:FoldPlaneAnnotationEnumeration", false }, // Enumeration
		{ "gpml:isActive", "xs:boolean", true }, // TimeDependentPropertyValue<>
		{ "gpml:leftPlate", "gpml:plateId", false },
	//	{ "gpml:leftUnit", "gpml:FeatureReference", false }, // FeatureReference<AbstractRockUnit>
	//	{ "gml:metaDataProperty", "gml:_MetaData", false },
		{ "gpml:motion", "gpml:StrikeSlipEnumeration", true }, // Enumeration. Just for fun, it's also time-dependent.
		{ "gml:name", "xs:string", false },
		{ "gpml:oldPlatesHeader", "gpml:OldPlatesHeader", false },
		{ "gpml:outlineOf", "gml:Polygon", true }, // _Geometry
		{ "gpml:polarityChronId", "gpml:PolarityChronId", false },
		{ "gpml:polarityChronOffset", "xs:double", false },
		{ "gpml:position", "gml:Point", false },
		{ "gpml:primarySlipComponent", "gpml:SlipComponentEnumeration", false }, // Enumeration
		{ "gpml:reconstructedPlateId", "gpml:plateId", false }, // For InstantaneousFeatures
		{ "gpml:reconstructedTime", "gml:TimeInstant", false }, // For InstantaneousFeatures
		{ "gpml:reconstructionPlateId", "gpml:plateId", true }, // For ReconstructableFeatures
		{ "gpml:rightPlate", "gpml:plateId", false },
	//	{ "gpml:rightUnit", "gpml:FeatureReference", false }, // FeatureReference<AbstractRockUnit>
	//	{ "gpml:shipTrack", "gpml:FeatureReference", false }, // FeatureReference<MagneticAnomalyShipTrack>
		{ "gpml:strikeSlip", "gpml:StrikeSlipEnumeration", false },
		{ "gpml:subductingSlab", "gpml:SubductionSideEnumeration", true }, // TimeDependentPropertyValue<>
	//	{ "gpml:totalReconstructionPole", "gpml:FiniteRotation", true }, // For TotalReconstructionSequence. IrregularSampling<FiniteRotation>
	//	{ "gpml:type", "gpml:AbsoluteReferenceFrameEnumeration", false }, // Enumeration. For AbsoluteReferenceFrame.
		{ "gpml:unclassifiedGeometry", "gml:MultiPoint", true },	// _Geometry
		{ "gml:validTime", "gml:TimePeriod", false },
	};
	
	void
	populate_property_name_combobox(
			QComboBox *combobox)
	{
		// Add all property names present in the property_name_info_table, and their
		// suggested property type mapping.
		const PropertyNameInfo *begin = property_name_info_table;
		const PropertyNameInfo *end = begin + NUM_ELEMS(property_name_info_table);
		for ( ; begin != end; ++begin) {
			// Set up property name combo box from table.
			combobox->addItem(begin->name, begin->suggested_type);
		}
	}
	
	void
	populate_property_type_combobox(
			QComboBox *combobox,
			const GPlatesQtWidgets::EditWidgetGroupBox &edit_widget_group_box)
	{
		// Obtain a list of all property value type names that the EditWidgetGroupBox
		// is capable of handling, and ensure it is sorted.
		GPlatesQtWidgets::EditWidgetGroupBox::property_types_list_type list =
				edit_widget_group_box.get_handled_property_types_list();
		// Also include all property value types mentioned in the property_name_info_table,
		// because there may be types which are not implemented as an edit widget (yet).
		const PropertyNameInfo *info_begin = property_name_info_table;
		const PropertyNameInfo *info_end = info_begin + NUM_ELEMS(property_name_info_table);
		for ( ; info_begin != info_end; ++info_begin) {
			list.push_back(info_begin->suggested_type);
		}
		
		list.sort();
		list.unique();
		
		// Populate the combobox from the list of handled types.
		GPlatesQtWidgets::EditWidgetGroupBox::property_types_list_const_iterator it =
				list.begin();
		GPlatesQtWidgets::EditWidgetGroupBox::property_types_list_const_iterator end =
				list.end();
		for ( ; it != end; ++it) {
			combobox->addItem(*it);
		}
	}
	

	/**
	 * Attempts to convert a QString to a qualified PropertyName.
	 * If the namespace alias is 'gml' or 'gpml', this is relatively pain-free.
	 * However the user is currently free to enter whatever the hell they feel like.
	 */
	boost::optional<GPlatesModel::PropertyName>
	string_to_property_name(
			const QString &property_name_str)
	{
		boost::optional<GPlatesModel::PropertyName> property_name;
		QStringList parts = property_name_str.split(':');
		if (parts.size() == 2) {
			// alias:name format. Handle it if it is something we know (gml or gpml).
			// FIXME: Handle user-defined namespaces and aliases. Probably needs some
			// clever dialog to let the user edit the mapping.
			if (parts[0] == "gml") {
				return GPlatesModel::PropertyName::create_gml(parts[1]);
			} else if (parts[0] == "gpml") {
				return GPlatesModel::PropertyName::create_gpml(parts[1]);
			} else {
				// FIXME: Can't handle user defined namespaces.
				return boost::none;
			}
			
		} else if (parts.size() == 1) {
			// name (no alias). Assume the user is attempting to put it in the gpml namespace.
			return GPlatesModel::PropertyName::create_gpml(parts[1]);

		} else {
			// some invalid not qualified or 'overqualified' name.
			return boost::none;
		}
	}

	/**
	 * Attempts to convert a QString to a qualified TemplateTypeParameterType.
	 * If the namespace alias is 'gml' or 'gpml', this is relatively pain-free.
	 * However the user is currently free to enter whatever the hell they feel like.
	 */
	boost::optional<GPlatesPropertyValues::TemplateTypeParameterType>
	string_to_template_type(
			const QString &type_name_str)
	{
		boost::optional<GPlatesPropertyValues::TemplateTypeParameterType> type_name;
		QStringList parts = type_name_str.split(':');
		if (parts.size() == 2) {
			// alias:name format. Handle it if it is something we know (gml or gpml).
			// FIXME: Handle user-defined namespaces and aliases. Probably needs some
			// clever dialog to let the user edit the mapping.
			if (parts[0] == "gml") {
				return GPlatesPropertyValues::TemplateTypeParameterType::create_gml(parts[1]);
			} else if (parts[0] == "gpml") {
				return GPlatesPropertyValues::TemplateTypeParameterType::create_gpml(parts[1]);
			} else {
				// FIXME: Can't handle user defined namespaces.
				return boost::none;
			}
			
		} else if (parts.size() == 1) {
			// name (no alias). Assume the user is attempting to put it in the gpml namespace.
			return GPlatesPropertyValues::TemplateTypeParameterType::create_gpml(parts[1]);

		} else {
			// some invalid not qualified or 'overqualified' name.
			return boost::none;
		}
	}

	
	/**
	 * Wraps a newly-created PropertyValue in a time-dependent wrapper
	 * (GpmlConstantValue) if appropriate. If the property we are adding
	 * is not listed as taking time-dependent property values, this function
	 * simply returns the original PropertyValue::non_null_ptr_type.
	 *
	 * FIXME: We should really convert the property_info_table to use
	 * the new PropertyName class rather than QStrings, although we do
	 * need them as QStrings eventually because of the way QComboBox is
	 * feeding us the appropriate value type to select.
	 */
	GPlatesModel::PropertyValue::non_null_ptr_type
	add_time_dependency_if_necessary(
			const QString &property_name_as_string,
			const QString &property_type_as_string,
			const GPlatesModel::PropertyName &property_name,
			GPlatesModel::PropertyValue::non_null_ptr_type property_value)
	{
		// Assume no time dependency (The user might have entered their own
		// property name, and selected a PropertyValue type to create for it)
		bool is_time_dependent = false;
		// Look up the time-dependency from the property_info_table.
		const PropertyNameInfo *begin = property_name_info_table;
		const PropertyNameInfo *end = begin + NUM_ELEMS(property_name_info_table);
		for ( ; begin != end; ++begin) {
			if (begin->name == property_name_as_string) {
				is_time_dependent = begin->is_time_dependent;
				break;
			}
		}
		
		if (is_time_dependent) {
			// We need a time-dependent wrapper - set this up as a gpml:ConstantValue<thing>
			// for now, and the user can change it after the property is added with the
			// "ChangeTimeDependentPropertyDialog" (to be completed 2019)
			
			// We need to supply ConstantValue's constructor with a TemplateTypeParameterType of
			// the type of PropertyValue that it is wrapping.
			boost::optional<GPlatesPropertyValues::TemplateTypeParameterType> value_type = 
					string_to_template_type(property_type_as_string);
			
			if (value_type) {
				return GPlatesPropertyValues::GpmlConstantValue::create(property_value, *value_type);
			} else {
				// FIXME: Is this an exceptional state?
				return property_value;
			}
			
		} else {
			// Just keep it as an ordinary PropertyValue.
			return property_value;
		}
	}
}


GPlatesQtWidgets::AddPropertyDialog::AddPropertyDialog(
		GPlatesQtWidgets::EditFeaturePropertiesWidget &edit_widget,
		GPlatesPresentation::ViewState &view_state_,
		QWidget *parent_):
	QDialog(parent_, Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
	d_edit_feature_properties_widget_ptr(&edit_widget),
	d_edit_widget_group_box_ptr(new GPlatesQtWidgets::EditWidgetGroupBox(view_state_, this))
{
	setupUi(this);
	
	set_up_add_property_box();
	set_up_edit_widgets();
	populate_property_name_combobox(combobox_add_property_name);
	populate_property_type_combobox(combobox_add_property_type, *d_edit_widget_group_box_ptr);

	reset();
}


void
GPlatesQtWidgets::AddPropertyDialog::reset()
{
	// Doing setCurrentIndex() should cause a chain reaction that will reset everything else.
	// Except it doesn't, does it? No, if it's already on that value, there is no
	// 'change' event, so we can't rely on ANYTHING.
	static const int default_index = combobox_add_property_name->findText("gml:name");
	combobox_add_property_name->setCurrentIndex(default_index);
	set_appropriate_property_value_type(default_index);
	set_appropriate_edit_widget();
}


void
GPlatesQtWidgets::AddPropertyDialog::pop_up()
{
	reset();
	exec();
}


void
GPlatesQtWidgets::AddPropertyDialog::set_up_add_property_box()
{
	// Choose appropriate property value type for property name.
	QObject::connect(combobox_add_property_name, SIGNAL(currentIndexChanged(int)),
			this, SLOT(set_appropriate_property_value_type(int)));
	// Choose appropriate edit widget for property type.
	QObject::connect(combobox_add_property_type, SIGNAL(currentIndexChanged(int)),
			this, SLOT(set_appropriate_edit_widget()));

	// Add property when user hits "Add".
	QObject::connect(buttonBox, SIGNAL(accepted()),
			this, SLOT(add_property()));
}


void
GPlatesQtWidgets::AddPropertyDialog::set_up_edit_widgets()
{
	// Add the EditWidgetGroupBox. Ugly, but this is the price to pay if you want
	// to mix Qt Designer UIs with coded-by-hand UIs.
	QVBoxLayout *edit_layout = new QVBoxLayout;
	edit_layout->setSpacing(0);
	edit_layout->setMargin(0);
	edit_layout->addWidget(d_edit_widget_group_box_ptr);
	placeholder_edit_widget->setLayout(edit_layout);
	
	d_edit_widget_group_box_ptr->set_edit_verb("Add");
	QObject::connect(d_edit_widget_group_box_ptr, SIGNAL(commit_me()),
			buttonBox, SLOT(setFocus()));
}


void
GPlatesQtWidgets::AddPropertyDialog::set_appropriate_property_value_type(
		int index)
{
	// Update the "Type" combobox based on what property name is selected.
	QString suggested_value_type = combobox_add_property_name->itemData(index).toString();
	int target_index = combobox_add_property_type->findText(suggested_value_type);
	if (target_index != -1) {
		combobox_add_property_type->setCurrentIndex(target_index);
	}
}


void
GPlatesQtWidgets::AddPropertyDialog::set_appropriate_edit_widget()
{
	d_edit_widget_group_box_ptr->activate_widget_by_property_value_name(
			combobox_add_property_type->currentText());
}


void
GPlatesQtWidgets::AddPropertyDialog::add_property()
{
	if (d_edit_widget_group_box_ptr->is_edit_widget_active()) {
		// Calculate name.
		boost::optional<GPlatesModel::PropertyName> property_name = 
				string_to_property_name(combobox_add_property_name->currentText());
		if (property_name) {
		
			// If the name was something we understood, create the property value too.
			try {
				GPlatesModel::PropertyValue::non_null_ptr_type property_value =
						d_edit_widget_group_box_ptr->create_property_value_from_widget();
				
				property_value = add_time_dependency_if_necessary(
						combobox_add_property_name->currentText(),
						combobox_add_property_type->currentText(),
						*property_name,
						property_value);
				
				// Do the adding via EditFeaturePropertiesWidget, so that most of
				// the model-editing code is in one place.
				d_edit_feature_properties_widget_ptr->append_property_value_to_feature(
						property_value, *property_name);

			} catch (InvalidPropertyValueException &e) {
				// Not enough points for a constructable polyline, etc.
				QMessageBox::warning(this, tr("Property Value Invalid"),
						tr("The property can not be added: %1").arg(e.reason()),
						QMessageBox::Ok);
				return;
			}
		} else {
			// User supplied an incomprehensible property name.
			QMessageBox::warning(this, tr("Property Name Invalid"),
					tr("The supplied property name could not be understood. "
					"Please restrict property names to the 'gml:' or 'gpml:' namespace."),
					QMessageBox::Ok);
			return;
		}
	} else {
		// Something is wrong - we have no edit widget available.
		QMessageBox::warning(this, tr("Unable to add property"),
				tr("Sorry! Since there is no editing control available for this property "
				"value yet, it cannot be added to the feature."),
				QMessageBox::Ok);
		return;
	}
	accept();
}

