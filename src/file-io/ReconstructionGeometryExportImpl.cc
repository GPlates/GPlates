/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#include "ReconstructionGeometryExportImpl.h"


void
GPlatesFileIO::ReconstructionGeometryExportImpl::populate_feature_handle_to_collection_map(
		feature_handle_to_collection_map_type &feature_handle_to_collection_map,
		const std::vector<const File::Reference *> &reconstructable_files)
{
	// Iterate through the feature collections of the active reconstructable files.
	std::vector<const File::Reference *>::const_iterator reconstructable_files_iter;
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
