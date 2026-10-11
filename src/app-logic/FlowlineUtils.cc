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
#include <algorithm>

#include "boost/optional.hpp"

#include <QDebug>

#include "FlowlineUtils.h"

#include "app-logic/AppLogicUtils.h"
#include "app-logic/PlateVelocityUtils.h"
#include "app-logic/ReconstructionTree.h"
#include "app-logic/RotationUtils.h"
#include "maths/CalculateVelocity.h"
#include "maths/FiniteRotation.h"
#include "maths/MathsUtils.h"
#include "model/FeatureVisitor.h"
#include "model/Model.h"
#include "model/ModelUtils.h"
#include "property-values/GeoTimeInstant.h"
#include "property-values/GpmlArray.h"
#include "property-values/GpmlIrregularSampling.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GmlMultiPoint.h"
#include "property-values/GmlPoint.h"
#include "property-values/GmlTimeInstant.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/XsBoolean.h"



void
GPlatesAppLogic::FlowlineUtils::get_times_from_time_period_array(
	std::vector<double> &times,
	const GPlatesPropertyValues::GpmlArray &gpml_array)
{
	static const GPlatesPropertyValues::StructuralType gml_time_period_type =
		GPlatesPropertyValues::StructuralType::create_gml("TimePeriod");	

	if (gpml_array.get_value_type() != gml_time_period_type)
	{
		return;
	}
	
	const GPlatesModel::RevisionedVector<GPlatesModel::PropertyValue> &gpml_array_members = gpml_array.members();
	GPlatesModel::RevisionedVector<GPlatesModel::PropertyValue>::const_iterator iter = gpml_array_members.begin();
	GPlatesModel::RevisionedVector<GPlatesModel::PropertyValue>::const_iterator end = gpml_array_members.end();

	// We will use the last gml_time_period_ptr after the loop has completed so declare it now.
       const GPlatesPropertyValues::GmlTimePeriod* gml_time_period_ptr = NULL;

		for (; iter != end ; ++iter)
		{
			gml_time_period_ptr =
				dynamic_cast<const GPlatesPropertyValues::GmlTimePeriod*>((*iter).get_element().get());

			GPlatesPropertyValues::GeoTimeInstant geo_time_instant = 
				gml_time_period_ptr->end()->get_time_position();

			if (geo_time_instant.is_real())
			{
				times.push_back(geo_time_instant.value());
			}
		}
		if (gml_time_period_ptr)
		{
			GPlatesPropertyValues::GeoTimeInstant geo_time_instant =
				gml_time_period_ptr->begin()->get_time_position();
			if (geo_time_instant.is_real())
			{
				times.push_back(geo_time_instant.value());
			}
		}
}



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
	// Outside the time samples, start at the nearest one.
	double flowline_time = reconstruction_time;
	if (!time_samples.empty())
	{
		flowline_time = (std::max)(flowline_time, time_samples.front());
		flowline_time = (std::min)(flowline_time, time_samples.back());
	}

	times.push_back(flowline_time);

	std::vector<double>::const_iterator
		samples_iter = time_samples.begin(),
		samples_end = time_samples.end();

	// Get to the first time which is older than that time.
	while ((samples_iter != samples_end) && (*samples_iter <= flowline_time))
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
GPlatesAppLogic::FlowlineUtils::FlowlinePropertyFinder::visit_gpml_array(
		const GPlatesPropertyValues::GpmlArray &gpml_array)
{
	get_times_from_time_period_array(d_times,gpml_array);
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
		d_reconstruction_plate_id.reset(gpml_plate_id.get_value());
	}
	else if (current_top_level_propname() == left_plate_id_property_name)
	{
		d_left_plate.reset(gpml_plate_id.get_value());
	}
	else if (current_top_level_propname() == right_plate_id_property_name)
	{
		d_right_plate.reset(gpml_plate_id.get_value());
	}

}

void
GPlatesAppLogic::FlowlineUtils::FlowlinePropertyFinder::visit_gml_time_period(
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
	    // Also, cache the time of appearance/disappearance.
	    d_time_of_appearance = gml_time_period.begin()->get_time_position();
	    d_time_of_dissappearance = gml_time_period.end()->get_time_position();
	}
}


void
GPlatesAppLogic::FlowlineUtils::FlowlinePropertyFinder::visit_gml_time_instant(
		const GPlatesPropertyValues::GmlTimeInstant &gml_time_instant)
{
	static const GPlatesModel::PropertyName geometry_import_time_property_name =
		GPlatesModel::PropertyName::create_gpml("geometryImportTime");

	if (current_top_level_propname() == geometry_import_time_property_name &&
		gml_time_instant.get_time_position().is_real())
	{
		d_geometry_import_time = gml_time_instant.get_time_position().value();
	}
}


