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

#ifndef GPLATES_API_PY_RECONSTRUCT_SNAPSHOT_H
#define GPLATES_API_PY_RECONSTRUCT_SNAPSHOT_H

#include <utility>  // std::pair
#include <vector>
#include <map>
#include <boost/optional.hpp>

#include "PyFilePathFunctionArgument.h"
#include "PyRotationModel.h"
#include "PyFeatureCollectionFunctionArgument.h"

#include "app-logic/ReconstructedFeatureGeometry.h"
#include "app-logic/ReconstructedFlowline.h"
#include "app-logic/ReconstructedMotionPath.h"

#include "file-io/File.h"
#include "file-io/ReconstructionGeometryExportImpl.h"

#include "global/python.h"

#include "maths/PolygonOrientation.h"
#include "maths/types.h"

#include "model/FeatureHandle.h"
#include "model/types.h"

#include "property-values/GeoTimeInstant.h"

#include "scribe/ScribeLoadRef.h"
 // Try to only include the heavyweight "Scribe.h" in '.cc' files where possible.
#include "scribe/Transcribe.h"

#include "utils/ReferenceCount.h"


namespace GPlatesApi
{
	/**
	 * Enumeration to determine which reconstructed feature geometry types to output.
	 *
	 * This can be used as a bitwise combination of flags.
	 */
	namespace ReconstructType
	{
		enum Value
		{
			FEATURE_GEOMETRY = (1 << 0),
			MOTION_PATH = (1 << 1),
			FLOWLINE = (1 << 2)
		};

		// Use this (integer) type when extracting flags (of reconstructed feature geometry types).
		typedef unsigned int flags_type;

		// Mask of allowed bit flags for 'ReconstructType'.
		constexpr flags_type ALL_RECONSTRUCT_TYPES = (FEATURE_GEOMETRY | MOTION_PATH | FLOWLINE);

		// Default reconstructed feature geometry type.
		//
		// Note: 'pygplates.reconstruct()' and 'pygplates.ReconstructSnapshot.export_reconstructed_geometries()'
		//       only accept a *single* type.
		constexpr Value DEFAULT_RECONSTRUCT_TYPE = FEATURE_GEOMETRY;

		// Default reconstructed feature geometry types excludes motion paths and flowlines.
		//
		// Note: 'pygplates.ReconstructSnapshot.get_reconstructed_geometries()' accepts *multiple* types.
		constexpr flags_type DEFAULT_RECONSTRUCT_TYPES = DEFAULT_RECONSTRUCT_TYPE;
	};


