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

#include "ReconstructionGeometryUtils.h"

#include "Reconstruction.h"
#include "ReconstructionGeometryFinder.h"


namespace GPlatesAppLogic
{
	namespace
	{
		/**
		 * Returns those reconstruction geometries found by @a rg_finder that are in the subset
		 * @a reconstruction_geometries_subset.
		 */
		bool
		get_reconstruction_geometries_subset(
				ReconstructionGeometryUtils::reconstruction_geom_seq_type &reconstruction_geometries_observing_feature,
				const ReconstructionGeometryUtils::reconstruction_geom_seq_type &reconstruction_geometries_subset,
				const ReconstructionGeometryFinder &rg_finder)
		{
			bool found_match = false;

			ReconstructionGeometryFinder::rg_container_type::const_iterator found_rg_iter;
			for (found_rg_iter = rg_finder.found_rgs_begin();
				found_rg_iter != rg_finder.found_rgs_end();
				++found_rg_iter)
			{
				const ReconstructionGeometry::non_null_ptr_type &found_rg = *found_rg_iter;

				// If the found reconstruction geometry is not in the input reconstruction geometries then skip it.
				if (std::find(
					reconstruction_geometries_subset.begin(),
					reconstruction_geometries_subset.end(),
					found_rg) == reconstruction_geometries_subset.end())
				{
					continue;
				}

				// We found a match so add it to the caller's sequence.
				reconstruction_geometries_observing_feature.push_back(found_rg);

				found_match = true;
			}

			return found_match;
		}
	}
}


bool
GPlatesAppLogic::ReconstructionGeometryUtils::find_reconstruction_geometries_observing_feature(
		reconstruction_geom_seq_type &reconstruction_geometries_observing_feature,
		const reconstruction_geom_seq_type &reconstruction_geometries_subset,
		const ReconstructionGeometry &reconstruction_geometry,
		boost::optional<const std::vector<ReconstructHandle::type> &> reconstruct_handles)
{
	// Get the feature referenced by the old reconstruction geometry.
	boost::optional<GPlatesModel::FeatureHandle::weak_ref> feature_ref =
			ReconstructionGeometryUtils::get_feature_ref(&reconstruction_geometry);
	if (!feature_ref)
	{
		return false;
	}

	// Get the geometry property iterator from the old reconstruction geometry.
	boost::optional<GPlatesModel::FeatureHandle::iterator> geometry_property =
			ReconstructionGeometryUtils::get_geometry_property_iterator(&reconstruction_geometry);
	if (!geometry_property)
	{
		return false;
	}

	return find_reconstruction_geometries_observing_feature(
			reconstruction_geometries_observing_feature,
			reconstruction_geometries_subset,
			feature_ref.get(),
			geometry_property.get(),
			reconstruct_handles);
}


bool
GPlatesAppLogic::ReconstructionGeometryUtils::find_reconstruction_geometries_observing_feature(
		reconstruction_geom_seq_type &reconstruction_geometries_observing_feature,
		const reconstruction_geom_seq_type &reconstruction_geometries_subset,
		const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
		boost::optional<const std::vector<ReconstructHandle::type> &> reconstruct_handles)
{
	if ( !feature_ref.is_valid())
	{
		return false;
	}

	//
	// Iterate through the ReconstructionGeometrys that are observing 'feature_ref'.
	//
	// Of those ReconstructionGeometries, we're only interested in those that exist inside
	// the reconstruction geometries subset passed to us.
	//

	// Iterate over the ReconstructionGeometries that observe 'feature_ref' and were optionally
	// reconstructed from the reconstruction tree.
	ReconstructionGeometryFinder rg_finder(reconstruct_handles);
	rg_finder.find_rgs_of_feature(feature_ref);

	return get_reconstruction_geometries_subset(
			reconstruction_geometries_observing_feature, reconstruction_geometries_subset, rg_finder);
}


bool
GPlatesAppLogic::ReconstructionGeometryUtils::find_reconstruction_geometries_observing_feature(
		reconstruction_geom_seq_type &reconstruction_geometries_observing_feature,
		const reconstruction_geom_seq_type &reconstruction_geometries_subset,
		const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
		const GPlatesModel::FeatureHandle::iterator &geometry_property_iterator,
		boost::optional<const std::vector<ReconstructHandle::type> &> reconstruct_handles)
{
	if ( !feature_ref.is_valid() || !geometry_property_iterator.is_still_valid())
	{
		return false;
	}

	//
	// Iterate through the ReconstructionGeometrys that are observing 'feature_ref' and
	// that were generated from 'feature_ref's geometry property 'geometry_property_iterator'.
	//
	// Of those ReconstructionGeometries, we're only interested in those that exist inside
	// the reconstruction geometries subset passed to us.
	//

	// Iterate over the ReconstructionGeometries that observe 'feature_ref' and were reconstructed
	// from its 'geometry_property_iterator' feature property and optionally from the reconstruction tree.
	ReconstructionGeometryFinder rg_finder(geometry_property_iterator, reconstruct_handles);
	rg_finder.find_rgs_of_feature(feature_ref);

	return get_reconstruction_geometries_subset(
			reconstruction_geometries_observing_feature, reconstruction_geometries_subset, rg_finder);
}
