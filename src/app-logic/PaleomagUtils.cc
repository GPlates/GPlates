/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 Geological Survey of Norway
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
#include <QDebug>

#include "AppLogicUtils.h"
#include "PaleomagUtils.h"

#include "maths/PolylineOnSphere.h" // FIXME: for testing

#include "gui/ColourProxy.h"
#include "model/PropertyName.h"
#include "model/ReconstructedFeatureGeometry.h"
#include "model/Reconstruction.h"
#include "model/ReconstructionTree.h"
#include "property-values/GmlPoint.h"
#include "property-values/GeoTimeInstant.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/XsDouble.h"
#include "view-operations/RenderedGeometryFactory.h"

bool
GPlatesAppLogic::PaleomagUtils::detect_paleomag_features(
	GPlatesModel::FeatureCollectionHandle::weak_ref feature_collection)
{

	if (!feature_collection.is_valid())
	{
		return false;
	}

	// Visitor to detect paleomag features in the feature collection.
	PaleomagUtils::DetectPaleomagFeatures detect_paleomag_features;

	GPlatesAppLogic::AppLogicUtils::visit_feature_collection(
		feature_collection, detect_paleomag_features);

	return detect_paleomag_features.has_paleomag_features();

}	


void
GPlatesAppLogic::PaleomagUtils::VgpRenderer::visit_gml_point(
	GPlatesPropertyValues::GmlPoint &gml_point)
{
	static const GPlatesModel::PropertyName site_name = 
		GPlatesModel::PropertyName::create_gpml("averageSampleSitePosition");
		
	static const GPlatesModel::PropertyName vgp_name =
		GPlatesModel::PropertyName::create_gpml("polePosition");
		
	if (current_top_level_propname() == site_name)
	{
		d_site_point.reset(*gml_point.point());
	}
	else
	if (current_top_level_propname() == vgp_name)
	{
		d_vgp_point.reset(*gml_point.point());
	}
}

void
GPlatesAppLogic::PaleomagUtils::VgpRenderer::visit_xs_double(
	GPlatesPropertyValues::XsDouble &xs_double)
{
	static const GPlatesModel::PropertyName a95_name = 
		GPlatesModel::PropertyName::create_gpml("poleA95");
		
	static const GPlatesModel::PropertyName dm_name = 
		GPlatesModel::PropertyName::create_gpml("poleDm");
		
	static const GPlatesModel::PropertyName dp_name = 
		GPlatesModel::PropertyName::create_gpml("poleDp");		
	
	if (current_top_level_propname() == a95_name)
	{
		d_a95.reset(xs_double.value());
	}
	else
	if (current_top_level_propname() == dm_name)
	{
		d_dm.reset(xs_double.value());
	}
	else if (current_top_level_propname() == dp_name)
	{
		d_dp.reset(xs_double.value());
	}
}

void
GPlatesAppLogic::PaleomagUtils::VgpRenderer::visit_gpml_plate_id(
	GPlatesPropertyValues::GpmlPlateId &gpml_plate_id)
{
	static const GPlatesModel::PropertyName plate_id_name = 
		GPlatesModel::PropertyName::create_gpml("reconstructionPlateId");
		
	if (current_top_level_propname() == plate_id_name)
	{
		d_plate_id.reset(gpml_plate_id.value());
	}
}


void
GPlatesAppLogic::PaleomagUtils::VgpRenderer::visit_gpml_constant_value(
	GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
{
	gpml_constant_value.value()->accept_visitor(*this);
}

void
GPlatesAppLogic::PaleomagUtils::VgpRenderer::visit_gml_time_period(
	GPlatesPropertyValues::GmlTimePeriod &gml_time_period)
{
	static const GPlatesModel::PropertyName valid_time_name = 
		GPlatesModel::PropertyName::create_gml("validTime");

	if (current_top_level_propname() == valid_time_name)
	{
		d_begin_time.reset(gml_time_period.begin()->time_position());
		d_end_time.reset(gml_time_period.end()->time_position());
	}	
}



void
GPlatesAppLogic::PaleomagUtils::VgpRenderer::finalise_post_feature_properties(
	GPlatesModel::FeatureHandle &feature_handle)
{

	if (!d_vgp_point || !d_site_point)
	{	
		return;
	}

	if (d_reconstruction_time)
	{
		GPlatesPropertyValues::GeoTimeInstant time = 
			GPlatesPropertyValues::GeoTimeInstant(*d_reconstruction_time);

		// If we're outside the valid time of the feature, don't draw the error circle/ellipse.	
		if (d_begin_time && time.is_strictly_earlier_than(*d_begin_time))
		{
			return;
		}
		if (d_end_time && time.is_strictly_later_than(*d_end_time))
		{
			return;
		}
	}


	if (d_plate_id)
	{
		// We have a plate id, so rotate our vgp and site as appropriate.
		GPlatesModel::ReconstructionTree &reconstruction_tree 
			= d_reconstruction.reconstruction_tree();

		GPlatesMaths::FiniteRotation rotation = 
			reconstruction_tree.get_composed_absolute_rotation(*d_plate_id).first;

		if (d_additional_rotation)
		{
			rotation = GPlatesMaths::compose(*d_additional_rotation,rotation);
		}

		d_vgp_point.reset(rotation*(*d_vgp_point));	
		d_site_point.reset(rotation*(*d_site_point));
	}



	if (!d_draw_error_as_ellipse && d_a95)
	{

		GPlatesViewOperations::RenderedGeometry rendered_small_circle = 
			GPlatesViewOperations::create_rendered_small_circle(
			*d_vgp_point,
			GPlatesMaths::degreesToRadians(*d_a95),
			d_colour);

		d_target_layer->add_rendered_geometry(rendered_small_circle);		
	}
	else if (d_draw_error_as_ellipse && d_dm && d_dp )
	{
		GPlatesMaths::GreatCircle great_circle(*d_site_point,*d_vgp_point);
				
		GPlatesViewOperations::RenderedGeometry rendered_ellipse = 
			GPlatesViewOperations::create_rendered_ellipse(
			*d_vgp_point,
			GPlatesMaths::degreesToRadians(*d_dp),
			GPlatesMaths::degreesToRadians(*d_dm),
			great_circle,
			d_colour);

		d_target_layer->add_rendered_geometry(rendered_ellipse);			
	}


}


