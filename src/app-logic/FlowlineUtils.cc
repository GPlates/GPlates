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

#include "FlowlineUtils.h"

#include "app-logic/AppLogicUtils.h"
#include "app-logic/ReconstructionTree.h"
#include "app-logic/ReconstructUtils.h"
#include "maths/FiniteRotation.h"
#include "maths/MathsUtils.h"
#include "model/FeatureVisitor.h"
#include "model/Model.h"
#include "model/ModelUtils.h"
#include "property-values/GpmlIrregularSampling.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GmlMultiPoint.h"
#include "property-values/GmlPoint.h"
#include "property-values/XsBoolean.h"

namespace
{

	void
	display_rotation(
		const GPlatesMaths::FiniteRotation &rotation)
	{
		if (represents_identity_rotation(rotation.unit_quat()))
		{
			qDebug() << "Identity rotation.";
			return;
		}

		using namespace GPlatesMaths;
		const UnitQuaternion3D old_uq = rotation.unit_quat();



		const boost::optional<GPlatesMaths::UnitVector3D> &axis_hint = rotation.axis_hint();		
		UnitQuaternion3D::RotationParams params = old_uq.get_rotation_params(axis_hint);
		real_t angle = params.angle;

		PointOnSphere point(params.axis);
		LatLonPoint llp = make_lat_lon_point(point);

		qDebug() << "Pole: Lat" << llp.latitude() << ", lon: " << llp.longitude() << ", angle: " 
			<< GPlatesMaths::convert_rad_to_deg(params.angle.dval());

	}


}

void
GPlatesAppLogic::FlowlineUtils::get_times_from_irregular_sampling(
	std::vector<double> &times,
	const GPlatesPropertyValues::GpmlIrregularSampling &irregular_sampling)
{
	std::vector<GPlatesPropertyValues::GpmlTimeSample>::const_iterator 
		iter = irregular_sampling.time_samples().begin(),
		end = irregular_sampling.time_samples().end();

	for (; iter != end ; ++iter)
	{
		times.push_back(iter->valid_time()->time_position().value());
	}
}



// Halves the angle of the provided FiniteRotation.
void
GPlatesAppLogic::FlowlineUtils::get_half_angle_rotation(
	GPlatesMaths::FiniteRotation &rotation)
{
	if (represents_identity_rotation(rotation.unit_quat()))
	{
		return;
	}

	using namespace GPlatesMaths;
	const UnitQuaternion3D old_uq = rotation.unit_quat();
	const boost::optional<GPlatesMaths::UnitVector3D> &axis_hint = rotation.axis_hint();		
	UnitQuaternion3D::RotationParams params = old_uq.get_rotation_params(axis_hint);
	GPlatesMaths::real_t angle = params.angle;

	GPlatesMaths::real_t new_angle = angle/2.;

	UnitQuaternion3D new_uq = UnitQuaternion3D::create_rotation(params.axis,new_angle);
	rotation = FiniteRotation::create(new_uq,axis_hint);
}

void
GPlatesAppLogic::FlowlineUtils::fill_times_vector(
	std::vector<double> &times,
	const double &reconstruction_time,
	const std::vector<double> &time_samples)
{
	times.push_back(reconstruction_time);

	std::vector<double>::const_iterator
		samples_iter = time_samples.begin(),
		samples_end = time_samples.end();

	// Get to the first time which is older than our current reconstruction time. 
	while ((samples_iter != samples_end) && (*samples_iter <= reconstruction_time))
	{
		++samples_iter;
	}	

	// Add the remaining times. 
	while (samples_iter != samples_end)
	{
		times.push_back(*samples_iter);
		++samples_iter;
	}	
}

void
GPlatesAppLogic::FlowlineUtils::FlowlinePropertyFinder::visit_gpml_irregular_sampling(
		const gpml_irregular_sampling_type &gpml_irregular_sampling)
{
	get_times_from_irregular_sampling(d_times,gpml_irregular_sampling);
}


