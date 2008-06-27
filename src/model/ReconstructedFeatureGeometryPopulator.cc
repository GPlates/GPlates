/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007, 2008 The University of Sydney, Australia
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
#include "Reconstruction.h"
#include "ReconstructionTree.h"
#include "FeatureHandle.h"
#include "InlinePropertyContainer.h"

#include "property-values/GmlLineString.h"
#include "property-values/GmlPoint.h"
#include "property-values/GmlOrientableCurve.h"
#include "property-values/GmlPoint.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlPlateId.h"

#include "maths/PointOnSphere.h"
#include "maths/PolylineOnSphere.h"


GPlatesModel::ReconstructedFeatureGeometryPopulator::ReconstructedFeatureGeometryPopulator(
		const double &recon_time,
		unsigned long root_plate_id,
		Reconstruction &recon,
		ReconstructionTree &recon_tree,
		reconstructed_geometries_type &reconstructed_geometries,
		bool should_keep_features_without_recon_plate_id):
	d_recon_time(GPlatesPropertyValues::GeoTimeInstant(recon_time)),
	d_root_plate_id(GPlatesModel::integer_plate_id_type(root_plate_id)),
	d_recon_ptr(&recon),
	d_recon_tree_ptr(&recon_tree),
	d_reconstructed_geometries_to_populate(&reconstructed_geometries),
	d_should_keep_features_without_recon_plate_id(should_keep_features_without_recon_plate_id)
{  }


void
GPlatesModel::ReconstructedFeatureGeometryPopulator::visit_feature_handle(
		FeatureHandle &feature_handle)
{
	d_accumulator = ReconstructedFeatureGeometryAccumulator();

	// Now visit each of the properties in turn, twice -- firstly, to find a reconstruction
	// plate ID and to determine whether the feature is defined at this reconstruction time;
	// after that, to perform the reconstructions (if appropriate) using the plate ID.

	// The first time through, we're not reconstructing, just gathering information.
	d_accumulator->d_perform_reconstructions = false;
	visit_feature_properties(feature_handle);

	// So now we've visited the properties of this feature.  Let's find out if we were able
	// to obtain all the information we need.
	if ( ! d_accumulator->d_feature_is_defined_at_recon_time) {
		// Quick-out: No need to continue.
		d_accumulator = boost::none;
		return;
	}
	if ( ! d_accumulator->d_recon_plate_id) {
		// We couldn't obtain the reconstruction plate ID.

		// So, how do we react to this situation?  Do we ignore features which don't have a
		// reconsruction plate ID, or do we "reconstruct" their geometries using the
		// identity rotation, so the features simply sit still on the globe?  Fortunately,
		// the client code has already told us how it wants us to behave...
		if ( ! d_should_keep_features_without_recon_plate_id) {
			d_accumulator = boost::none;
			return;
		}
		// Otherwise, the code later will "reconstruct" with the identity rotation.
	} else {
		// We obtained the reconstruction plate ID.  We now have all the information we
		// need to reconstruct according to the reconstruction plate ID.
		d_accumulator->d_recon_rotation =
				d_recon_tree_ptr->get_composed_absolute_rotation(*(d_accumulator->d_recon_plate_id)).first;
	}

	// Now for the second pass through the properties of the feature:  This time we reconstruct
	// any geometries we find.
	d_accumulator->d_perform_reconstructions = true;
	visit_feature_properties(feature_handle);

	d_accumulator = boost::none;
}


void
GPlatesModel::ReconstructedFeatureGeometryPopulator::visit_feature_properties(
		FeatureHandle &feature_handle)
{
	FeatureHandle::properties_iterator iter = feature_handle.properties_begin();
	FeatureHandle::properties_iterator end = feature_handle.properties_end();
	for ( ; iter != end; ++iter) {
		// Elements of this properties vector can be NULL pointers.  (See the comment in
		// "model/FeatureRevision.h" for more details.)
		if (*iter != NULL) {
			// FIXME: This d_current_property thing could go in the {Const,}FeatureVisitor base.
			d_accumulator->d_current_property = iter;
			(*iter)->accept_visitor(*this);
		}
	}
}


void
GPlatesModel::ReconstructedFeatureGeometryPopulator::visit_inline_property_container(
		InlinePropertyContainer &inline_property_container)
{
	visit_property_values(inline_property_container);
}


