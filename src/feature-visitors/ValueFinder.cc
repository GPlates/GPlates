/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date: 2008-07-24 19:17:34 -0700 (Thu, 24 Jul 2008) $
 * 
 * Copyright (C) 2007, 2008 The University of Sydney, Australia
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

#include <algorithm>  // std::find
#include <sstream>
#include <string>
#include <iostream>

#include "ValueFinder.h"

#include "model/FeatureHandle.h"
#include "model/InlinePropertyContainer.h"
#include "model/FeatureRevision.h"

#include "property-values/GmlTimeInstant.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GpmlOldPlatesHeader.h"
#include "property-values/Enumeration.h"
#include "property-values/XsBoolean.h"
#include "property-values/XsDouble.h"
#include "property-values/XsInteger.h"
#include "property-values/XsString.h"

#include "qt-widgets/EditTimePeriodWidget.h"

#include "utils/UnicodeStringUtils.h"


namespace
{
	template<typename C, typename E>
	bool
	contains_elem(
			const C &container,
			const E &elem)
	{
		return (std::find(container.begin(), container.end(), elem) != container.end());
	}
}


void
GPlatesFeatureVisitors::ValueFinder::visit_feature_handle(
		const GPlatesModel::FeatureHandle &feature_handle)
{
	// Now visit each of the properties in turn.
	visit_feature_properties(feature_handle);
}


void
GPlatesFeatureVisitors::ValueFinder::visit_inline_property_container(
		const GPlatesModel::InlinePropertyContainer &inline_property_container)
{
	const GPlatesModel::PropertyName &curr_prop_name = inline_property_container.property_name();

	if ( ! d_property_names_to_allow.empty()) {
		// We're not allowing all property names.
		if ( ! contains_elem(d_property_names_to_allow, curr_prop_name)) {
			// The current property name is not allowed.
			return;
		}
	}

	visit_property_values(inline_property_container);
}


void
GPlatesFeatureVisitors::ValueFinder::visit_gpml_constant_value(
		const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
{
	gpml_constant_value.value()->accept_visitor(*this);
}


void
GPlatesFeatureVisitors::ValueFinder::visit_gpml_old_plates_header(
		const GPlatesPropertyValues::GpmlOldPlatesHeader &gpml_old_plates_header)
{
	std::string old_id = gpml_old_plates_header.old_feature_id();
	d_found_value.push_back( old_id );
}



void
GPlatesFeatureVisitors::ValueFinder::visit_gpml_plate_id(
		const GPlatesPropertyValues::GpmlPlateId &gpml_plate_id)
{
	std::ostringstream oss;
	oss << gpml_plate_id.value();
	d_found_value.push_back( oss.str() );
}

void
GPlatesFeatureVisitors::ValueFinder::visit_xs_string(
	const GPlatesPropertyValues::XsString &xs_string)
{
	std::string s = GPlatesUtils::make_qstring( xs_string.value() ).toStdString();
	d_found_value.push_back( GPlatesUtils::make_qstring( xs_string.value() ).toStdString() );
}




