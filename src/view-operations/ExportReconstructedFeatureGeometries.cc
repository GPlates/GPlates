/* $Id$ */

/**
 * \file Exports visible reconstructed feature geometries to a file.
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

#include <algorithm>
#include <map>
#include <vector>
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
#include <QFileInfo>

#include "ExportReconstructedFeatureGeometries.h"

#include "RenderedGeometryUtils.h"
#include "RenderedGeometryCollection.h"
#include "app-logic/ReconstructionGeometryUtils.h"
#include "file-io/ReconstructedFeatureGeometryExport.h"
#include "model/ReconstructedFeatureGeometry.h"


namespace
{
	//! Convenience typedef for referenced files.
	typedef GPlatesFileIO::ReconstructedFeatureGeometryExport::referenced_files_collection_type
			referenced_files_collection_type;

	//! Convenience typedef for active reconstructable files.
	typedef GPlatesViewOperations::ExportReconstructedFeatureGeometries::active_files_collection_type
			active_files_collection_type;

	//! Convenience typedef for sequence of RFGs.
	typedef std::vector<GPlatesModel::ReconstructedFeatureGeometry *>
			reconstructed_feature_geom_seq_type;

	//! Convenience typedef for iterator into global list of loaded feature collection files.
	typedef GPlatesAppState::ApplicationState::file_info_const_iterator file_info_const_iterator_type;

	/**
	 * Typedef for mapping from @a FeatureHandle to the feature collection file it came from.
	 */
	typedef std::map<const GPlatesModel::FeatureHandle *, file_info_const_iterator_type>
			feature_handle_to_collection_map_type;

	//! Convenience typedef for a sequence of grouped RFGs.
	typedef GPlatesFileIO::ReconstructedFeatureGeometryExport::feature_geometry_group_seq_type
			feature_geometry_group_seq_type;


	/**
	 * Populates mapping of feature handle to feature collection file.
	 */
	void
	populate_feature_handle_to_collection_map(
			feature_handle_to_collection_map_type &feature_handle_to_collection_map,
			const active_files_collection_type &active_reconstructable_files)
	{
		// Iterate through the feature collections of the active reconstructable files.
		active_files_collection_type::const_iterator active_reconstructable_files_iter;
		for (active_reconstructable_files_iter = active_reconstructable_files.begin();
			active_reconstructable_files_iter != active_reconstructable_files.end();
			++active_reconstructable_files_iter)
		{
			const file_info_const_iterator_type &active_recon_file_info_iter =
					*active_reconstructable_files_iter;

			if (!active_recon_file_info_iter->get_feature_collection())
			{
				continue;
			}

			const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection_handle =
					*active_recon_file_info_iter->get_feature_collection();

			if (!feature_collection_handle.is_valid())
			{
				continue;
			}

			// Iterate through the feature handles in the current feature collection.
			GPlatesModel::FeatureCollectionHandle::features_const_iterator features_iter;
			for (features_iter = feature_collection_handle->features_begin();
				features_iter != feature_collection_handle->features_end();
				++features_iter)
			{
				const GPlatesModel::FeatureHandle *feature_handle_ptr = (*features_iter).get();

				// Add feature handle key to our mapping.
				feature_handle_to_collection_map[feature_handle_ptr] = active_recon_file_info_iter;
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
			const GPlatesModel::ReconstructedFeatureGeometry *rfg = *rfg_iter;
			const GPlatesModel::FeatureHandle *feature_handle_ptr = rfg->feature_handle_ptr();

			const feature_handle_to_collection_map_type::const_iterator map_iter =
					feature_handle_to_collection_map.find(feature_handle_ptr);
			if (map_iter == feature_handle_to_collection_map.end())
			{
				continue;
			}

			const file_info_const_iterator_type &file_info_iterator = map_iter->second;
			referenced_files.push_back(file_info_iterator);
		}

		using boost::lambda::_1;
		using boost::lambda::_2;

		// Sort in preparation for removing duplicates.
		// Here the magic of boost lambda takes '&*_1' and converts
		// 'std::list<GPlatesFileIO::FileInfo>::const_iterator' to
		// 'const GPlatesFileIO::FileInfo &' with 'operator *' and then to
		// 'const GPlatesFileIO::FileInfo *' with 'operator &'.
		// So we end up sorting on 'const GPlatesFileIO::FileInfo *' raw pointers.
		std::sort(referenced_files.begin(), referenced_files.end(),
				&*_1 < &*_2);

		// Remove duplicates.
		referenced_files.erase(
				std::unique(referenced_files.begin(), referenced_files.end()),
				referenced_files.end());
	}


	/**
	 * Returns a list of files that reference the visible RFGs.
	 * Result is stored in @a referenced_files.
	 */
	void
	get_files_referenced_by_geometries(
			referenced_files_collection_type &referenced_files,
			const reconstructed_feature_geom_seq_type &reconstructed_feature_geometry_seq,
			const active_files_collection_type &active_reconstructable_files)
	{
		feature_handle_to_collection_map_type feature_handle_to_collection_map;

		populate_feature_handle_to_collection_map(
				feature_handle_to_collection_map,
				active_reconstructable_files);

		get_unique_list_of_referenced_files(
				referenced_files,
				reconstructed_feature_geometry_seq,
				feature_handle_to_collection_map);
	}


	/**
	 * Returns a sequence of groups of RFGs (grouped by their feature).
	 * Result is stored in @a grouped_rfgs_seq.
	 */
	void
	group_rfgs_with_their_feature(
			feature_geometry_group_seq_type &grouped_rfgs_seq,
			const reconstructed_feature_geom_seq_type &reconstructed_feature_geometry_seq)
	{
		// Copy sequence so we can sort the RFGs by feature.
		reconstructed_feature_geom_seq_type rfgs_sorted_by_feature(
				reconstructed_feature_geometry_seq);

		using boost::lambda::_1;
		using boost::lambda::_2;
		using boost::lambda::bind;

		// Sort in preparation for grouping RFGs by feature.
		std::sort(rfgs_sorted_by_feature.begin(), rfgs_sorted_by_feature.end(),
				bind(&GPlatesModel::ReconstructedFeatureGeometry::feature_handle_ptr, _1) <
						bind(&GPlatesModel::ReconstructedFeatureGeometry::feature_handle_ptr, _2));

		const GPlatesModel::FeatureHandle *current_feature_handle_ptr = NULL;

		// Iterate through the sorted sequence and put adjacent RFGs with the same feature
		// into a group.
		reconstructed_feature_geom_seq_type::const_iterator sorted_rfg_iter;
		for (sorted_rfg_iter = rfgs_sorted_by_feature.begin();
			sorted_rfg_iter != rfgs_sorted_by_feature.end();
			++sorted_rfg_iter)
		{
			const GPlatesModel::ReconstructedFeatureGeometry *rfg = *sorted_rfg_iter;
			const GPlatesModel::FeatureHandle *feature_handle_ptr = rfg->feature_handle_ptr();

			if (feature_handle_ptr != current_feature_handle_ptr)
			{
				// Start a new group.
				const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref =
						GPlatesModel::FeatureHandle::get_const_weak_ref(
								rfg->get_feature_ref());

				grouped_rfgs_seq.push_back(
						GPlatesFileIO::ReconstructedFeatureGeometryExport::FeatureGeometryGroup(
								feature_ref));

				current_feature_handle_ptr = feature_handle_ptr;
			}

			// Add the current RFG to the current feature.
			grouped_rfgs_seq.back().recon_feature_geoms.push_back(rfg);
		}
	}
}


