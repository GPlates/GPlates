/**
 * Copyright (C) 2024 The University of Sydney, Australia
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

#ifndef GPLATES_API_PY_RECONSTRUCT_MODEL_H
#define GPLATES_API_PY_RECONSTRUCT_MODEL_H

#include <vector>
#include <map>
#include <boost/optional.hpp>

#include "PyFeatureCollectionFunctionArgument.h"
#include "PyReconstructSnapshot.h"
#include "PyRotationModel.h"

#include "file-io/File.h"

#include "global/python.h"

#include "maths/types.h"

#include "model/FeatureCollectionHandle.h"
#include "model/types.h"

#include "property-values/GeoTimeInstant.h"

#include "scribe/ScribeLoadRef.h"
 // Try to only include the heavyweight "Scribe.h" in '.cc' files where possible.
#include "scribe/Transcribe.h"

#include "utils/KeyValueCache.h"
#include "utils/ReferenceCount.h"


namespace GPlatesApi
{
	/**
	 * Reconstructed features, and their associated rotation model.
	 */
	class ReconstructModel :
			public GPlatesUtils::ReferenceCount<ReconstructModel>
	{
	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<ReconstructModel> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const ReconstructModel> non_null_ptr_to_const_type;


		/**
		 * Create a reconstruct model from reconstructable features and associated rotation model.
		 */
		static
		non_null_ptr_type
		create(
				const FeatureCollectionSequenceFunctionArgument &reconstructable_features,
				// Note we're using 'RotationModelFunctionArgument::function_argument_type' instead of
				// just 'RotationModelFunctionArgument' since we want to know if it's an existing RotationModel...
				const RotationModelFunctionArgument::function_argument_type &rotation_model_argument,
				boost::optional<GPlatesModel::integer_plate_id_type> anchor_plate_id,
				boost::optional<unsigned int> reconstruct_snapshot_cache_size);


		/**
		 * Returns the reconstruct snapshot (reconstructed features) for the specified time (creating and caching them if necessary).
		 */
		ReconstructSnapshot::non_null_ptr_type
		get_reconstruct_snapshot(
				const double &reconstruction_time);


		/**
		 * Return the feature collections used to create this reconstruct model.
		 */
		const std::vector<GPlatesModel::FeatureCollectionHandle::non_null_ptr_type> &
		get_reconstructable_feature_collections() const
		{
			return d_reconstructable_feature_collections;
		}


		/**
		 * Return the feature collection files used to create this reconstruct model.
		 *
		 * NOTE: Any feature collections that did not come from files will have empty filenames.
		 */
		const std::vector<GPlatesFileIO::File::non_null_ptr_type> &
		get_reconstructable_files() const
		{
			return d_reconstructable_files;
		}


		/**
		 * Get the rotation model.
		 */
		RotationModel::non_null_ptr_type
		get_rotation_model() const
		{
			return d_rotation_model;
		}

		/**
		 * Returns the anchor plate ID.
		 */
		GPlatesModel::integer_plate_id_type
		get_anchor_plate_id() const
		{
			return get_rotation_model()->get_reconstruction_tree_creator().get_default_anchor_plate_id();
		}

	private:

		//! Typedef for a least-recently used cache mapping reconstruction times to reconstruct snapshots.
		typedef GPlatesUtils::KeyValueCache<GPlatesMaths::real_t/*time*/, ReconstructSnapshot::non_null_ptr_type> reconstruct_snapshots_type;


		/**
		 * Rotation model associated with reconstructable features.
		 */
		RotationModel::non_null_ptr_type d_rotation_model;

		/**
		 * Feature collections/files.
		 */
		std::vector<GPlatesModel::FeatureCollectionHandle::non_null_ptr_type> d_reconstructable_feature_collections;
		std::vector<GPlatesFileIO::File::non_null_ptr_type> d_reconstructable_files;

		/**
		 * Number of reconstruct snapshots to cache (at different time instants) - none means unlimited.
		 */
		boost::optional<unsigned int> d_reconstruct_snapshot_cache_size;

		/**
		 * Cache of reconstruct snapshots at various time instants.
		 */
		reconstruct_snapshots_type d_reconstruct_snapshot_cache;


		ReconstructModel(
				const RotationModel::non_null_ptr_type &rotation_model,
				const std::vector<GPlatesFileIO::File::non_null_ptr_type> &reconstructable_files,
				boost::optional<unsigned int> reconstruct_snapshot_cache_size);

		/**
		 * Set up for reconstruction once the files have been constructed.
		 */
		void
		initialise();

		/**
		 * Reconstructed features for the specified time and returns them as a reconstruct snapshot.
		 */
		ReconstructSnapshot::non_null_ptr_type
		create_reconstruct_snapshot(
				const GPlatesMaths::real_t &reconstruction_time);

	private: // Transcribe...

		friend class GPlatesScribe::Access;

		static
		GPlatesScribe::TranscribeResult
		transcribe_construct_data(
				GPlatesScribe::Scribe &scribe,
				GPlatesScribe::ConstructObject<ReconstructModel> &reconstruct_model);

		GPlatesScribe::TranscribeResult
		transcribe(
				GPlatesScribe::Scribe &scribe,
				bool transcribed_construct_data);

		static
		void
		save_construct_data(
				GPlatesScribe::Scribe &scribe,
				const ReconstructModel &reconstruct_model);

		static
		bool
		load_construct_data(
				GPlatesScribe::Scribe &scribe,
				GPlatesScribe::LoadRef<RotationModel::non_null_ptr_type> &rotation_model,
				std::vector<GPlatesFileIO::File::non_null_ptr_type> &reconstructable_files,
				boost::optional<unsigned int> &reconstruct_snapshot_cache_size);
	};
}

#endif // GPLATES_API_PY_RECONSTRUCT_MODEL_H
