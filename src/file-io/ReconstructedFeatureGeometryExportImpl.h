/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
 * Copyright (C) 2010 Geological Survey of Norway
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

#ifndef GPLATES_FILE_IO_RECONSTRUCTEDFEATUREGEOMETRYEXPORTIMPL_H
#define GPLATES_FILE_IO_RECONSTRUCTEDFEATUREGEOMETRYEXPORTIMPL_H

#include <list>
#include <vector>
#include <QFileInfo>

#include "file-io/File.h"

#include "model/FeatureHandle.h"
#include "model/types.h"


namespace GPlatesAppLogic
{
	class ReconstructedFeatureGeometry;
}

namespace GPlatesFileIO
{
	namespace ReconstructedFeatureGeometryExportImpl
	{
		//! Typedef for sequence of feature collection files.
		typedef std::vector<const GPlatesFileIO::File::Reference *> files_collection_type;

		/**
		 * Typedef for a sequence of @a ReconstructedFeatureGeometry pointers.
		 */
		typedef std::vector<const GPlatesAppLogic::ReconstructedFeatureGeometry *>
			reconstructed_feature_geom_seq_type;


		/**
		 * Groups @a ReconstructedFeatureGeometry objects with their feature.
		 */
		struct FeatureGeometryGroup
		{
			FeatureGeometryGroup(
					const GPlatesModel::FeatureHandle::const_weak_ref &_feature_ref) :
				feature_ref(_feature_ref)
			{  }

			GPlatesModel::FeatureHandle::const_weak_ref feature_ref;
			reconstructed_feature_geom_seq_type recon_feature_geoms;
		};


		/**
		 * Typedef for a sequence of @a FeatureGeometryGroup objects.
		 */
		typedef std::list<FeatureGeometryGroup> feature_geometry_group_seq_type;

		//! Typedef for sequence of file references that reference a collection of geometries.
		typedef std::vector<const GPlatesFileIO::File::Reference *> referenced_files_collection_type;



		/**
		 * Groups @a FeatureGeometryGroup objects with their feature collection.                                                                     
		 */
		struct FeatureCollectionFeatureGroup
		{
			FeatureCollectionFeatureGroup(
				const GPlatesFileIO::File::Reference *file_ptr_):
				file_ptr(file_ptr_)
			{}

			const GPlatesFileIO::File::Reference *file_ptr;
			feature_geometry_group_seq_type feature_geometry_groups;
		};


		/**
		 * Typedef for a sequence of @a FeatureCollectionFeatureGroup objects. 
		 */
		typedef std::list<FeatureCollectionFeatureGroup> feature_collection_feature_group_seq_type;


		/**
		 * Typedef for mapping from @a FeatureHandle to the feature collection file it came from.
		 */
		typedef std::map<const GPlatesModel::FeatureHandle *, const GPlatesFileIO::File::Reference *>
			feature_handle_to_collection_map_type;

		/**
		 * Returns a list of files that reference the RFGs.
		 * Result is stored in @a referenced_files.
		 */
		void
		get_files_referenced_by_geometries(
				referenced_files_collection_type &referenced_files,
				const reconstructed_feature_geom_seq_type &reconstructed_feature_geometry_seq,
				const files_collection_type &reconstructable_files,
				feature_handle_to_collection_map_type &feature_to_collection_map);


		/**
		 * Returns a sequence of groups of RFGs (grouped by their feature).
		 * Result is stored in @a grouped_rfgs_seq.
		 */
		void
		group_rfgs_with_their_feature(
				feature_geometry_group_seq_type &grouped_rfgs_seq,
				const reconstructed_feature_geom_seq_type &reconstructed_feature_geometry_seq);


		/**
		 * Fills @a grouped_features_seq with the sorted contents of @a grouped_rfgs_seq.
		 * 
		 * @a grouped_rfgs_seq is sorted by feature collection. 
		 */
		void
		group_feature_geom_groups_with_their_collection(
				const feature_handle_to_collection_map_type &feature_handle_to_collection_map,
				feature_collection_feature_group_seq_type &grouped_features_seq,
				const feature_geometry_group_seq_type &grouped_rfgs_seq);
	}
}

#endif // GPLATES_FILE_IO_RECONSTRUCTEDFEATUREGEOMETRYEXPORTIMPL_H
