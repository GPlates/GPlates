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

#include <QString>
#include <QStringList>
#include "EditEnumerationWidget.h"

#include "property-values/Enumeration.h"
#include "model/ModelUtils.h"
#include "utils/UnicodeStringUtils.h"


#define NUM_ELEMS(a) (sizeof(a) / sizeof((a)[0]))


namespace
{
	/**
	 * Typedef for multimap used to convert static table into a map of QStringList.
	 */
	typedef std::multimap <const QString, const QString> intermediate_map_type;
	typedef intermediate_map_type::const_iterator intermediate_map_const_iterator;

	/**
	 * Struct used to define the const array of all possible enumeration values,
	 * for each individual gpml enumeration type.
	 */
	struct EnumerationTypeInfo
	{
		const QString name;
		const QString value;
	};
	
	/**
	 * Static table used to define all legal values for enumeration types.
	 * It will be loaded into an STL multimap.
	 */
	static const EnumerationTypeInfo enumeration_info_table[] = {
		{ "gpml:ContinentalBoundaryCrustEnumeration", "Continental" },
		{ "gpml:ContinentalBoundaryCrustEnumeration", "Oceanic" },
		{ "gpml:ContinentalBoundaryCrustEnumeration", "Unknown" },

		{ "gpml:ContinentalBoundaryEdgeEnumeration", "InnerContinentalBoundary" },
		{ "gpml:ContinentalBoundaryEdgeEnumeration", "OuterContinentalBoundary" },
		{ "gpml:ContinentalBoundaryEdgeEnumeration", "Unknown" },

		{ "gpml:ContinentalBoundarySideEnumeration", "Left" },
		{ "gpml:ContinentalBoundarySideEnumeration", "Right" },
		{ "gpml:ContinentalBoundarySideEnumeration", "Unknown" },

		{ "gpml:SubductionSideEnumeration", "Left" },
		{ "gpml:SubductionSideEnumeration", "Right" },
		{ "gpml:SubductionSideEnumeration", "Unknown" },

		{ "gpml:StrikeSlipEnumeration", "LeftLateral" },
		{ "gpml:StrikeSlipEnumeration", "RightLateral" },
		{ "gpml:StrikeSlipEnumeration", "None" },
		{ "gpml:StrikeSlipEnumeration", "Unknown" },

		{ "gpml:DipSlipEnumeration", "Extension" },
		{ "gpml:DipSlipEnumeration", "Compression" },
		{ "gpml:DipSlipEnumeration", "None" },
		{ "gpml:DipSlipEnumeration", "Unknown" },

		{ "gpml:DipSideEnumeration", "Left" },
		{ "gpml:DipSideEnumeration", "Right" },
		{ "gpml:DipSideEnumeration", "Undefined" },
		{ "gpml:DipSideEnumeration", "Unknown" },

		{ "gpml:SlipComponentEnumeration", "StrikeSlip" },
		{ "gpml:SlipComponentEnumeration", "DipSlip" },
		{ "gpml:SlipComponentEnumeration", "None" },
		{ "gpml:SlipComponentEnumeration", "Unknown" },

		{ "gpml:FoldPlaneAnnotationEnumeration", "Syncline" },
		{ "gpml:FoldPlaneAnnotationEnumeration", "Anticline" },
		{ "gpml:FoldPlaneAnnotationEnumeration", "None" },
		{ "gpml:FoldPlaneAnnotationEnumeration", "Unknown" },

		{ "gpml:AbsoluteReferenceFrameEnumeration", "Paleomag" },
		{ "gpml:AbsoluteReferenceFrameEnumeration", "HotSpot" },
		{ "gpml:AbsoluteReferenceFrameEnumeration", "Mantle" },
		{ "gpml:AbsoluteReferenceFrameEnumeration", "NoNetTorque" },
		{ "gpml:AbsoluteReferenceFrameEnumeration", "Other" },
	};
		
	
	bool
	is_property_value_type_handled(
			const QString &property_value_name)
	{
		const EnumerationTypeInfo *begin = enumeration_info_table;
		const EnumerationTypeInfo *end = begin + NUM_ELEMS(enumeration_info_table);
		for ( ; begin != end; ++begin) {
			if (begin->name == property_value_name) {
				return true;
			}
		}
		return false;
	}
	