double
GPlatesAppLogic::FlowlineUtils::FlowlinePropertyFinder::get_spreading_start_time() const
{
	return d_geometry_import_time ? d_geometry_import_time.get() : d_times.front();
}


void
GPlatesAppLogic::FlowlineUtils::FlowlinePropertyFinder::visit_xs_string(
	const GPlatesPropertyValues::XsString &xs_string)
{
	static const GPlatesModel::PropertyName name_property_name =
		GPlatesModel::PropertyName::create_gml("name");

	if (current_top_level_propname() && *current_top_level_propname() == name_property_name)
	{
		d_name = GPlatesUtils::make_qstring(xs_string.get_value());
	}
}


bool
GPlatesAppLogic::FlowlineUtils::FlowlinePropertyFinder::can_process_flowline()
{
    // Process the flowline if:
    // we can process the seed point and
    // we have left and right plate ids
    // we have a reconstruction time and
    // we have a non-empty times vector.
	if (d_times.empty() ||
	   !d_left_plate ||
	   !d_right_plate ||
	   !d_reconstruction_time ||
	   !can_process_seed_point())
	{
	    return false;
	}

	// NOTE: The reconstruction time need not lie between the end points of the times vector
	// (as for motion paths). Younger than the youngest time, each half of the flowline starts at
	// that time (not at the seed point, which is where the spreading centre is). Older than the
	// oldest time, there is only the seed point, carried by the left plate from where the spreading
	// centre was at that time.

	return true;
}

bool
GPlatesAppLogic::FlowlineUtils::FlowlinePropertyFinder::can_process_seed_point()
{
	return (d_feature_is_defined_at_recon_time && d_has_geometry);
}

bool
GPlatesAppLogic::FlowlineUtils::FlowlinePropertyFinder::can_correct_seed_point()
{
    // We can correct the seed point location (at feature-creation time) if:
    // we have left and right plate ids and
    // we have a reconstruction time and
    // we have a non-empty times vector.
	if (d_times.empty() ||
	   !d_left_plate ||
	   !d_right_plate ||
	   !d_reconstruction_time)
	{
	    return false;
	}
	return true;
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

	d_times.clear();

	d_feature_is_defined_at_recon_time = true;
	d_time_of_appearance = boost::none;
	d_time_of_dissappearance = boost::none;
	d_left_plate = boost::none;
	d_right_plate = boost::none;
	d_reconstruction_plate_id = boost::none;
	d_geometry_import_time = boost::none;
	d_name.clear();
	d_has_geometry = false;

	return true;
}


void
GPlatesAppLogic::FlowlineUtils::calculate_flowline(
        const GPlatesMaths::PointOnSphere &reconstructed_seed_point,
		const FlowlinePropertyFinder &flowline_parameters,
		std::vector<GPlatesMaths::PointOnSphere> &flowline,
		const ReconstructionTreeCreator &reconstruction_tree_creator,
		const std::vector<GPlatesMaths::FiniteRotation> &rotations)
{
	using namespace GPlatesMaths;

	flowline.push_back(reconstructed_seed_point);

	std::vector<FiniteRotation>::const_iterator		iter = rotations.begin(),
		end = rotations.end();

	GPlatesMaths::PointOnSphere current_point = reconstructed_seed_point;

	for (; iter != end ; ++iter)
	{
		const PointOnSphere new_point = *iter * current_point;
		flowline.push_back(new_point);
		current_point = new_point;
	}

}


GPlatesMaths::PointOnSphere
GPlatesAppLogic::FlowlineUtils::reconstruct_seed_point(
	const GPlatesMaths::PointOnSphere &seed_point,
	const std::vector<GPlatesMaths::FiniteRotation> &rotations,
	bool reverse)
{
    GPlatesMaths::PointOnSphere current_point = seed_point;

    if (reverse)
    {
		std::vector<GPlatesMaths::FiniteRotation>::const_reverse_iterator
			iter = rotations.rbegin(),
			end = rotations.rend();

		for(; iter != end; ++iter)
		{
			current_point = get_reverse(*iter) * current_point;
		}

    }
    else
    {
		std::vector<GPlatesMaths::FiniteRotation>::const_iterator
			iter = rotations.begin(),
			end = rotations.end();

		for (; iter != end ; ++iter)
		{
			current_point  = (*iter) * current_point;
		}
    }

    return current_point;
}

