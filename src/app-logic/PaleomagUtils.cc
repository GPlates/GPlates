/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009, 2010 Geological Survey of Norway
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
#include <QDebug>

#include "AppLogicUtils.h"
#include "PaleomagUtils.h"

#include "maths/PolylineOnSphere.h" // FIXME: for testing
#include "maths/MathsUtils.h"

#include "app-logic/Reconstruct.h"
#include "gui/ColourProxy.h"
#include "model/PropertyName.h"
#include "model/ReconstructedFeatureGeometry.h"
#include "model/Reconstruction.h"
#include "model/ReconstructionTree.h"
#include "presentation/ViewState.h"
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
		d_site_iterator.reset(*current_top_level_propiter());
	}
	else
	if (current_top_level_propname() == vgp_name)
	{
		d_vgp_point.reset(*gml_point.point());
		d_vgp_iterator.reset(*current_top_level_propiter());
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
		
	static const GPlatesModel::PropertyName age_name = 
		GPlatesModel::PropertyName::create_gpml("averageAge");				
	
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
	else if (current_top_level_propname() == age_name)
	{
		d_age.reset(xs_double.value());
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
// Visibility of the vgp features is now determined by the render settings, controlled
// via the user interface.
// So I don't think it makes sense to have a valid time property for these features
// now. 
// If such a valid time property exists in the feature, it won't (currently) have any effect on the 
// rendering. 
#if 0
	static const GPlatesModel::PropertyName valid_time_name = 
		GPlatesModel::PropertyName::create_gml("validTime");

	if (current_top_level_propname() == valid_time_name)
	{
		d_begin_time.reset(gml_time_period.begin()->time_position());
		d_end_time.reset(gml_time_period.end()->time_position());
	}	
#endif
}


void
GPlatesAppLogic::PaleomagUtils::VgpRenderer::finalise_post_feature_properties(
	GPlatesModel::FeatureHandle &feature_handle)
{

	if (!d_vgp_point)
	{	
		return;
	}

	bool should_draw_vgp = false;
	
	// Check the render settings and use them to decide if the vgp should be drawn for 
	// the current time.
	GPlatesPresentation::ViewState::VGPRenderSettings &vgp_render_settings =
		d_view_state_ptr->get_vgp_render_settings();
	
	GPlatesPresentation::ViewState::VGPRenderSettings::VGPVisibilitySetting visibility_setting =
		vgp_render_settings.get_vgp_visibility_setting();
		
	double time = d_view_state_ptr->get_reconstruct().get_current_reconstruction_time();
	GPlatesPropertyValues::GeoTimeInstant geo_time = 
							GPlatesPropertyValues::GeoTimeInstant(time);
	double delta_t = vgp_render_settings.get_vgp_delta_t();

	switch(visibility_setting)
	{
		case GPlatesPresentation::ViewState::VGPRenderSettings::ALWAYS_VISIBLE:
			should_draw_vgp = true;
			break;
		case GPlatesPresentation::ViewState::VGPRenderSettings::TIME_WINDOW:

			if ((geo_time.is_later_than_or_coincident_with(vgp_render_settings.get_vgp_earliest_time()))
				&& (geo_time.is_earlier_than_or_coincident_with(vgp_render_settings.get_vgp_latest_time())))
			{
				should_draw_vgp = true;
			}
			break;
		case GPlatesPresentation::ViewState::VGPRenderSettings::DELTA_T_AROUND_AGE:
			if (d_age)
			{
				GPlatesPropertyValues::GeoTimeInstant earliest_time =
					GPlatesPropertyValues::GeoTimeInstant(*d_age + delta_t);
				GPlatesPropertyValues::GeoTimeInstant latest_time =
					GPlatesPropertyValues::GeoTimeInstant(*d_age - delta_t);
				
				if ((geo_time.is_later_than_or_coincident_with(earliest_time)) &&
					(geo_time.is_earlier_than_or_coincident_with(latest_time)))
				{
					should_draw_vgp = true;
				}
			}
			break;			
			
	} // switch

	if (!should_draw_vgp)
	{
		return;
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
		if (d_site_point)
		{	
			d_site_point.reset(rotation*(*d_site_point));
		}
	}

	if (d_site_point)
	{
		GPlatesModel::ReconstructedFeatureGeometry::non_null_ptr_type rfg = 
			GPlatesModel::ReconstructedFeatureGeometry::create(
			d_site_point->get_non_null_pointer(),
			feature_handle,
			*d_site_iterator,
			d_plate_id,
			boost::none);		
	
	
		GPlatesViewOperations::RenderedGeometry rendered_geom =
		GPlatesViewOperations::RenderedGeometryFactory::create_rendered_point_on_sphere(
			*d_site_point,d_colour,SITE_POINT_SIZE);
			
		GPlatesViewOperations::RenderedGeometry queryable_rendered_geom =
			GPlatesViewOperations::RenderedGeometryFactory::create_rendered_reconstruction_geometry(
				rfg,rendered_geom);				
			
		d_target_layer->add_rendered_geometry(queryable_rendered_geom);
		
		if (d_should_add_to_reconstruction)
		{	// Add to the reconstruction geometries too, so that it gets highlighted.
			d_reconstruction.geometries().push_back(rfg);
			d_reconstruction.geometries().back()->set_reconstruction_ptr(&d_reconstruction);
		}
	}

	if (d_vgp_point)
	{
		GPlatesModel::ReconstructedFeatureGeometry::non_null_ptr_type rfg = 
			GPlatesModel::ReconstructedFeatureGeometry::create(
			d_vgp_point->get_non_null_pointer(),
			feature_handle,
			*d_vgp_iterator,
			d_plate_id,
			boost::none);		


		GPlatesViewOperations::RenderedGeometry rendered_geom =
			GPlatesViewOperations::RenderedGeometryFactory::create_rendered_point_on_sphere(
				*d_vgp_point,d_colour,POLE_POINT_SIZE);

		GPlatesViewOperations::RenderedGeometry queryable_rendered_geom =
			GPlatesViewOperations::RenderedGeometryFactory::create_rendered_reconstruction_geometry(
			rfg,rendered_geom);				

		d_target_layer->add_rendered_geometry(queryable_rendered_geom);

		if (d_should_add_to_reconstruction)
		{	// Add to the reconstruction geometries too, so that it gets highlighted.
			d_reconstruction.geometries().push_back(rfg);
			d_reconstruction.geometries().back()->set_reconstruction_ptr(&d_reconstruction);
		}

	}

	// Note that the circle/ellipse will not be added to the reconstruction geometry collection.
	if (vgp_render_settings.should_draw_circular_error() && d_a95)
	{
		GPlatesViewOperations::RenderedGeometry rendered_small_circle = 
			GPlatesViewOperations::create_rendered_small_circle(
			*d_vgp_point,
			GPlatesMaths::convert_deg_to_rad(*d_a95),
			d_colour);

		// The circle/ellipse geometries are not (currently) queryable, so we
		// just add the rendered geometry to the layer.

		d_target_layer->add_rendered_geometry(rendered_small_circle);	
	
	}
	// We can only draw an ellipse if we have dm and dp defined, and if we have
	// a site point. We need the site point so that we can align the ellipse axes
	// appropriately. 
	else if (!vgp_render_settings.should_draw_circular_error() && d_dm && d_dp && d_site_point )
	{
		GPlatesMaths::GreatCircle great_circle(*d_site_point,*d_vgp_point);
				
		GPlatesViewOperations::RenderedGeometry rendered_ellipse = 
			GPlatesViewOperations::create_rendered_ellipse(
			*d_vgp_point,
			GPlatesMaths::convert_deg_to_rad(*d_dp),
			GPlatesMaths::convert_deg_to_rad(*d_dm),
			great_circle,
			d_colour);

		// The circle/ellipse geometries are not (currently) queryable, so we
		// just add the rendered geometry to the layer.

		d_target_layer->add_rendered_geometry(rendered_ellipse);	

	}

}


