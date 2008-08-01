/* $Id$ */

/**
 * \file 
 * Implementation of the Plates4 data writer.
 *
 * Most recent change:
 *   $Date$
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

#include "PlatesLineFormatWriter.h"
#include <ostream>
#include <fstream>
#include <vector>

#include "model/FeatureHandle.h"
#include "model/InlinePropertyContainer.h"
#include "model/FeatureRevision.h"

#include "property-values/GmlLineString.h"
#include "property-values/GmlOrientableCurve.h"
#include "property-values/GmlPoint.h"
#include "property-values/GmlTimeInstant.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlFiniteRotation.h"
#include "property-values/GpmlFiniteRotationSlerp.h"
#include "property-values/GpmlIrregularSampling.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GpmlTimeSample.h"
#include "property-values/GpmlOldPlatesHeader.h"
#include "property-values/XsString.h"

#include "maths/Real.h"
#include "maths/PolylineOnSphere.h"
#include "maths/LatLonPointConversions.h"
#include "utils/StringFormattingUtils.h"
#include "global/GPlatesAssert.h"
#include <unicode/ustream.h>


namespace
{
	/* A point on a polyline in the PLATES4 format includes a "draw command"
	 * after the coordninates of the point.  This is a number (2 or 3) which 
	 * tells us whether to draw a line (from the previous point) to the point,
	 * or to start the next line at the point.
	 */
	enum pen_pos_t { PEN_DRAW_TO_POINT = 2, PEN_SKIP_TO_POINT = 3 };


	void
	print_plates_coordinate_line(
			std::ostream *os, 
			const GPlatesMaths::Real &lat, 
			const GPlatesMaths::Real &lon,
			pen_pos_t pen)
	{
		/*
		 * A coordinate in the PLATES4 format is written as decimal number with
		 * 4 digits precision after the decimal point, and it must take up 9
		 * characters altogether (i.e. including the decimal point and maybe
		 * a sign).
		 */
		static const unsigned PLATES_COORDINATE_PRECISION = 4;
		static const unsigned PLATES_COORDINATE_FIELDWIDTH = 9;
		static const unsigned PLATES_PEN_FIELDWIDTH = 1;

		/*
		 * We convert the coordinates to strings first, so that in case an exception
		 * is thrown, the ostream is not modified.
		 */
		std::string lat_str, lon_str, pen_str;
		try {
			lat_str = GPlatesUtils::formatted_double_to_string(lat.dval(),
				PLATES_COORDINATE_FIELDWIDTH, PLATES_COORDINATE_PRECISION);
			lon_str = GPlatesUtils::formatted_double_to_string(lon.dval(),
				PLATES_COORDINATE_FIELDWIDTH, PLATES_COORDINATE_PRECISION);
			pen_str = GPlatesUtils::formatted_int_to_string(static_cast<int>(pen),
				PLATES_PEN_FIELDWIDTH);
		} catch (const GPlatesUtils::InvalidFormattingParametersException &) {
			// The argument name in the above expression was removed to
			// prevent "unreferenced local variable" compiler warnings under MSVC.
			throw;
		}

		(*os) << lat_str << " " << lon_str << " " << pen_str << std::endl;
	}


	void
	print_plates_feature_termination_line(
			std::ostream *os)
	{
		print_plates_coordinate_line(os, 99.0, 99.0, PEN_SKIP_TO_POINT);
	}


	void
	print_plates_coordinate_line(
			std::ostream *os, 
			const GPlatesMaths::PointOnSphere &pos,
			pen_pos_t pen)
	{
		GPlatesMaths::LatLonPoint llp =
				GPlatesMaths::make_lat_lon_point(pos);
		print_plates_coordinate_line(os, llp.latitude(), llp.longitude(), pen);
	}


	void
	print_line_string(
			std::ostream *os,
			GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline)
	{
		GPlatesMaths::PolylineOnSphere::vertex_const_iterator iter = polyline->vertex_begin();
		GPlatesMaths::PolylineOnSphere::vertex_const_iterator end = polyline->vertex_end();

		// N.B. The class invariant of PolylineOnSphere guarentees that our iterators  iter
		// and  end  span at least two vertices.  See the documentation for PolylineOnSphere.

		Assert(iter != end, 
				GPlatesMaths::InvalidPolylineException(
					"Came across a polyline with no points in it!"));

		print_plates_coordinate_line(os, *iter, PEN_SKIP_TO_POINT);

		++iter;
		Assert(iter != end, 
				GPlatesMaths::InvalidPolylineException(
					"Came across a polyline with less than two points in it!"));

		for ( ; iter != end; ++iter) {
			print_plates_coordinate_line(os, *iter, PEN_DRAW_TO_POINT);
		}

		/*
		 * The PLATES4 format dictates that we include a special
		 * "line string has terminated" coordinate at the end of the line
		 * string.  The coordinate is always exactly "  99.0000   99.0000 3".
		 */
		print_plates_feature_termination_line(os);
	}


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


GPlatesFileIO::PlatesLineFormatWriter::PlatesLineFormatWriter(
		const FileInfo &file_info)
{
	d_output = new std::ofstream(file_info.get_qfileinfo().filePath().toStdString().c_str());
}


bool 
GPlatesFileIO::PlatesLineFormatWriter::PlatesLineFormatAccumulator::have_sufficient_info_for_output() const
{
	// To output this feature we need a geometry (a polyline or a point) and plate id.
	return (polyline || point) && (old_plates_header || plate_id);
}


