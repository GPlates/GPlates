/* $Id$ */

/**
 * \file 
 * Collects as much PLATES4 header information from features as possible.
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2006, 2007, 2008 The University of Sydney, Australia
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

#include <unicode/ustream.h>

#include "PlatesLineFormatHeaderVisitor.h"

#include "maths/ConstGeometryOnSphereVisitor.h"

#include "model/FeatureHandle.h"
#include "model/InlinePropertyContainer.h"

#include "property-values/GmlTimeInstant.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GpmlTimeSample.h"
#include "property-values/GpmlOldPlatesHeader.h"

#include "global/GPlatesAssert.h"


namespace
{
	/**
	* Convert a GeoTimeInstant instance to a double, for output in the PLATES4 line-format.
	*
	* This may involve the conversion of the GeoTimeInstant concepts of "distant past" and
	* "distant future" to the magic numbers 999.0 and -999.0 which are used in the PLATES4
	* line-format.
	*/
	const double &
		convert_geotimeinstant_to_double(
		const GPlatesPropertyValues::GeoTimeInstant &geo_time)
	{
		static const double distant_past_magic_number = 999.0;
		static const double distant_future_magic_number = -999.0;

		if (geo_time.is_distant_past()) {
			return distant_past_magic_number;
		} else if (geo_time.is_distant_future()) {
			return distant_future_magic_number;
		} else {
			return geo_time.value();
		}
	}


	const UnicodeString
		generate_geog_description(
		const UnicodeString &type, 
		const UnicodeString &id)
	{
		return "There was insufficient information to completely "\
			"generate the header for this data.  It came from GPlates, where "\
			"it had feature type \"" + type + "\" and feature id \"" + id + "\".";
	}
}


GPlatesFileIO::OldPlatesHeader::OldPlatesHeader(
	unsigned int region_number_,
	unsigned int reference_number_,
	unsigned int string_number_,
	const UnicodeString &geographic_description_,
	GPlatesModel::integer_plate_id_type plate_id_number_,
	double age_of_appearance_,
	double age_of_disappearance_,
	const UnicodeString &data_type_code_,
	unsigned int data_type_code_number_,
	const UnicodeString &data_type_code_number_additional_,
	GPlatesModel::integer_plate_id_type conjugate_plate_id_number_,
	unsigned int colour_code_,
	unsigned int number_of_points_) :
region_number(region_number_),
reference_number(reference_number_),
string_number(string_number_),
geographic_description(geographic_description_),
plate_id_number(plate_id_number_),
age_of_appearance(age_of_appearance_),
age_of_disappearance(age_of_disappearance_),
data_type_code(data_type_code_),
data_type_code_number(data_type_code_number_),
data_type_code_number_additional(data_type_code_number_additional_),
conjugate_plate_id_number(conjugate_plate_id_number_),
colour_code(colour_code_),
number_of_points(number_of_points_)
{
}


bool
GPlatesFileIO::PlatesLineFormatHeaderVisitor::get_old_plates_header(
	const GPlatesModel::FeatureHandle& feature_handle, OldPlatesHeader& old_plates_header)
{
	d_accum = PlatesHeaderAccumulator();

	// These two strings will be used to write the "Geographic description"
	// field in the case that no old PLATES4 header is found.
	d_accum.feature_type = feature_handle.feature_type().get_name();
	d_accum.feature_id   = feature_handle.feature_id().get();

	// Visit feature to collect property values.
	feature_handle.accept_visitor(*this);

	// Build an old plates header from the information we've gathered.
	if (d_accum.old_plates_header) {
		old_plates_header = *d_accum.old_plates_header;
	} else {
		// use the feature type and feature id info to make up a description.
		old_plates_header.geographic_description = 
			generate_geog_description(*d_accum.feature_type, *d_accum.feature_id);
	}

	/*
	* Override the old plates header values with any that GPlates has added.
	*/
	if (d_accum.plate_id) {
		old_plates_header.plate_id_number = *d_accum.plate_id;
	}
	if (d_accum.conj_plate_id) {
		old_plates_header.conjugate_plate_id_number = *d_accum.conj_plate_id;
	}
	if (d_accum.age_of_appearance) {
		old_plates_header.age_of_appearance = 
			convert_geotimeinstant_to_double(*d_accum.age_of_appearance);
	}
	if (d_accum.age_of_disappearance) {
		old_plates_header.age_of_disappearance = 
			convert_geotimeinstant_to_double(*d_accum.age_of_disappearance);
	}

	// Need at least an old plates header or plate id to be able to output header+geometry.
	return d_accum.plate_id || d_accum.plate_id;
}


void
GPlatesFileIO::PlatesLineFormatHeaderVisitor::visit_feature_handle(
	const GPlatesModel::FeatureHandle &feature_handle)
{
	// Visit each of the properties in turn.
	visit_feature_properties(feature_handle);
}