void
GPlatesViewOperations::ExportReconstructedFeatureGeometries::export_visible_geometries(
		const QString &filename,
		const GPlatesModel::Reconstruction &reconstruction,
		const GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
		const active_files_collection_type &active_reconstructable_files,
		const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
		const double &reconstruction_time)
{
	// Get any ReconstructionGeometry objects that are visible in the RECONSTRUCTION_LAYER
	// of the RenderedGeometryCollection.
	RenderedGeometryUtils::reconstruction_geom_seq_type reconstruction_geom_seq;
	RenderedGeometryUtils::get_reconstruction_geometries(
			reconstruction_geom_seq,
			rendered_geom_collection,
			GPlatesViewOperations::RenderedGeometryCollection::RECONSTRUCTION_LAYER);

	// Get any ReconstructionGeometry objects that are of type ReconstructedFeatureGeometry.
	reconstructed_feature_geom_seq_type reconstruct_feature_geom_seq;
	GPlatesAppLogic::ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type_sequence(
			reconstruction_geom_seq.begin(),
			reconstruction_geom_seq.end(),
			reconstruct_feature_geom_seq);

	// Get the list of active reconstructable feature collection files that contain
	// the features referenced by the visible ReconstructedFeatureGeometry objects.
	referenced_files_collection_type referenced_files;
	get_files_referenced_by_geometries(
			referenced_files, reconstruct_feature_geom_seq, active_reconstructable_files);

	// Group the RFGs by their feature.
	feature_geometry_group_seq_type grouped_rfgs_seq;
	group_rfgs_with_their_feature(grouped_rfgs_seq, reconstruct_feature_geom_seq);

	// Export the RFGs to a file format based on the filename extension.
	QFileInfo file_info(filename);
	GPlatesFileIO::ReconstructedFeatureGeometryExport::export_geometries(
			grouped_rfgs_seq,
			file_info,
			referenced_files,
			reconstruction_anchor_plate_id,
			reconstruction_time);
}
