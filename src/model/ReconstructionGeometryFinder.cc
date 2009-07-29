/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
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


#include "ReconstructionGeometryFinder.h"

#include "ReconstructedFeatureGeometry.h"
#include "Reconstruction.h"
#include "ResolvedTopologicalGeometry.h"


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
	property_name_matches(
			const GPlatesModel::ResolvedTopologicalGeometry &rtg,
			const GPlatesModel::PropertyName &property_name_to_match)
	{
		if ( ! rtg.property().is_valid()) {
			// Nothing we can do here.
			return false;
		}
		return ((*rtg.property())->property_name() == property_name_to_match);
	}


	inline
	bool
	reconstruction_matches(
			const GPlatesModel::ReconstructedFeatureGeometry &rfg,
			const GPlatesModel::Reconstruction *reconstruction_to_match)
	{
		return (rfg.reconstruction() == reconstruction_to_match);
	}


	inline
	bool
	reconstruction_matches(
			const GPlatesModel::ResolvedTopologicalGeometry &rtg,
			const GPlatesModel::Reconstruction *reconstruction_to_match)
	{
		return (rtg.reconstruction() == reconstruction_to_match);
	}
}


void
GPlatesModel::ReconstructionGeometryFinder::visit_reconstructed_feature_geometry(
		ReconstructedFeatureGeometry &rfg)
{
	visit_reconstruction_geometry_derived_type(rfg);
}


void
GPlatesModel::ReconstructionGeometryFinder::visit_resolved_topological_geometry(
		ResolvedTopologicalGeometry &rtg)
{
	visit_reconstruction_geometry_derived_type(rtg);
}


template <class ReconstructionGeometryDerivedType>
void
GPlatesModel::ReconstructionGeometryFinder::visit_reconstruction_geometry_derived_type(
		ReconstructionGeometryDerivedType &recon_geom_derived_obj)
{
	if (d_property_name_to_match && d_reconstruction_to_match) {
		// Both a property-name-to-match and a Reconstruction-to-match were supplied, so
		// limit the results to those RGs which are contained within that Reconstruction
		// and were reconstructed from a geometry with that property name.
		if (property_name_matches(recon_geom_derived_obj, *d_property_name_to_match) &&
				reconstruction_matches(recon_geom_derived_obj, d_reconstruction_to_match)) {
			d_found_rgs.push_back(recon_geom_derived_obj.get_non_null_pointer());
		}
	} else if (d_property_name_to_match) {
		// A property-name-to-match was supplied, so limit the results to those RGs which
		// were reconstructed from a geometry with that property name.
		if (property_name_matches(recon_geom_derived_obj, *d_property_name_to_match)) {
			d_found_rgs.push_back(recon_geom_derived_obj.get_non_null_pointer());
		}
	} else if (d_reconstruction_to_match) {
		// A Reconstruction-to-match was supplied, so limit the results to those RGs which
		// are contained within that Reconstruction.
		if (reconstruction_matches(recon_geom_derived_obj, d_reconstruction_to_match)) {
			d_found_rgs.push_back(recon_geom_derived_obj.get_non_null_pointer());
		}
	} else {
		// Collect any and all RGs.
		d_found_rgs.push_back(recon_geom_derived_obj.get_non_null_pointer());
	}
}