void
GPlatesFileIO::PlatesLineFormatWriter::print_header_lines(
		std::ostream *os, 
		const PlatesLineFormatAccumulator::OldPlatesHeader &old_plates_header)
{
	using namespace GPlatesUtils;

	/*
	 * The magic numbers that appear below are taken from p38 of the PLATES4
	 * User's Manual.
	 */

	// First line of the PLATES4 header.
	(*os) << formatted_int_to_string(old_plates_header.region_number, 2)
		<< formatted_int_to_string(old_plates_header.reference_number, 2)
		<< " "
		<< formatted_int_to_string(old_plates_header.string_number, 4)
		<< " "
		<< old_plates_header.geographic_description 
		<< std::endl;

	// Second line of the PLATES4 header.
	(*os) << " "
		<< formatted_int_to_string(old_plates_header.plate_id_number, 3)
		<< " "
		<< formatted_double_to_string(old_plates_header.age_of_appearance, 6, 1)
		<< " "
		<< formatted_double_to_string(old_plates_header.age_of_disappearance, 6, 1)
		<< " "
		<< old_plates_header.data_type_code
		<< formatted_int_to_string(old_plates_header.data_type_code_number, 4)
		<< " "
		<< formatted_int_to_string(old_plates_header.conjugate_plate_id_number, 3)
		<< " "
		<< formatted_int_to_string(old_plates_header.colour_code, 3)
		<< " "
		<< formatted_int_to_string(old_plates_header.number_of_points, 5)
		<< std::endl;
}


void
GPlatesFileIO::PlatesLineFormatWriter::visit_feature_handle(
		const GPlatesModel::FeatureHandle &feature_handle)
{
	d_accum = PlatesLineFormatAccumulator();

	// These two strings will be used to write the "Geographic description"
	// field in the case that no old PLATES4 header is found.
	d_accum.feature_type = feature_handle.feature_type().get_name();
	d_accum.feature_id   = feature_handle.feature_id().get();

	// Visit each of the properties in turn.
	visit_feature_properties(feature_handle);

	// Check that we have enough data to actually output something
	if ( ! d_accum.have_sufficient_info_for_output()) {
		// XXX: Should we emit some kind of "data loss" warning?
		return;
	}
	
	// Build an old plates header from the information we've gathered.
	PlatesLineFormatAccumulator::OldPlatesHeader old_plates_header;
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

	// Find out if our geometry is a polyline or a point.
	if (d_accum.polyline) {
		old_plates_header.number_of_points = 
			(*d_accum.polyline)->number_of_vertices();
	} else {
		// have_sufficient_info_for_output() guarentees that we must have a point.
		old_plates_header.number_of_points = 1;
	}

	print_header_lines(d_output, old_plates_header);
	if (d_accum.polyline) {
		print_line_string(d_output, *d_accum.polyline);
	} else {
		print_plates_coordinate_line(d_output, **d_accum.point, PEN_SKIP_TO_POINT);
		print_plates_feature_termination_line(d_output);
	}
}


void
GPlatesFileIO::PlatesLineFormatWriter::visit_inline_property_container(
		const GPlatesModel::InlinePropertyContainer &inline_property_container)
{
	d_accum.current_propname = inline_property_container.property_name();

	visit_property_values(inline_property_container);
}


void
GPlatesFileIO::PlatesLineFormatWriter::visit_gml_line_string(
		const GPlatesPropertyValues::GmlLineString &gml_line_string)
{
	d_accum.polyline = gml_line_string.polyline();
}


void
GPlatesFileIO::PlatesLineFormatWriter::visit_gml_orientable_curve(
		const GPlatesPropertyValues::GmlOrientableCurve &gml_orientable_curve)
{
	gml_orientable_curve.base_curve()->accept_visitor(*this);
}


void
GPlatesFileIO::PlatesLineFormatWriter::visit_gml_point(
		const GPlatesPropertyValues::GmlPoint &gml_point)
{
	d_accum.point = gml_point.point();
}


void
GPlatesFileIO::PlatesLineFormatWriter::visit_gml_time_instant(
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
GPlatesFileIO::PlatesLineFormatWriter::visit_gml_time_period(
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
GPlatesFileIO::PlatesLineFormatWriter::visit_gpml_constant_value(
		const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
{
	gpml_constant_value.value()->accept_visitor(*this);
}


void
GPlatesFileIO::PlatesLineFormatWriter::visit_gpml_finite_rotation(
		const GPlatesPropertyValues::GpmlFiniteRotation &gpml_finite_rotation)
{

}


void
GPlatesFileIO::PlatesLineFormatWriter::visit_gpml_finite_rotation_slerp(
		const GPlatesPropertyValues::GpmlFiniteRotationSlerp &gpml_finite_rotation_slerp)
{

}


void
GPlatesFileIO::PlatesLineFormatWriter::visit_gpml_irregular_sampling(
		const GPlatesPropertyValues::GpmlIrregularSampling &gpml_irregular_sampling)
{

}


void
GPlatesFileIO::PlatesLineFormatWriter::visit_gpml_plate_id(
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
GPlatesFileIO::PlatesLineFormatWriter::visit_gpml_time_sample(
		const GPlatesPropertyValues::GpmlTimeSample &gpml_time_sample)
{

}


void
GPlatesFileIO::PlatesLineFormatWriter::visit_gpml_old_plates_header(
		const GPlatesPropertyValues::GpmlOldPlatesHeader &gpml_old_plates_header)
{
	d_accum.old_plates_header = PlatesLineFormatAccumulator::OldPlatesHeader(
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
GPlatesFileIO::PlatesLineFormatWriter::visit_xs_string(
		const GPlatesPropertyValues::XsString &xs_string)
{
	
}
