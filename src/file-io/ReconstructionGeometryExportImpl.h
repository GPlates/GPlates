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

#ifndef GPLATES_FILE_IO_RECONSTRUCTIONGEOMETRYEXPORTIMPL_H
#define GPLATES_FILE_IO_RECONSTRUCTIONGEOMETRYEXPORTIMPL_H

#if defined(_MSC_VER) && _MSC_VER <= 1400
////Visual C++ 2005
#pragma warning( disable : 4503 )
#endif 

#include <algorithm>
#include <functional>
#include <list>
#include <map>
#include <utility>
#include <vector>
#include "global/CompilerWarnings.h"
PUSH_MSVC_WARNINGS
// Disable Visual Studio warning "qualifier applied to function type has no meaning; ignored" in boost 1.36.0
DISABLE_MSVC_WARNING(4181)
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
POP_MSVC_WARNINGS
#include <QFileInfo>
#include <QString>

#include "app-logic/ReconstructionGeometry.h"
#include "app-logic/ReconstructionGeometryUtils.h"

#include "file-io/File.h"

#include "model/FeatureHandle.h"
#include "model/types.h"


namespace GPlatesFileIO
{
	namespace ReconstructionGeometryExportImpl
	{
		/**
		 * Typedef for a sequence of referenced files.
		 */
		typedef std::vector<const File::Reference *> referenced_files_collection_type;


		/**
		 * Groups @a ReconstructedFeatureGeometry derived objects with their feature.
		 */
		template <class ReconstructionGeometryType>
		struct FeatureGeometryGroup
		{
			explicit
			FeatureGeometryGroup(
					const GPlatesModel::FeatureHandle::const_weak_ref &_feature_ref) :
				feature_ref(_feature_ref)
			{  }


			GPlatesModel::FeatureHandle::const_weak_ref feature_ref;
			std::vector<const ReconstructionGeometryType *> recon_geoms;
		};


		/**
		 * Groups @a FeatureGeometryGroup objects with their feature collection.                                                                     
		 */
		template <class ReconstructionGeometryType>
		struct FeatureCollectionFeatureGroup
		{
			FeatureCollectionFeatureGroup(
					const GPlatesFileIO::File::Reference *file_ptr_) :
				file_ptr(file_ptr_)
			{}


			const GPlatesFileIO::File::Reference *file_ptr;
			std::list< FeatureGeometryGroup<ReconstructionGeometryType> > feature_geometry_groups;
		};


		/**
		 * Typedef for mapping from @a FeatureHandle to the feature collection file it came from and
		 * the order in which is occurs relative to other features in the feature collections.
		 */
		typedef std::map<
				const GPlatesModel::FeatureHandle *,
				std::pair<const GPlatesFileIO::File::Reference *, unsigned int/*feature order*/> >
						feature_handle_to_collection_map_type;


		/**
		 * Populates mapping of feature handle to feature collection file.
		 *
		 * Also stores the ordering (as an integer) of each feature as the are ordered within
		 * and across the feature collections (according to @a reconstructable_files).
		 */
		void
		populate_feature_handle_to_collection_map(
				feature_handle_to_collection_map_type &feature_handle_to_collection_map,
				const std::vector<const File::Reference *> &reconstructable_files);


		/**
		 * Returns a unique list of files that reference the visible ReconstructionGeometry objects.
		 * Result is stored in @a referenced_files.
		 */
		template <class ReconstructionGeometryType>
		void
		get_unique_list_of_referenced_files(
				referenced_files_collection_type &referenced_files,
				const std::vector<const ReconstructionGeometryType *> &reconstruction_geometry_seq,
				const feature_handle_to_collection_map_type &feature_handle_to_collection_map);


		/**
		 * Returns a list of files that reference the @a ReconstructionGeometry derived objects.
		 * Result is stored in @a referenced_files.
		 *
		 * Also populates @a feature_to_collection_map.
		 */
		template <class ReconstructionGeometryType>
		void
		get_files_referenced_by_geometries(
				referenced_files_collection_type &referenced_files,
				const std::vector<const ReconstructionGeometryType *> &reconstruction_geometry_seq,
				const std::vector<const File::Reference *> &reconstructable_files,
				feature_handle_to_collection_map_type &feature_handle_to_collection_map)
		{
			populate_feature_handle_to_collection_map(
					feature_handle_to_collection_map,
					reconstructable_files);

			get_unique_list_of_referenced_files(
					referenced_files,
					reconstruction_geometry_seq,
					feature_handle_to_collection_map);
		}