namespace
{
	/**
	 * The half-stage rotation, in the frame of @a fixed_plate_id, of @a moving_plate_id from
	 * @a from_time to @a to_time (backward or forward in time).
	 */
	GPlatesMaths::FiniteRotation
	get_half_stage_rotation(
			const double &from_time,
			const double &to_time,
			const GPlatesModel::integer_plate_id_type &fixed_plate_id,
			const GPlatesModel::integer_plate_id_type &moving_plate_id,
			const GPlatesAppLogic::ReconstructionTreeCreator &reconstruction_tree_creator)
	{
		GPlatesMaths::FiniteRotation stage_pole =
				GPlatesAppLogic::RotationUtils::get_stage_pole(
						*reconstruction_tree_creator.get_reconstruction_tree(from_time),
						*reconstruction_tree_creator.get_reconstruction_tree(to_time),
						moving_plate_id,
						fixed_plate_id);

		GPlatesAppLogic::FlowlineUtils::get_half_angle_rotation(stage_pole);

		return stage_pole;
	}

	/**
	 * Adds a "gpml:geometryImportTime" to @a feature_ref. Returns true if it was added.
	 */
	bool
	add_geometry_import_time(
			const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
			const double &geometry_import_time)
	{
		return static_cast<bool>(
				GPlatesModel::ModelUtils::add_property(
						feature_ref,
						GPlatesModel::PropertyName::create_gpml("geometryImportTime"),
						GPlatesModel::ModelUtils::create_gml_time_instant(
								GPlatesPropertyValues::GeoTimeInstant(geometry_import_time))));
	}
}


void
GPlatesAppLogic::FlowlineUtils::fill_flowline_rotations(
	const double &reconstruction_time,
	const std::vector<double> &flowline_times,
	const double &spreading_start_time,
	const GPlatesModel::integer_plate_id_type &fixed_plate_id,
	const GPlatesModel::integer_plate_id_type &moving_plate_id,
	const ReconstructionTreeCreator &reconstruction_tree_creator,
	std::vector<GPlatesMaths::FiniteRotation> &flowline_rotations)
{
	// Each vertex is built from the previous one (the first from the seed point, at the
	// reconstruction time), so each step is from the previous vertex's time.
	double previous_time = reconstruction_time;

	std::vector<double>::const_iterator times_iter = flowline_times.begin();
	for ( ; times_iter != flowline_times.end(); ++times_iter)
	{
		const double vertex_time = *times_iter;
		if (vertex_time <= previous_time)
		{
			continue;
		}

		// The spreading start time splits the step it lies in, so that the vertex doesn't depend on
		// the reconstruction time (the seed point's rotations are also split there).
		if (previous_time < spreading_start_time &&
			spreading_start_time < vertex_time)
		{
			flowline_rotations.push_back(
					GPlatesMaths::compose(
							get_half_stage_rotation(
									spreading_start_time, vertex_time,
									fixed_plate_id, moving_plate_id, reconstruction_tree_creator),
							get_half_stage_rotation(
									previous_time, spreading_start_time,
									fixed_plate_id, moving_plate_id, reconstruction_tree_creator)));
		}
		else
		{
			flowline_rotations.push_back(
					get_half_stage_rotation(
							previous_time, vertex_time,
							fixed_plate_id, moving_plate_id, reconstruction_tree_creator));
		}

		previous_time = vertex_time;
	}
}

GPlatesMaths::FiniteRotation
GPlatesAppLogic::FlowlineUtils::get_left_to_right_plate_frame_rotation(
	const double &flowline_start_time,
	const GPlatesModel::integer_plate_id_type &left_plate_id,
	const GPlatesModel::integer_plate_id_type &right_plate_id,
	const ReconstructionTreeCreator &reconstruction_tree_creator)
{
	// R(0->t,right->left)
	//    = R(0->t,right->A) * R(0->t,A->left)
	//    = inverse[R(0->t,A->right)] * R(0->t,A->left)
	//
	// ...where 'A' is the anchor plate and 't' is the flowline's spreading start time.
	ReconstructionTree::non_null_ptr_to_const_type reconstruction_tree =
			reconstruction_tree_creator.get_reconstruction_tree(flowline_start_time);

	return GPlatesMaths::compose(
			GPlatesMaths::get_reverse(
					reconstruction_tree->get_composed_absolute_rotation(right_plate_id)),
			reconstruction_tree->get_composed_absolute_rotation(left_plate_id));
}

