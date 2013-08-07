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

#include "ReconstructedFeatureGeometryFinder.h"

#include "ReconstructedFeatureGeometry.h"
#include "ReconstructedFlowline.h"
#include "ReconstructedMotionPath.h"
#include "ReconstructedVirtualGeomagneticPole.h"


namespace
{
	inline
	bool
	reconstruction_tree_matches(
			const GPlatesAppLogic::ReconstructedFeatureGeometry &rfg,
			const GPlatesAppLogic::ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree_to_match)
	{
		return (rfg.get_reconstruction_tree() == reconstruction_tree_to_match);
	}


	inline
	bool
	property_name_matches(
			const GPlatesAppLogic::ReconstructedFeatureGeometry &rfg,
			const GPlatesModel::PropertyName &property_name_to_match)
	{
		const GPlatesModel::FeatureHandle::iterator &iter = rfg.property();
		return iter.is_still_valid() &&
			((*iter)->get_property_name() == property_name_to_match);
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


	/**
	 * Returns true if the reconstruct handle of @a rfg matches any of the handles in @a reconstruct_handles_to_match.
	 */
	bool
	reconstruct_handle_matches(
			const GPlatesAppLogic::ReconstructedFeatureGeometry &rfg,
			const std::vector<GPlatesAppLogic::ReconstructHandle::type> &reconstruct_handles_to_match)
	{
		// Get the RFG's reconstruct handle.
		const boost::optional<GPlatesAppLogic::ReconstructHandle::type> &rfg_reconstruct_handle_opt = rfg.get_reconstruct_handle();

		// If the RFG does not have a reconstruct handle then it cannot be matched so return false.
		if (!rfg_reconstruct_handle_opt)
		{
			return false;
		}
		const GPlatesAppLogic::ReconstructHandle::type &rfg_reconstruct_handle = rfg_reconstruct_handle_opt.get();

		// Search the sequence of restricted reconstruct handles for a match.
		std::vector<GPlatesAppLogic::ReconstructHandle::type>::const_iterator reconstruct_handle_iter =
				reconstruct_handles_to_match.begin();
		std::vector<GPlatesAppLogic::ReconstructHandle::type>::const_iterator reconstruct_handle_end =
				reconstruct_handles_to_match.end();
		for ( ; reconstruct_handle_iter != reconstruct_handle_end; ++reconstruct_handle_iter)
		{
			if (rfg_reconstruct_handle == *reconstruct_handle_iter)
			{
				// We found a match.
				return true;
			}
		}

		return false;
	}
}


void
GPlatesAppLogic::ReconstructedFeatureGeometryFinder::visit_reconstructed_feature_geometry(
		ReconstructedFeatureGeometry &rfg)
{
	// If a ReconstructionTree-to-match was supplied then limit the results to those RFGs which
	// reference that ReconstructionTree.
	if (d_reconstruction_tree_to_match &&
		!reconstruction_tree_matches(rfg, d_reconstruction_tree_to_match.get()))
	{
		return;
	}

	// If a property-name-to-match was supplied then limit the results to those RFGs which
	// were reconstructed from a geometry with that property name.
	if (d_property_name_to_match &&
		!property_name_matches(rfg, d_property_name_to_match.get()))
	{
		return;
	}

	// If a properties-iterator-to-match was supplied then limit the results to those RFGs which
	// were reconstructed from a geometry with that properties iterator.
	if (d_properties_iterator_to_match &&
		!properties_iterator_matches(rfg, d_properties_iterator_to_match.get()))
	{
		return;
	}

	// If a reconstruct-handles-to-match was supplied then limit the results to those RFGs which
	// has a reconstruct handle matching one of those supplied.
	if (d_reconstruct_handles_to_match &&
		!reconstruct_handle_matches(rfg, d_reconstruct_handles_to_match.get()))
	{
		return;
	}

	// If we get here then collect any and all RFGs.
	d_found_rfgs.push_back(rfg.get_non_null_pointer());
}


void
GPlatesAppLogic::ReconstructedFeatureGeometryFinder::visit_reconstructed_flowline(
		ReconstructedFlowline &rf)
{
	visit_reconstructed_feature_geometry(rf);
}


void
GPlatesAppLogic::ReconstructedFeatureGeometryFinder::visit_reconstructed_motion_path(
		ReconstructedMotionPath &rmp)
{
	visit_reconstructed_feature_geometry(rmp);
}


void
GPlatesAppLogic::ReconstructedFeatureGeometryFinder::visit_reconstructed_virtual_geomagnetic_pole(
		GPlatesAppLogic::ReconstructedVirtualGeomagneticPole &rvgp)
{
	visit_reconstructed_feature_geometry(rvgp);
}
