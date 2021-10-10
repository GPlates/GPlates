/* $Id$ */

/**
 * \file 
 * Implementation of the Plates4 rotation writer.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2007 The University of Sydney, Australia
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

#include "PlatesRotationFormatWriter.h"
#include <ostream>
#include <vector>

#include "model/FeatureHandle.h"
#include "model/InlinePropertyContainer.h"
#include "model/FeatureRevision.h"

#include "property-values/GmlTimeInstant.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlFiniteRotation.h"
#include "property-values/GpmlIrregularSampling.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GpmlTimeSample.h"
#include "property-values/XsString.h"

#include "maths/Real.h"
#include "maths/PolylineOnSphere.h"
#include "maths/LatLonPointConversions.h"
#include "maths/UnitQuaternion3D.h"
#include "utils/StringFormattingUtils.h"
#include "utils/MathUtils.h"
#include <unicode/ustream.h>
#include <boost/none.hpp>


namespace
{
	void
	print_rotation_line_details(
			std::ostream *os,
			int moving_plate_id,
			const double &time,
			const double &latitude,
			const double &longitude,
			const double &angle,
			int fixed_plate_id,
			const UnicodeString &comment)
	{
		using namespace GPlatesUtils;

		(*os) << formatted_int_to_string(moving_plate_id, 3, '0')
			<< " "
			<< formatted_double_to_string(time, 5, 2, true)
			<< " "
			<< formatted_double_to_string(latitude, 6, 2, true)
			<< " "
			<< formatted_double_to_string(longitude, 7, 2, true)
			<< " "
			<< formatted_double_to_string(angle, 7, 2, true)
			<< "  "
			<< formatted_int_to_string(fixed_plate_id, 3, '0')
			<< " !"
			<< comment
			<< std::endl;
	}
}


void
GPlatesFileIO::PlatesRotationFormatWriter::PlatesRotationFormatAccumulator::print_rotation_lines(
		std::ostream *os)
{
	std::list< ReconstructionPoleData >::const_iterator
		iter = reconstruction_poles.begin(),
		end = reconstruction_poles.end();
	for ( ; iter != end; ++iter) 
	{
		if (! iter->have_sufficient_info_for_output()) {
			continue;
		}

		UnicodeString str_comment("");
		if (iter->comment) {
			str_comment = *(iter->comment);
		}
		
		int moving_plate_id_or_comment = *moving_plate_id;

		// If the 'is_disabled' flag is set, this rotation is considered "commented out". 
		// This is represented by giving the rotation a moving plate id of 999.
		if (iter->is_disabled && *(iter->is_disabled)) {
			moving_plate_id_or_comment = 999;
		}

		GPlatesMaths::UnitQuaternion3D quat = iter->finite_rotation->unit_quat();
		if (GPlatesMaths::represents_identity_rotation(quat)) 
		{
			print_rotation_line_details(os, moving_plate_id_or_comment, *(iter->time), 0.0, 0.0, 0.0, 
					*fixed_plate_id, str_comment);
		} 
		else 
		{
			GPlatesMaths::UnitQuaternion3D::RotationParams rot_params =
				quat.get_rotation_params();

			GPlatesMaths::LatLonPoint pole = 
				GPlatesMaths::make_lat_lon_point(GPlatesMaths::PointOnSphere(rot_params.axis));

			print_rotation_line_details(os, moving_plate_id_or_comment, *(iter->time),
					pole.latitude(), pole.longitude(), GPlatesUtils::convert_rad_to_deg(rot_params.angle.dval()),
					*fixed_plate_id, str_comment);
		}
	}
}


bool 
GPlatesFileIO::PlatesRotationFormatWriter::PlatesRotationFormatAccumulator::have_sufficient_info_for_output() const 
{
	// The only thing we can do without is the comment.
	return moving_plate_id && fixed_plate_id && ! reconstruction_poles.empty();
}


void
GPlatesFileIO::PlatesRotationFormatWriter::visit_feature_handle(
		const GPlatesModel::FeatureHandle &feature_handle)
{
	static const GPlatesModel::FeatureType
		gpmlTotalReconstructionSequence = 
			GPlatesModel::FeatureType::create_gpml("TotalReconstructionSequence"),
		gpmlAbsoluteReferenceFrame =
			GPlatesModel::FeatureType::create_gpml("AbsoluteReferenceFrame");

	if ((feature_handle.feature_type() != gpmlTotalReconstructionSequence)
			&& (feature_handle.feature_type() != gpmlAbsoluteReferenceFrame)) {
		// These are not the features you're looking for.
		return;
	}

	// Reset the accumulator.
	d_accum = PlatesRotationFormatAccumulator();

	// Visit each of the properties in turn.
	visit_feature_properties(feature_handle);

	// Print reconstruction poles when we can.
	if (d_accum.have_sufficient_info_for_output()) {
		d_accum.print_rotation_lines(d_output);
	}
}


void
GPlatesFileIO::PlatesRotationFormatWriter::visit_inline_property_container(
		const GPlatesModel::InlinePropertyContainer &inline_property_container)
{
	d_accum.last_property_seen = inline_property_container.property_name();

	visit_property_values(inline_property_container);
}


void
GPlatesFileIO::PlatesRotationFormatWriter::visit_gml_line_string(
		const GPlatesPropertyValues::GmlLineString &gml_line_string)
{ }


void
GPlatesFileIO::PlatesRotationFormatWriter::visit_gml_orientable_curve(
		const GPlatesPropertyValues::GmlOrientableCurve &gml_orientable_curve)
{ }


void
GPlatesFileIO::PlatesRotationFormatWriter::visit_gml_point(
		const GPlatesPropertyValues::GmlPoint &gml_point)
{ }


void
GPlatesFileIO::PlatesRotationFormatWriter::visit_gml_time_instant(
		const GPlatesPropertyValues::GmlTimeInstant &gml_time_instant) 
{ }


void
GPlatesFileIO::PlatesRotationFormatWriter::visit_gml_time_period(
		const GPlatesPropertyValues::GmlTimePeriod &gml_time_period)
{ }


void
GPlatesFileIO::PlatesRotationFormatWriter::visit_gpml_constant_value(
		const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
{
	gpml_constant_value.value()->accept_visitor(*this);
}


void
GPlatesFileIO::PlatesRotationFormatWriter::visit_gpml_finite_rotation(
		const GPlatesPropertyValues::GpmlFiniteRotation &gpml_finite_rotation)
{
	d_accum.current_pole().finite_rotation = gpml_finite_rotation.finite_rotation();
}


void
GPlatesFileIO::PlatesRotationFormatWriter::visit_gpml_finite_rotation_slerp(
		const GPlatesPropertyValues::GpmlFiniteRotationSlerp &gpml_finite_rotation_slerp)
{ }


void
GPlatesFileIO::PlatesRotationFormatWriter::visit_gpml_irregular_sampling(
		const GPlatesPropertyValues::GpmlIrregularSampling &gpml_irregular_sampling)
{
	// Iterate through the time samples (hence finite rotations) in the current
	// total reconstruction sequence.
	std::vector< GPlatesPropertyValues::GpmlTimeSample >::const_iterator 
		iter = gpml_irregular_sampling.time_samples().begin(),
		end = gpml_irregular_sampling.time_samples().end();
	for ( ; iter != end; ++iter) {
		iter->accept_visitor(*this);
	}
}


void
GPlatesFileIO::PlatesRotationFormatWriter::visit_gpml_plate_id(
		const GPlatesPropertyValues::GpmlPlateId &gpml_plate_id)
{
	static const GPlatesModel::PropertyName
		fixedReferenceFrame(
				GPlatesUtils::XmlNamespaces::GPML_NAMESPACE,
				GPlatesUtils::XmlNamespaces::GPML_STANDARD_ALIAS,
				QString("fixedReferenceFrame")),
		movingReferenceFrame(
				GPlatesUtils::XmlNamespaces::GPML_NAMESPACE,
				GPlatesUtils::XmlNamespaces::GPML_STANDARD_ALIAS,
				QString("movingReferenceFrame"));

	if (*d_accum.last_property_seen == fixedReferenceFrame) {
		d_accum.fixed_plate_id = gpml_plate_id.value();
	} else if (*d_accum.last_property_seen == movingReferenceFrame) {
		d_accum.moving_plate_id = gpml_plate_id.value();
	} else {
		// Do nothing: the plate id must not be associated to a finite rotation.
	}
}


void
GPlatesFileIO::PlatesRotationFormatWriter::visit_gpml_time_sample(
		const GPlatesPropertyValues::GpmlTimeSample &gpml_time_sample)
{
	// Start a new reconstruction pole
	d_accum.reconstruction_poles.push_back(PlatesRotationFormatAccumulator::ReconstructionPoleData());

	d_accum.current_pole().time = gpml_time_sample.valid_time()->time_position().value();
	d_accum.current_pole().is_disabled = gpml_time_sample.is_disabled();
	
	// Visit the finite rotation inside this time sample.
	gpml_time_sample.value()->accept_visitor(*this);

	// Visit the comment.
	if (gpml_time_sample.description()) {
		gpml_time_sample.description()->accept_visitor(*this);
	}
}


void
GPlatesFileIO::PlatesRotationFormatWriter::visit_gpml_old_plates_header(
		const GPlatesPropertyValues::GpmlOldPlatesHeader &gpml_old_plates_header)
{ }


void
GPlatesFileIO::PlatesRotationFormatWriter::visit_xs_string(
		const GPlatesPropertyValues::XsString &xs_string)
{
	d_accum.current_pole().comment = xs_string.value().get();
}