	const intermediate_map_type &
	build_intermediate_map()
	{
		static intermediate_map_type intermediate_map;
		
		const EnumerationTypeInfo *begin = enumeration_info_table;
		const EnumerationTypeInfo *end = begin + NUM_ELEMS(enumeration_info_table);
		for ( ; begin != end; ++begin) {
			intermediate_map.insert(std::make_pair(begin->name, begin->value));
		}
		
		return intermediate_map;
	}
	
	const QStringList
	get_enumeration_string_list(
			const QString &property_value_name)
	{
		static const intermediate_map_type map = build_intermediate_map();
		QStringList list;
		
		intermediate_map_const_iterator it = map.lower_bound(property_value_name);
		intermediate_map_const_iterator end = map.upper_bound(property_value_name);
		for ( ; it != end; ++it) {
			list.append(it->second);
		}
		return list;
	}
}


GPlatesQtWidgets::EditEnumerationWidget::EditEnumerationWidget(
		QWidget *parent_):
	AbstractEditWidget(parent_)
{
	setupUi(this);
	reset_widget_to_default_values();
	
	QObject::connect(combobox_enumeration, SIGNAL(activated(int)),
			this, SLOT(handle_combobox_change()));
	
	setFocusPolicy(Qt::StrongFocus);
}


void
GPlatesQtWidgets::EditEnumerationWidget::configure_for_property_value_type(
		const QString &property_value_name)
{
	if (is_property_value_type_handled(property_value_name)) {
		d_property_value_name = property_value_name;
		combobox_enumeration->clear();
		combobox_enumeration->addItems(get_enumeration_string_list(d_property_value_name));
	} else {
		throw PropertyValueNotSupportedException();
	}
}


void
GPlatesQtWidgets::EditEnumerationWidget::reset_widget_to_default_values()
{
	combobox_enumeration->clear();
	combobox_enumeration->addItems(get_enumeration_string_list(d_property_value_name));
	set_clean();
}


void
GPlatesQtWidgets::EditEnumerationWidget::update_widget_from_enumeration(
		const GPlatesPropertyValues::Enumeration &enumeration)
{
	// Get the type of Enumeration to use from the Enumeration property value.
	QString enum_type = GPlatesUtils::make_qstring_from_icu_string(
			enumeration.type().get());
	configure_for_property_value_type(enum_type);
	
	QString enum_value = GPlatesUtils::make_qstring_from_icu_string(
			enumeration.value().get());
	int enum_index = combobox_enumeration->findText(enum_value);
	if (enum_index != -1) {
		// Present the user with the current enumeration value.
		combobox_enumeration->setCurrentIndex(enum_index);
	} else {
		// Found a value we're not supposed to have for this enumeration type.
		// Add it because The User Is Always Right!
		combobox_enumeration->addItem(enum_value);
		combobox_enumeration->setCurrentIndex(combobox_enumeration->count() - 1);
	}
	set_clean();
}


GPlatesModel::PropertyValue::non_null_ptr_type
GPlatesQtWidgets::EditEnumerationWidget::create_property_value_from_widget() const
{
	if (is_property_value_type_handled(d_property_value_name)) {
		const QString value = combobox_enumeration->currentText();
		GPlatesModel::PropertyValue::non_null_ptr_type property_value = 
				GPlatesPropertyValues::Enumeration::create(
						GPlatesUtils::make_icu_string_from_qstring(d_property_value_name),
						GPlatesUtils::make_icu_string_from_qstring(value));
		return property_value;
	} else {
		throw PropertyValueNotSupportedException();
	}
}

