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

#include <algorithm>  // std::find
#include <boost/none.hpp>

#include "TotalReconstructionSequencePlateIdFinder.h"

#include "model/InlinePropertyContainer.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlPlateId.h"


GPlatesFeatureVisitors::TotalReconstructionSequencePlateIdFinder::TotalReconstructionSequencePlateIdFinder()
{
	d_property_names_to_allow.push_back(
			GPlatesModel::PropertyName::create_gpml("fixedReferenceFrame"));
	d_property_names_to_allow.push_back(
			GPlatesModel::PropertyName::create_gpml("movingReferenceFrame"));
}


void
GPlatesFeatureVisitors::TotalReconstructionSequencePlateIdFinder::visit_feature_handle(
		const GPlatesModel::FeatureHandle &feature_handle)
{
	// Now visit each of the properties in turn.
	visit_feature_properties(feature_handle);
}


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


void
GPlatesFeatureVisitors::TotalReconstructionSequencePlateIdFinder::visit_inline_property_container(
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
GPlatesFeatureVisitors::TotalReconstructionSequencePlateIdFinder::visit_gpml_constant_value(
		const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
{
	gpml_constant_value.value()->accept_visitor(*this);
}


void
GPlatesFeatureVisitors::TotalReconstructionSequencePlateIdFinder::visit_gpml_plate_id(
		const GPlatesPropertyValues::GpmlPlateId &gpml_plate_id)
{
	static const GPlatesModel::PropertyName fixed_ref_frame_property_name =
			GPlatesModel::PropertyName::create_gpml("fixedReferenceFrame");
	static const GPlatesModel::PropertyName moving_ref_frame_property_name =
			GPlatesModel::PropertyName::create_gpml("movingReferenceFrame");

	// Note that we're going to assume that we've read a property name...
	if (*d_most_recent_propname_read == fixed_ref_frame_property_name) {
		// We're dealing with the fixed ref-frame of the Total Reconstruction Sequence.
		d_fixed_ref_frame_plate_id = gpml_plate_id.value();
	} else if (*d_most_recent_propname_read == moving_ref_frame_property_name) {
		// We're dealing with the moving ref-frame of the Total Reconstruction Sequence.
		d_moving_ref_frame_plate_id = gpml_plate_id.value();
	}
}


void
GPlatesFeatureVisitors::TotalReconstructionSequencePlateIdFinder::reset()
{
	d_most_recent_propname_read = boost::none;
	d_fixed_ref_frame_plate_id = boost::none;
	d_moving_ref_frame_plate_id = boost::none;
}
