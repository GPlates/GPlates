/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, 2009 Geological Survey of Norway
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

#include <iostream>
#include <QString>
#include <QList>
#include <boost/optional.hpp>

#include "ShapefileAttributeFinder.h"
#include "model/FeatureHandle.h"
#include "model/TopLevelPropertyInline.h"
#include "model/FeatureRevision.h"

#include "property-values/GmlTimeInstant.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlKeyValueDictionary.h"
#include "property-values/GpmlKeyValueDictionaryElement.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GpmlPolarityChronId.h"
#include "property-values/GpmlMeasure.h"
#include "property-values/XsBoolean.h"
#include "property-values/XsDouble.h"
#include "property-values/XsInteger.h"
#include "property-values/XsString.h"

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
GPlatesFeatureVisitors::ShapefileAttributeFinder::visit_feature_handle(
		const GPlatesModel::FeatureHandle &feature_handle)
{
	// Visit each of the properties in turn.
	visit_feature_properties(feature_handle);
}


void
GPlatesFeatureVisitors::ShapefileAttributeFinder::visit_top_level_property_inline(
		const GPlatesModel::TopLevelPropertyInline &top_level_property_inline)
{
	
	QString property_name = GPlatesUtils::make_qstring_from_icu_string(top_level_property_inline.property_name().get_name());

	if (property_name != "shapefileAttributes") 
	{
		return;
	}
	
	visit_property_values(top_level_property_inline);
}


void
GPlatesFeatureVisitors::ShapefileAttributeFinder::visit_gpml_key_value_dictionary(
		const GPlatesPropertyValues::GpmlKeyValueDictionary &dictionary)
{

	std::vector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement>::const_iterator 
		iter = dictionary.elements().begin(),
		end = dictionary.elements().end();
	for ( ; iter != end; ++iter) {
		find_shapefile_attribute_in_element(*iter);
	}

}

void
GPlatesFeatureVisitors::ShapefileAttributeFinder::visit_xs_boolean(
		const GPlatesPropertyValues::XsBoolean &xs_boolean)
{
	d_found_qvariants.push_back(QVariant(xs_boolean.value()));
}


void
GPlatesFeatureVisitors::ShapefileAttributeFinder::visit_xs_double(
	const GPlatesPropertyValues::XsDouble& xs_double)
{
	d_found_qvariants.push_back(QVariant(xs_double.value()));
}

void
GPlatesFeatureVisitors::ShapefileAttributeFinder::visit_xs_integer(
	const GPlatesPropertyValues::XsInteger& xs_integer)
{
	d_found_qvariants.push_back(QVariant(xs_integer.value()));
}

void
GPlatesFeatureVisitors::ShapefileAttributeFinder::visit_xs_string(
		const GPlatesPropertyValues::XsString &xs_string)
{
	QString qstring = GPlatesUtils::make_qstring(xs_string.value());
	d_found_qvariants.push_back(QVariant(qstring));
}



void
GPlatesFeatureVisitors::ShapefileAttributeFinder::find_shapefile_attribute_in_element(
		const GPlatesPropertyValues::GpmlKeyValueDictionaryElement &element)
{
	QString temp = GPlatesUtils::make_qstring(element.key()->value());
	
	if (GPlatesUtils::make_qstring(
		 element.key()->value()) != d_attribute_name)
	{
		return;
	}
	element.value()->accept_visitor(*this);
}

