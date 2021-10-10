/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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

#include "GeometricPropertyValueConstructor.h"

#include "maths/FiniteRotation.h"
#include "maths/ConstGeometryOnSphereVisitor.h"
#include "property-values/GmlMultiPoint.h"
#include "property-values/GmlPoint.h"
#include "property-values/GmlPolygon.h"
#include "property-values/GmlLineString.h"
#include "property-values/TemplateTypeParameterType.h"
#include "property-values/GpmlConstantValue.h"


namespace
{
	/**
	 * Apply a reverse reconstruction to the given temporary geometry, so that the
	 * coordinates are set to present-day location given the supplied plate id and
	 * current reconstruction tree.
	 *
	 * @a G should be a non_null_ptr_to_const_type of some suitable GeometryOnSphere
	 * derivation, with an implementation of FiniteRotation::operator*() available.
	 */
	template <class G>
	G
	reverse_reconstruct(
			G geometry,
			const GPlatesModel::integer_plate_id_type plate_id,
			GPlatesModel::ReconstructionTree &recon_tree)
	{
		// Get the composed absolute rotation needed to bring a thing on that plate
		// in the present day to this time.
		const GPlatesMaths::FiniteRotation rotation =
				recon_tree.get_composed_absolute_rotation(plate_id).first;
		const GPlatesMaths::FiniteRotation reverse = GPlatesMaths::get_reverse(rotation);
		
		// Apply the reverse rotation.
		G present_day_geometry = reverse * geometry;
		
		return present_day_geometry;
	}
}


boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type>
GPlatesGui::GeometricPropertyValueConstructor::convert(
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry_ptr,
		boost::optional<GPlatesModel::ReconstructionTree &> reconstruction_tree_opt_ref,
		boost::optional<GPlatesModel::integer_plate_id_type> plate_id_opt,
		bool wrap_with_gpml_constant_value)
{
	// Set parameters for this visitation.
	if (reconstruction_tree_opt_ref) {
		d_recon_tree_ptr = &(*reconstruction_tree_opt_ref);
	} else {
		// Probably not the best way to do this but I'm in a hurry! -JC
		d_recon_tree_ptr = NULL;
	}
	d_plate_id_opt = plate_id_opt;
	d_wrap_with_gpml_constant_value = wrap_with_gpml_constant_value;

	// Clear any previous return value and do the visit.
	d_property_value_opt = boost::none;
	geometry_ptr->accept_visitor(*this);
	return d_property_value_opt;
}


void
GPlatesGui::GeometricPropertyValueConstructor::visit_multi_point_on_sphere(
		GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere)
{
	boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type> prop_val_opt;
	
	if (d_plate_id_opt && d_recon_tree_ptr != NULL) {
		// Reverse reconstruct the geometry to present-day.
		GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type present_day =
				reverse_reconstruct(multi_point_on_sphere, *d_plate_id_opt, *d_recon_tree_ptr);
		// Convert MultiPointOnSphere to GmlMultiPoint.
		prop_val_opt = GPlatesPropertyValues::GmlMultiPoint::create(present_day);
	} else {
		// No reverse-reconstruction, just make a PropertyValue.
		prop_val_opt = GPlatesPropertyValues::GmlMultiPoint::create(multi_point_on_sphere);
	}
	// At this point, we should have a valid PropertyValue.
	// Now - do we want to add a GpmlConstantValue wrapper around it?
	if (d_wrap_with_gpml_constant_value) {
		// The GpmlOnePointSixReader complains bitterly if it does not find a ConstantValue
		// wrapper around geometry, although GPlates is happy enough to display geometry
		// without it.
		static const GPlatesPropertyValues::TemplateTypeParameterType value_type =
				GPlatesPropertyValues::TemplateTypeParameterType::create_gml("MultiPoint");
		prop_val_opt = GPlatesPropertyValues::GpmlConstantValue::create(
				*prop_val_opt, value_type);
	}
	
	// Return the prepared PropertyValue.
	d_property_value_opt = prop_val_opt;
}


