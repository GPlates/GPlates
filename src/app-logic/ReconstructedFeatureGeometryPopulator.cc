/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007, 2008, 2009 The University of Sydney, Australia
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

#include "ReconstructionGeometryUtils.h"

#include "model/Reconstruction.h"
#include "model/ReconstructionTree.h"
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


GPlatesAppLogic::ReconstructedFeatureGeometryPopulator::ReconstructedFeatureGeometryPopulator(
		const double &recon_time,
		unsigned long root_plate_id,
		GPlatesModel::Reconstruction &recon,
		GPlatesModel::ReconstructionTree &recon_tree,
		bool should_keep_features_without_recon_plate_id):
	d_recon_time(GPlatesPropertyValues::GeoTimeInstant(recon_time)),
	d_root_plate_id(GPlatesModel::integer_plate_id_type(root_plate_id)),
	d_reconstruction(recon),
	d_reconstruction_tree(recon_tree),
	d_reconstruction_params(recon_time),
	d_should_keep_features_without_recon_plate_id(should_keep_features_without_recon_plate_id)
{  }


void
GPlatesAppLogic::ReconstructedFeatureGeometryPopulator::visit_feature_handle(
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
		return;
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
			return;
		}
		// Otherwise, the code later will "reconstruct" with the identity rotation.
	}
	else
	{
		// We obtained the reconstruction plate ID.  We now have all the information we
		// need to reconstruct according to the reconstruction plate ID.
		d_recon_rotation = d_reconstruction_tree.get_composed_absolute_rotation(
				*d_reconstruction_params.get_recon_plate_id()).first;
	}

	// Now visit the feature to reconstruct any geometries we find.
	visit_feature_properties(feature_handle);
}


void
GPlatesAppLogic::ReconstructedFeatureGeometryPopulator::visit_gml_line_string(
		GPlatesPropertyValues::GmlLineString &gml_line_string)
{
	using namespace GPlatesMaths;

	GPlatesModel::FeatureHandle::iterator property = *current_top_level_propiter();

	if (d_reconstruction_params.get_recon_plate_id()) {
		const FiniteRotation &r = *d_recon_rotation;
		PolylineOnSphere::non_null_ptr_to_const_type reconstructed_polyline =
				r * gml_line_string.polyline();

		GPlatesModel::ReconstructedFeatureGeometry::non_null_ptr_type rfg_ptr =
				GPlatesModel::ReconstructedFeatureGeometry::create(
						reconstructed_polyline,
						*(current_top_level_propiter()->handle_weak_ref()),
						*(current_top_level_propiter()),
						d_reconstruction_params.get_recon_plate_id(),
						d_reconstruction_params.get_time_of_appearance());
		ReconstructionGeometryUtils::add_reconstruction_geometry_to_reconstruction(
				rfg_ptr, d_reconstruction);
	} else {
		// We must be reconstructing using the identity rotation.
		GPlatesModel::ReconstructedFeatureGeometry::non_null_ptr_type rfg_ptr =
				GPlatesModel::ReconstructedFeatureGeometry::create(
						gml_line_string.polyline(),
						*(current_top_level_propiter()->handle_weak_ref()),
						*(current_top_level_propiter()),
						boost::none,
						d_reconstruction_params.get_time_of_appearance());
		ReconstructionGeometryUtils::add_reconstruction_geometry_to_reconstruction(
				rfg_ptr, d_reconstruction);
	}
}


void
GPlatesAppLogic::ReconstructedFeatureGeometryPopulator::visit_gml_multi_point(
		GPlatesPropertyValues::GmlMultiPoint &gml_multi_point)
{
	using namespace GPlatesMaths;

	GPlatesModel::FeatureHandle::iterator property = *(current_top_level_propiter());

	if (d_reconstruction_params.get_recon_plate_id()) {
		const FiniteRotation &r = *d_recon_rotation;
		MultiPointOnSphere::non_null_ptr_to_const_type reconstructed_multipoint =
				r * gml_multi_point.multipoint();

		GPlatesModel::ReconstructedFeatureGeometry::non_null_ptr_type rfg_ptr =
				GPlatesModel::ReconstructedFeatureGeometry::create(
						reconstructed_multipoint,
						*(current_top_level_propiter()->handle_weak_ref()),
						*(current_top_level_propiter()),
						d_reconstruction_params.get_recon_plate_id(),
						d_reconstruction_params.get_time_of_appearance());
		ReconstructionGeometryUtils::add_reconstruction_geometry_to_reconstruction(
				rfg_ptr, d_reconstruction);
	} else {
		// We must be reconstructing using the identity rotation.
		GPlatesModel::ReconstructedFeatureGeometry::non_null_ptr_type rfg_ptr =
				GPlatesModel::ReconstructedFeatureGeometry::create(
						gml_multi_point.multipoint(),
						*(current_top_level_propiter()->handle_weak_ref()),
						*(current_top_level_propiter()),
						boost::none,
						d_reconstruction_params.get_time_of_appearance());
		ReconstructionGeometryUtils::add_reconstruction_geometry_to_reconstruction(
				rfg_ptr, d_reconstruction);
	}
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
	using namespace GPlatesMaths;

	GPlatesModel::FeatureHandle::iterator property = *(current_top_level_propiter());

	if (d_reconstruction_params.get_recon_plate_id()) {
		const FiniteRotation &r = *d_recon_rotation;
		PointOnSphere::non_null_ptr_to_const_type reconstructed_point =
				r * gml_point.point();

		GPlatesModel::ReconstructedFeatureGeometry::non_null_ptr_type rfg_ptr =
				GPlatesModel::ReconstructedFeatureGeometry::create(
						reconstructed_point,
						*(current_top_level_propiter()->handle_weak_ref()),
						*(current_top_level_propiter()),
						d_reconstruction_params.get_recon_plate_id(),
						d_reconstruction_params.get_time_of_appearance());
		ReconstructionGeometryUtils::add_reconstruction_geometry_to_reconstruction(
				rfg_ptr, d_reconstruction);
	} else {
		// We must be reconstructing using the identity rotation.
		GPlatesModel::ReconstructedFeatureGeometry::non_null_ptr_type rfg_ptr =
				GPlatesModel::ReconstructedFeatureGeometry::create(
						gml_point.point(),
						*(current_top_level_propiter()->handle_weak_ref()),
						*(current_top_level_propiter()),
						boost::none,
						d_reconstruction_params.get_time_of_appearance());
		ReconstructionGeometryUtils::add_reconstruction_geometry_to_reconstruction(
				rfg_ptr, d_reconstruction);
	}
}


