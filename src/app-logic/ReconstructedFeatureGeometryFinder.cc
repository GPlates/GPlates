/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2009, 2010 The University of Sydney, Australia
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

#include "ReconstructedFeatureGeometry.h"
#include "ReconstructedFeatureGeometryFinder.h"


namespace
{
	inline
	bool
	property_name_matches(
			const GPlatesAppLogic::ReconstructedFeatureGeometry &rfg,
			const GPlatesModel::PropertyName &property_name_to_match)
	{
		const GPlatesModel::FeatureHandle::iterator &iter = rfg.property();
		return iter.is_still_valid() &&
			((*iter)->property_name() == property_name_to_match);
	}


	inline
	bool
	properties_iterator_matches(
			const GPlatesAppLogic::ReconstructedFeatureGeometry &rfg,
			const GPlatesModel::FeatureHandle::iterator &properties_iterator_to_match)
	{
		const GPlatesModel::FeatureHandle::iterator &iter = rfg.property();
		return iter.is_still_valid() &&
			(iter == properties_iterator_to_match);
	}


	inline
	bool
	reconstruction_tree_matches(
			const GPlatesAppLogic::ReconstructedFeatureGeometry &rfg,
			const GPlatesAppLogic::ReconstructionTree *reconstruction_tree_to_match)
	{
		return (rfg.reconstruction_tree() == reconstruction_tree_to_match);
	}
}


void
GPlatesAppLogic::ReconstructedFeatureGeometryFinder::visit_reconstructed_feature_geometry(
		ReconstructedFeatureGeometry &rfg)
{
	if (d_property_name_to_match && d_reconstruction_tree_to_match) {
		// Both a property-name-to-match and a ReconstructionTree-to-match were supplied, so
		// limit the results to those RFGs which reference that ReconstructionTree
		// and were reconstructed from a geometry with that property name.
		if (property_name_matches(rfg, *d_property_name_to_match) &&
				reconstruction_tree_matches(rfg, d_reconstruction_tree_to_match)) {
			d_found_rfgs.push_back(rfg.get_non_null_pointer());
		}
	} else if (d_property_name_to_match) {
		// A property-name-to-match was supplied, so limit the results to those RFGs which
		// were reconstructed from a geometry with that property name.
		if (property_name_matches(rfg, *d_property_name_to_match)) {
			d_found_rfgs.push_back(rfg.get_non_null_pointer());
		}
	} else if (d_properties_iterator_to_match && d_reconstruction_tree_to_match) {
		// Both a properties-iterator-to-match and a ReconstructionTree-to-match were supplied, so
		// limit the results to those RFGs which reference that ReconstructionTree
		// and were reconstructed from a geometry with that properties iterator.
		if (properties_iterator_matches(rfg, *d_properties_iterator_to_match) &&
				reconstruction_tree_matches(rfg, d_reconstruction_tree_to_match)) {
			d_found_rfgs.push_back(rfg.get_non_null_pointer());
		}
	} else if (d_properties_iterator_to_match) {
		// A properties-iterator-to-match was supplied, so limit the results to those RFGs which
		// were reconstructed from a geometry with that properties iterator.
		if (properties_iterator_matches(rfg, *d_properties_iterator_to_match)) {
			d_found_rfgs.push_back(rfg.get_non_null_pointer());
		}
	} else if (d_reconstruction_tree_to_match) {
		// A ReconstructionTree-to-match was supplied, so limit the results to those RFGs which
		// reference that ReconstructionTree.
		if (reconstruction_tree_matches(rfg, d_reconstruction_tree_to_match)) {
			d_found_rfgs.push_back(rfg.get_non_null_pointer());
		}
	} else {
		// Collect any and all RFGs.
		d_found_rfgs.push_back(rfg.get_non_null_pointer());
	}
}
