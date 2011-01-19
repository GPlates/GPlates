/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007, 2008, 2009, 2010, 2011 The University of Sydney, Australia
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

#include <boost/none.hpp>  // boost::none

#include "ReconstructedFeatureGeometryPopulator.h"

#include "FlowlineUtils.h"
#include "Reconstruction.h"
#include "ReconstructionGeometryCollection.h"
#include "ReconstructionGeometryUtils.h"
#include "ReconstructionTree.h"
#include "ReconstructUtils.h"

#include "model/FeatureHandle.h"
#include "model/TopLevelPropertyInline.h"

#include "property-values/GmlLineString.h"
#include "property-values/GmlMultiPoint.h"
#include "property-values/GmlOrientableCurve.h"
#include "property-values/GmlPoint.h"
#include "property-values/GmlPolygon.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlPlateId.h"

#include "maths/MultiPointOnSphere.h"
#include "maths/PointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolylineOnSphere.h"


namespace
{
	/**
	 * Used to determine if @a ReconstructedFeatureGeometryPopulator can reconstruct a feature.
	 */
	class CanReconstructFeature :
			public GPlatesModel::ConstFeatureVisitor
	{
	public:
		CanReconstructFeature() :
			d_can_reconstruct(false),
			d_has_geometry(false),
			d_has_reconstruction_plate_id(false)
		{  }

		//! Returns true any features visited by us can be reconstructed.
		bool
		can_reconstruct()
		{
			return d_can_reconstruct;
		}

	private:
		virtual
		bool
		initialise_pre_feature_properties(
				const GPlatesModel::FeatureHandle &feature_handle)
		{
			d_has_geometry = false;
			d_has_reconstruction_plate_id = false;

			return true;
		}

		virtual
		void
		finalise_post_feature_properties(
				const GPlatesModel::FeatureHandle &feature_handle)
		{
			if (d_has_reconstruction_plate_id && d_has_geometry)
			{
				d_can_reconstruct = true;
			}
		}

		virtual
		void
		visit_gml_line_string(
				const GPlatesPropertyValues::GmlLineString &gml_line_string)
		{
			d_has_geometry = true;
		}

		virtual
		void
		visit_gml_multi_point(
				const GPlatesPropertyValues::GmlMultiPoint &gml_multi_point)
		{
			d_has_geometry = true;
		}

		virtual
		void
		visit_gml_orientable_curve(
				const GPlatesPropertyValues::GmlOrientableCurve &gml_orientable_curve)
		{
			d_has_geometry = true;
		}

		virtual
		void
		visit_gml_point(
				const GPlatesPropertyValues::GmlPoint &gml_point)
		{
			d_has_geometry = true;
		}
		
		virtual
		void
		visit_gml_polygon(
				const GPlatesPropertyValues::GmlPolygon &gml_polygon)
		{
			d_has_geometry = true;
		}

		virtual
		void
		visit_gpml_constant_value(
				const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
		{
			gpml_constant_value.value()->accept_visitor(*this);
		}

		virtual
		void
		visit_gpml_plate_id(
				const GPlatesPropertyValues::GpmlPlateId &gpml_plate_id)
		{
			static GPlatesModel::PropertyName reconstruction_plate_id_property_name =
					GPlatesModel::PropertyName::create_gpml("reconstructionPlateId");

			// Note that we're going to assume that we're in a property...
			if (current_top_level_propname() == reconstruction_plate_id_property_name)
			{
				d_has_reconstruction_plate_id = true;
			}
		}


		bool d_can_reconstruct;

		bool d_has_geometry;
		bool d_has_reconstruction_plate_id;
	};
}


bool
GPlatesAppLogic::ReconstructedFeatureGeometryPopulator::can_process(
		const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref)
{
	CanReconstructFeature can_reconstruct_visitor;

	can_reconstruct_visitor.visit_feature(feature_ref);

	return can_reconstruct_visitor.can_reconstruct();
}


GPlatesAppLogic::ReconstructedFeatureGeometryPopulator::ReconstructedFeatureGeometryPopulator(
		ReconstructionGeometryCollection &reconstruction_geometry_collection,
		const ReconstructLayerTaskParams &reconstruct_params,
		bool should_keep_features_without_recon_plate_id):
	d_reconstruction_geometry_collection(reconstruction_geometry_collection),
	d_reconstruction_tree(reconstruction_geometry_collection.reconstruction_tree()),
	d_recon_time(
			GPlatesPropertyValues::GeoTimeInstant(
					reconstruction_geometry_collection.get_reconstruction_time())),
	d_reconstruction_params(reconstruction_geometry_collection.get_reconstruction_time()),
	d_should_keep_features_without_recon_plate_id(should_keep_features_without_recon_plate_id),
	d_reconstruct_params(reconstruct_params)
{  }


bool
GPlatesAppLogic::ReconstructedFeatureGeometryPopulator::initialise_pre_feature_properties(
		GPlatesModel::FeatureHandle &feature_handle)
{
	//
	// Firstly find a reconstruction plate ID and determine whether the feature is defined
	// at this reconstruction time.
	//

	d_reconstruction_params.visit_feature(feature_handle.reference());


	//
	// Secondly perform the reconstructions (if appropriate) using the plate ID.
	//

	// So now we've visited the properties of this feature.  Let's find out if we were able
	// to obtain all the information we need.
	if ( ! d_reconstruction_params.is_feature_defined_at_recon_time())
	{
		// Quick-out: No need to continue.
		return false;
	}

	if ( ! d_reconstruction_params.get_recon_plate_id())
	{
		// We couldn't obtain the reconstruction plate ID.

		// So, how do we react to this situation?  Do we ignore features which don't have a
		// reconsruction plate ID, or do we "reconstruct" their geometries using the
		// identity rotation, so the features simply sit still on the globe?  Fortunately,
		// the client code has already told us how it wants us to behave...
		if ( ! d_should_keep_features_without_recon_plate_id)
		{
			return false;
		}
		// Otherwise, the code later will "reconstruct" with the identity rotation.
	}
	else
	{
		// We obtained the reconstruction plate ID.  We now have all the information we
		// need to reconstruct according to the reconstruction plate ID.
		d_recon_rotation = d_reconstruction_tree->get_composed_absolute_rotation(
				*d_reconstruction_params.get_recon_plate_id()).first;
	}

	//detect VGP feature and set the flag.
	ReconstructionGeometryUtils::DetectPaleomagFeatures vgp_detector;
	vgp_detector.visit_feature_handle(feature_handle);
	if(vgp_detector.has_paleomag_features())
	{
		d_is_vgp_feature = true;
		d_VGP_params = ReconstructedVirtualGeomagneticPoleParams();
	}
	else
	{
		d_is_vgp_feature = false;
	}

	// Detect Flowline features.
	FlowlineUtils::DetectFlowlineFeatures flowlines_detector;
	flowlines_detector.visit_feature_handle(feature_handle);
	if(flowlines_detector.has_flowline_features())
	{
	    d_is_flowline_feature = true;
	}
	else
	{
	    d_is_flowline_feature = false;
	}

	// Now visit the feature to reconstruct any geometries we find.
	return true;
}


void
GPlatesAppLogic::ReconstructedFeatureGeometryPopulator::visit_gml_line_string(
		GPlatesPropertyValues::GmlLineString &gml_line_string)
{
	using namespace GPlatesMaths;

	GPlatesModel::FeatureHandle::iterator property = *current_top_level_propiter();

	PolylineOnSphere::non_null_ptr_to_const_type reconstructed_polyline =
				gml_line_string.polyline();
	boost::optional<GPlatesModel::integer_plate_id_type> plate_id = d_reconstruction_params.get_recon_plate_id();

	if (d_reconstruction_params.get_reconstruction_method() == GPlatesAppLogic::ReconstructionMethod::HALF_STAGE_ROTATION)
	{
		boost::optional<FiniteRotation> rot;
		if (rot = get_half_stage_rotation())
		{
			reconstructed_polyline = (*rot) * reconstructed_polyline;
		}
	} 
	else if (plate_id) 
	{
		reconstructed_polyline = (*d_recon_rotation) * reconstructed_polyline;
	}
		
	ReconstructedFeatureGeometry::non_null_ptr_type rfg_ptr =
			ReconstructedFeatureGeometry::create(
					d_reconstruction_tree,
					reconstructed_polyline,
					*(current_top_level_propiter()->handle_weak_ref()),
					*(current_top_level_propiter()),
					plate_id,
					d_reconstruction_params.get_time_of_appearance());
	d_reconstruction_geometry_collection.add_reconstruction_geometry(rfg_ptr);
}


void
GPlatesAppLogic::ReconstructedFeatureGeometryPopulator::visit_gml_multi_point(
		GPlatesPropertyValues::GmlMultiPoint &gml_multi_point)
{

	if (d_is_flowline_feature)
	{
	    return;
	}
	using namespace GPlatesMaths;
	GPlatesModel::FeatureHandle::iterator property = *(current_top_level_propiter());
	boost::optional<GPlatesModel::integer_plate_id_type> plate_id = 
			d_reconstruction_params.get_recon_plate_id();
	MultiPointOnSphere::non_null_ptr_to_const_type reconstructed_multipoint =
			gml_multi_point.multipoint();

	if (d_reconstruction_params.get_reconstruction_method() == GPlatesAppLogic::ReconstructionMethod::HALF_STAGE_ROTATION)
	{
		boost::optional<FiniteRotation> rot;
		if (rot = get_half_stage_rotation())
		{
			reconstructed_multipoint = (*rot) * reconstructed_multipoint;
		}
	}
	else if (plate_id)
	{
		const FiniteRotation &r = *d_recon_rotation;
		reconstructed_multipoint =
				r * gml_multi_point.multipoint();
	}

	ReconstructedFeatureGeometry::non_null_ptr_type rfg_ptr =
			ReconstructedFeatureGeometry::create(
					d_reconstruction_tree,
					reconstructed_multipoint,
					*(current_top_level_propiter()->handle_weak_ref()),
					*(current_top_level_propiter()),
					plate_id,
					d_reconstruction_params.get_time_of_appearance());
	d_reconstruction_geometry_collection.add_reconstruction_geometry(rfg_ptr);
}


void
GPlatesAppLogic::ReconstructedFeatureGeometryPopulator::visit_gml_orientable_curve(
		GPlatesPropertyValues::GmlOrientableCurve &gml_orientable_curve)
{
	gml_orientable_curve.base_curve()->accept_visitor(*this);
}


void
GPlatesAppLogic::ReconstructedFeatureGeometryPopulator::visit_gml_point(
		GPlatesPropertyValues::GmlPoint &gml_point)
{
	if (d_is_vgp_feature)
	{
		handle_vgp_gml_point(gml_point);
		return;
	}

	if (d_is_flowline_feature)
	{
	    return;
	}
	using namespace GPlatesMaths;

	GPlatesModel::FeatureHandle::iterator property = *(current_top_level_propiter());
	boost::optional<GPlatesModel::integer_plate_id_type> plate_id = 
			d_reconstruction_params.get_recon_plate_id();
	PointOnSphere::non_null_ptr_to_const_type reconstructed_point =
			gml_point.point();

	if (d_reconstruction_params.get_reconstruction_method() == GPlatesAppLogic::ReconstructionMethod::HALF_STAGE_ROTATION)
	{
		boost::optional<FiniteRotation> rot;
		if (rot = get_half_stage_rotation())
		{
			reconstructed_point = (*rot) * reconstructed_point;
		}
	}
	else if (plate_id)
	{
		const FiniteRotation &r = *d_recon_rotation;
		reconstructed_point = r * gml_point.point();
	}
	ReconstructedFeatureGeometry::non_null_ptr_type rfg_ptr =
			ReconstructedFeatureGeometry::create(
					d_reconstruction_tree,
					reconstructed_point,
					*(current_top_level_propiter()->handle_weak_ref()),
					*(current_top_level_propiter()),
					plate_id,
					d_reconstruction_params.get_time_of_appearance());
	d_reconstruction_geometry_collection.add_reconstruction_geometry(rfg_ptr);
}


void
GPlatesAppLogic::ReconstructedFeatureGeometryPopulator::visit_gml_polygon(
		GPlatesPropertyValues::GmlPolygon &gml_polygon)
{
	using namespace GPlatesMaths;

	GPlatesModel::FeatureHandle::iterator property = *(current_top_level_propiter());
	boost::optional<GPlatesModel::integer_plate_id_type> plate_id = 
			d_reconstruction_params.get_recon_plate_id();
	PolygonOnSphere::non_null_ptr_to_const_type reconstructed_exterior =
			gml_polygon.exterior();

	boost::optional<FiniteRotation> rot;
	if (d_reconstruction_params.get_reconstruction_method() == GPlatesAppLogic::ReconstructionMethod::HALF_STAGE_ROTATION)
	{
		if (rot = get_half_stage_rotation())
		{
			reconstructed_exterior = (*rot) * reconstructed_exterior;
			
		}
	}
	else if (plate_id)
	{
		// Reconstruct the exterior PolygonOnSphere,
		// then add it to the d_reconstruction_geometries_to_populate vector.
		rot = d_recon_rotation;
		reconstructed_exterior = (*rot) * gml_polygon.exterior();
	}

	ReconstructedFeatureGeometry::non_null_ptr_type rfg_ptr =
			ReconstructedFeatureGeometry::create(
					d_reconstruction_tree,
					reconstructed_exterior,
					*(current_top_level_propiter()->handle_weak_ref()),
					*(current_top_level_propiter()),
					plate_id,
					d_reconstruction_params.get_time_of_appearance());
	d_reconstruction_geometry_collection.add_reconstruction_geometry(rfg_ptr);
		
	// Repeat the same procedure for each of the interior rings, if any.
	GPlatesPropertyValues::GmlPolygon::ring_const_iterator it = gml_polygon.interiors_begin();
	GPlatesPropertyValues::GmlPolygon::ring_const_iterator end = gml_polygon.interiors_end();
	for ( ; it != end; ++it) 
	{
		PolygonOnSphere::non_null_ptr_to_const_type reconstructed_interior = *it;
		if(rot)
		{
			reconstructed_interior = (*rot) * (*it);
		}
		ReconstructedFeatureGeometry::non_null_ptr_type interior_rfg_ptr =
				ReconstructedFeatureGeometry::create(
						d_reconstruction_tree,
						reconstructed_interior,
						*(current_top_level_propiter()->handle_weak_ref()),
						*(current_top_level_propiter()),
						plate_id,
						d_reconstruction_params.get_time_of_appearance());
		d_reconstruction_geometry_collection.add_reconstruction_geometry(interior_rfg_ptr);
	}
}


void
GPlatesAppLogic::ReconstructedFeatureGeometryPopulator::visit_gpml_constant_value(
		GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
{
	gpml_constant_value.value()->accept_visitor(*this);
}

void
GPlatesAppLogic::ReconstructedFeatureGeometryPopulator::handle_vgp_gml_point(
		GPlatesPropertyValues::GmlPoint &gml_point)
{
	static const GPlatesModel::PropertyName site_name = 
		GPlatesModel::PropertyName::create_gpml("averageSampleSitePosition");
		
	static const GPlatesModel::PropertyName vgp_name =
		GPlatesModel::PropertyName::create_gpml("polePosition");
	
	if(!d_VGP_params)	
	{
		d_VGP_params = ReconstructedVirtualGeomagneticPoleParams();
	}

	boost::optional<GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type> reconstructed_point;

	if (d_reconstruction_params.get_recon_plate_id()) 
	{
		using namespace GPlatesMaths;
		const FiniteRotation &r = *d_recon_rotation;
		reconstructed_point = r * gml_point.point();
	}
	else
	{
		reconstructed_point = gml_point.point();
	}

	if (current_top_level_propname() == site_name)
	{
		d_VGP_params->d_site_point.reset(*reconstructed_point);
		d_VGP_params->d_site_iterator.reset(*current_top_level_propiter());
	}
	else if (current_top_level_propname() == vgp_name)
	{
		d_VGP_params->d_vgp_point.reset(*reconstructed_point);
		d_VGP_params->d_vgp_iterator.reset(*current_top_level_propiter());
	}
}

void
GPlatesAppLogic::ReconstructedFeatureGeometryPopulator::visit_xs_double(
		GPlatesPropertyValues::XsDouble &xs_double)
{
	if(!d_is_vgp_feature)
	{
		return;
	}

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
		d_VGP_params->d_a95.reset(xs_double.value());
	}
	else if (current_top_level_propname() == dm_name)
	{
		d_VGP_params->d_dm.reset(xs_double.value());
	}
	else if (current_top_level_propname() == dp_name)
	{
		d_VGP_params->d_dp.reset(xs_double.value());
	}
	else if (current_top_level_propname() == age_name)
	{
		d_VGP_params->d_age.reset(xs_double.value());
	}
}


void
GPlatesAppLogic::ReconstructedFeatureGeometryPopulator::finalise_post_feature_properties(
		GPlatesModel::FeatureHandle &feature_handle)
{
	if(!d_is_vgp_feature)
	{
		return;
	}

	if(!d_reconstruct_params.should_draw_vgp(d_recon_time.value(),*d_VGP_params->d_age))
	{
		return;
	}

	if(d_VGP_params->d_vgp_point)
	{
		ReconstructedFeatureGeometry::non_null_ptr_type rfg_ptr =
			ReconstructedVirtualGeomagneticPole::create(
					*d_VGP_params,
					d_reconstruction_tree,
					(*d_VGP_params->d_vgp_point),
					*(*d_VGP_params->d_vgp_iterator).handle_weak_ref(),
					(*d_VGP_params->d_vgp_iterator),
					d_reconstruction_params.get_recon_plate_id(),
					d_reconstruction_params.get_time_of_appearance());
		d_reconstruction_geometry_collection.add_reconstruction_geometry(rfg_ptr);
	}

	if(d_VGP_params->d_site_point)
	{
		ReconstructedFeatureGeometry::non_null_ptr_type rfg_ptr =
				ReconstructedFeatureGeometry::create(
						d_reconstruction_tree,
						(*d_VGP_params->d_site_point),
						*(*d_VGP_params->d_site_iterator).handle_weak_ref(),
						(*d_VGP_params->d_site_iterator),
						d_reconstruction_params.get_recon_plate_id(),
						d_reconstruction_params.get_time_of_appearance());
		d_reconstruction_geometry_collection.add_reconstruction_geometry(rfg_ptr);
	}
}

boost::optional<GPlatesMaths::FiniteRotation>
GPlatesAppLogic::ReconstructedFeatureGeometryPopulator::get_half_stage_rotation()
{


        return ReconstructUtils::get_half_stage_rotation(
                    *d_reconstruction_tree,
                    *d_reconstruction_params.get_left_plate_id(),
                    *d_reconstruction_params.get_right_plate_id());


}