	/**
	 * Snapshot, at a specific reconstruction time, of the reconstruction of reconstructable features.
	 */
	class ReconstructSnapshot :
			public GPlatesUtils::ReferenceCount<ReconstructSnapshot>
	{
	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<ReconstructSnapshot> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const ReconstructSnapshot> non_null_ptr_to_const_type;

		//! Typedef for reconstructed geometries grouped by feature.
		typedef GPlatesFileIO::ReconstructionGeometryExportImpl::FeatureGeometryGroup<GPlatesAppLogic::ReconstructedFeatureGeometry>
				feature_geometry_group_type;


		/**
		 * Create a reconstruct snapshot, at specified reconstruction time, from reconstructable features and associated rotation model.
		 */
		static
		non_null_ptr_type
		create(
				const FeatureCollectionSequenceFunctionArgument &reconstructable_features_argument,
				const RotationModelFunctionArgument &rotation_model_argument,
				const double &reconstruction_time,
				boost::optional<GPlatesModel::integer_plate_id_type> anchor_plate_id);

		/**
		 * Create a reconstruct snapshot, at specified reconstruction time, from reconstructable files and
		 * associated rotation model (with embedded anchor plate ID).
		 */
		static
		non_null_ptr_type
		create(
				const std::vector<GPlatesFileIO::File::non_null_ptr_type> &reconstructable_files,
				const RotationModel::non_null_ptr_type &rotation_model,
				const double &reconstruction_time)
		{
			return non_null_ptr_type(
					new ReconstructSnapshot(
							rotation_model,
							reconstructable_files,
							reconstruction_time));
		}


		/**
		 * Get reconstructed feature geometries.
		 */
		const std::vector<GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_type> &
		get_reconstructed_feature_geometries() const
		{
			return d_reconstructed_feature_geometries;
		}

		/**
		 * Get reconstructed motion paths.
		 */
		const std::vector<GPlatesAppLogic::ReconstructedMotionPath::non_null_ptr_type> &
		get_reconstructed_motion_paths() const
		{
			return d_reconstructed_motion_paths;
		}

		/**
		 * Get reconstructed flowlines.
		 */
		const std::vector<GPlatesAppLogic::ReconstructedFlowline::non_null_ptr_type> &
		get_reconstructed_flowlines() const
		{
			return d_reconstructed_flowlines;
		}

		/**
		 * Get features grouped with their reconstructed geometries (feature geometries, motion paths and flowlines).
		 *
		 * The features are sorted in the order of the features in the reconstructable files (and the order across files).
		 *
		 * By default returns only features associated with reconstructed feature geometries (excludes motion paths and flowlines).
		 */
		std::list<feature_geometry_group_type>
		get_reconstructed_features(
				ReconstructType::flags_type reconstruct_types = ReconstructType::DEFAULT_RECONSTRUCT_TYPES) const;

		/**
		 * Get reconstructed geometries (feature geometries, motion paths and flowlines).
		 *
		 * The reconstructed geometries are NOT sorted (use @a get_reconstructed_features if you want sorting).
		 *
		 * By default returns only reconstructed feature geometries (excludes motion paths and flowlines).
		 */
		std::vector<GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_type>
		get_reconstructed_geometries(
				ReconstructType::flags_type reconstruct_types = ReconstructType::DEFAULT_RECONSTRUCT_TYPES) const;

		/**
		 * Export reconstructed geometries (feature geometries, motion paths and flowlines) to a file.
		 *
		 * If @a wrap_to_dateline is true then wrap/clip reconstructed geometries to the dateline
		 * (currently ignored unless exporting to an ESRI Shapefile format file). Defaults to true.
		 *
		 * If @a force_boundary_orientation is not none then force boundary orientation (clockwise or counter-clockwise)
		 * of those reconstructed feature geometries (excludes *motion paths* and *flowlines*) that are polygons.
		 * Currently ignored by ESRI Shapefile which always uses clockwise.
		 *
		 * By default exports only reconstructed feature geometries (excludes motion paths and flowlines).
		 */
		void
		export_reconstructed_geometries(
				const FilePathFunctionArgument &export_file_name,
				ReconstructType::Value reconstruct_type = ReconstructType::DEFAULT_RECONSTRUCT_TYPE,
				bool wrap_to_dateline = true,
				boost::optional<GPlatesMaths::PolygonOrientation::Orientation> force_boundary_orientation = boost::none) const;


		/**
		 * Get the reconstructable files.
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
		 * Returns the reconstruction time of this snapshot.
		 */
		double
		get_reconstruction_time() const
		{
			return d_reconstruction_time;
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

		/**
		 * Rotation model associated with reconstructable features.
		 */
		RotationModel::non_null_ptr_type d_rotation_model;

		/**
		 * Reconstructable files.
		 */
		std::vector<GPlatesFileIO::File::non_null_ptr_type> d_reconstructable_files;

		/**
		 * Reconstruction time of snapshot.
		 */
		double d_reconstruction_time;

		std::vector<GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_type> d_reconstructed_feature_geometries;
		std::vector<GPlatesAppLogic::ReconstructedMotionPath::non_null_ptr_type> d_reconstructed_motion_paths;
		std::vector<GPlatesAppLogic::ReconstructedFlowline::non_null_ptr_type> d_reconstructed_flowlines;


		ReconstructSnapshot(
				const RotationModel::non_null_ptr_type &rotation_model,
				const std::vector<GPlatesFileIO::File::non_null_ptr_type> &reconstructable_files,
				const double &reconstruction_time);

		/**
		 * Set up for reconstruction once the reconstructable files/parameters have been constructed.
		 */
		void
		initialise_reconstructed_geometries();

		/**
		 * Group features with their reconstructed geometries.
		 *
		 * Note: Features are sorted in the order of the features in the reconstructable files (and the order across files).
		 */
		void
		find_feature_geometry_groups(
				std::list<ReconstructSnapshot::feature_geometry_group_type> &grouped_reconstructed_geometries,
				const std::vector<GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_type> &reconstructed_geometries) const;

		void
		export_reconstructed_feature_geometries(
				const QString &export_file_name,
				const std::vector<const GPlatesFileIO::File::Reference *> &reconstructable_file_ptrs,
				const std::vector<const GPlatesFileIO::File::Reference *> &reconstruction_file_ptrs,
				const GPlatesModel::integer_plate_id_type &anchor_plate_id,
				const double &reconstruction_time,
				bool export_wrap_to_dateline,
				boost::optional<GPlatesMaths::PolygonOrientation::Orientation> force_boundary_orientation) const;

		void
		export_reconstructed_motion_paths(
				const QString &export_file_name,
				const std::vector<const GPlatesFileIO::File::Reference *> &reconstructable_file_ptrs,
				const std::vector<const GPlatesFileIO::File::Reference *> &reconstruction_file_ptrs,
				const GPlatesModel::integer_plate_id_type &anchor_plate_id,
				const double &reconstruction_time,
				bool export_wrap_to_dateline) const;

		void
		export_reconstructed_flowlines(
				const QString &export_file_name,
				const std::vector<const GPlatesFileIO::File::Reference *> &reconstructable_file_ptrs,
				const std::vector<const GPlatesFileIO::File::Reference *> &reconstruction_file_ptrs,
				const GPlatesModel::integer_plate_id_type &anchor_plate_id,
				const double &reconstruction_time,
				bool export_wrap_to_dateline) const;

	private: // Transcribe...

		friend class GPlatesScribe::Access;

		static
		GPlatesScribe::TranscribeResult
		transcribe_construct_data(
				GPlatesScribe::Scribe &scribe,
				GPlatesScribe::ConstructObject<ReconstructSnapshot> &reconstruct_snapshot);

		GPlatesScribe::TranscribeResult
		transcribe(
				GPlatesScribe::Scribe &scribe,
				bool transcribed_construct_data);

		static
		void
		save_construct_data(
				GPlatesScribe::Scribe &scribe,
				const ReconstructSnapshot &reconstruct_snapshot);

		static
		bool
		load_construct_data(
				GPlatesScribe::Scribe &scribe,
				GPlatesScribe::LoadRef<RotationModel::non_null_ptr_type> &rotation_model,
				std::vector<GPlatesFileIO::File::non_null_ptr_type> &reconstructable_files,
				double &reconstruction_time);
	};
}

#endif // GPLATES_API_PY_RECONSTRUCT_SNAPSHOT_H
