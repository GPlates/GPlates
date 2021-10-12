/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
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


#include "ReconstructionGeometryFinder.h"

#include "ReconstructedFeatureGeometry.h"
#include "Reconstruction.h"
#include "ResolvedTopologicalBoundary.h"
#include "ResolvedTopologicalNetwork.h"
#include "ReconstructedVirtualGeomagneticPole.h"


namespace
{
	inline
	bool
	property_name_matches(
			const GPlatesAppLogic::ReconstructedFeatureGeometry &rfg,
			const GPlatesModel::PropertyName &property_name_to_match)
	{
		return rfg.property().is_still_valid() &&
			((*rfg.property())->property_name() == property_name_to_match);
	}

	inline
	bool
	property_name_matches(
			const GPlatesAppLogic::ReconstructedVirtualGeomagneticPole &rvgp,
			const GPlatesModel::PropertyName &property_name_to_match)
	{
		return rvgp.property().is_still_valid() &&
			((*rvgp.property())->property_name() == property_name_to_match);
	}

	inline
	bool
	property_name_matches(
			const GPlatesAppLogic::ResolvedTopologicalBoundary &rtb,
			const GPlatesModel::PropertyName &property_name_to_match)
	{
		return rtb.property().is_still_valid() &&
			((*rtb.property())->property_name() == property_name_to_match);
	}


	inline
	bool
	property_name_matches(
			const GPlatesAppLogic::ResolvedTopologicalNetwork &rtn,
			const GPlatesModel::PropertyName &property_name_to_match)
	{
		return rtn.property().is_still_valid() &&
			((*rtn.property())->property_name() == property_name_to_match);
	}


	inline
	bool
	reconstruction_tree_matches(
			const GPlatesAppLogic::ReconstructionGeometry &rg,
			const GPlatesAppLogic::ReconstructionTree *reconstruction_tree_to_match)
	{
		return (rg.reconstruction_tree() == reconstruction_tree_to_match);
	}
}


void
GPlatesAppLogic::ReconstructionGeometryFinder::visit_reconstructed_feature_geometry(
		ReconstructedFeatureGeometry &rfg)
{
	visit_reconstruction_geometry_derived_type(rfg);
}

void
GPlatesAppLogic::ReconstructionGeometryFinder::visit_reconstructed_virtual_geomagnetic_pole(
				GPlatesAppLogic::ReconstructedVirtualGeomagneticPole &rvgp)
{
	visit_reconstruction_geometry_derived_type(rvgp);
}

void
GPlatesAppLogic::ReconstructionGeometryFinder::visit_resolved_topological_boundary(
		ResolvedTopologicalBoundary &rtb)
{
	visit_reconstruction_geometry_derived_type(rtb);
}


void
GPlatesAppLogic::ReconstructionGeometryFinder::visit_resolved_topological_network(
		ResolvedTopologicalNetwork &rtn)
{
	visit_reconstruction_geometry_derived_type(rtn);
}


template <class ReconstructionGeometryDerivedType>
void
GPlatesAppLogic::ReconstructionGeometryFinder::visit_reconstruction_geometry_derived_type(
		ReconstructionGeometryDerivedType &recon_geom_derived_obj)
{
	if (d_property_name_to_match && d_reconstruction_tree_to_match) {
		// Both a property-name-to-match and a ReconstructionTree-to-match were supplied, so
		// limit the results to those RGs which reference that ReconstructionTree
		// and were reconstructed from a geometry with that property name.
		if (property_name_matches(recon_geom_derived_obj, *d_property_name_to_match) &&
				reconstruction_tree_matches(recon_geom_derived_obj, d_reconstruction_tree_to_match)) {
			d_found_rgs.push_back(recon_geom_derived_obj.get_non_null_pointer());
		}
	} else if (d_property_name_to_match) {
		// A property-name-to-match was supplied, so limit the results to those RGs which
		// were reconstructed from a geometry with that property name.
		if (property_name_matches(recon_geom_derived_obj, *d_property_name_to_match)) {
			d_found_rgs.push_back(recon_geom_derived_obj.get_non_null_pointer());
		}
	} else if (d_reconstruction_tree_to_match) {
		// A ReconstructionTree-to-match was supplied, so limit the results to those RGs which
		// reference that ReconstructionTree.
		if (reconstruction_tree_matches(recon_geom_derived_obj, d_reconstruction_tree_to_match)) {
			d_found_rgs.push_back(recon_geom_derived_obj.get_non_null_pointer());
		}
	} else {
		// Collect any and all RGs.
		d_found_rgs.push_back(recon_geom_derived_obj.get_non_null_pointer());
	}
}