void
GPlatesModel::ReconstructedFeatureGeometryPopulator::visit_gml_line_string(
		GPlatesPropertyValues::GmlLineString &gml_line_string)
{
	using namespace GPlatesMaths;

	if (d_accumulator->d_perform_reconstructions) {
		FeatureHandle::properties_iterator property = *(d_accumulator->d_current_property);

		if (d_accumulator->d_recon_plate_id) {
			const FiniteRotation &r = *d_accumulator->d_recon_rotation;
			PolylineOnSphere::non_null_ptr_to_const_type reconstructed_polyline =
					r * gml_line_string.polyline();

			ReconstructedFeatureGeometry rfg(reconstructed_polyline,
					*d_accumulator->d_current_property->collection_handle_ptr(),
					*d_accumulator->d_current_property,
					*d_accumulator->d_recon_plate_id);
			d_reconstructed_geometries_to_populate->push_back(rfg);
			d_reconstructed_geometries_to_populate->back().set_reconstruction_ptr(d_recon_ptr);
		} else {
			// We must be reconstructing using the identity rotation.
			ReconstructedFeatureGeometry rfg(gml_line_string.polyline(),
					*d_accumulator->d_current_property->collection_handle_ptr(),
					*d_accumulator->d_current_property);
			d_reconstructed_geometries_to_populate->push_back(rfg);
			d_reconstructed_geometries_to_populate->back().set_reconstruction_ptr(d_recon_ptr);
		}
	}
}


void
GPlatesModel::ReconstructedFeatureGeometryPopulator::visit_gml_orientable_curve(
		GPlatesPropertyValues::GmlOrientableCurve &gml_orientable_curve)
{
	gml_orientable_curve.base_curve()->accept_visitor(*this);
}


void
GPlatesModel::ReconstructedFeatureGeometryPopulator::visit_gml_point(
		GPlatesPropertyValues::GmlPoint &gml_point)
{
	using namespace GPlatesMaths;

	if (d_accumulator->d_perform_reconstructions) {
		FeatureHandle::properties_iterator property = *(d_accumulator->d_current_property);

		if (d_accumulator->d_recon_plate_id) {
			const FiniteRotation &r = *d_accumulator->d_recon_rotation;
			PointOnSphere::non_null_ptr_to_const_type reconstructed_point =
					r * gml_point.point();

			ReconstructedFeatureGeometry rfg(reconstructed_point,
					*d_accumulator->d_current_property->collection_handle_ptr(),
					*d_accumulator->d_current_property,
					*d_accumulator->d_recon_plate_id);
			d_reconstructed_geometries_to_populate->push_back(rfg);
			d_reconstructed_geometries_to_populate->back().set_reconstruction_ptr(d_recon_ptr);
		} else {
			// We must be reconstructing using the identity rotation.
			ReconstructedFeatureGeometry rfg(gml_point.point(),
					*d_accumulator->d_current_property->collection_handle_ptr(),
					*d_accumulator->d_current_property);
			d_reconstructed_geometries_to_populate->push_back(rfg);
			d_reconstructed_geometries_to_populate->back().set_reconstruction_ptr(d_recon_ptr);
		}
	}
}


void
GPlatesModel::ReconstructedFeatureGeometryPopulator::visit_gml_time_period(
		GPlatesPropertyValues::GmlTimePeriod &gml_time_period)
{
	static const PropertyName valid_time_property_name =
		PropertyName::create_gml("validTime");

	if ( ! d_accumulator->d_perform_reconstructions) {
		// We're gathering information, not performing reconstructions.

		// Note that we're going to assume that we're in a property...
		if (d_accumulator->current_property_name() == valid_time_property_name) {
			// This time period is the "valid time" time period.
			if ( ! gml_time_period.contains(d_recon_time)) {
				// Oh no!  This feature instance is not defined at the recon time!
				d_accumulator->d_feature_is_defined_at_recon_time = false;
			}
		}
	}
}


void
GPlatesModel::ReconstructedFeatureGeometryPopulator::visit_gpml_constant_value(
		GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
{
	gpml_constant_value.value()->accept_visitor(*this);
}


void
GPlatesModel::ReconstructedFeatureGeometryPopulator::visit_gpml_plate_id(
		GPlatesPropertyValues::GpmlPlateId &gpml_plate_id)
{
	static PropertyName reconstruction_plate_id_property_name =
		PropertyName::create_gpml("reconstructionPlateId");

	if ( ! d_accumulator->d_perform_reconstructions) {
		// We're gathering information, not performing reconstructions.

		// Note that we're going to assume that we're in a property...
		if (d_accumulator->current_property_name() == reconstruction_plate_id_property_name) {
			// This plate ID is the reconstruction plate ID.
			d_accumulator->d_recon_plate_id = gpml_plate_id.value();
		}
	}
}