void
GPlatesGui::GeometricPropertyValueConstructor::visit_point_on_sphere(
		GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point_on_sphere)
{
	boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type> prop_val_opt;
	
	if (d_plate_id_opt && d_recon_tree_ptr != NULL) {
		// Reverse reconstruct the geometry to present-day.
		GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type present_day =
				reverse_reconstruct(point_on_sphere, *d_plate_id_opt, *d_recon_tree_ptr);
		// Convert PointOnSphere to GmlPoint.
		prop_val_opt = GPlatesPropertyValues::GmlPoint::create(*present_day);
	} else {
		// No reverse-reconstruction, just make a PropertyValue.
		prop_val_opt = GPlatesPropertyValues::GmlPoint::create(*point_on_sphere);
	}
	// At this point, we should have a valid PropertyValue.
	// Now - do we want to add a GpmlConstantValue wrapper around it?
	if (d_wrap_with_gpml_constant_value) {
		// The GpmlOnePointSixReader complains bitterly if it does not find a ConstantValue
		// wrapper around geometry, although GPlates is happy enough to display geometry
		// without it.
		static const GPlatesPropertyValues::TemplateTypeParameterType value_type =
				GPlatesPropertyValues::TemplateTypeParameterType::create_gml("Point");
		prop_val_opt = GPlatesPropertyValues::GpmlConstantValue::create(
				*prop_val_opt, value_type);
	}
	
	// Return the prepared PropertyValue.
	d_property_value_opt = prop_val_opt;
}


void
GPlatesGui::GeometricPropertyValueConstructor::visit_polygon_on_sphere(
		GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere)
{
	// Convert PolygonOnSphere to GmlPolygon with one exterior ring.
	// FIXME: We could make this more intelligent and open up the possibility of making
	// polygons with interiors.
	boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type> prop_val_opt;
	
	if (d_plate_id_opt && d_recon_tree_ptr != NULL) {
		// Reverse reconstruct the geometry to present-day.
		GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type present_day =
				reverse_reconstruct(polygon_on_sphere, *d_plate_id_opt, *d_recon_tree_ptr);
		// Convert PolygonOnSphere to GmlPolygon.
		prop_val_opt = GPlatesPropertyValues::GmlPolygon::create(present_day);
	} else {
		// No reverse-reconstruction, just make a PropertyValue.
		prop_val_opt = GPlatesPropertyValues::GmlPolygon::create(polygon_on_sphere);
	}
	// At this point, we should have a valid PropertyValue.
	// Now - do we want to add a GpmlConstantValue wrapper around it?
	if (d_wrap_with_gpml_constant_value) {
		// The GpmlOnePointSixReader complains bitterly if it does not find a ConstantValue
		// wrapper around geometry, although GPlates is happy enough to display geometry
		// without it.
		static const GPlatesPropertyValues::TemplateTypeParameterType value_type =
				GPlatesPropertyValues::TemplateTypeParameterType::create_gml("Polygon");
		prop_val_opt = GPlatesPropertyValues::GpmlConstantValue::create(
				*prop_val_opt, value_type);
	}
	
	// Return the prepared PropertyValue.
	d_property_value_opt = prop_val_opt;
}


void
GPlatesGui::GeometricPropertyValueConstructor::visit_polyline_on_sphere(
		GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere)
{
	// Convert PolylineOnSphere to GmlLineString.
	// FIXME: OrientableCurve??
	boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type> prop_val_opt;
	
	if (d_plate_id_opt && d_recon_tree_ptr != NULL) {
		// Reverse reconstruct the geometry to present-day.
		GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type present_day =
				reverse_reconstruct(polyline_on_sphere, *d_plate_id_opt, *d_recon_tree_ptr);
		// Convert PolylineOnSphere to GmlLineString.
		prop_val_opt = GPlatesPropertyValues::GmlLineString::create(present_day);
	} else {
		// No reverse-reconstruction, just make a PropertyValue.
		prop_val_opt = GPlatesPropertyValues::GmlLineString::create(polyline_on_sphere);
	}
	// At this point, we should have a valid PropertyValue.
	// Now - do we want to add a GpmlConstantValue wrapper around it?
	if (d_wrap_with_gpml_constant_value) {
		// The GpmlOnePointSixReader complains bitterly if it does not find a ConstantValue
		// wrapper around geometry, although GPlates is happy enough to display geometry
		// without it.
		static const GPlatesPropertyValues::TemplateTypeParameterType value_type =
				GPlatesPropertyValues::TemplateTypeParameterType::create_gml("LineString");
		prop_val_opt = GPlatesPropertyValues::GpmlConstantValue::create(
				*prop_val_opt, value_type);
	}
	
	// Return the prepared PropertyValue.
	d_property_value_opt = prop_val_opt;
}