void
GPlatesAppLogic::FlowlineUtils::fill_seed_point_rotations(
    const double &current_time,
    const std::vector<double> &flowline_times,
    const double &spreading_start_time,
    const GPlatesModel::integer_plate_id_type &left_plate_id,
    const GPlatesModel::integer_plate_id_type &right_plate_id,
	const ReconstructionTreeCreator &reconstruction_tree_creator,
    std::vector<GPlatesMaths::FiniteRotation> &seed_point_rotations)
{
	if (flowline_times.empty())
	{
		return;
	}

	// Older than the oldest time, the seed point stays where the spreading centre was at that time
	// (and is carried by the left plate), so it stays continuous across the oldest time.
	const double spreading_time = (std::min)(current_time, flowline_times.back());

	// Steps from the spreading start time to the spreading time, at the flowline times in between.
	std::vector<double> step_times(1, spreading_start_time);
	if (spreading_time > spreading_start_time)
	{
		// Backward in time.
		std::vector<double>::const_iterator times_iter = flowline_times.begin();
		for ( ; times_iter != flowline_times.end(); ++times_iter)
		{
			if (*times_iter > spreading_start_time && *times_iter < spreading_time)
			{
				step_times.push_back(*times_iter);
			}
		}
	}
	else
	{
		// Forward in time (younger than the spreading start time).
		std::vector<double>::const_reverse_iterator times_iter = flowline_times.rbegin();
		for ( ; times_iter != flowline_times.rend(); ++times_iter)
		{
			if (*times_iter < spreading_start_time && *times_iter > spreading_time)
			{
				step_times.push_back(*times_iter);
			}
		}
	}
	step_times.push_back(spreading_time);

	for (unsigned int n = 1; n < step_times.size(); ++n)
	{
		if (step_times[n] != step_times[n - 1])
		{
			seed_point_rotations.push_back(
					get_half_stage_rotation(
							step_times[n - 1], step_times[n],
							left_plate_id, right_plate_id, reconstruction_tree_creator));
		}
	}
}

GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
GPlatesAppLogic::FlowlineUtils::reconstruct_flowline_seed_points(
	GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type seed_points,
	const double &current_time,
	const ReconstructionTreeCreator &reconstruction_tree_creator,
	const GPlatesModel::FeatureHandle::weak_ref &feature_handle,
	bool reverse)
{
    GPlatesAppLogic::FlowlineUtils::FlowlinePropertyFinder finder(current_time);
    finder.visit_feature(feature_handle);

    if (!finder.can_correct_seed_point())
    {
		return seed_points;
    }

	// The same rotation as the seed point velocities use, so they can't disagree.
	const GPlatesMaths::FiniteRotation seed_point_rotation =
		get_seed_point_rotation(
			current_time,
			finder.get_times(),
			finder.get_spreading_start_time(),
			finder.get_left_plate().get(),
			finder.get_right_plate().get(),
			reconstruction_tree_creator);

	return reverse
		? get_reverse(seed_point_rotation) * seed_points
		: seed_point_rotation * seed_points;
}

GPlatesMaths::FiniteRotation
GPlatesAppLogic::FlowlineUtils::get_seed_point_rotation(
	const double &reconstruction_time,
	const std::vector<double> &flowline_times,
	const double &spreading_start_time,
	const GPlatesModel::integer_plate_id_type &left_plate_id,
	const GPlatesModel::integer_plate_id_type &right_plate_id,
	const ReconstructionTreeCreator &reconstruction_tree_creator)
{
	std::vector<GPlatesMaths::FiniteRotation> seed_point_rotations;
	fill_seed_point_rotations(
		reconstruction_time,
		flowline_times,
		spreading_start_time,
		left_plate_id,
		right_plate_id,
		reconstruction_tree_creator,
		seed_point_rotations);

	// The half-stage rotations apply first (in order), then the left plate's rotation.
	GPlatesMaths::FiniteRotation seed_point_rotation =
		reconstruction_tree_creator.get_reconstruction_tree(reconstruction_time)
			->get_composed_absolute_rotation(left_plate_id);

	std::vector<GPlatesMaths::FiniteRotation>::const_reverse_iterator
		iter = seed_point_rotations.rbegin(),
		end = seed_point_rotations.rend();
	for (; iter != end; ++iter)
	{
		seed_point_rotation = GPlatesMaths::compose(seed_point_rotation, *iter);
	}

	return seed_point_rotation;
}