		/**
		 * Returns a sequence of groups of ReconstructionGeometry objects (grouped by their feature)
		 * with result is stored in @a grouped_recon_geoms_seq.
		 *
		 * Sorts feature groups according to the order their features appear in their feature collection
		 * and across feature collections (ie, first feature collection features then second, etc)
		 * as determined by @a feature_to_collection_map.
		 * This ensures that the ReconstructionGeometry's are exported in the same order as the
		 * features they were reconstructed from.
		 */
		template <class ReconstructionGeometryType>
		void
		group_reconstruction_geometries_with_their_feature(
				std::list< FeatureGeometryGroup<ReconstructionGeometryType> > &grouped_recon_geoms_seq,
				const std::vector<const ReconstructionGeometryType *> &reconstruction_geometry_seq,
				const feature_handle_to_collection_map_type &feature_to_collection_map);


		/**
		 * Fills @a grouped_features_seq with the sorted contents of @a grouped_recon_geoms_seq.
		 * 
		 * @a grouped_recon_geoms_seq is sorted by feature collection. 
		 */
		template <class ReconstructionGeometryType>
		void
		group_feature_geom_groups_with_their_collection(
				const feature_handle_to_collection_map_type &feature_handle_to_collection_map,
				std::list< FeatureCollectionFeatureGroup<ReconstructionGeometryType> > &grouped_features_seq,
				const std::list< FeatureGeometryGroup<ReconstructionGeometryType> > &grouped_recon_geoms_seq);


		/**
		 * Creates an output filename for each entry in @a grouped_features_seq and stores
		 * in @a output_filenames.
		 *
		 * The order of filenames in @a output_filenames matches the order of groups
		 * in @a grouped_features_seq.
		 */
		template <class ReconstructionGeometryType>
		void
		get_output_filenames(
			std::vector<QString> &output_filenames,
			const QString &output_filename,
			const std::list< FeatureCollectionFeatureGroup<ReconstructionGeometryType> > &grouped_features_seq,
			bool export_separate_output_directory_per_input_file);


		/**
		 * Builds filename as "<export_path>/<collection_filename>_<export_filename>".
		 */
		QString
		build_flat_structure_filename(
			const QString &export_path,
			const QString &collection_filename,
			const QString &export_filename);


		/**
		 * Builds filename as "<export_path>/<collection_filename>/<export_filename>".
		 *
		 * Creates "<export_path>/<collection_filename>/" directory if it doesn't exist.
		 */
		QString
		build_folder_structure_filename(
			const QString &export_path,
			const QString &collection_filename,
			const QString &export_filename);
	}


	////////////////////
	// Implementation //
	////////////////////


	namespace ReconstructionGeometryExportImpl
	{
		/**
		 * Predicate to determine if @a FeatureCollectionFeatureGroup object has specific file pointer.
		 */
		template <class ReconstructionGeometryType>
		class ContainsSameFilePointerPredicate
		{
		public:
			explicit
			ContainsSameFilePointerPredicate(const GPlatesFileIO::File::Reference * file_ptr_):
				file_ptr(file_ptr_)
			{  }


			bool 
			operator()(
				const FeatureCollectionFeatureGroup<ReconstructionGeometryType>& elem) const
			{
				return elem.file_ptr == file_ptr;
			}


		private:
			const GPlatesFileIO::File::Reference *file_ptr;
		};


		/**
		 * Compares feature handle pointers of two @a ReconstructionGeometry derived objects.
		 *
		 * Sorting features according to the order they appear in their feature collection and
		 * across feature collections (ie, first feature collection features then second, etc).
		 * This ensures that the ReconstructionGeometry's are exported in the same order as the
		 * features they were reconstructed from.
		 */
		template <class ReconstructionGeometryType>
		class SortByFeatureOrderInCollections
		{
		public:

			//! Pointer-to-data-member determines which file offset (main or coverage) to use in comparison.
			explicit
			SortByFeatureOrderInCollections(
					const feature_handle_to_collection_map_type &feature_handle_to_collection_map) :
				d_feature_handle_to_collection_map(feature_handle_to_collection_map)
			{  }

