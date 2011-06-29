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
#include "ReconstructedFlowline.h"
#include "ReconstructedMotionPath.h"
#include "Reconstruction.h"
#include "ResolvedTopologicalBoundary.h"
#include "ResolvedTopologicalNetwork.h"
#include "ReconstructedVirtualGeomagneticPole.h"


namespace
{
	inline
	bool
	reconstruction_tree_matches(
			const GPlatesAppLogic::ReconstructionGeometry &rg,
			const GPlatesAppLogic::ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree_to_match)
	{
		return (rg.reconstruction_tree() == reconstruction_tree_to_match);
	}


	template <class ReconstructionGeometryDerivedType>
	inline
	bool
	property_name_matches(
			const ReconstructionGeometryDerivedType &rg,
			const GPlatesModel::PropertyName &property_name_to_match)
	{
		return rg.property().is_still_valid() &&
			((*rg.property())->property_name() == property_name_to_match);
	}


	template <class ReconstructionGeometryDerivedType>
	inline
	bool
	properties_iterator_matches(
			const ReconstructionGeometryDerivedType &rg,
			const GPlatesModel::FeatureHandle::iterator &properties_iterator_to_match)
	{
		const GPlatesModel::FeatureHandle::iterator &iter = rg.property();
		return iter.is_still_valid() &&
			(iter == properties_iterator_to_match);
	}
}


void
GPlatesAppLogic::ReconstructionGeometryFinder::visit_reconstructed_feature_geometry(
		ReconstructedFeatureGeometry &rfg)
{
	visit_reconstruction_geometry_derived_type(rfg);
}

void
GPlatesAppLogic::ReconstructionGeometryFinder::visit_reconstructed_flowline(
	ReconstructedFlowline &rf)
{
	visit_reconstruction_geometry_derived_type(rf);
}

void
GPlatesAppLogic::ReconstructionGeometryFinder::visit_reconstructed_motion_path(
	ReconstructedMotionPath &rmp)
{
	visit_reconstruction_geometry_derived_type(rmp);
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
		ReconstructionGeometryDerivedType &rg)
{
	// If a ReconstructionTree-to-match was supplied then limit the results to those RGs which
	// reference that ReconstructionTree.
	if (d_reconstruction_tree_to_match &&
		!reconstruction_tree_matches(rg, d_reconstruction_tree_to_match.get()))
	{
		return;
	}

	// If a property-name-to-match was supplied then limit the results to those RGs which
	// were reconstructed from a geometry with that property name.
	if (d_property_name_to_match &&
		!property_name_matches(rg, d_property_name_to_match.get()))
	{
		return;
	}

	// If a properties-iterator-to-match was supplied then limit the results to those RGs which
	// were reconstructed from a geometry with that properties iterator.
	if (d_properties_iterator_to_match &&
		!properties_iterator_matches(rg, d_properties_iterator_to_match.get()))
	{
		return;
	}

	// If we get here then collect any and all RGs.
	d_found_rgs.push_back(rg.get_non_null_pointer());
}