void
GPlatesAppLogic::ReconstructedFeatureGeometryPopulator::visit_gml_polygon(
		GPlatesPropertyValues::GmlPolygon &gml_polygon)
{
	using namespace GPlatesMaths;

	GPlatesModel::FeatureHandle::iterator property = *(current_top_level_propiter());

	if (d_reconstruction_params.get_recon_plate_id()) {
		// Reconstruct the exterior PolygonOnSphere,
		// then add it to the d_reconstruction_geometries_to_populate vector.
		const FiniteRotation &r = *d_recon_rotation;
		PolygonOnSphere::non_null_ptr_to_const_type reconstructed_exterior =
				r * gml_polygon.exterior();

		GPlatesModel::ReconstructedFeatureGeometry::non_null_ptr_type rfg_ptr =
				GPlatesModel::ReconstructedFeatureGeometry::create(
						reconstructed_exterior,
						*(current_top_level_propiter()->handle_weak_ref()),
						*(current_top_level_propiter()),
						d_reconstruction_params.get_recon_plate_id(),
						d_reconstruction_params.get_time_of_appearance());
		ReconstructionGeometryUtils::add_reconstruction_geometry_to_reconstruction(
				rfg_ptr, d_reconstruction);
		
		// Repeat the same procedure for each of the interior rings, if any.
		GPlatesPropertyValues::GmlPolygon::ring_const_iterator it = gml_polygon.interiors_begin();
		GPlatesPropertyValues::GmlPolygon::ring_const_iterator end = gml_polygon.interiors_end();
		for ( ; it != end; ++it) {
			PolygonOnSphere::non_null_ptr_to_const_type reconstructed_interior =
					r * (*it);

			GPlatesModel::ReconstructedFeatureGeometry::non_null_ptr_type interior_rfg_ptr =
					GPlatesModel::ReconstructedFeatureGeometry::create(
							reconstructed_interior,
							*(current_top_level_propiter()->handle_weak_ref()),
							*(current_top_level_propiter()),
							d_reconstruction_params.get_recon_plate_id(),
							d_reconstruction_params.get_time_of_appearance());
			ReconstructionGeometryUtils::add_reconstruction_geometry_to_reconstruction(
					interior_rfg_ptr, d_reconstruction);
		}
	} else {
		// We must be reconstructing using the identity rotation.
		// Add the exterior PolygonOnSphere to the vector directly.
		GPlatesModel::ReconstructedFeatureGeometry::non_null_ptr_type rfg_ptr =
				GPlatesModel::ReconstructedFeatureGeometry::create(
						gml_polygon.exterior(),
						*(current_top_level_propiter()->handle_weak_ref()),
						*(current_top_level_propiter()),
						boost::none,
						d_reconstruction_params.get_time_of_appearance());
		ReconstructionGeometryUtils::add_reconstruction_geometry_to_reconstruction(
				rfg_ptr, d_reconstruction);

		// Repeat the same procedure for each of the interior rings, if any.
		GPlatesPropertyValues::GmlPolygon::ring_const_iterator it = gml_polygon.interiors_begin();
		GPlatesPropertyValues::GmlPolygon::ring_const_iterator end = gml_polygon.interiors_end();
		for ( ; it != end; ++it) {
			GPlatesModel::ReconstructedFeatureGeometry::non_null_ptr_type interior_rfg_ptr =
					GPlatesModel::ReconstructedFeatureGeometry::create(
							*it,
							*(current_top_level_propiter()->handle_weak_ref()),
							*(current_top_level_propiter()),
							boost::none,
							d_reconstruction_params.get_time_of_appearance());
			ReconstructionGeometryUtils::add_reconstruction_geometry_to_reconstruction(
					interior_rfg_ptr, d_reconstruction);
		}
	}
}


void
GPlatesAppLogic::ReconstructedFeatureGeometryPopulator::visit_gpml_constant_value(
		GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
{
	gpml_constant_value.value()->accept_visitor(*this);
}
