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

#include "MotionTrackUtils.h"

#include "app-logic/AppLogicUtils.h"
#include "app-logic/FlowlineUtils.h"
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
GPlatesAppLogic::MotionTrackUtils::MotionTrackPropertyFinder::visit_gpml_irregular_sampling(
		const gpml_irregular_sampling_type &gpml_irregular_sampling)
{
	FlowlineUtils::get_times_from_irregular_sampling(d_times,gpml_irregular_sampling);
}


void
GPlatesAppLogic::MotionTrackUtils::MotionTrackPropertyFinder::visit_gpml_plate_id(
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

bool
GPlatesAppLogic::MotionTrackUtils::MotionTrackPropertyFinder::can_process_motion_track()
{

	if (d_reconstruction_plate_id && 
		d_relative_plate_id &&
		!d_times.empty())
	{
		return true;
	}	

	return false;
}

void
GPlatesAppLogic::MotionTrackUtils::MotionTrackPropertyFinder::finalise_post_feature_properties(
	const GPlatesModel::FeatureHandle &feature_handle)
{

}

bool
GPlatesAppLogic::MotionTrackUtils::MotionTrackPropertyFinder::initialise_pre_feature_properties(
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
GPlatesAppLogic::MotionTrackUtils::calculate_motion_track(
	const GPlatesMaths::PointOnSphere &seed_point, 
	const MotionTrackPropertyFinder &motion_track_parameters,
	std::vector<GPlatesMaths::PointOnSphere> &motion_track,
	const GPlatesAppLogic::ReconstructionTree::non_null_ptr_to_const_type &tree,
	const std::vector<GPlatesMaths::FiniteRotation> &rotations)
{

	using namespace GPlatesMaths;

	PointOnSphere::non_null_ptr_to_const_type reconstructed_seed_point = ReconstructUtils::reconstruct(
			seed_point.get_non_null_pointer(),
			motion_track_parameters.get_reconstruction_plate_id().get(),
			*tree);


	std::vector<FiniteRotation>::const_reverse_iterator
		iter = rotations.rbegin(),
		end = rotations.rend();

	for (; iter != end ; ++iter)
	{
		PointOnSphere::non_null_ptr_to_const_type motion_track_point = 
			*iter * reconstructed_seed_point;
		motion_track.push_back(*motion_track_point);
	}

	motion_track.push_back(*reconstructed_seed_point);


}
