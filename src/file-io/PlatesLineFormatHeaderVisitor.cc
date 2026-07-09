/* $Id$ */

/**
 * \file 
 * Collects as much PLATES4 header information from features as possible.
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 The University of Sydney, Australia
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

#include "PlatesLineFormatHeaderVisitor.h"
#include "PlatesFormatUtils.h"

#include "feature-visitors/PropertyValueFinder.h"

#include "maths/ConstGeometryOnSphereVisitor.h"

#include "model/FeatureHandle.h"
#include "model/TopLevelPropertyInline.h"

#include "property-values/XsBoolean.h"
#include "property-values/Enumeration.h"
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
	double
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

	/**
	 * Generate a geographic description when we have nothing to put there.
	 */
	const GPlatesUtils::UnicodeString
	generate_geog_description()
	{
		return "new feature";
	}

	/**
	 * Add <identity>feature_id</identity> to end of geographic description.
	 */
	void
	append_feature_id_to_geog_description(
			const GPlatesModel::FeatureId &feature_id,
			GPlatesUtils::UnicodeString &geog_description)
	{
		GPlatesUtils::UnicodeString feature_id_tag = " <identity>" + feature_id.get() + "</identity>";
		geog_description += feature_id_tag;
	}
}

GPlatesFileIO::OldPlatesHeader::OldPlatesHeader(
	unsigned int region_number_,
	unsigned int reference_number_,
	unsigned int string_number_,
	const GPlatesUtils::UnicodeString &geographic_description_,
	GPlatesModel::integer_plate_id_type plate_id_number_,
	double age_of_appearance_,
	double age_of_disappearance_,
	const GPlatesUtils::UnicodeString &data_type_code_,
	unsigned int data_type_code_number_,
	const GPlatesUtils::UnicodeString &data_type_code_number_additional_,
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


GPlatesPropertyValues::GpmlOldPlatesHeader::non_null_ptr_type
GPlatesFileIO::OldPlatesHeader::create_gpml_old_plates_header()
{
	return GPlatesPropertyValues::GpmlOldPlatesHeader::create(
			region_number,
			reference_number,
			string_number,
			geographic_description,
			plate_id_number,
			age_of_appearance,
			age_of_disappearance,
			data_type_code,
			data_type_code_number,
			data_type_code_number_additional,
			conjugate_plate_id_number,
			colour_code,
			number_of_points);
}


GPlatesFileIO::PlatesLineFormatHeaderVisitor::PlatesLineFormatHeaderVisitor()
{
}

void
GPlatesFileIO::PlatesLineFormatHeaderVisitor::get_old_plates_header(
		const GPlatesModel::FeatureHandle::const_weak_ref &feature,
		OldPlatesHeader& old_plates_header,
		bool append_feature_id_to_geographic_description)
{
	d_accum = PlatesHeaderAccumulator();

	// Visit feature to collect property values.
	visit_feature(feature);

	// Build an old plates header from the information we've gathered.
	if (d_accum.old_plates_header)
	{
		old_plates_header = *d_accum.old_plates_header;
	}
	else
	{
		//
		// If we don't have an old plates header then fill in the missing attributes
		// as best we can.
		//

		if (d_accum.geographic_description) {
			old_plates_header.geographic_description = *d_accum.geographic_description;
		} else {
			old_plates_header.geographic_description = generate_geog_description();
		}

		// NOTE: we cannot write out default numbers '0' here since apparently
		// some fortran programs and Intertec will ignore features with this string
		// in the header.
		// So write out numbers that are not likely to correspond to numbers used by
		// another feature. '99' and '9999' are chosen because Plates4 uses "999"
		// as a comment value and '99.0' as an end-of-coordinates value indicating
		// that these values are probably not meaningful.
		old_plates_header.region_number = 99;
		old_plates_header.reference_number = 99;
		old_plates_header.string_number = 9999;

		// Determine the two-letter PLATES data type code string based on the feature type.
		old_plates_header.data_type_code = PlatesFormatUtils::get_plates_data_type_code(feature);
	}

	// Regardless of whether there's a gpml:oldPlatesHeader property we need to
	// add the feature id somewhere. The end of the geographic description seems
	// like a good place.
	if (append_feature_id_to_geographic_description)
	{
		append_feature_id_to_geog_description(feature->feature_id(),
				old_plates_header.geographic_description);
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
}


void
GPlatesFileIO::PlatesLineFormatHeaderVisitor::visit_gml_time_instant(
	const GPlatesPropertyValues::GmlTimeInstant &gml_time_instant)
{
	// Only the "gml:validTime" is meaningful for the ages of appearance and disappearance.
	static const GPlatesModel::PropertyName validTime =
		GPlatesModel::PropertyName::create_gml("validTime");
	if (*current_top_level_propname() != validTime) {
		return;
	}

	if ( ! d_accum.age_of_appearance) {
		d_accum.age_of_appearance = gml_time_instant.get_time_position();
		d_accum.age_of_disappearance = gml_time_instant.get_time_position();
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
	if (*current_top_level_propname() != validTime) {
		return;
	}

	if ( ! d_accum.age_of_appearance) {
		d_accum.age_of_appearance = gml_time_period.begin()->get_time_position();
		d_accum.age_of_disappearance = gml_time_period.end()->get_time_position();
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

	if (*current_top_level_propname() == reconstructionPlateId) {
		if ( ! d_accum.plate_id) {
			d_accum.plate_id = gpml_plate_id.get_value();
		} else {
			// The plate ID was already set, which means that there was already a
			// "gpml:reconstructionPlateId" property which contains a "gpml:PlateId".
			// FIXME: Should we warn about this?
		}
	} else if (*current_top_level_propname() == conjugatePlateId) {
		if ( ! d_accum.conj_plate_id) {
			d_accum.conj_plate_id = gpml_plate_id.get_value();
		} else {
			// The conjugate plate ID was already set, which means that there was
			// already a "gpml:conjugatePlateId" property which contains a
			// "gpml:PlateId".
			// FIXME: Should we warn about this?
		}
	} else if (*current_top_level_propname() == reconstructedPlateId) {
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
		gpml_old_plates_header.get_region_number(),
		gpml_old_plates_header.get_reference_number(),
		gpml_old_plates_header.get_string_number(),
		gpml_old_plates_header.get_geographic_description(),
		gpml_old_plates_header.get_plate_id_number(),
		gpml_old_plates_header.get_age_of_appearance(),
		gpml_old_plates_header.get_age_of_disappearance(),
		gpml_old_plates_header.get_data_type_code(),
		gpml_old_plates_header.get_data_type_code_number(),
		gpml_old_plates_header.get_data_type_code_number_additional(),
		gpml_old_plates_header.get_conjugate_plate_id_number(),
		gpml_old_plates_header.get_colour_code(),
		gpml_old_plates_header.get_number_of_points());
}

void
GPlatesFileIO::PlatesLineFormatHeaderVisitor::visit_xs_string(
	const GPlatesPropertyValues::XsString &xs_string)
{
	static const GPlatesModel::PropertyName name_property_name =
		GPlatesModel::PropertyName::create_gml("name");
	if (*current_top_level_propname() != name_property_name) {
		return;
	}

	// Only store 'gml:name' property in geographic description if it exists
	// and it not empty.
	if ( ! d_accum.geographic_description) {
		const GPlatesUtils::UnicodeString &name = xs_string.get_value().get();
		if (!name.isEmpty())
		{
			d_accum.geographic_description = name;
		}
	} else {
		// The name_property_name was already set, which means that there was already a
		// "gml:name" property.
		// FIXME: Should we warn about this?
	}
}