void
GPlatesAppLogic::FlowlineUtils::FlowlinePropertyFinder::visit_gpml_plate_id(
		const GPlatesPropertyValues::GpmlPlateId &gpml_plate_id)
{
	static GPlatesModel::PropertyName reconstruction_plate_id_property_name =
		GPlatesModel::PropertyName::create_gpml("reconstructionPlateId");
	static GPlatesModel::PropertyName left_plate_id_property_name =
		GPlatesModel::PropertyName::create_gpml("leftPlate");
	static GPlatesModel::PropertyName right_plate_id_property_name =
		GPlatesModel::PropertyName::create_gpml("rightPlate");


	if (current_top_level_propname() == reconstruction_plate_id_property_name)
	{
		d_reconstruction_plate_id.reset(gpml_plate_id.value());
	}
	else if (current_top_level_propname() == left_plate_id_property_name)
	{
		d_left_plate.reset(gpml_plate_id.value());
	}
	else if (current_top_level_propname() == right_plate_id_property_name)
	{
		d_right_plate.reset(gpml_plate_id.value());
	}

}

void
GPlatesAppLogic::FlowlineUtils::FlowlinePropertyFinder::visit_xs_string(
	const GPlatesPropertyValues::XsString &xs_string)
{
	static const GPlatesModel::PropertyName name_property_name =
		GPlatesModel::PropertyName::create_gml("name");

	if (current_top_level_propname() && *current_top_level_propname() == name_property_name)
	{
		d_name = GPlatesUtils::make_qstring(xs_string.value());
	}
}


bool
GPlatesAppLogic::FlowlineUtils::FlowlinePropertyFinder::can_process_flowline()
{

	if (d_left_plate && 
		d_right_plate &&
		!d_times.empty())
	{
		return true;
	}	

	return false;
}

void
GPlatesAppLogic::FlowlineUtils::FlowlinePropertyFinder::finalise_post_feature_properties(
	const GPlatesModel::FeatureHandle &feature_handle)
{}

bool
GPlatesAppLogic::FlowlineUtils::FlowlinePropertyFinder::initialise_pre_feature_properties(
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

	return true;
}


void
GPlatesAppLogic::FlowlineUtils::calculate_upstream_symmetric_flowline(
	const GPlatesMaths::PointOnSphere &seed_point, 
	const FlowlinePropertyFinder &flowline_parameters,
	std::vector<GPlatesMaths::PointOnSphere> &flowline,
	const GPlatesAppLogic::ReconstructionTree::non_null_ptr_to_const_type &tree,
	const std::vector<GPlatesMaths::FiniteRotation> &rotations)
{
	using namespace GPlatesMaths;

	PointOnSphere::non_null_ptr_to_const_type reconstructed_seed_point
		 = ReconstructUtils::reconstruct_as_half_stage(
			seed_point.get_non_null_pointer(),
			flowline_parameters.get_left_plate().get(),
			flowline_parameters.get_right_plate().get(),
			*tree);


	flowline.push_back(*reconstructed_seed_point);

	std::vector<FiniteRotation>::const_iterator
		iter = rotations.begin(),
		end = rotations.end();

	for (; iter != end ; ++iter)
	{
		PointOnSphere::non_null_ptr_to_const_type flowline_point = 
			*iter * reconstructed_seed_point;
		flowline.push_back(*flowline_point);
	}

}


void
GPlatesAppLogic::FlowlineUtils::calculate_downstream_symmetric_flowline(
	const GPlatesMaths::PointOnSphere &seed_point, 
	const FlowlinePropertyFinder &flowline_parameters,
	std::vector<GPlatesMaths::PointOnSphere> &flowline,
	const GPlatesAppLogic::ReconstructionTree::non_null_ptr_to_const_type &tree,
	const std::vector<GPlatesMaths::FiniteRotation> &rotations)
{
	using namespace GPlatesMaths;

	PointOnSphere::non_null_ptr_to_const_type reconstructed_seed_point = ReconstructUtils::reconstruct_as_half_stage(
		seed_point.get_non_null_pointer(),
		flowline_parameters.get_left_plate().get(),
		flowline_parameters.get_right_plate().get(),
		*tree);


	flowline.push_back(*reconstructed_seed_point);

	std::vector<FiniteRotation>::const_iterator
		iter = rotations.begin(),
		end = rotations.end();

	for (; iter != end ; ++iter)
	{
		GPlatesMaths::FiniteRotation rot = *iter;
		rot = get_reverse(rot);
		PointOnSphere::non_null_ptr_to_const_type flowline_point = 
			rot * reconstructed_seed_point;
		flowline.push_back(*flowline_point);
	}

}
