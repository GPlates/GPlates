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
#include "ReconstructionTree.h"


boost::optional<GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type>
GPlatesAppLogic::ReconstructionGeometryUtils::find_reconstruction_geometry(
		const ReconstructionGeometry &reconstruction_geometry,
		const ReconstructionTree &reconstruction_tree)
{
	// Get the feature referenced by the old reconstruction geometry.
	boost::optional<GPlatesModel::FeatureHandle::weak_ref> feature_ref =
			get_feature_ref(&reconstruction_geometry);
	if (!feature_ref)
	{
		return boost::none;
	}

	// Get the geometry property iterator from the old reconstruction geometry.
	boost::optional<GPlatesModel::FeatureHandle::iterator> geometry_property =
			get_geometry_property_iterator(&reconstruction_geometry);
	if (!geometry_property)
	{
		return boost::none;
	}

	return find_reconstruction_geometry(
			feature_ref.get(),
			geometry_property.get(),
			reconstruction_tree);
}


boost::optional<GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type>
GPlatesAppLogic::ReconstructionGeometryUtils::find_reconstruction_geometry(
		const GPlatesModel::FeatureHandle::weak_ref &feature_ref,
		const GPlatesModel::FeatureHandle::iterator &geometry_property_iterator,
		const ReconstructionTree &reconstruction_tree)
{
	if ( !feature_ref.is_valid() || !geometry_property_iterator.is_still_valid() )
	{
		return boost::none;
	}

	//
	// Iterate through the ReconstructionGeometrys that are observing 'feature_ref' and
	// that were generated from 'feature_ref's geometry property 'geometry_property_iterator'.
	//
	// Of those ReconstructionGeometries, we're only interested in those that were reconstructed
	// using the reconstruction tree 'reconstruction_tree'.
	//
	// This is necessary because there might be more than one tree (because the user might
	// have multiple reconstruction tree layers - in other words, some geometries are
	// reconstructed with one tree and other geometries with another tree).
	// It is up to the caller to make sure the reconstruction tree is not an old one from
	// a previous reconstruction time (ie, the reconstruction geomety returned will have
	// been reconstructed using the specified reconstruction tree so if it's an old tree
	// then either an old geometry or nothing will be returned).
	//

	// Iterate over the ReconstructionGeometries that were reconstructed using
	// 'reconstruction_tree' and that observe 'feature_ref'.
	ReconstructionGeometryFinder rgFinder(&reconstruction_tree);
	rgFinder.find_rgs_of_feature(feature_ref);

	ReconstructionGeometryFinder::rg_container_type::const_iterator rgIter;
	for (rgIter = rgFinder.found_rgs_begin();
		rgIter != rgFinder.found_rgs_end();
		++rgIter)
	{
		const ReconstructionGeometry *new_rg = rgIter->get();

		// See if the new ReconstructionGeometry has a geometry property.
		boost::optional<GPlatesModel::FeatureHandle::iterator> new_geometry_property =
				get_geometry_property_iterator(new_rg);
		if (!new_geometry_property)
		{
			continue;
		}

		if (new_geometry_property.get() == geometry_property_iterator)
		{
			// We found a match so returned a shared pointer.
			return GPlatesUtils::get_non_null_pointer(new_rg);
		}
	}

	// It appears that there is no ReconstructionGeometry that was reconstructed by
	// the specified reconstruction tree and which corresponds to the geometry property.
	return boost::none;
}
