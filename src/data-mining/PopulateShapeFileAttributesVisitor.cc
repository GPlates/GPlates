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

#include "PopulateShapeFileAttributesVisitor.h"

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

bool
GPlatesDataMining::PopulateShapeFileAttributesVisitor::initialise_pre_property_values(
		const GPlatesModel::TopLevelPropertyInline &top_level_property_inline)
{
	static const GPlatesModel::PropertyName n = GPlatesModel::PropertyName::create_gpml("shapefileAttributes");
	return top_level_property_inline.property_name() == n;
}


void
GPlatesDataMining::PopulateShapeFileAttributesVisitor::visit_gpml_key_value_dictionary(
		const GPlatesPropertyValues::GpmlKeyValueDictionary &dictionary)
{
	std::vector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement>::const_iterator 
		iter = dictionary.elements().begin(),
		end = dictionary.elements().end();
	for ( ; iter != end; ++iter) 
	{
		d_names.push_back(iter->key()->value().get().qstring() );
	}
}

void
GPlatesDataMining::PopulateShapeFileAttributesVisitor::visit_xs_boolean(
		const GPlatesPropertyValues::XsBoolean &xs_boolean)
{
}


void
GPlatesDataMining::PopulateShapeFileAttributesVisitor::visit_xs_double(
	const GPlatesPropertyValues::XsDouble& xs_double)
{
}

void
GPlatesDataMining::PopulateShapeFileAttributesVisitor::visit_xs_integer(
	const GPlatesPropertyValues::XsInteger& xs_integer)
{
}

void
GPlatesDataMining::PopulateShapeFileAttributesVisitor::visit_xs_string(
		const GPlatesPropertyValues::XsString &xs_string)
{
}