void
GPlatesFileIO::PlatesLineFormatHeaderVisitor::visit_inline_property_container(
	const GPlatesModel::InlinePropertyContainer &inline_property_container)
{
	d_accum.current_propname = inline_property_container.property_name();

	visit_property_values(inline_property_container);
}


void
GPlatesFileIO::PlatesLineFormatHeaderVisitor::visit_gml_time_instant(
	const GPlatesPropertyValues::GmlTimeInstant &gml_time_instant)
{
	// Only the "gml:validTime" is meaningful for the ages of appearance and disappearance.
	static const GPlatesModel::PropertyName validTime =
		GPlatesModel::PropertyName::create_gml("validTime");
	if (*d_accum.current_propname != validTime) {
		return;
	}

	if ( ! d_accum.age_of_appearance) {
		d_accum.age_of_appearance = gml_time_instant.time_position();
		d_accum.age_of_disappearance = gml_time_instant.time_position();
	} else {
		// The age of appearance was already set, which means that there was already a
		// "gml:validTime" property which contains a "gml:TimeInstant" or "gml:TimePeriod".
		// FIXME: Should we warn about this?
	}
}


void
GPlatesFileIO::PlatesLineFormatHeaderVisitor::visit_gml_time_period(
	const GPlatesPropertyValues::GmlTimePeriod &gml_time_period)
{
	// Only the "gml:validTime" is meaningful for the ages of appearance and disappearance.
	static const GPlatesModel::PropertyName validTime =
		GPlatesModel::PropertyName::create_gml("validTime");
	if (*d_accum.current_propname != validTime) {
		return;
	}

	if ( ! d_accum.age_of_appearance) {
		d_accum.age_of_appearance = gml_time_period.begin()->time_position();
		d_accum.age_of_disappearance = gml_time_period.end()->time_position();
	} else {
		// The age of appearance was already set, which means that there was already a
		// "gml:validTime" property which contains a "gml:TimeInstant" or "gml:TimePeriod".
		// FIXME: Should we warn about this?
	}
}


void
GPlatesFileIO::PlatesLineFormatHeaderVisitor::visit_gpml_constant_value(
	const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
{
	gpml_constant_value.value()->accept_visitor(*this);
}


void
GPlatesFileIO::PlatesLineFormatHeaderVisitor::visit_gpml_plate_id(
	const GPlatesPropertyValues::GpmlPlateId &gpml_plate_id)
{
	// This occurs in "gpml:ReconstructableFeature" instances.
	static const GPlatesModel::PropertyName reconstructionPlateId = 
		GPlatesModel::PropertyName::create_gpml("reconstructionPlateId");

	// This occurs in "gpml:Isochron" and its instantaneous equivalent.
	static const GPlatesModel::PropertyName conjugatePlateId =
		GPlatesModel::PropertyName::create_gpml("conjugatePlateId");

	// This occurs in "gpml:InstantaneousFeature" instances.
	static const GPlatesModel::PropertyName reconstructedPlateId =
		GPlatesModel::PropertyName::create_gpml("reconstructedPlateId");

	if (*d_accum.current_propname == reconstructionPlateId) {
		if ( ! d_accum.plate_id) {
			d_accum.plate_id = gpml_plate_id.value();
		} else {
			// The plate ID was already set, which means that there was already a
			// "gpml:reconstructionPlateId" property which contains a "gpml:PlateId".
			// FIXME: Should we warn about this?
		}
	} else if (*d_accum.current_propname == conjugatePlateId) {
		if ( ! d_accum.conj_plate_id) {
			d_accum.conj_plate_id = gpml_plate_id.value();
		} else {
			// The conjugate plate ID was already set, which means that there was
			// already a "gpml:conjugatePlateId" property which contains a
			// "gpml:PlateId".
			// FIXME: Should we warn about this?
		}
	} else if (*d_accum.current_propname == reconstructedPlateId) {
		// Nothing to do here.
	} else {
		// We've encountered a plate ID inside an unrecognised property-name.
		// FIXME:  Should we complain/warn/log about this?
	}
}


void
GPlatesFileIO::PlatesLineFormatHeaderVisitor::visit_gpml_old_plates_header(
	const GPlatesPropertyValues::GpmlOldPlatesHeader &gpml_old_plates_header)
{
	d_accum.old_plates_header = OldPlatesHeader(
		gpml_old_plates_header.region_number(),
		gpml_old_plates_header.reference_number(),
		gpml_old_plates_header.string_number(),
		gpml_old_plates_header.geographic_description(),
		gpml_old_plates_header.plate_id_number(),
		gpml_old_plates_header.age_of_appearance(),
		gpml_old_plates_header.age_of_disappearance(),
		gpml_old_plates_header.data_type_code(),
		gpml_old_plates_header.data_type_code_number(),
		gpml_old_plates_header.data_type_code_number_additional(),
		gpml_old_plates_header.conjugate_plate_id_number(),
		gpml_old_plates_header.colour_code(),
		gpml_old_plates_header.number_of_points());
}
