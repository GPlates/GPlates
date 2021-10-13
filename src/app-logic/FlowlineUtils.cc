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
#include "property-values/GeoTimeInstant.h"
#include "property-values/GpmlArray.h"
#include "property-values/GpmlIrregularSampling.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GmlMultiPoint.h"
#include "property-values/GmlPoint.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/XsBoolean.h"



void
GPlatesAppLogic::FlowlineUtils::get_times_from_time_period_array(
	std::vector<double> &times,
	const GPlatesPropertyValues::GpmlArray &gpml_array)
{
	static const GPlatesPropertyValues::TemplateTypeParameterType gml_time_period_type =
		GPlatesPropertyValues::TemplateTypeParameterType::create_gml("TimePeriod");	

	if (gpml_array.type() != gml_time_period_type)
	{
		return;
	}
	
	std::vector<GPlatesModel::PropertyValue::non_null_ptr_type>::const_iterator 
		iter = gpml_array.members().begin(),
		end = gpml_array.members().end();

	// We will use the last gml_time_period_ptr after the loop has completed so declare it now.
        GPlatesPropertyValues::GmlTimePeriod* gml_time_period_ptr = NULL;

		for (; iter != end ; ++iter)
		{
			gml_time_period_ptr =
				dynamic_cast<GPlatesPropertyValues::GmlTimePeriod*>((*iter).get());

			GPlatesPropertyValues::GeoTimeInstant geo_time_instant = 
				gml_time_period_ptr->end()->time_position();

			if (geo_time_instant.is_real())
			{
				times.push_back(geo_time_instant.value());
			}
		}
		if (gml_time_period_ptr)
		{
			GPlatesPropertyValues::GeoTimeInstant geo_time_instant =
				gml_time_period_ptr->begin()->time_position();
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
	    // Also, cache the time of appearance/dissappearance.
	    d_time_of_appearance = gml_time_period.begin()->time_position();
	    d_time_of_dissappearance = gml_time_period.end()->time_position();
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
    // Process the flowline if:
    // we can process the seed point and
    // we have left and right plate ids
    // we have a reconstruction time and
    // we have a non-empty times vector and
    // the reconstruction time lies between the end points of the times vector.
	if (d_times.empty() ||
	   !d_left_plate ||
	   !d_right_plate ||
	   !d_reconstruction_time ||
	   !can_process_seed_point())
	{
	    return false;
	}

	// Assumes the times are sorted.
	double oldest = d_times.back();
	double youngest = d_times.front();


	if ((d_reconstruction_time->value() <= oldest) &&
	     (d_reconstruction_time->value() >= youngest))
	{
		return true;
	}	

	return false;
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
	d_has_geometry = false;

	return true;
}


void
GPlatesAppLogic::FlowlineUtils::calculate_flowline(
        const GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type &seed_point,
	const FlowlinePropertyFinder &flowline_parameters,
	std::vector<GPlatesMaths::PointOnSphere> &flowline,
	const GPlatesAppLogic::ReconstructionTree::non_null_ptr_to_const_type &tree,
	const std::vector<GPlatesMaths::FiniteRotation> &rotations)
{
	using namespace GPlatesMaths;

	flowline.push_back(*seed_point);

	std::vector<FiniteRotation>::const_iterator
		iter = rotations.begin(),
		end = rotations.end();

	GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type current_point = seed_point;

	for (; iter != end ; ++iter)
	{
		PointOnSphere::non_null_ptr_to_const_type new_point =
			*iter * current_point;
		flowline.push_back(*new_point);
		current_point = new_point;
	}

}


GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type
GPlatesAppLogic::FlowlineUtils::reconstruct_seed_point(
	GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type seed_point,
	const std::vector<GPlatesMaths::FiniteRotation> &rotations,
	bool reverse)
{
    GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type current_point = seed_point;

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

GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
GPlatesAppLogic::FlowlineUtils::reconstruct_seed_points(
	GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type seed_points,
	const std::vector<GPlatesMaths::FiniteRotation> &rotations,
	bool reverse)
{
    GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type current_points = seed_points;

    if (reverse)
    {
		std::vector<GPlatesMaths::FiniteRotation>::const_reverse_iterator
			iter = rotations.rbegin(),
			end = rotations.rend();

		for(; iter != end; ++iter)
		{
			current_points = get_reverse(*iter) * current_points;
		}

    }
    else
    {
		std::vector<GPlatesMaths::FiniteRotation>::const_iterator
			iter = rotations.begin(),
			end = rotations.end();

		for (; iter != end ; ++iter)
		{
			current_points  = (*iter) * current_points;
		}
    }

    return current_points;
}

void
GPlatesAppLogic::FlowlineUtils::fill_seed_point_rotations(
    const double &current_time,
    const std::vector<double> &flowline_times,
    const GPlatesModel::integer_plate_id_type &left_plate_id,
    const GPlatesModel::integer_plate_id_type &right_plate_id,
    const GPlatesAppLogic::ReconstructionTree::non_null_ptr_to_const_type tree_ptr,
    std::vector<GPlatesMaths::FiniteRotation> &seed_point_rotations)
{
    GPlatesModel::integer_plate_id_type anchor = tree_ptr->get_anchor_plate_id();

    std::vector<double>::const_iterator
            t_iter = flowline_times.begin(),
            t_prev_iter = flowline_times.begin();

    ++t_iter;

    if (current_time > flowline_times.back())
    {
        return;
    }

    for (; *t_iter < current_time; ++t_iter, ++t_prev_iter)
    {

        GPlatesAppLogic::ReconstructionTree::non_null_ptr_type tree_at_time_t_ptr =
                GPlatesAppLogic::ReconstructUtils::create_reconstruction_tree(
                    *t_iter,
                    anchor,
                    tree_ptr->get_reconstruction_features());

        GPlatesAppLogic::ReconstructionTree::non_null_ptr_type tree_at_prev_time_ptr =
                GPlatesAppLogic::ReconstructUtils::create_reconstruction_tree(
                    *t_prev_iter,
                    anchor,
                    tree_ptr->get_reconstruction_features());

        // The stage pole for the moving plate w.r.t. the fixed plate, from t_prev to t
        GPlatesMaths::FiniteRotation stage_pole =
                GPlatesAppLogic::ReconstructUtils::get_stage_pole(
                    *tree_at_prev_time_ptr,
                    *tree_at_time_t_ptr,
                    right_plate_id,
                    left_plate_id);

        FlowlineUtils::get_half_angle_rotation(stage_pole);

       // ReconstructUtils::display_rotation(stage_pole);
        seed_point_rotations.push_back(stage_pole);

    }

    if (*t_prev_iter < current_time)
    {
        // And one more, from the last time reached to the current time.
        GPlatesAppLogic::ReconstructionTree::non_null_ptr_type tree_at_time_t_ptr =
                GPlatesAppLogic::ReconstructUtils::create_reconstruction_tree(
                    *t_prev_iter,
                    anchor,
                    tree_ptr->get_reconstruction_features());

        GPlatesAppLogic::ReconstructionTree::non_null_ptr_type tree_at_current_time_ptr =
                GPlatesAppLogic::ReconstructUtils::create_reconstruction_tree(
                    current_time,
                    anchor,
                    tree_ptr->get_reconstruction_features());

        GPlatesMaths::FiniteRotation stage_pole =
                GPlatesAppLogic::ReconstructUtils::get_stage_pole(
                    *tree_at_time_t_ptr,
                    *tree_at_current_time_ptr,
                    right_plate_id,
                    left_plate_id);

        FlowlineUtils::get_half_angle_rotation(stage_pole);
        seed_point_rotations.push_back(stage_pole);

       // ReconstructUtils::display_rotation(stage_pole);
    }
}

GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
GPlatesAppLogic::FlowlineUtils::reconstruct_flowline_seed_points(
	GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type seed_points,
	const GPlatesAppLogic::ReconstructionTree::non_null_ptr_to_const_type current_reconstruction_tree_ptr,
	const GPlatesModel::FeatureHandle::weak_ref &feature_handle,
	bool reverse)
{
    GPlatesAppLogic::FlowlineUtils::FlowlinePropertyFinder finder(
		current_reconstruction_tree_ptr->get_reconstruction_time());
    finder.visit_feature(feature_handle);

    if (!finder.can_correct_seed_point())
    {
		return seed_points;
    }

    std::vector<GPlatesMaths::FiniteRotation> seed_point_rotations;

    GPlatesAppLogic::FlowlineUtils::fill_seed_point_rotations(
		current_reconstruction_tree_ptr->get_reconstruction_time(),
		finder.get_times(),
		finder.get_left_plate().get(),
		finder.get_right_plate().get(),
		current_reconstruction_tree_ptr,
		seed_point_rotations);

    GPlatesMaths::FiniteRotation plate_correction =
	    current_reconstruction_tree_ptr->get_composed_absolute_rotation(finder.get_left_plate().get()).first;

    if (reverse)
    {
		seed_points = get_reverse(plate_correction) * seed_points;
    }

    GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type corrected_seed_points =
	    GPlatesAppLogic::FlowlineUtils::reconstruct_seed_points(seed_points,seed_point_rotations,reverse);

    if (!reverse)
    {
		corrected_seed_points = plate_correction * corrected_seed_points;
    }

    return corrected_seed_points;
}

GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
GPlatesAppLogic::FlowlineUtils::correct_end_point_to_centre(
    GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry_,
    const GPlatesModel::integer_plate_id_type &plate_1,
    const GPlatesModel::integer_plate_id_type &plate_2,
    const std::vector<double> &flowline_feature_times,
    const GPlatesAppLogic::ReconstructionTree::non_null_ptr_to_const_type &tree,
    const double &reconstruction_time)
{
    std::vector<double> times;
    std::vector<GPlatesMaths::FiniteRotation> flowline_rotations;

    fill_times_vector(
		times,
		reconstruction_time,
		flowline_feature_times);

    // FIXME: This (almost) duplicates code from the FlowlineGeometryPopulator. Refactor.

    // We'll work from the current time, backwards in time.
    std::vector<double>::const_iterator
	    iter = times.begin(),
	    end = times.end();

    GPlatesModel::integer_plate_id_type anchor = tree->get_anchor_plate_id();

    // Save the "previous" tree for use in the loop.
    GPlatesAppLogic::ReconstructionTree::non_null_ptr_type tree_at_prev_time_ptr =
	    GPlatesAppLogic::ReconstructUtils::create_reconstruction_tree(
	    *iter,
	    anchor,
	    tree->get_reconstruction_features());

    // Step forward beyond the current time
    ++iter;

    for (; iter != end ; ++iter)
    {

	    GPlatesAppLogic::ReconstructionTree::non_null_ptr_type tree_at_time_t_ptr =
		    GPlatesAppLogic::ReconstructUtils::create_reconstruction_tree(
		    *iter,
		    anchor,
		    tree->get_reconstruction_features());

	    GPlatesMaths::FiniteRotation stage_pole =
		    GPlatesAppLogic::ReconstructUtils::get_stage_pole(
		    *tree_at_prev_time_ptr,
		    *tree_at_time_t_ptr,
		    plate_2,
		    plate_1);


	    // Halve the stage pole and store it.
	    FlowlineUtils::get_half_angle_rotation(stage_pole);
	    flowline_rotations.push_back(stage_pole);

	    tree_at_prev_time_ptr = tree_at_time_t_ptr;

    }

    GPlatesMaths::FiniteRotation correction =
	    tree->get_composed_absolute_rotation(plate_1).first;


    geometry_ = get_reverse(correction) * geometry_;

    std::vector<GPlatesMaths::FiniteRotation>::const_reverse_iterator
	    riter = flowline_rotations.rbegin(),
	    rend = flowline_rotations.rend();

    for (; riter != rend ; ++riter)
    {
		geometry_ = get_reverse(*riter) * geometry_;
    }

    return correction * geometry_;
}
