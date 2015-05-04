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
#include "ReconstructedSmallCircle.h"
#include "ReconstructedVirtualGeomagneticPole.h"
#include "Reconstruction.h"
#include "ResolvedTopologicalGeometry.h"
#include "ResolvedTopologicalNetwork.h"



namespace
{
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


	/**
	 * Returns true if the reconstruct handle of @a rg matches any of the handles in @a reconstruct_handles_to_match.
	 */
	bool
	reconstruct_handle_matches(
			const GPlatesAppLogic::ReconstructionGeometry &rg,
			const std::vector<GPlatesAppLogic::ReconstructHandle::type> &reconstruct_handles_to_match)
	{
		// Get the RG's reconstruct handle.
		const boost::optional<GPlatesAppLogic::ReconstructHandle::type> &rg_reconstruct_handle_opt =
				rg.get_reconstruct_handle();

		// If the RG does not have a reconstruct handle then it cannot be matched so return false.
		if (!rg_reconstruct_handle_opt)
		{
			return false;
		}
		const GPlatesAppLogic::ReconstructHandle::type &rg_reconstruct_handle = rg_reconstruct_handle_opt.get();

		// Search the sequence of restricted reconstruct handles for a match.
		std::vector<GPlatesAppLogic::ReconstructHandle::type>::const_iterator reconstruct_handle_iter =
				reconstruct_handles_to_match.begin();
		std::vector<GPlatesAppLogic::ReconstructHandle::type>::const_iterator reconstruct_handle_end =
				reconstruct_handles_to_match.end();
		for ( ; reconstruct_handle_iter != reconstruct_handle_end; ++reconstruct_handle_iter)
		{
			if (rg_reconstruct_handle == *reconstruct_handle_iter)
			{
				// We found a match.
				return true;
			}
		}

		return false;
	}
}


void
GPlatesAppLogic::ReconstructionGeometryFinder::visit_reconstructed_feature_geometry(
		ReconstructedFeatureGeometry &rfg)
{
	visit_reconstruction_geometry_derived_type(rfg);
}


void
GPlatesAppLogic::ReconstructionGeometryFinder::visit_resolved_topological_geometry(
		ResolvedTopologicalGeometry &rtb)
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

	// If a reconstruct-handles-to-match was supplied then limit the results to those RGs which
	// has a reconstruct handle matching one of those supplied.
	if (d_reconstruct_handles_to_match &&
		!reconstruct_handle_matches(rg, d_reconstruct_handles_to_match.get()))
	{
		return;
	}

	// If we get here then collect any and all RGs.
	d_found_rgs.push_back(rg.get_non_null_pointer());
}