			bool
			operator()(
					const ReconstructionGeometryType *lhs_recon_geom,
					const ReconstructionGeometryType *rhs_recon_geom) const
			{
				boost::optional<GPlatesModel::FeatureHandle *> lhs_feature = 
						GPlatesAppLogic::ReconstructionGeometryUtils::get_feature_handle_ptr(lhs_recon_geom);
				boost::optional<GPlatesModel::FeatureHandle *> rhs_feature = 
						GPlatesAppLogic::ReconstructionGeometryUtils::get_feature_handle_ptr(rhs_recon_geom);

				// If not both valid features then just rely on boost::optional comparison.
				if (!(lhs_feature && rhs_feature))
				{
					return lhs_feature < rhs_feature;
				}

				const feature_handle_to_collection_map_type::const_iterator lhs_map_iter =
						d_feature_handle_to_collection_map.find(lhs_feature.get());
				const feature_handle_to_collection_map_type::const_iterator rhs_map_iter =
						d_feature_handle_to_collection_map.find(rhs_feature.get());
				if (lhs_map_iter == d_feature_handle_to_collection_map.end())
				{
					return rhs_map_iter != d_feature_handle_to_collection_map.end();
				}

				if (rhs_map_iter == d_feature_handle_to_collection_map.end())
				{
					return false;
				}

				const unsigned int lhs_feature_order = lhs_map_iter->second.second;
				const unsigned int rhs_feature_order = rhs_map_iter->second.second;

				return lhs_feature_order < rhs_feature_order;
			}

		private:
			const feature_handle_to_collection_map_type &d_feature_handle_to_collection_map;
		};


		/**
		 * Returns a unique list of files that reference the visible ReconstructionGeometry objects.
		 * Result is stored in @a referenced_files.
		 */
		template <class ReconstructionGeometryType>
		void
		get_unique_list_of_referenced_files(
				referenced_files_collection_type &referenced_files,
				const std::vector<const ReconstructionGeometryType *> &reconstruction_geometry_seq,
				const feature_handle_to_collection_map_type &feature_handle_to_collection_map)
		{
			// Iterate through the list of ReconstructionGeometry objects and build up a unique list of
			// feature collection files referenced by them.
			typename std::vector<const ReconstructionGeometryType *>::const_iterator recon_geom_iter;
			for (recon_geom_iter = reconstruction_geometry_seq.begin();
				recon_geom_iter != reconstruction_geometry_seq.end();
				++recon_geom_iter)
			{
				const ReconstructionGeometryType *recon_geom = *recon_geom_iter;

				const boost::optional<GPlatesModel::FeatureHandle *> feature_handle_ptr =
						GPlatesAppLogic::ReconstructionGeometryUtils::get_feature_handle_ptr(recon_geom);
				if (!feature_handle_ptr)
				{
					continue;
				}

				const feature_handle_to_collection_map_type::const_iterator map_iter =
						feature_handle_to_collection_map.find(feature_handle_ptr.get());
				if (map_iter == feature_handle_to_collection_map.end())
				{
					continue;
				}

				const GPlatesFileIO::File::Reference *file = map_iter->second.first;
				referenced_files.push_back(file);
			}

			// Sort in preparation for removing duplicates.
			// We end up sorting on 'const GPlatesFileIO::File::weak_ref' objects.
			std::sort(referenced_files.begin(), referenced_files.end(), boost::lambda::_1 < boost::lambda::_2);

			// Remove duplicates.
			referenced_files.erase(
					std::unique(referenced_files.begin(), referenced_files.end()),
					referenced_files.end());
		}


		template <class ReconstructionGeometryType>
		void
		group_reconstruction_geometries_with_their_feature(
				std::list< FeatureGeometryGroup<ReconstructionGeometryType> > &grouped_recon_geoms_seq,
				const std::vector<const ReconstructionGeometryType *> &reconstruction_geometry_seq,
				const feature_handle_to_collection_map_type &feature_to_collection_map)
		{
			// Copy sequence so we can sort the ReconstructionGeometry objects by feature.
			std::vector<const ReconstructionGeometryType *> recon_geoms_sorted_by_feature(reconstruction_geometry_seq);

			// Sort in preparation for grouping ReconstructionGeometry objects by feature.
			// Also sorting features according to the order they appear in their feature collection and
			// across feature collections (ie, first feature collection features then second, etc).
			// This ensures that the ReconstructionGeometry's are exported in the same order as the
			// features they were reconstructed from.
			// Using 'stable_sort()' to keep the order of ReconstructionGeometry objects *within* a feature.
			std::stable_sort(
					recon_geoms_sorted_by_feature.begin(),
					recon_geoms_sorted_by_feature.end(),
					SortByFeatureOrderInCollections<ReconstructionGeometryType>(feature_to_collection_map));

			GPlatesModel::FeatureHandle::weak_ref current_feature_ref;

			// Iterate through the sorted sequence and put adjacent ReconstructionGeometry objects
			// with the same feature into a group.
			typename std::vector<const ReconstructionGeometryType *>::const_iterator sorted_recon_geom_iter;
			for (sorted_recon_geom_iter = recon_geoms_sorted_by_feature.begin();
				sorted_recon_geom_iter != recon_geoms_sorted_by_feature.end();
				++sorted_recon_geom_iter)
			{
				const ReconstructionGeometryType *recon_geom = *sorted_recon_geom_iter;

				const boost::optional<GPlatesModel::FeatureHandle::weak_ref> feature_ref =
						GPlatesAppLogic::ReconstructionGeometryUtils::get_feature_ref(recon_geom);
				if (!feature_ref || !feature_ref.get())
				{
					continue;
				}

				if (feature_ref.get() != current_feature_ref)
				{
					// Start a new group.
					grouped_recon_geoms_seq.push_back(
							FeatureGeometryGroup<ReconstructionGeometryType>(feature_ref.get()));

					current_feature_ref = feature_ref.get();
				}

				// Add the current ReconstructionGeometry object to the current feature.
				grouped_recon_geoms_seq.back().recon_geoms.push_back(recon_geom);
			}
		}


