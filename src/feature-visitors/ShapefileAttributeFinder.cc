/* $Id: ShapefileAttributeFinder.cc 2828 2008-04-24 07:49:42Z jclark $ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date: 2008-04-24 09:49:42 +0200 (to, 24 apr 2008) $
 * 
 * Copyright (C) 2008, Geological Survey of Norway
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
#include <QList>
#include <boost/optional.hpp>

#include "ShapefileAttributeFinder.h"
#include "model/FeatureHandle.h"
#include "model/InlinePropertyContainer.h"
#include "model/FeatureRevision.h"

#include "property-values/GmlTimeInstant.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/GpmlConstantValue.h"
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
	
	
	QVariant
	geo_time_instant_to_qvariant(
			const GPlatesPropertyValues::GeoTimeInstant &time_position)
	{
		if (time_position.is_real()) {
			return QVariant(time_position.value());
		} else if (time_position.is_distant_past()) {
			return QVariant(QObject::tr("distant past"));
		} else if (time_position.is_distant_future()) {
			return QVariant(QObject::tr("distant future"));
		} else {
			return QVariant(QObject::tr("<Invalid time position>"));
		}
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
GPlatesFeatureVisitors::ShapefileAttributeFinder::visit_inline_property_container(
		const GPlatesModel::InlinePropertyContainer &inline_property_container)
{
	QString property_name = GPlatesUtils::make_qstring_from_icu_string(inline_property_container.property_name().get_name());
	QString property_namespace_alias = GPlatesUtils::make_qstring_from_icu_string(inline_property_container.property_name().get_namespace_alias());


	if ((property_name != d_attribute_name) || (property_namespace_alias != "shapefile"))
	 
	{
		return;

	}

	visit_property_values(inline_property_container);
}


void
GPlatesFeatureVisitors::ShapefileAttributeFinder::visit_gml_time_instant(
		const GPlatesPropertyValues::GmlTimeInstant &gml_time_instant)
{
	const GPlatesPropertyValues::GeoTimeInstant &time_position = gml_time_instant.time_position();
	d_found_qvariants.push_back(geo_time_instant_to_qvariant(time_position));
}


void
GPlatesFeatureVisitors::ShapefileAttributeFinder::visit_gml_time_period(
		const GPlatesPropertyValues::GmlTimePeriod &gml_time_period)
{
	const GPlatesPropertyValues::GeoTimeInstant begin = gml_time_period.begin()->time_position();
	const GPlatesPropertyValues::GeoTimeInstant end = gml_time_period.end()->time_position();
	
	QString str = QString("%1 - %2")
			.arg(geo_time_instant_to_qvariant(begin).toString())
			.arg(geo_time_instant_to_qvariant(end).toString());
		d_found_qvariants.push_back(QVariant(str));

}


void
GPlatesFeatureVisitors::ShapefileAttributeFinder::visit_gpml_constant_value(
		const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
{
	gpml_constant_value.value()->accept_visitor(*this);
}


void
GPlatesFeatureVisitors::ShapefileAttributeFinder::visit_gpml_plate_id(
		const GPlatesPropertyValues::GpmlPlateId &gpml_plate_id)
{
	d_found_qvariants.push_back(QVariant(static_cast<quint32>(gpml_plate_id.value())));
}


void
GPlatesFeatureVisitors::ShapefileAttributeFinder::visit_gpml_polarity_chron_id(
		const GPlatesPropertyValues::GpmlPolarityChronId &gpml_polarity_chron_id)
{
	const boost::optional<QString> &era = gpml_polarity_chron_id.get_era();
	const boost::optional<unsigned int> &major = gpml_polarity_chron_id.get_major_region();
	const boost::optional<QString> &minor = gpml_polarity_chron_id.get_minor_region();

	QString str;
	if (era) {
		str.append(*era);
		str.append(" ");
	}
	if (major) {
		str.append(QString::number(*major));
	}
	if (minor) {
		str.append(*minor);
	}
	d_found_qvariants.push_back(QVariant(str));
}


void
GPlatesFeatureVisitors::ShapefileAttributeFinder::visit_gpml_measure(
		const GPlatesPropertyValues::GpmlMeasure &gpml_measure)
{
	// FIXME: Ideally we'd render things like the degrees symbol depending on the value of
	// the uom attribute.
	d_found_qvariants.push_back(QVariant(gpml_measure.quantity()));
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


