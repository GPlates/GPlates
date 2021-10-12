/* $Id$ */

/**
 * \file
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, 2009 The University of Sydney, Australia
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


#include "FeatureClassifier.h"

#include "model/TopLevelPropertyInline.h"
#include "property-values/GpmlConstantValue.h"


namespace
{
	template<typename C, typename E>
	bool
	contains_elem(
			const C &container,
			const E &elem)
	{
		return (std::find(container.begin(), container.end(), elem) != container.end());
	}
}


GPlatesFeatureVisitors::FeatureClassifier::FeatureClassifier():
		d_looks_like_reconstruction_feature(false),
		d_looks_like_reconstructable_feature(false),
		d_looks_like_instantaneous_feature(false),
		d_reconstruction_feature_count(0),
		d_reconstructable_feature_count(0),
		d_instantaneous_feature_count(0),
		d_total_feature_count(0)
{
	// Reconstruction Features:
	static const GPlatesModel::PropertyName fixed_reference_frame_prop_name =
			GPlatesModel::PropertyName::create_gpml("fixed_reference_frame_prop_name");
	static const GPlatesModel::PropertyName moving_reference_frame_prop_name =
			GPlatesModel::PropertyName::create_gpml("movingReferenceFrame");
	d_property_names_to_allow.push_back(fixed_reference_frame_prop_name);
	d_property_names_to_allow.push_back(moving_reference_frame_prop_name);

	// Reconstructable Features:
	static const GPlatesModel::PropertyName reconstruction_plate_id_prop_name =
			GPlatesModel::PropertyName::create_gpml("reconstructionPlateId");
	d_property_names_to_allow.push_back(reconstruction_plate_id_prop_name);

	// Instantaneous Features:
	static const GPlatesModel::PropertyName reconstructed_plate_id_prop_name =
			GPlatesModel::PropertyName::create_gpml("reconstructedPlateId");
	static const GPlatesModel::PropertyName reconstructed_time_prop_name =
			GPlatesModel::PropertyName::create_gpml("reconstructedTime");
	d_property_names_to_allow.push_back(reconstructed_plate_id_prop_name);
	d_property_names_to_allow.push_back(reconstructed_time_prop_name);
}


void
GPlatesFeatureVisitors::FeatureClassifier::reset()
{
	d_reconstruction_feature_count = 0;
	d_reconstructable_feature_count = 0;
	d_instantaneous_feature_count = 0;
	d_total_feature_count = 0;
}


bool
GPlatesFeatureVisitors::FeatureClassifier::initialise_pre_feature_properties(
		const GPlatesModel::FeatureHandle &feature_handle)
{
	// Reset the boolean flags so we can have a quick peek at the tell-tale
	// properties of this feature.
	d_looks_like_reconstruction_feature = false;
	d_looks_like_reconstructable_feature = false;
	d_looks_like_instantaneous_feature = false;

	return true;
}


void
GPlatesFeatureVisitors::FeatureClassifier::finalise_post_feature_properties(
		const GPlatesModel::FeatureHandle &feature_handle)
{
	d_total_feature_count++;
	
	// Then, given what we've seen, identify the kind of feature we saw.
	if (d_looks_like_reconstruction_feature &&
			! d_looks_like_reconstructable_feature &&
			! d_looks_like_instantaneous_feature) {
		d_reconstruction_feature_count++;
		
	} else if ( ! d_looks_like_reconstruction_feature &&
			d_looks_like_reconstructable_feature &&
			! d_looks_like_instantaneous_feature) {
		d_reconstructable_feature_count++;

	} else if ( ! d_looks_like_reconstruction_feature &&
			! d_looks_like_reconstructable_feature &&
			d_looks_like_instantaneous_feature) {
		d_instantaneous_feature_count++;
	}
}


bool
GPlatesFeatureVisitors::FeatureClassifier::initialise_pre_property_values(
		const GPlatesModel::TopLevelPropertyInline &top_level_property_inline)
{
	const GPlatesModel::PropertyName &curr_prop_name = top_level_property_inline.property_name();
	if ( ! d_property_names_to_allow.empty()) {
		// We're not allowing all property names.
		if ( ! contains_elem(d_property_names_to_allow, curr_prop_name)) {
			// The current property name is not allowed.
			return false;
		}
	}
	return true;
}


void
GPlatesFeatureVisitors::FeatureClassifier::visit_gml_time_instant(
		const GPlatesPropertyValues::GmlTimeInstant &gml_time_instant)
{
	// Instantaneous Features:
	static const GPlatesModel::PropertyName reconstructed_time_prop =
			GPlatesModel::PropertyName::create_gpml("reconstructedTime");

	// Note that we're going to assume that we've read a property name in order
	// to have reached this point.
	if (*current_top_level_propname() == reconstructed_time_prop) {
		// We're dealing with an Instantaneous Feature.
		d_looks_like_instantaneous_feature = true;
	}
}


void
GPlatesFeatureVisitors::FeatureClassifier::visit_gpml_constant_value(
		const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
{
	gpml_constant_value.value()->accept_visitor(*this);
}


void
GPlatesFeatureVisitors::FeatureClassifier::visit_gpml_plate_id(
		const GPlatesPropertyValues::GpmlPlateId &gpml_plate_id)
{
	// Reconstruction Features:
	static const GPlatesModel::PropertyName fixed_ref_frame_prop =
			GPlatesModel::PropertyName::create_gpml("fixedReferenceFrame");
	static const GPlatesModel::PropertyName moving_ref_frame_prop =
			GPlatesModel::PropertyName::create_gpml("movingReferenceFrame");
	// Reconstructable Features:
	static const GPlatesModel::PropertyName reconstruction_plate_id_prop =
			GPlatesModel::PropertyName::create_gpml("reconstructionPlateId");
	// Instantaneous Features:
	static const GPlatesModel::PropertyName reconstructed_plate_id_prop =
			GPlatesModel::PropertyName::create_gpml("reconstructedPlateId");

	// Note that we're going to assume that we've read a property name in order
	// to have reached this point.
	if (*current_top_level_propname() == fixed_ref_frame_prop ||
			*current_top_level_propname() == moving_ref_frame_prop) {
		// We're dealing with a Total Reconstruction Sequence.
		d_looks_like_reconstruction_feature = true;
	} else if (*current_top_level_propname() == reconstruction_plate_id_prop) {
		// We're dealing with a Reconstructable Feature.
		d_looks_like_reconstructable_feature = true;
	} else if (*current_top_level_propname() == reconstructed_plate_id_prop) {
		// We're dealing with an Instantaneous Feature.
		d_looks_like_instantaneous_feature = true;
	}
}

