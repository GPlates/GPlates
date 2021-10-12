/* $Id$ */
 
/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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

#include <boost/foreach.hpp>

#include "ReconstructRasterPolygons.h"

#include "model/PropertyName.h"

#include "property-values/GmlPolygon.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlPlateId.h"


void
GPlatesAppLogic::ReconstructRasterPolygons::update_rotations(
		const ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree)
{
	d_current_reconstruction_tree = reconstruction_tree;

	// Iterate through the rotation groups and update their rotations.
	BOOST_FOREACH(
			const rotation_group_map_type::value_type &rotation_group_entry,
			d_rotation_groups)
	{
		// The plate id of this rotation group.
		const GPlatesModel::integer_plate_id_type plate_id = rotation_group_entry.first;

		// The rotation group.
		RotationGroup &rotation_group = *rotation_group_entry.second;

		// Get the updated rotation.
		rotation_group.current_rotation =
				d_current_reconstruction_tree->get_composed_absolute_rotation(
						plate_id).first.unit_quat();
	}
}


void
GPlatesAppLogic::ReconstructRasterPolygons::get_rotation_groups_sorted_by_plate_id(
		std::vector<RotationGroup::non_null_ptr_to_const_type> &rotation_groups) const
{
	// Place the no rotation group in first.
	if (d_no_plate_id_rotation_group)
	{
		rotation_groups.push_back(d_no_plate_id_rotation_group.get());
	}

	// The other rotation groups grouped are already sorted by plate id.
	BOOST_FOREACH(
			const rotation_group_map_type::value_type &rotation_group_entry,
			d_rotation_groups)
	{
		rotation_groups.push_back(rotation_group_entry.second);
	}
}


bool
GPlatesAppLogic::ReconstructRasterPolygons::initialise_pre_feature_properties(
		const GPlatesModel::FeatureHandle &/*feature_handle*/)
{
	// Start accumulating information for the current feature.
	d_feature_info_accumulator = FeatureInfoAccumulator();

	return true;
}


void
GPlatesAppLogic::ReconstructRasterPolygons::finalise_post_feature_properties(
		feature_handle_type &/*feature_handle*/)
{
	// If we found any polygons...
	if (!d_feature_info_accumulator.polygon_regions.empty())
	{
		RotationGroup &rotation_group = get_rotation_group();

		// Update the time of appearance/disappearance of each polygon and add to rotation group.
		BOOST_FOREACH(
				const ReconstructablePolygonRegion::non_null_ptr_type &polygon_region,
				d_feature_info_accumulator.polygon_regions)
		{
			polygon_region->time_of_appearance = d_feature_info_accumulator.time_of_appearance;
			polygon_region->time_of_disappearance = d_feature_info_accumulator.time_of_disappearance;

			rotation_group.polygon_regions.push_back(polygon_region);
		}
	}

	// Finished gathering information about the current feature.
	d_feature_info_accumulator = FeatureInfoAccumulator();
}


void
GPlatesAppLogic::ReconstructRasterPolygons::visit_gml_polygon(
		const GPlatesPropertyValues::GmlPolygon &gml_polygon)
{
	ReconstructablePolygonRegion::non_null_ptr_type polygon_region =
			ReconstructablePolygonRegion::create(gml_polygon.exterior());

	// Add any interior polygons.
	polygon_region->interior_polygons.insert(
			polygon_region->interior_polygons.end(),
			gml_polygon.interiors_begin(),
			gml_polygon.interiors_end());

	// Keep a list of polygon regions in the current feature (in case have more than
	// one polygon geometry property for some reason).
	d_feature_info_accumulator.polygon_regions.push_back(polygon_region);
}


void
GPlatesAppLogic::ReconstructRasterPolygons::visit_gml_time_period(
		const GPlatesPropertyValues::GmlTimePeriod &gml_time_period)
{
	static const GPlatesModel::PropertyName valid_time_property_name =
		GPlatesModel::PropertyName::create_gml("validTime");

	// Note that we're going to assume that we're in a property...
	if (current_top_level_propname() == valid_time_property_name)
	{
		d_feature_info_accumulator.time_of_appearance = gml_time_period.begin()->time_position();
		d_feature_info_accumulator.time_of_disappearance = gml_time_period.end()->time_position();
	}
}


void
GPlatesAppLogic::ReconstructRasterPolygons::visit_gpml_constant_value(
		const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
{
	gpml_constant_value.value()->accept_visitor(*this);
}


void
GPlatesAppLogic::ReconstructRasterPolygons::visit_gpml_plate_id(
		const GPlatesPropertyValues::GpmlPlateId &gpml_plate_id)
{
	static GPlatesModel::PropertyName reconstruction_plate_id_property_name =
			GPlatesModel::PropertyName::create_gpml("reconstructionPlateId");

	// Note that we're going to assume that we're in a property...
	if (current_top_level_propname() == reconstruction_plate_id_property_name)
	{
		// This plate ID is the reconstruction plate ID.
		d_feature_info_accumulator.recon_plate_id = gpml_plate_id.value();
	}
}


GPlatesAppLogic::ReconstructRasterPolygons::RotationGroup &
GPlatesAppLogic::ReconstructRasterPolygons::get_rotation_group()
{
	//
	// Get, or create, the rotation group to put the polygon regions
	// of the just visited feature into.
	//

	if (!d_feature_info_accumulator.recon_plate_id)
	{
		if (!d_no_plate_id_rotation_group)
		{
			d_no_plate_id_rotation_group = RotationGroup::create(
					GPlatesMaths::UnitQuaternion3D::create_identity_rotation());
		}

		return *d_no_plate_id_rotation_group.get();
	}

	// The polygon plate id.
	const GPlatesModel::integer_plate_id_type plate_id =
			d_feature_info_accumulator.recon_plate_id.get();

	// See if there's already a rotation group for the plate id.
	rotation_group_map_type::iterator rotation_group_iter =
			d_rotation_groups.find(plate_id);
	if (rotation_group_iter == d_rotation_groups.end())
	{
		// Get the initial rotation for this plate id.
		const GPlatesMaths::UnitQuaternion3D &initial_rotation =
				d_current_reconstruction_tree->get_composed_absolute_rotation(plate_id)
						.first.unit_quat();

		// Create a new rotation group.
		RotationGroup::non_null_ptr_type rotation_group =
				RotationGroup::create(initial_rotation);

		// Insert the rotation group into our map.
		rotation_group_iter = d_rotation_groups.insert(
				rotation_group_map_type::value_type(plate_id, rotation_group)).first;
	}

	// Return the rotation group.
	return *rotation_group_iter->second;
}
