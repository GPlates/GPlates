/* $Id$ */

/**
 * \file
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
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


#include "FeatureCollectionClassifier.h"

#include "model/InlinePropertyContainer.h"
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


GPlatesFeatureVisitors::FeatureCollectionClassifier::FeatureCollectionClassifier():
		d_looks_like_reconstruction_feature(false),
		d_looks_like_reconstructable_feature(false),
		d_looks_like_instantaneous_feature(false),
		d_reconstruction_feature_count(0),
		d_reconstructable_feature_count(0),
		d_instantaneous_feature_count(0),
		d_total_feature_count(0)
{
	// Reconstruction Features:
	d_property_names_to_allow.push_back(
			GPlatesModel::PropertyName::create_gpml("fixedReferenceFrame"));
	d_property_names_to_allow.push_back(
			GPlatesModel::PropertyName::create_gpml("movingReferenceFrame"));
	// Reconstructable Features:
	d_property_names_to_allow.push_back(
			GPlatesModel::PropertyName::create_gpml("reconstructionPlateId"));
	// Instantaneous Features:
	d_property_names_to_allow.push_back(
			GPlatesModel::PropertyName::create_gpml("reconstructedPlateId"));
	d_property_names_to_allow.push_back(
			GPlatesModel::PropertyName::create_gpml("reconstructedTime"));
}


void
GPlatesFeatureVisitors::FeatureCollectionClassifier::scan_feature_collection(
		GPlatesModel::FeatureCollectionHandle::const_weak_ref feature_collection_ref)
{
	GPlatesModel::FeatureCollectionHandle::features_const_iterator
			it = feature_collection_ref->features_begin(), 
			end = feature_collection_ref->features_end();
	for ( ; it != end; ++it) {
		visit_feature_handle(**it);
	}
}

void
GPlatesFeatureVisitors::FeatureCollectionClassifier::reset()
{
	d_reconstruction_feature_count = 0;
	d_reconstructable_feature_count = 0;
	d_instantaneous_feature_count = 0;
	d_total_feature_count = 0;
}


void
GPlatesFeatureVisitors::FeatureCollectionClassifier::visit_feature_handle(
		const GPlatesModel::FeatureHandle &feature_handle)
{
	// Reset the boolean flags so we can have a quick peek at the tell-tale
	// properties of this feature.
	d_looks_like_reconstruction_feature = false;
	d_looks_like_reconstructable_feature = false;
	d_looks_like_instantaneous_feature = false;
	
	// Now visit each of the properties in turn.
	visit_feature_properties(feature_handle);
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


void
GPlatesFeatureVisitors::FeatureCollectionClassifier::visit_inline_property_container(
		const GPlatesModel::InlinePropertyContainer &inline_property_container)
{
	const GPlatesModel::PropertyName &curr_prop_name = inline_property_container.property_name();
	if ( ! d_property_names_to_allow.empty()) {
		// We're not allowing all property names.
		if ( ! contains_elem(d_property_names_to_allow, curr_prop_name)) {
			// The current property name is not allowed.
			return;
		}
	}
	d_most_recent_propname_read = curr_prop_name;

	visit_property_values(inline_property_container);
}


void
GPlatesFeatureVisitors::FeatureCollectionClassifier::visit_gml_time_instant(
		const GPlatesPropertyValues::GmlTimeInstant &gml_time_instant)
{
	// Instantaneous Features:
	static const GPlatesModel::PropertyName reconstructed_time_prop =
			GPlatesModel::PropertyName::create_gpml("reconstructedTime");

	// Note that we're going to assume that we've read a property name in order
	// to have reached this point.
	if (*d_most_recent_propname_read == reconstructed_time_prop) {
		// We're dealing with an Instantaneous Feature.
		d_looks_like_instantaneous_feature = true;
	}
}


void
GPlatesFeatureVisitors::FeatureCollectionClassifier::visit_gpml_constant_value(
		const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
{
	gpml_constant_value.value()->accept_visitor(*this);
}


void
GPlatesFeatureVisitors::FeatureCollectionClassifier::visit_gpml_plate_id(
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
	if (*d_most_recent_propname_read == fixed_ref_frame_prop ||
			*d_most_recent_propname_read == moving_ref_frame_prop) {
		// We're dealing with a Total Reconstruction Sequence.
		d_looks_like_reconstruction_feature = true;
	} else if (*d_most_recent_propname_read == reconstruction_plate_id_prop) {
		// We're dealing with a Reconstructable Feature.
		d_looks_like_reconstructable_feature = true;
	} else if (*d_most_recent_propname_read == reconstructed_plate_id_prop) {
		// We're dealing with an Instantaneous Feature.
		d_looks_like_instantaneous_feature = true;
	}
}

