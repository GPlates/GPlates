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

#include <algorithm>
#include <map>
// Disable Visual Studio warning "qualifier applied to reference type; ignored" in boost 1.36.0
#if defined(_MSC_VER)
#	pragma warning( push )
#	pragma warning( disable : 4181 )
#endif
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
#if defined(_MSC_VER)
#	pragma warning( pop )
#endif

#include "ReconstructedFeatureGeometryExportImpl.h"

#include "app-logic/ReconstructedFeatureGeometry.h"


namespace
{
	//! Convenience typedef for referenced files.
	typedef GPlatesFileIO::ReconstructedFeatureGeometryExportImpl::referenced_files_collection_type
			referenced_files_collection_type;

	//! Convenience typedef for active reconstructable files.
	typedef GPlatesFileIO::ReconstructedFeatureGeometryExportImpl::files_collection_type
			files_collection_type;

	//! Convenience typedef for sequence of RFGs.
	typedef GPlatesFileIO::ReconstructedFeatureGeometryExportImpl::reconstructed_feature_geom_seq_type
			reconstructed_feature_geom_seq_type;

	/**
	 * Typedef for mapping from @a FeatureHandle to the feature collection file it came from.
	 */
	typedef std::map<const GPlatesModel::FeatureHandle *, const GPlatesFileIO::File::Reference *>
			feature_handle_to_collection_map_type;

	//! Convenience typedef for a sequence of grouped RFGs.
	typedef GPlatesFileIO::ReconstructedFeatureGeometryExportImpl::feature_geometry_group_seq_type
			feature_geometry_group_seq_type;


	/**
	 * Populates mapping of feature handle to feature collection file.
	 */
	void
	populate_feature_handle_to_collection_map(
			feature_handle_to_collection_map_type &feature_handle_to_collection_map,
			const files_collection_type &reconstructable_files)
	{
		// Iterate through the feature collections of the active reconstructable files.
		files_collection_type::const_iterator reconstructable_files_iter;
		for (reconstructable_files_iter = reconstructable_files.begin();
			reconstructable_files_iter != reconstructable_files.end();
			++reconstructable_files_iter)
		{
			const GPlatesFileIO::File::Reference *recon_file = *reconstructable_files_iter;

			const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection_handle =
					recon_file->get_feature_collection();

			if (!feature_collection_handle.is_valid())
			{
				continue;
			}

			// Iterate through the feature handles in the current feature collection.
			GPlatesModel::FeatureCollectionHandle::const_iterator features_iter;
			for (features_iter = feature_collection_handle->begin();
				features_iter != feature_collection_handle->end();
				++features_iter)
			{
				const GPlatesModel::FeatureHandle *feature_handle_ptr = (*features_iter).get();

				// Add feature handle key to our mapping.
				feature_handle_to_collection_map[feature_handle_ptr] = recon_file;
			}
		}
	}


	/**
	 * Returns a unique list of files that reference the visible RFGs.
	 * Result is stored in @a referenced_files.
	 */
	void
	get_unique_list_of_referenced_files(
			referenced_files_collection_type &referenced_files,
			const reconstructed_feature_geom_seq_type &reconstructed_feature_geometry_seq,
			const feature_handle_to_collection_map_type &feature_handle_to_collection_map)
	{
		// Iterate through the list of RFGs and build up a unique list of
		// feature collection files referenced by them.
		reconstructed_feature_geom_seq_type::const_iterator rfg_iter;
		for (rfg_iter = reconstructed_feature_geometry_seq.begin();
			rfg_iter != reconstructed_feature_geometry_seq.end();
			++rfg_iter)
		{
			const GPlatesAppLogic::ReconstructedFeatureGeometry *rfg = *rfg_iter;
			const GPlatesModel::FeatureHandle *feature_handle_ptr = rfg->feature_handle_ptr();

			const feature_handle_to_collection_map_type::const_iterator map_iter =
					feature_handle_to_collection_map.find(feature_handle_ptr);
			if (map_iter == feature_handle_to_collection_map.end())
			{
				continue;
			}

			const GPlatesFileIO::File::Reference *file = map_iter->second;
			referenced_files.push_back(file);
		}

		using boost::lambda::_1;
		using boost::lambda::_2;

		// Sort in preparation for removing duplicates.
		// We end up sorting on 'const GPlatesFileIO::File::weak_ref' objects.
		std::sort(referenced_files.begin(), referenced_files.end(), _1 < _2);

		// Remove duplicates.
		referenced_files.erase(
				std::unique(referenced_files.begin(), referenced_files.end()),
				referenced_files.end());
	}
}


void
GPlatesFileIO::ReconstructedFeatureGeometryExportImpl::get_files_referenced_by_geometries(
		referenced_files_collection_type &referenced_files,
		const reconstructed_feature_geom_seq_type &reconstructed_feature_geometry_seq,
		const files_collection_type &reconstructable_files)
{
	feature_handle_to_collection_map_type feature_handle_to_collection_map;

	populate_feature_handle_to_collection_map(
			feature_handle_to_collection_map,
			reconstructable_files);

	get_unique_list_of_referenced_files(
			referenced_files,
			reconstructed_feature_geometry_seq,
			feature_handle_to_collection_map);
}


void
GPlatesFileIO::ReconstructedFeatureGeometryExportImpl::group_rfgs_with_their_feature(
		feature_geometry_group_seq_type &grouped_rfgs_seq,
		const reconstructed_feature_geom_seq_type &reconstructed_feature_geometry_seq)
{
	// Copy sequence so we can sort the RFGs by feature.
	reconstructed_feature_geom_seq_type rfgs_sorted_by_feature(
			reconstructed_feature_geometry_seq);

	using boost::lambda::_1;
	using boost::lambda::_2;

	// Sort in preparation for grouping RFGs by feature.
	std::sort(rfgs_sorted_by_feature.begin(), rfgs_sorted_by_feature.end(),
		boost::lambda::bind(&GPlatesAppLogic::ReconstructedFeatureGeometry::feature_handle_ptr, _1) <
					boost::lambda::bind(
							&GPlatesAppLogic::ReconstructedFeatureGeometry::feature_handle_ptr, _2));

	const GPlatesModel::FeatureHandle *current_feature_handle_ptr = NULL;

	// Iterate through the sorted sequence and put adjacent RFGs with the same feature
	// into a group.
	reconstructed_feature_geom_seq_type::const_iterator sorted_rfg_iter;
	for (sorted_rfg_iter = rfgs_sorted_by_feature.begin();
		sorted_rfg_iter != rfgs_sorted_by_feature.end();
		++sorted_rfg_iter)
	{
		const GPlatesAppLogic::ReconstructedFeatureGeometry *rfg = *sorted_rfg_iter;
		const GPlatesModel::FeatureHandle *feature_handle_ptr = rfg->feature_handle_ptr();

		if (feature_handle_ptr != current_feature_handle_ptr)
		{
			// Start a new group.
			const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref =
					rfg->get_feature_ref();

			grouped_rfgs_seq.push_back(FeatureGeometryGroup(feature_ref));

			current_feature_handle_ptr = feature_handle_ptr;
		}

		// Add the current RFG to the current feature.
		grouped_rfgs_seq.back().recon_feature_geoms.push_back(rfg);
	}
}
