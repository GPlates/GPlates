/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, 2009 The University of Sydney, Australia
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

#include "ToQvariantConverter.h"
#include "model/FeatureHandle.h"
#include "model/TopLevelPropertyInline.h"
#include "model/FeatureRevision.h"

#include "property-values/GmlTimeInstant.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GpmlPolarityChronId.h"
#include "property-values/GpmlMeasure.h"
#include "property-values/GpmlOldPlatesHeader.h"
#include "property-values/Enumeration.h"
#include "property-values/UninterpretedPropertyValue.h"
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
			const GPlatesPropertyValues::GeoTimeInstant &time_position,
			int role)
	{
		if (time_position.is_real()) {
			return QVariant(time_position.value());
		} else if (time_position.is_distant_past()) {
			if (role == Qt::EditRole) {
				return QVariant(QString("http://gplates.org/times/distantPast"));
			} else {
				return QVariant(QObject::tr("distant past"));
			}
		} else if (time_position.is_distant_future()) {
			if (role == Qt::EditRole) {
				return QVariant(QString("http://gplates.org/times/distantFuture"));
			} else {
				return QVariant(QObject::tr("distant future"));
			}
		} else {
			return QVariant(QObject::tr("<Invalid time position>"));
		}
	}

}


void
GPlatesFeatureVisitors::ToQvariantConverter::visit_feature_handle(
		const GPlatesModel::FeatureHandle &feature_handle)
{
	// Visit each of the properties in turn.
	visit_feature_properties(feature_handle);
}


void
GPlatesFeatureVisitors::ToQvariantConverter::visit_top_level_property_inline(
		const GPlatesModel::TopLevelPropertyInline &top_level_property_inline)
{
	visit_property_values(top_level_property_inline);
}


void
GPlatesFeatureVisitors::ToQvariantConverter::visit_enumeration(
		const GPlatesPropertyValues::Enumeration &enumeration)
{
	QString qstring = GPlatesUtils::make_qstring_from_icu_string(enumeration.value().get());
	d_found_values.push_back(QVariant(qstring));
}


void
GPlatesFeatureVisitors::ToQvariantConverter::visit_gml_time_instant(
		const GPlatesPropertyValues::GmlTimeInstant &gml_time_instant)
{
	const GPlatesPropertyValues::GeoTimeInstant &time_position = gml_time_instant.time_position();
	d_found_values.push_back(geo_time_instant_to_qvariant(time_position, d_role));
}


void
GPlatesFeatureVisitors::ToQvariantConverter::visit_gml_time_period(
		const GPlatesPropertyValues::GmlTimePeriod &gml_time_period)
{
	const GPlatesPropertyValues::GeoTimeInstant begin = gml_time_period.begin()->time_position();
	const GPlatesPropertyValues::GeoTimeInstant end = gml_time_period.end()->time_position();
	
	if (d_role == Qt::EditRole) {
		QList<QVariant> list;
		list.append(geo_time_instant_to_qvariant(begin, d_role));
		list.append(geo_time_instant_to_qvariant(end, d_role));
		d_found_values.push_back(QVariant(list));
	} else {
		QString str = QString("%1 - %2")
				.arg(geo_time_instant_to_qvariant(begin, d_role).toString())
				.arg(geo_time_instant_to_qvariant(end, d_role).toString());
		d_found_values.push_back(QVariant(str));
	}
}


void
GPlatesFeatureVisitors::ToQvariantConverter::visit_gpml_constant_value(
		const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
{
	d_found_time_dependencies.push_back(QVariant("ConstantValue"));
	gpml_constant_value.value()->accept_visitor(*this);
}


void
GPlatesFeatureVisitors::ToQvariantConverter::visit_gpml_plate_id(
		const GPlatesPropertyValues::GpmlPlateId &gpml_plate_id)
{
	d_found_values.push_back(QVariant(static_cast<quint32>(gpml_plate_id.value())));
}


void
GPlatesFeatureVisitors::ToQvariantConverter::visit_gpml_polarity_chron_id(
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
	d_found_values.push_back(QVariant(str));
}


void
GPlatesFeatureVisitors::ToQvariantConverter::visit_gpml_measure(
		const GPlatesPropertyValues::GpmlMeasure &gpml_measure)
{
	// FIXME: Ideally we'd render things like the degrees symbol depending on the value of
	// the uom attribute. (urn:ogc:def:uom:OGC:1.0:degree)
	// Naturally this would be for DisplayRole only; EditRole would need the raw double value.
	d_found_values.push_back(QVariant(gpml_measure.quantity()));
}


void
GPlatesFeatureVisitors::ToQvariantConverter::visit_gpml_old_plates_header(
		const GPlatesPropertyValues::GpmlOldPlatesHeader &gpml_old_plates_header)
{
	static const QChar zero_padded('0');
	static const QChar space_padded(' ');
	
	const GPlatesPropertyValues::GpmlOldPlatesHeader &header = gpml_old_plates_header;
	d_found_values.push_back(QVariant(QString("%L1 %2 %3 %4 %5 %L6 %L7 %8 %9 %10 %11 %12 %13")
			.arg(header.region_number(), 2, 10, zero_padded)
			.arg(header.reference_number(), 2, 10, zero_padded)
			.arg(header.string_number(), 4, 10, zero_padded)
			.arg(GPlatesUtils::make_qstring_from_icu_string(header.geographic_description()))
			.arg(header.plate_id_number(), 3, 10, zero_padded)
			.arg(header.age_of_appearance(), 6, 'f', 1, space_padded)
			.arg(header.age_of_disappearance(), 6, 'f', 1, space_padded)
			.arg(GPlatesUtils::make_qstring_from_icu_string(header.data_type_code()))
			.arg(header.data_type_code_number(), 4, 10, zero_padded)
			.arg(GPlatesUtils::make_qstring_from_icu_string(header.data_type_code_number_additional()))
			.arg(header.conjugate_plate_id_number(), 3, 10, zero_padded)
			.arg(header.colour_code(), 3, 10, zero_padded)
			.arg(header.number_of_points(), 5, 10, zero_padded)
			));
}


void
GPlatesFeatureVisitors::ToQvariantConverter::visit_uninterpreted_property_value(
		const GPlatesPropertyValues::UninterpretedPropertyValue &uninterpreted_prop_val)
{
	QString buf;
	QXmlStreamWriter writer(&buf);
	writer.writeDefaultNamespace("http://www.gplates.org/gplates");
	uninterpreted_prop_val.value()->write_to(writer);
	d_found_values.push_back(buf);
}


void
GPlatesFeatureVisitors::ToQvariantConverter::visit_xs_boolean(
		const GPlatesPropertyValues::XsBoolean &xs_boolean)
{
	d_found_values.push_back(QVariant(xs_boolean.value()));
}


void
GPlatesFeatureVisitors::ToQvariantConverter::visit_xs_double(
	const GPlatesPropertyValues::XsDouble& xs_double)
{
	d_found_values.push_back(QVariant(xs_double.value()));
}

void
GPlatesFeatureVisitors::ToQvariantConverter::visit_xs_integer(
	const GPlatesPropertyValues::XsInteger& xs_integer)
{
	d_found_values.push_back(QVariant(xs_integer.value()));
}

void
GPlatesFeatureVisitors::ToQvariantConverter::visit_xs_string(
		const GPlatesPropertyValues::XsString &xs_string)
{
	QString qstring = GPlatesUtils::make_qstring(xs_string.value());
	d_found_values.push_back(QVariant(qstring));
}


