/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#include "ReconstructedFeatureGeometryFinder.h"


namespace
{
	inline
	bool
	property_name_matches(
			const GPlatesModel::ReconstructedFeatureGeometry &rfg,
			const GPlatesModel::PropertyName &property_name_to_match)
	{
		if ( ! rfg.property().is_valid()) {
			// Nothing we can do here.
			return false;
		}
		return ((*rfg.property())->property_name() == property_name_to_match);
	}


	inline
	bool
	properties_iterator_matches(
			const GPlatesModel::ReconstructedFeatureGeometry &rfg,
			const GPlatesModel::FeatureHandle::children_iterator &properties_iterator_to_match)
	{
		if ( ! rfg.property().is_valid()) {
			// Nothing we can do here.
			return false;
		}
		return (rfg.property() == properties_iterator_to_match);
	}


	inline
	bool
	reconstruction_matches(
			const GPlatesModel::ReconstructedFeatureGeometry &rfg,
			const GPlatesModel::Reconstruction *reconstruction_to_match)
	{
		return (rfg.reconstruction() == reconstruction_to_match);
	}
}


void
GPlatesModel::ReconstructedFeatureGeometryFinder::visit_reconstructed_feature_geometry(
		ReconstructedFeatureGeometry &rfg)
{
	if (d_property_name_to_match && d_reconstruction_to_match) {
		// Both a property-name-to-match and a Reconstruction-to-match were supplied, so
		// limit the results to those RFGs which are contained within that Reconstruction
		// and were reconstructed from a geometry with that property name.
		if (property_name_matches(rfg, *d_property_name_to_match) &&
				reconstruction_matches(rfg, d_reconstruction_to_match)) {
			d_found_rfgs.push_back(rfg.get_non_null_pointer());
		}
	} else if (d_property_name_to_match) {
		// A property-name-to-match was supplied, so limit the results to those RFGs which
		// were reconstructed from a geometry with that property name.
		if (property_name_matches(rfg, *d_property_name_to_match)) {
			d_found_rfgs.push_back(rfg.get_non_null_pointer());
		}
	} else if (d_properties_iterator_to_match && d_reconstruction_to_match) {
		// Both a properties-iterator-to-match and a Reconstruction-to-match were supplied, so
		// limit the results to those RFGs which are contained within that Reconstruction
		// and were reconstructed from a geometry with that properties iterator.
		if (properties_iterator_matches(rfg, *d_properties_iterator_to_match) &&
				reconstruction_matches(rfg, d_reconstruction_to_match)) {
			d_found_rfgs.push_back(rfg.get_non_null_pointer());
		}
	} else if (d_properties_iterator_to_match) {
		// A properties-iterator-to-match was supplied, so limit the results to those RFGs which
		// were reconstructed from a geometry with that properties iterator.
		if (properties_iterator_matches(rfg, *d_properties_iterator_to_match)) {
			d_found_rfgs.push_back(rfg.get_non_null_pointer());
		}
	} else if (d_reconstruction_to_match) {
		// A Reconstruction-to-match was supplied, so limit the results to those RFGs which
		// are contained within that Reconstruction.
		if (reconstruction_matches(rfg, d_reconstruction_to_match)) {
			d_found_rfgs.push_back(rfg.get_non_null_pointer());
		}
	} else {
		// Collect any and all RFGs.
		d_found_rfgs.push_back(rfg.get_non_null_pointer());
	}
}
