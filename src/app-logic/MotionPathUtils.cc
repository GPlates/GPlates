/* $Id: FlowlineUtils.cc 8209 2010-04-27 14:24:11Z rwatson $ */

/**
* \file 
* File specific comments.
*
* Most recent change:
*   $Date: 2010-04-27 16:24:11 +0200 (ti, 27 apr 2010) $
* 
* Copyright (C) 2009, 2010 Geological Survey of Norway.
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
#include "boost/optional.hpp"

#include <QDebug>

#include "MotionPathUtils.h"

#include "app-logic/AppLogicUtils.h"
#include "app-logic/FlowlineUtils.h"
#include "app-logic/ReconstructionTree.h"
#include "app-logic/ReconstructUtils.h"

#include "maths/FiniteRotation.h"
#include "maths/MathsUtils.h"
#include "maths/types.h"

#include "model/FeatureVisitor.h"
#include "model/Model.h"
#include "model/ModelUtils.h"

#include "property-values/GpmlArray.h"
#include "property-values/GpmlIrregularSampling.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GmlMultiPoint.h"
#include "property-values/GmlPoint.h"
#include "property-values/XsBoolean.h"




void
GPlatesAppLogic::MotionPathUtils::MotionPathPropertyFinder::visit_gpml_array(
	const GPlatesPropertyValues::GpmlArray &gpml_array)
{
	FlowlineUtils::get_times_from_time_period_array(d_times,gpml_array);
}


void
GPlatesAppLogic::MotionPathUtils::MotionPathPropertyFinder::visit_gpml_plate_id(
		const GPlatesPropertyValues::GpmlPlateId &gpml_plate_id)
{
	static GPlatesModel::PropertyName reconstruction_plate_id_property_name =
		GPlatesModel::PropertyName::create_gpml("reconstructionPlateId");
	static GPlatesModel::PropertyName relative_plate_id_property_name =
		GPlatesModel::PropertyName::create_gpml("relativePlate");


	if (current_top_level_propname() == reconstruction_plate_id_property_name)
	{
		d_reconstruction_plate_id.reset(gpml_plate_id.value());
	}
	else if (current_top_level_propname() == relative_plate_id_property_name)
	{
		d_relative_plate_id.reset(gpml_plate_id.value());
	}


}


void
GPlatesAppLogic::MotionPathUtils::MotionPathPropertyFinder::finalise_post_feature_properties(
	const GPlatesModel::FeatureHandle &feature_handle)
{

}

bool
GPlatesAppLogic::MotionPathUtils::MotionPathPropertyFinder::initialise_pre_feature_properties(
	const GPlatesModel::FeatureHandle &feature_handle)
{
	d_feature_info.append(
		GPlatesUtils::make_qstring_from_icu_string(feature_handle.feature_type().get_name()));

	d_feature_info.append(" <identity>");
	d_feature_info.append(
		GPlatesUtils::make_qstring_from_icu_string(feature_handle.feature_id().get()));
	d_feature_info.append("</identity>");

	d_feature_info.append(" <revision>");
	d_feature_info.append(
		GPlatesUtils::make_qstring_from_icu_string(feature_handle.revision_id().get()));
	d_feature_info.append("</revision>");

	d_times.clear();

	d_feature_is_defined_at_recon_time = true;
	d_time_of_appearance = boost::none;
	d_time_of_dissappearance = boost::none;
	d_reconstruction_plate_id = boost::none;
	d_relative_plate_id = boost::none;
	d_has_geometry = false;

	return true;
}

void
GPlatesAppLogic::MotionPathUtils::calculate_motion_track(
	const GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type &present_day_seed_point, 
	const MotionPathPropertyFinder &motion_track_parameters,
	std::vector<GPlatesMaths::PointOnSphere> &motion_track,
	const std::vector<GPlatesMaths::FiniteRotation> &rotations)
{
	using namespace GPlatesMaths;

	std::vector<FiniteRotation>::const_reverse_iterator
		iter = rotations.rbegin(),
		end = rotations.rend();

	for (; iter != end ; ++iter)
	{
		PointOnSphere::non_null_ptr_to_const_type current_point = *iter * present_day_seed_point;
		motion_track.push_back(*current_point);
	}
}

void
GPlatesAppLogic::MotionPathUtils::fill_times_vector(
	std::vector<double> &times,
	const double &reconstruction_time,
	const std::vector<double> &time_samples)
{
	std::vector<double>::const_iterator
		samples_iter = time_samples.begin(),
		samples_end = time_samples.end();

	// Get to the first time which is older than our current reconstruction time. 
	while ((samples_iter != samples_end) && (*samples_iter <= GPlatesMaths::real_t(reconstruction_time)))
	{
		++samples_iter;
	}	

	// Add the reconstruction time if it lies between the end points of the times vector.
	if (!time_samples.empty() &&
		time_samples.front() <= GPlatesMaths::real_t(reconstruction_time) &&
		time_samples.back() > GPlatesMaths::real_t(reconstruction_time))
	{
		times.push_back(reconstruction_time);
	}

	// Add the remaining times. 
	while (samples_iter != samples_end)
	{
		times.push_back(*samples_iter);
		++samples_iter;
	}	
}

void
GPlatesAppLogic::MotionPathUtils::MotionPathPropertyFinder::visit_gml_time_period(
		const GPlatesPropertyValues::GmlTimePeriod &gml_time_period)
{
	static const GPlatesModel::PropertyName valid_time_property_name =
		GPlatesModel::PropertyName::create_gml("validTime");

	// Note that we're going to assume that we're in a property...
	if (current_top_level_propname() == valid_time_property_name)
	{
	    // This time period is the "valid time" time period.
	    if (d_reconstruction_time && !gml_time_period.contains(*d_reconstruction_time))
	    {
		    // Oh no!  This feature instance is not defined at the recon time!
		    d_feature_is_defined_at_recon_time = false;
	    }
	    // Also, cache the time of appearance/dissappearance.
	    d_time_of_appearance = gml_time_period.begin()->time_position();
	    d_time_of_dissappearance = gml_time_period.end()->time_position();
	}
}

bool
GPlatesAppLogic::MotionPathUtils::MotionPathPropertyFinder::can_process_motion_path()
{
    // Process the motion path if:

    // we have recon and relative plate ids
    // we have a reconstruction time and
    // we have a non-empty times vector and
    // the reconstruction time lies between the feature begin/time ends and
    // we have geometries.
	if (d_times.empty() ||
	   !d_reconstruction_plate_id ||
	   !d_relative_plate_id ||
	   !d_feature_is_defined_at_recon_time ||
	   !d_has_geometry )
	{
	    return false;
	}

	// NOTE: We no longer require the reconstruction time to be between the end points of the times vector.
	// This enables us to display/export at, for example, present day when the time vector does
	// not include present day (such as a motion path representing part of a hotspot trail).

	return true;
}

bool
GPlatesAppLogic::MotionPathUtils::MotionPathPropertyFinder::can_process_seed_point()
{
	return (d_feature_is_defined_at_recon_time && d_has_geometry);
}


