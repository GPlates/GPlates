/* $Id: ReconstructedFeatureGeometryExportImpl.h -1   $ */

/**
 * \file 
 * $Revision: -1 $
 * $Date: $
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

#ifndef GPLATES_FILE_IO_RECONSTRUCTEDFLOWLINEEXPORTIMPL_H
#define GPLATES_FILE_IO_RECONSTRUCTEDFLOWLINEEXPORTIMPL_H

#include <list>
#include <vector>
#include <QFileInfo>

#include "file-io/File.h"

#include "model/FeatureHandle.h"
#include "model/types.h"


namespace GPlatesAppLogic
{
	class ReconstructedFeatureGeometry;
	class ReconstructedFlowline;
	class ReconstructionGeometry;
}

namespace GPlatesFileIO
{
	namespace ReconstructedFlowlineExportImpl
	{
		//! Typedef for sequence of feature collection files.
		typedef std::vector<const GPlatesFileIO::File::Reference *> files_collection_type;

		/**
		 * Typedef for a sequence of @a ReconstructionGeometry pointers.
		 */
		typedef std::vector<const GPlatesAppLogic::ReconstructedFlowline *>
			reconstructed_flowline_seq_type;


		/**
		 * Groups @a ReconstructedFlowline objects with their feature.
		 */
		struct FlowlineGroup
		{
			FlowlineGroup(
					const GPlatesModel::FeatureHandle::const_weak_ref &_feature_ref) :
				feature_ref(_feature_ref)
			{  }

			GPlatesModel::FeatureHandle::const_weak_ref feature_ref;
			reconstructed_flowline_seq_type recon_flowlines;
		};


		/**
		 * Typedef for a sequence of @a FlowlineGroup objects.
		 */
		typedef std::list<FlowlineGroup> flowline_group_seq_type;

		//! Typedef for sequence of file references that reference a collection of geometries.
		typedef std::vector<const GPlatesFileIO::File::Reference *> referenced_files_collection_type;



		/**
		 * Groups @a FeatureGeometryGroup objects with their feature collection.                                                                     
		 */
		struct FeatureCollectionFlowlineGroup
		{
			FeatureCollectionFlowlineGroup(
				const GPlatesFileIO::File::Reference *file_ptr_):
				file_ptr(file_ptr_)
			{}

			const GPlatesFileIO::File::Reference *file_ptr;
			flowline_group_seq_type flowline_groups;
		};


		/**
		 * Typedef for a sequence of @a FeatureCollectionFeatureGroup objects. 
		 */
		typedef std::list<FeatureCollectionFlowlineGroup> feature_collection_flowline_group_seq_type;


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
				const reconstructed_flowline_seq_type &reconstructed_flowline_seq,
				const files_collection_type &reconstructable_files,
				feature_handle_to_collection_map_type &feature_to_collection_map);

		/**
		 * Returns a sequence of groups of flowlines (grouped by their feature).
		 * Result is stored in @a grouped_flowline_seq.
		 */
		void
		group_flowlines_with_their_feature(
				flowline_group_seq_type &grouped_flowline_seq,
				const reconstructed_flowline_seq_type &reconstructed_flowline_seq);


		/**
		 * Fills @a grouped_features_seq with the sorted contents of @a grouped_flowlines_seq.
		 * 
		 * @a grouped_flowlines_seq is sorted by feature collection. 
		 */
		void
		group_flowline_groups_with_their_collection(
				const feature_handle_to_collection_map_type &feature_handle_to_collection_map,
				feature_collection_flowline_group_seq_type &grouped_features_seq,
				const flowline_group_seq_type &grouped_flowlines_seq);
	}
}

#endif // GPLATES_FILE_IO_RECONSTRUCTEDFLOWLINEEXPORTIMPL_H
