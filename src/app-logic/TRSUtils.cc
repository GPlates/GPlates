/* $Id: EditTimeSequenceWidget.cc 8310 2010-05-06 15:02:23Z rwatson $ */

/**
 * \file 
 * $Revision: 8310 $
 * $Date: 2010-05-06 17:02:23 +0200 (to, 06 mai 2010) $ 
 * 
 * Copyright (C) 2011 Geological Survey of Norway
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

#include "feature-visitors/TotalReconstructionSequencePlateIdFinder.h"
#include "feature-visitors/TotalReconstructionSequenceTimePeriodFinder.h"
#include "model/types.h"
#include "property-values/GeoTimeInstant.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlIrregularSampling.h"
#include "property-values/GpmlPlateId.h"

#include "TRSUtils.h"


namespace
{
	// Copied from TotalReconstructionSequencePlateIdFinder
	template<typename C, typename E>
	bool
		contains_elem(
		const C &container,
		const E &elem)
	{
		return (std::find(container.begin(), container.end(), elem) != container.end());
	}
}

GPlatesAppLogic::TRSUtils::TRSFinder::TRSFinder()
{
	d_property_names_to_allow.push_back(
		GPlatesModel::PropertyName::create_gpml("fixedReferenceFrame"));
	d_property_names_to_allow.push_back(
		GPlatesModel::PropertyName::create_gpml("movingReferenceFrame"));
	d_property_names_to_allow.push_back(
		GPlatesModel::PropertyName::create_gpml("mprsAttributes"));
	d_property_names_to_allow.push_back(
		GPlatesModel::PropertyName::create_gpml("totalReconstructionPole"));
}

bool
GPlatesAppLogic::TRSUtils::TRSFinder::initialise_pre_property_values(
        top_level_property_inline_type &top_level_property_inline)
{
	const GPlatesModel::PropertyName &curr_prop_name = top_level_property_inline.get_property_name();
	if ( ! d_property_names_to_allow.empty()) {
		// We're not allowing all property names.
		if ( ! contains_elem(d_property_names_to_allow, curr_prop_name)) {
			// The current property name is not allowed.
			return false;
		}
	}
	return true;
}

void
GPlatesAppLogic::TRSUtils::TRSFinder::visit_gpml_irregular_sampling(
	GPlatesPropertyValues::GpmlIrregularSampling &gpml_irregular_sampling)
{
	static const GPlatesModel::PropertyName prop_name =
		GPlatesModel::PropertyName::create_gpml("totalReconstructionPole");

	if (current_top_level_propname() == prop_name)
	{
		d_irregular_sampling_iterator.reset(*current_top_level_propiter());
		d_irregular_sampling.reset(gpml_irregular_sampling.clone());
	}
}

void
GPlatesAppLogic::TRSUtils::TRSFinder::visit_gpml_constant_value(
	GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
{
	gpml_constant_value.value()->accept_visitor(*this);
}

void
GPlatesAppLogic::TRSUtils::TRSFinder::visit_gpml_plate_id(
	GPlatesPropertyValues::GpmlPlateId &gpml_plate_id)
{
	static const GPlatesModel::PropertyName fixed_ref_frame_property_name =
		GPlatesModel::PropertyName::create_gpml("fixedReferenceFrame");
	static const GPlatesModel::PropertyName moving_ref_frame_property_name =
		GPlatesModel::PropertyName::create_gpml("movingReferenceFrame");

	// Note that we're going to assume that we've read a property name...
	if (*current_top_level_propname() == fixed_ref_frame_property_name) {
		// We're dealing with the fixed ref-frame of the Total Reconstruction Sequence.
		d_fixed_ref_frame_plate_id = gpml_plate_id.get_value();
		d_fixed_ref_frame_iterator = current_top_level_propiter();
	} else if (*current_top_level_propname() == moving_ref_frame_property_name) {
		// We're dealing with the moving ref-frame of the Total Reconstruction Sequence.
		d_moving_ref_frame_plate_id = gpml_plate_id.get_value();
		d_moving_ref_frame_iterator = current_top_level_propiter();
	}
}

void
GPlatesAppLogic::TRSUtils::TRSFinder::reset()
{
	d_fixed_ref_frame_plate_id = boost::none;
	d_moving_ref_frame_plate_id = boost::none;
	d_fixed_ref_frame_iterator = boost::none;
	d_moving_ref_frame_iterator = boost::none;
	d_irregular_sampling = boost::none;
	d_irregular_sampling_iterator = boost::none;
}

bool
GPlatesAppLogic::TRSUtils::TRSFinder::can_process_trs()
{
	return (d_irregular_sampling &&
			d_irregular_sampling_iterator &&
			d_moving_ref_frame_plate_id &&
			d_moving_ref_frame_iterator &&
			d_fixed_ref_frame_plate_id &&
			d_fixed_ref_frame_iterator);
}

QString
GPlatesAppLogic::TRSUtils::build_trs_summary_string_from_trs_feature(
	const GPlatesModel::FeatureHandle::weak_ref &trs_feature)
{
	// This code taken largely from the TotalReconstructionSequenceDialog.

	using namespace GPlatesFeatureVisitors;

	TotalReconstructionSequencePlateIdFinder plate_id_finder;
	TotalReconstructionSequenceTimePeriodFinder time_period_finder(false);

	using namespace GPlatesModel;

	// First, extract the plate ID and timePeriod values from the TRS.

	plate_id_finder.reset();
	plate_id_finder.visit_feature(trs_feature);
	if (( ! plate_id_finder.fixed_ref_frame_plate_id()) ||
		( ! plate_id_finder.moving_ref_frame_plate_id())) {
			// We did not find either or both of the fixed plate ID or moving
			// plate ID.  Hence, we'll assume that this is not a reconstruction
			// feature.
			return QObject::tr("Did not find plate ids in the TRS feature.");

	}
	integer_plate_id_type fixed_plate_id = *plate_id_finder.fixed_ref_frame_plate_id();
	integer_plate_id_type moving_plate_id = *plate_id_finder.moving_ref_frame_plate_id();

	time_period_finder.reset();
	time_period_finder.visit_feature(trs_feature);
	if (( ! time_period_finder.begin_time()) ||
		( ! time_period_finder.end_time())) {
			// We did not find the begin time and end time.  Hence, we'll
			// assume that this is not a valid reconstruction feature, since it
			// does not contain a valid IrregularSampling (since we couldn't
			// find at least one TimeSample).
			return QObject::tr("Did not find begin and end times in the TRS feature.");
	}
	GPlatesPropertyValues::GeoTimeInstant begin_time = *time_period_finder.begin_time();
	GPlatesPropertyValues::GeoTimeInstant end_time = *time_period_finder.end_time();

	QLocale locale_;

	// This is a string to display if the begin-time or end-time is in either
	// the distant past or distant future (which it should not be).
	// Assume that this string won't change after the first time this function
	// is called, so we can keep the QString in a static local var.
	static QString invalid_time =
		QObject::tr("invalid time");
	QString begin_time_as_str = invalid_time;
	if (begin_time.is_real()) {
		begin_time_as_str = locale_.toString(begin_time.value());
	}
	QString end_time_as_str = invalid_time;
	if (end_time.is_real()) {
		end_time_as_str = locale_.toString(end_time.value());
	}

	QString feature_descr =
		QObject::tr(
		"%1 rel %2\t[%3 : %4]")
		.arg(moving_plate_id, 3, 10, QLatin1Char('0'))
		.arg(fixed_plate_id, 3, 10, QLatin1Char('0'))
		.arg(end_time_as_str)
		.arg(begin_time_as_str);

	return feature_descr;

}

bool
GPlatesAppLogic::TRSUtils::one_of_trs_plate_ids_is_999
	(const GPlatesModel::FeatureHandle::weak_ref &trs_feature)
{
	using namespace GPlatesFeatureVisitors;

	TotalReconstructionSequencePlateIdFinder plate_id_finder;

	using namespace GPlatesModel;

	// First, extract the plate ID and timePeriod values from the TRS.

	plate_id_finder.reset();
	plate_id_finder.visit_feature(trs_feature);
	if (( ! plate_id_finder.fixed_ref_frame_plate_id()) ||
		( ! plate_id_finder.moving_ref_frame_plate_id())) {
			// We did not find either or both of the fixed plate ID or moving
			// plate ID.  Hence, we'll assume that this is not a reconstruction
			// feature.
			return false;

	}
	integer_plate_id_type fixed_plate_id = *plate_id_finder.fixed_ref_frame_plate_id();
	integer_plate_id_type moving_plate_id = *plate_id_finder.moving_ref_frame_plate_id();

	return ((moving_plate_id == 999) ||
			(fixed_plate_id == 999));
}