		template <class ReconstructionGeometryType>
		void
		group_feature_geom_groups_with_their_collection(
			const feature_handle_to_collection_map_type &feature_handle_to_collection_map,
			std::list< FeatureCollectionFeatureGroup<ReconstructionGeometryType> > &grouped_features_seq,
			const std::list< FeatureGeometryGroup<ReconstructionGeometryType> > &grouped_recon_geoms_seq)
		{
			typename std::list< FeatureGeometryGroup<ReconstructionGeometryType> >::const_iterator feature_iter =
					grouped_recon_geoms_seq.begin();
			for (; feature_iter != grouped_recon_geoms_seq.end() ; ++feature_iter)
			{
				GPlatesModel::FeatureHandle::const_weak_ref handle_ref = feature_iter->feature_ref;

				// Need a pointer to use the map. 
				const GPlatesModel::FeatureHandle *handle_ptr = handle_ref.handle_ptr();
				const feature_handle_to_collection_map_type::const_iterator map_iter =
					feature_handle_to_collection_map.find(handle_ptr);
				if (map_iter != feature_handle_to_collection_map.end())
				{
					const GPlatesFileIO::File::Reference *file_ptr = map_iter->second.first;
					
					ContainsSameFilePointerPredicate<ReconstructionGeometryType> predicate(file_ptr);
					typename std::list< FeatureCollectionFeatureGroup<ReconstructionGeometryType> >::iterator it =
							std::find_if(grouped_features_seq.begin(), grouped_features_seq.end(), predicate);
					if (it != grouped_features_seq.end())
					{
						// We found the file_ref in the FeatureCollectionFeatureGroup, so add this group_of_features to it.
						it->feature_geometry_groups.push_back(*feature_iter);
					}
					else
					{
						// We have found a new collection, so create an entry in the grouped_features_seq
						FeatureCollectionFeatureGroup<ReconstructionGeometryType> group_of_features(file_ptr);
						group_of_features.feature_geometry_groups.push_back(*feature_iter);
						grouped_features_seq.push_back(group_of_features);
					}
				}
			}
		}


		template <class ReconstructionGeometryType>
		void
		get_output_filenames(
				std::vector<QString> &output_filenames,
				const QString &filename,
				const std::list< FeatureCollectionFeatureGroup<ReconstructionGeometryType> > &grouped_features_seq,
				bool export_separate_output_directory_per_input_file)
		{
			typename std::list< FeatureCollectionFeatureGroup<ReconstructionGeometryType> >::const_iterator 
					it = grouped_features_seq.begin(),
					end = grouped_features_seq.end();

			QFileInfo export_qfile_info(filename);
			QString export_path = export_qfile_info.absolutePath();
			QString export_filename = export_qfile_info.fileName();

			for (; it != end; ++it)
			{
				const File::Reference *file_ptr = it->file_ptr;	
				FileInfo file_info = file_ptr->get_file_info();
				QFileInfo qfile_info = file_info.get_qfileinfo();
				QString collection_filename = qfile_info.completeBaseName();

				const QString output_filename = export_separate_output_directory_per_input_file
						// Folder-structure output...
						? build_folder_structure_filename(
								export_path, collection_filename, export_filename)
						// Flat-structure output...
						: build_flat_structure_filename(
								export_path, collection_filename, export_filename);

				output_filenames.push_back(output_filename);
			} // iterate over collections
		}
	}
}

#endif // GPLATES_FILE_IO_RECONSTRUCTIONGEOMETRYEXPORTIMPL_H
