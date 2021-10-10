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

#include "model/PropertyName.h"


#define NUM_ELEMS(a) (sizeof(a) / sizeof((a)[0]))


namespace
{
	struct PropertyNameInfo
	{
		const QString name;
		const QString suggested_type;
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
	 *
	 * FIXME: Perhaps some kind of flag could be used to indicate properties which might
	 * support a TimeDependentPropertyValue (although this varies...). This flag could be
	 * passed to the EditWidgetGroupBox to signal that it should enable its "Time Dependent Property"
	 * editing widget on top of the regular edit widget.
	 */
	static const PropertyNameInfo property_name_info_table[] = {
		{ "gpml:angle", "gpml:angle" },
		{ "gpml:boundary", "gml:Polygon" }, // TimeDependentPropertyValue<>
		{ "gpml:centerLineOf", "gml:_Geometry" }, // TimeDependentPropertyValue<>
		{ "gpml:continentalSide", "gpml:ContinentalBoundarySideEnumeration" }, // Enumeration
		{ "gml:description", "xs:string" },
		{ "gpml:dipAngle", "gpml:angle" },
		{ "gpml:dipSide", "gpml:DipSideEnumeration" }, // Enumeration
		{ "gpml:dipSlip", "gpml:DipSlipEnumeration" }, // Enumeration
		{ "gpml:edge", "gpml:ContinentalBoundaryEdgeEnumeration" }, // Enumeration
		{ "gpml:errorBounds", "gml:_Geometry" }, // TimeDependentPropertyValue<_Geometry>
	//	{ "gpml:fixedReferenceFrame", "gpml:plateId" }, // For TotalReconstructionSequence
	//	{ "gpml:movingReferenceFrame", "gpml:plateId" }, // For TotalReconstructionSequence
		{ "gpml:foldAnnotation", "gpml:FoldPlaneAnnotationEnumeration" }, // Enumeration
		{ "gpml:isActive", "xs:boolean" }, // TimeDependentPropertyValue<>
		{ "gpml:leftPlate", "gpml:plateId" },
	//	{ "gpml:leftUnit", "gpml:FeatureReference" }, // FeatureReference<AbstractRockUnit>
	//	{ "gml:metaDataProperty", "gml:_MetaData" },
		{ "gpml:motion", "gpml:StrikeSlipEnumeration" }, // Enumeration. Just for fun, it's also time-dependent.
		{ "gml:name", "xs:string" },
		{ "gpml:oldPlatesHeader", "gpml:OldPlatesHeader" },
		{ "gpml:outlineOf", "gml:_Geometry" },
		{ "gpml:polarityChronId", "gpml:PolarityChronId" },
		{ "gpml:polarityChronOffset", "xs:double" },
		{ "gpml:position", "gml:Point" },
		{ "gpml:primarySlipComponent", "gpml:SlipComponentEnumeration" }, // Enumeration
		{ "gpml:reconstructedPlateId", "gpml:plateId" }, // For InstantaneousFeatures
		{ "gpml:reconstructedTime", "gml:TimeInstant" }, // For InstantaneousFeatures
		{ "gpml:reconstructionPlateId", "gpml:plateId" }, // For ReconstructableFeatures
		{ "gpml:rightPlate", "gpml:plateId" },
	//	{ "gpml:rightUnit", "gpml:FeatureReference" }, // FeatureReference<AbstractRockUnit>
	//	{ "gpml:shipTrack", "gpml:FeatureReference" }, // FeatureReference<MagneticAnomalyShipTrack>
		{ "gpml:strikeSlip", "gpml:StrikeSlipEnumeration" },
		{ "gpml:subductingSlab", "gpml:SubductionSideEnumeration" }, // TimeDependentPropertyValue<>
	//	{ "gpml:totalReconstructionPole", "gpml:FiniteRotation" }, // For TotalReconstructionSequence. IrregularSampling<FiniteRotation>
	//	{ "gpml:type", "gpml:AbsoluteReferenceFrameEnumeration" }, // Enumeration. For AbsoluteReferenceFrame.
		{ "gpml:unclassifiedGeometry", "gml:_Geometry" },
		{ "gml:validTime", "gml:TimePeriod" },
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

}


GPlatesQtWidgets::AddPropertyDialog::AddPropertyDialog(
		GPlatesGui::FeaturePropertyTableModel &property_model,
		QWidget *parent_):
	QDialog(parent_),
	d_property_model_ptr(&property_model),
	d_edit_widget_group_box_ptr(new GPlatesQtWidgets::EditWidgetGroupBox(this))
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
			GPlatesModel::PropertyValue::non_null_ptr_type property_value =
					d_edit_widget_group_box_ptr->create_property_value_from_widget();
					
			// Do the adding via FeaturePropertyTableModel, so that when we have Undo/Redo,
			// it can worry about that and no-one else has to.
			d_property_model_ptr->append_property_value_to_feature(
					property_value, *property_name);
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

