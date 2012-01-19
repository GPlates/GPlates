/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date: 2010-08-23 05:33:53 +0200 (ma, 23 aug 2010) $
 * 
 * Copyright (C) 2011, 2012 Geological Survey of Norway
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

#include "app-logic/ReconstructedSmallCircle.h"
#include "app-logic/ReconstructionGeometry.h"
#include "property-values/GeoTimeInstant.h"
#include "property-values/GmlPoint.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlMeasure.h"
#include "property-values/GpmlPlateId.h"
#include "SmallCircleGeometryPopulator.h"

GPlatesAppLogic::SmallCircleGeometryPopulator::SmallCircleGeometryPopulator(
	std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> &reconstructed_feature_geometries,
	const ReconstructionTreeCreator &reconstruction_tree_creator,
	const double &reconstruction_time):
		d_reconstructed_feature_geometries(reconstructed_feature_geometries),
		d_reconstruction_tree_creator(reconstruction_tree_creator),
		d_reconstruction_time(reconstruction_time),
		d_feature_is_defined_at_recon_time(true)
{

}

bool
GPlatesAppLogic::SmallCircleGeometryPopulator::initialise_pre_feature_properties(
	GPlatesModel::FeatureHandle &feature_handle)
{
	d_feature_is_defined_at_recon_time = true;
	return true;
}

void
GPlatesAppLogic::SmallCircleGeometryPopulator::finalise_post_feature_properties(
	GPlatesModel::FeatureHandle &feature_handle)
{
	if (d_feature_is_defined_at_recon_time 
		&& d_centre  
		&& d_radius_in_degrees)
	{
		
		// The reconstruction tree for the current reconstruction time.
		ReconstructionTree::non_null_ptr_to_const_type reconstruction_tree =
			d_reconstruction_tree_creator.get_reconstruction_tree(d_reconstruction_time.value());		
		
                if (d_reconstruction_plate_id)
                {
                        *d_centre = reconstruction_tree->get_composed_absolute_rotation(
                                    d_reconstruction_plate_id.get()).first *
                                    *d_centre;
                }



		GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_type small_circle_rg =
			ReconstructedSmallCircle::create(
				reconstruction_tree,
				*d_centre,
				GPlatesMaths::convert_deg_to_rad(*d_radius_in_degrees),
				feature_handle,
				*d_geometry_iterator,
				d_reconstruction_plate_id);



		d_reconstructed_feature_geometries.push_back(small_circle_rg);

	}
}

void
GPlatesAppLogic::SmallCircleGeometryPopulator::visit_gml_point(
	GPlatesPropertyValues::GmlPoint &gml_point)
{
	if (current_top_level_propname())
	{
		static const GPlatesModel::PropertyName small_circle_centre_property_name = 
			GPlatesModel::PropertyName::create_gpml("centre");
		GPlatesModel::PropertyName property_name = *current_top_level_propname();	
		if (property_name != small_circle_centre_property_name)
		{
			return;
		}
		d_centre.reset(gml_point.point());

		d_geometry_iterator.reset(*(current_top_level_propiter()));
	}	

}

void
GPlatesAppLogic::SmallCircleGeometryPopulator::visit_gml_time_period(
	GPlatesPropertyValues::GmlTimePeriod &gml_time_period)
{
	if (current_top_level_propname())
	{
		static const GPlatesModel::PropertyName valid_time_property_name =
			GPlatesModel::PropertyName::create_gml("validTime");

		// Note that we're going to assume that we're in a property...
		if (current_top_level_propname() == valid_time_property_name)
		{
			// This time period is the "valid time" time period.
			if (!gml_time_period.contains(d_reconstruction_time))
			{
				// Oh no!  This feature instance is not defined at the recon time!
				d_feature_is_defined_at_recon_time = false;
			}

		}
	}
}

void
GPlatesAppLogic::SmallCircleGeometryPopulator::visit_gpml_measure(
	GPlatesPropertyValues::GpmlMeasure &gpml_measure)
{
	if (current_top_level_propname())
	{
		static const GPlatesModel::PropertyName small_circle_radius_property_name = 
			GPlatesModel::PropertyName::create_gpml("angularRadius");
		GPlatesModel::PropertyName property_name = *current_top_level_propname();	
		if (property_name != small_circle_radius_property_name)
		{
			return;
		}

		d_radius_in_degrees.reset(gpml_measure.quantity());
	}	
}

void
GPlatesAppLogic::SmallCircleGeometryPopulator::visit_gpml_constant_value(
	GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
{
	gpml_constant_value.value()->accept_visitor(*this);
}

void
GPlatesAppLogic::SmallCircleGeometryPopulator::visit_gpml_plate_id(
	GPlatesPropertyValues::GpmlPlateId &gpml_plate_id)
{
	d_reconstruction_plate_id.reset(gpml_plate_id.value());
}
