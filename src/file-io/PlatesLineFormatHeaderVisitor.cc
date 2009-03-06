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

#include "feature-visitors/PropertyValueFinder.h"

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
	 * Used in the data type code field of PLATES header to indicate an unknown or invalid type.
	 */
	const UnicodeString INVALID_DATA_TYPE_CODE = "XX";

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

	/**
	 * Generate a geographic description when we have nothing to put there.
	 */
	const UnicodeString
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
			UnicodeString &geog_description)
	{
		UnicodeString feature_id_tag = " <identity>" + feature_id.get() + "</identity>";
		geog_description += feature_id_tag;
	}

	/**
	 * If specified feature has "isActive" boolean property then return active data type code
	 * if its value is true otherwise return inactive data type code.
	 * If specified feature does not have "isActive" boolean property then
	 * return inactive data type code.
	 */
	UnicodeString
	get_data_type_code_for_active_inactive_feature(
			const GPlatesModel::FeatureHandle &feature_handle,
			const UnicodeString &active_data_type_code,
			const UnicodeString &inactive_data_type_code)
	{
		static const GPlatesModel::PropertyName is_active_property_name = 
			GPlatesModel::PropertyName::create_gpml("isActive");

		// See if active or not.
		const GPlatesPropertyValues::XsBoolean *is_active_property_value;
		if (GPlatesFeatureVisitors::get_property_value(
				feature_handle, is_active_property_name, &is_active_property_value))
		{
			return is_active_property_value->value() ? active_data_type_code : inactive_data_type_code;
		}

		// No "isActive" property on feature so assume inactive.
		return inactive_data_type_code;
	}

	UnicodeString
	get_data_type_code_for_aseismic_ridge(
			const GPlatesModel::FeatureHandle &)
	{
		return "AR";
	}

	UnicodeString
	get_data_type_code_for_bathymetry(
			const GPlatesModel::FeatureHandle &)
	{
		return "BA";
	}

	UnicodeString
	get_data_type_code_for_basin(
			const GPlatesModel::FeatureHandle &)
	{
		return "BS";
	}

	UnicodeString
	get_data_type_code_for_continental_boundary(
			const GPlatesModel::FeatureHandle &)
	{
		// Could also be "CM" or "CO" according to "PlatesLineFormatHeaderVisitor.h".
		return "CB";
	}

	UnicodeString
	get_data_type_code_for_continental_fragment(
			const GPlatesModel::FeatureHandle &)
	{
		return "CF";
	}

	UnicodeString
	get_data_type_code_for_craton(
			const GPlatesModel::FeatureHandle &)
	{
		return "CR";
	}

	UnicodeString
	get_data_type_code_for_coastline(
			const GPlatesModel::FeatureHandle &)
	{
		return "CS";
	}

	UnicodeString
	get_data_type_code_for_extended_continental_crust(
			const GPlatesModel::FeatureHandle &)
	{
		return "EC";
	}

	UnicodeString
	get_data_type_code_for_fault(
			const GPlatesModel::FeatureHandle &feature_handle)
	{
		//
		// This function is effectively the reverse of what's in "PlatesLineFormatReader.cc".
		//

		static const GPlatesModel::PropertyName dipslip_property_name = 
			GPlatesModel::PropertyName::create_gpml("dipSlip");

		const GPlatesPropertyValues::Enumeration *dipslip_property_value;
		if (GPlatesFeatureVisitors::get_property_value(
				feature_handle, dipslip_property_name, &dipslip_property_value))
		{
			static GPlatesPropertyValues::EnumerationType dipslip_enumeration_type(
				"gpml:DipSlipEnumeration");
			static GPlatesPropertyValues::EnumerationContent dipslip_enumeration_value_compression(
				"Compression");
			static GPlatesPropertyValues::EnumerationContent dipslip_enumeration_value_extension(
				"Extension");

			if (dipslip_enumeration_type.is_equal_to(dipslip_property_value->type()))
			{
				if (dipslip_enumeration_value_compression.is_equal_to(dipslip_property_value->value()))
				{
					return "NF";
				}
				if (dipslip_enumeration_value_extension.is_equal_to(dipslip_property_value->value()))
				{
					static const GPlatesModel::PropertyName subcategory_property_name = 
						GPlatesModel::PropertyName::create_gpml("subcategory");

					const GPlatesPropertyValues::XsString *subcategory_property_value;
					if (GPlatesFeatureVisitors::get_property_value(
							feature_handle, subcategory_property_name, &subcategory_property_value))
					{
						static const GPlatesPropertyValues::TextContent thrust_string("Thrust");

						if (subcategory_property_value->value().is_equal_to(thrust_string))
						{
							return "TH";
						}
					}

					return "RF";
				}
			}
		}

		static const GPlatesModel::PropertyName strike_slip_property_name = 
			GPlatesModel::PropertyName::create_gpml("strikeSlip");

		const GPlatesPropertyValues::Enumeration *strike_slip_property_value;
		if (GPlatesFeatureVisitors::get_property_value(
				feature_handle, strike_slip_property_name, &strike_slip_property_value))
		{
			static GPlatesPropertyValues::EnumerationType strike_slip_enumeration_type(
				"gpml:StrikeSlipEnumeration");
			static GPlatesPropertyValues::EnumerationContent strike_slip_enumeration_value_unknown(
				"Unknown");

			if (strike_slip_enumeration_type.is_equal_to(strike_slip_property_value->type()))
			{
				if (strike_slip_enumeration_value_unknown.is_equal_to(strike_slip_property_value->value()))
				{
					return "SS";
				}
			}
		}

		return "FT";
	}

	UnicodeString
	get_data_type_code_for_fracture_zone(
			const GPlatesModel::FeatureHandle &)
	{
		return "FZ";
	}

	UnicodeString
	get_data_type_code_for_grid_mark(
			const GPlatesModel::FeatureHandle &)
	{
		return "GR";
	}

	UnicodeString
	get_data_type_code_for_gravimetry(
			const GPlatesModel::FeatureHandle &)
	{
		return "GV";
	}

	UnicodeString
	get_data_type_code_for_heat_flow(
			const GPlatesModel::FeatureHandle &)
	{
		return "HF";
	}

	UnicodeString
	get_data_type_code_for_hot_spot(
			const GPlatesModel::FeatureHandle &)
	{
		return "HS";
	}

	UnicodeString
	get_data_type_code_for_hot_spot_trail(
			const GPlatesModel::FeatureHandle &)
	{
		return "HT";
	}

	UnicodeString
	get_data_type_code_for_island_arc(
			const GPlatesModel::FeatureHandle &feature_handle)
	{
		return get_data_type_code_for_active_inactive_feature(feature_handle, "IA", "IR");
	}

	UnicodeString
	get_data_type_code_for_isochron(
			const GPlatesModel::FeatureHandle &)
	{
		// Could also be "IM" according to "PlatesLineFormatHeaderVisitor.h".
		return "IC";
	}

	UnicodeString
	get_data_type_code_for_isopach(
			const GPlatesModel::FeatureHandle &)
	{
		return "IP";
	}

	// -might- be Ice Shelf, might be Isochron. We don't know.
	UnicodeString
	get_data_type_code_for_unclassified_feature(
			const GPlatesModel::FeatureHandle &)
	{
		// Could also be "IS" according to "PlatesLineFormatHeaderVisitor.h".
		return "UN";
	}

	UnicodeString
	get_data_type_code_for_geological_lineation(
			const GPlatesModel::FeatureHandle &)
	{
		return "LI";
	}

	UnicodeString
	get_data_type_code_for_magnetics(
			const GPlatesModel::FeatureHandle &)
	{
		return "MA";
	}

	UnicodeString
	get_data_type_code_for_orogenic_belt(
			const GPlatesModel::FeatureHandle &)
	{
		// Could also be "OR" according to "PlatesLineFormatHeaderVisitor.h".
		return "OB";
	}

	UnicodeString
	get_data_type_code_for_ophiolite_belt(
			const GPlatesModel::FeatureHandle &)
	{
		return "OP";
	}

	UnicodeString
	get_data_type_code_for_inferred_paleo_boundary(
			const GPlatesModel::FeatureHandle &)
	{
		return "PB";
	}

	UnicodeString
	get_data_type_code_for_magnetic_pick(
			const GPlatesModel::FeatureHandle &)
	{
		// Could also be "PC" according to "PlatesLineFormatHeaderVisitor.h".
		return "PM";
	}

	UnicodeString
	get_data_type_code_for_ridge_segment(
			const GPlatesModel::FeatureHandle &feature_handle)
	{
		return get_data_type_code_for_active_inactive_feature(feature_handle, "RI", "XR");
	}

	UnicodeString
	get_data_type_code_for_seamount(
			const GPlatesModel::FeatureHandle &)
	{
		return "SM";
	}

	UnicodeString
	get_data_type_code_for_suture(
			const GPlatesModel::FeatureHandle &)
	{
		return "SU";
	}

	UnicodeString
	get_data_type_code_for_terrane_boundary(
			const GPlatesModel::FeatureHandle &)
	{
		return "TB";
	}

	UnicodeString
	get_data_type_code_for_transitional_crust(
			const GPlatesModel::FeatureHandle &)
	{
		return "TC";
	}

	UnicodeString
	get_data_type_code_for_transform(
			const GPlatesModel::FeatureHandle &)
	{
		return "TF";
	}

	UnicodeString
	get_data_type_code_for_topography(
			const GPlatesModel::FeatureHandle &)
	{
		return "TO";
	}

	UnicodeString
	get_data_type_code_for_subduction_zone_active(
			const GPlatesModel::FeatureHandle &feature_handle)
	{
		return get_data_type_code_for_active_inactive_feature(feature_handle, "TR", "XT");
	}

	UnicodeString
	get_data_type_code_for_volcano(
			const GPlatesModel::FeatureHandle &)
	{
		return "VO";
	}

	UnicodeString
	get_data_type_code_for_large_igneous_province(
			const GPlatesModel::FeatureHandle &)
	{
		return "VP";
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


GPlatesFileIO::PlatesLineFormatHeaderVisitor::PlatesLineFormatHeaderVisitor()
{
	build_plates_data_type_code_map();
}

bool
GPlatesFileIO::PlatesLineFormatHeaderVisitor::get_old_plates_header(
	const GPlatesModel::FeatureHandle& feature_handle, OldPlatesHeader& old_plates_header)
{
	d_accum = PlatesHeaderAccumulator();

	// Visit feature to collect property values.
	feature_handle.accept_visitor(*this);

	// Build an old plates header from the information we've gathered.
	if (d_accum.old_plates_header) {
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
		old_plates_header.data_type_code = get_plates_data_type_code(feature_handle);
	}

	// Regardless of whether there's a gpml:oldPlatesHeader property we need to
	// add the feature id somewhere. The end of the geographic description seems
	// like a good place.
	append_feature_id_to_geog_description(feature_handle.feature_id(),
			old_plates_header.geographic_description);

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
		// We've encountered a plate ID inside an unrecognised property-name_property_name.
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

void
GPlatesFileIO::PlatesLineFormatHeaderVisitor::visit_xs_string(
	const GPlatesPropertyValues::XsString &xs_string)
{
	static const GPlatesModel::PropertyName name_property_name =
		GPlatesModel::PropertyName::create_gml("name");
	if (*d_accum.current_propname != name_property_name) {
		return;
	}

	// Only store 'gml:name' property in geographic description if it exists
	// and it not empty.
	if ( ! d_accum.geographic_description) {
		const UnicodeString &name = xs_string.value().get();
		if (!name.isEmpty())
		{
			d_accum.geographic_description = name;
		}
	} else {
		// The name_property_name was already set, which means that there was already a
		// "gml:name_property_name" property.
		// FIXME: Should we warn about this?
	}
}

UnicodeString
GPlatesFileIO::PlatesLineFormatHeaderVisitor::get_plates_data_type_code(
		const GPlatesModel::FeatureHandle &feature_handle) const
{
	const GPlatesModel::FeatureType &feature_type = feature_handle.feature_type();

	// Use the feature type to lookup up the function used to determine the data code type.
	plates_data_type_code_map_type::const_iterator data_type_code_iter =
			d_plates_data_type_code_map.find(feature_type);
	if (data_type_code_iter == d_plates_data_type_code_map.end())
	{
		// Cannot map feature to a Plates data type so indicate this.
		return INVALID_DATA_TYPE_CODE;
	}

	// The function required to determine the PLATES data type code.
	get_data_type_code_function_type get_data_type_code_function = data_type_code_iter->second;

	// Call function to determine data type code.
	return get_data_type_code_function(feature_handle);
}

void
GPlatesFileIO::PlatesLineFormatHeaderVisitor::build_plates_data_type_code_map()
{
	//
	// This is effectively the inverse of the mapping found in "PlatesLineFormatReader.cc".
	//

	d_plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("AseismicRidge")] =
			get_data_type_code_for_aseismic_ridge;
	d_plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("Bathymetry")] = 
			get_data_type_code_for_bathymetry;
	d_plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("Basin")] = 
			get_data_type_code_for_basin;
	d_plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("PassiveContinentalBoundary")] = 
			get_data_type_code_for_continental_boundary;
	d_plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("ContinentalFragment")] = 
			get_data_type_code_for_continental_fragment;
	d_plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("Craton")] = 
			get_data_type_code_for_craton;
	d_plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("Coastline")] = 
			get_data_type_code_for_coastline;
	d_plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("ExtendedContinentalCrust")] = 
			get_data_type_code_for_extended_continental_crust;
	d_plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("Fault")] = 
			get_data_type_code_for_fault;
	d_plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("FractureZone")] = 
			get_data_type_code_for_fracture_zone;
	d_plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("OldPlatesGridMark")] = 
			get_data_type_code_for_grid_mark;
	d_plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("Gravimetry")] = 
			get_data_type_code_for_gravimetry;
	d_plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("HeatFlow")] = 
			get_data_type_code_for_heat_flow;
	d_plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("HotSpot")] = 
			get_data_type_code_for_hot_spot;
	d_plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("HotSpotTrail")] = 
			get_data_type_code_for_hot_spot_trail;
	d_plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("IslandArc")] = 
			get_data_type_code_for_island_arc;
	d_plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("Isochron")] = 
			get_data_type_code_for_isochron;
	d_plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("SedimentThickness")] = 
			get_data_type_code_for_isopach;
	d_plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("UnclassifiedFeature")] = 
			get_data_type_code_for_unclassified_feature;
	d_plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("GeologicalLineation")] = 
			get_data_type_code_for_geological_lineation;
	d_plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("Magnetics")] = 
			get_data_type_code_for_magnetics;
	d_plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("OrogenicBelt")] = 
			get_data_type_code_for_orogenic_belt;
	d_plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("BasicRockUnit")] = 
			get_data_type_code_for_ophiolite_belt;
	d_plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("InferredPaleoBoundary")] = 
			get_data_type_code_for_inferred_paleo_boundary;
	d_plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("MagneticAnomalyIdentification")] = 
			get_data_type_code_for_magnetic_pick;
	d_plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("MidOceanRidge")] = 
			get_data_type_code_for_ridge_segment;
	d_plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("Seamount")] = 
			get_data_type_code_for_seamount;
	d_plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("Suture")] = 
			get_data_type_code_for_suture;
	d_plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("TerraneBoundary")] = 
			get_data_type_code_for_terrane_boundary;
	d_plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("TransitionalCrust")] = 
			get_data_type_code_for_transitional_crust;
	d_plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("Transform")] = 
			get_data_type_code_for_transform;
	d_plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("Topography")] = 
			get_data_type_code_for_topography;
	d_plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("SubductionZone")] = 
			get_data_type_code_for_subduction_zone_active;
	d_plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("Volcano")] = 
			get_data_type_code_for_volcano;
	d_plates_data_type_code_map[GPlatesModel::FeatureType::create_gpml("LargeIgneousProvince")] = 
			get_data_type_code_for_large_igneous_province;
}