GPlatesMaths::FiniteRotation
GPlatesAppLogic::FlowlineUtils::calculate_seed_point_stage_rotation(
	const double &reconstruction_time,
	const std::vector<double> &flowline_times,
	const double &spreading_start_time,
	const GPlatesModel::integer_plate_id_type &left_plate_id,
	const GPlatesModel::integer_plate_id_type &right_plate_id,
	const ReconstructionTreeCreator &reconstruction_tree_creator,
	const double &velocity_delta_time,
	VelocityDeltaTime::Type velocity_delta_time_type)
{
	const std::pair<double, double> time_range =
		PlateVelocityUtils::get_half_stage_rotation_velocity_time_range(
			left_plate_id,
			right_plate_id,
			reconstruction_tree_creator,
			reconstruction_time,
			velocity_delta_time,
			velocity_delta_time_type);

	return GPlatesMaths::calculate_stage_rotation(
		get_seed_point_rotation(
			time_range.second/*young*/,
			flowline_times,
			spreading_start_time,
			left_plate_id,
			right_plate_id,
			reconstruction_tree_creator),
		get_seed_point_rotation(
			time_range.first/*old*/,
			flowline_times,
			spreading_start_time,
			left_plate_id,
			right_plate_id,
			reconstruction_tree_creator));
}

GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
GPlatesAppLogic::FlowlineUtils::correct_end_point_to_centre(
    GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry_,
    const GPlatesModel::integer_plate_id_type &plate_1,
    const GPlatesModel::integer_plate_id_type &plate_2,
    const std::vector<double> &flowline_feature_times,
    const double &spreading_start_time,
	const ReconstructionTreeCreator &reconstruction_tree_creator,
    const double &reconstruction_time)
{
	// The same rotations that build the half of the flowline on plate 1 out from the seed point
	// (younger than the flowline's youngest time, the half doesn't start at the seed point, but it's
	// still built out from it).
	std::vector<GPlatesMaths::FiniteRotation> flowline_rotations;
	fill_flowline_rotations(
			reconstruction_time,
			flowline_feature_times,
			spreading_start_time,
			plate_1,
			plate_2,
			reconstruction_tree_creator,
			flowline_rotations);

	const GPlatesMaths::FiniteRotation correction =
			reconstruction_tree_creator.get_reconstruction_tree(reconstruction_time)
					->get_composed_absolute_rotation(plate_1);

	geometry_ = get_reverse(correction) * geometry_;

	std::vector<GPlatesMaths::FiniteRotation>::const_reverse_iterator
		riter = flowline_rotations.rbegin(),
		rend = flowline_rotations.rend();
	for ( ; riter != rend ; ++riter)
	{
		geometry_ = get_reverse(*riter) * geometry_;
	}

	// This is the seed point.
	return correction * geometry_;
}


boost::optional<double>
GPlatesAppLogic::FlowlineUtils::get_default_geometry_import_time(
		const GPlatesModel::FeatureHandle::weak_ref &feature_ref)
{
	static const GPlatesModel::FeatureType flowline_feature_type =
			GPlatesModel::FeatureType::create_gpml("Flowline");

	if (!feature_ref.is_valid() ||
		feature_ref->feature_type() != flowline_feature_type)
	{
		return boost::none;
	}

	FlowlinePropertyFinder finder;
	finder.visit_feature(feature_ref);
	if (finder.get_geometry_import_time() ||
		finder.get_times().empty())
	{
		return boost::none;
	}

	return finder.get_times().front();
}


bool
GPlatesAppLogic::FlowlineUtils::set_default_geometry_import_time(
		const GPlatesModel::FeatureHandle::weak_ref &feature_ref)
{
	const boost::optional<double> geometry_import_time = get_default_geometry_import_time(feature_ref);
	if (!geometry_import_time)
	{
		return false;
	}

	return add_geometry_import_time(feature_ref, geometry_import_time.get());
}


bool
GPlatesAppLogic::FlowlineUtils::edit_property_without_shifting_flowline(
		const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
		const GPlatesModel::PropertyName &property_name,
		const std::function<bool ()> &edit_property)
{
	static const GPlatesModel::PropertyName times_property_name =
			GPlatesModel::PropertyName::create_gpml("times");

	// Before the edit, which can change the youngest time.
	const boost::optional<double> geometry_import_time =
			(property_name == times_property_name)
			? get_default_geometry_import_time(feature_ref)
			: boost::none;

	const bool modified = edit_property();

	if (modified && geometry_import_time)
	{
		add_geometry_import_time(feature_ref, geometry_import_time.get());
	}

	return modified;
}
