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

#include <utility>
#include <vector>
#include <boost/foreach.hpp>
#include <boost/optional.hpp>
#include <boost/variant.hpp>
#include <QFileInfo>
#include <QString>

#include "PyReconstructSnapshot.h"

#include "PyFeatureCollectionFunctionArgument.h"
#include "PyGeometriesOnSphere.h"
#include "PyRotationModel.h"
#include "PythonConverterUtils.h"
#include "PythonHashDefVisitor.h"
#include "PythonPickle.h"
#include "PythonUtils.h"
#include "PythonVariableFunctionArguments.h"

#include "app-logic/GeometryCookieCutter.h"
#include "app-logic/PlateVelocityUtils.h"
#include "app-logic/ReconstructContext.h"
#include "app-logic/ReconstructHandle.h"
#include "app-logic/ReconstructionGeometryUtils.h"
#include "app-logic/ReconstructMethodInterface.h"
#include "app-logic/ReconstructMethodRegistry.h"
#include "app-logic/VelocityDeltaTime.h"
#include "app-logic/VelocityUnits.h"

#include "file-io/FeatureCollectionFileFormatRegistry.h"
#include "file-io/File.h"
#include "file-io/ReconstructedFeatureGeometryExport.h"
#include "file-io/ReconstructedFlowlineExport.h"
#include "file-io/ReconstructedMotionPathExport.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "maths/PolygonOrientation.h"

#include "model/FeatureCollectionHandle.h"
#include "model/types.h"

#include "property-values/GeoTimeInstant.h"

#include "scribe/Scribe.h"

#include "utils/Earth.h"


namespace bp = boost::python;


namespace GPlatesApi
{
	/**
	 * Enumeration to determine how to sort reconstructed static polygons.
	 */
	namespace SortReconstructedStaticPolygons
	{
		enum Value
		{
			BY_PLATE_ID,
			BY_PLATE_AREA
		};

		//! Convert from the GeometryCookieCutter::SortPlates equivalent enumeration.
		boost::optional<Value>
		convert(
				boost::optional<GPlatesAppLogic::GeometryCookieCutter::SortPlates> sort_plates)
		{
			if (!sort_plates)
			{
				return boost::none;
			}

			switch (sort_plates.get())
			{
			case GPlatesAppLogic::GeometryCookieCutter::SORT_BY_PLATE_ID:
				return BY_PLATE_ID;
			case GPlatesAppLogic::GeometryCookieCutter::SORT_BY_PLATE_AREA:
				return BY_PLATE_AREA;
			}

			GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
		}

		//! Convert to the GeometryCookieCutter::SortPlates equivalent enumeration.
		boost::optional<GPlatesAppLogic::GeometryCookieCutter::SortPlates>
		convert(
				boost::optional<Value> sort_reconstructed_static_polygons)
		{
			if (!sort_reconstructed_static_polygons)
			{
				return boost::none;
			}

			switch (sort_reconstructed_static_polygons.get())
			{
			case BY_PLATE_ID:
				return GPlatesAppLogic::GeometryCookieCutter::SORT_BY_PLATE_ID;
			case BY_PLATE_AREA:
				return GPlatesAppLogic::GeometryCookieCutter::SORT_BY_PLATE_AREA;
			}

			GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
		}
	}

	/**
	 * This is called directly from Python via 'ReconstructSnapshot.__init__()'.
	 */
	ReconstructSnapshot::non_null_ptr_type
	reconstruct_snapshot_create(
			const FeatureCollectionSequenceFunctionArgument &reconstructable_features,
			const RotationModelFunctionArgument &rotation_model_argument,
			const GPlatesPropertyValues::GeoTimeInstant &reconstruction_time,
			boost::optional<GPlatesModel::integer_plate_id_type> anchor_plate_id)
	{
		// Time must not be distant past/future.
		if (!reconstruction_time.is_real())
		{
			PyErr_SetString(PyExc_ValueError,
					"Time values cannot be distant-past (float('inf')) or distant-future (float('-inf')).");
			bp::throw_error_already_set();
		}

		return ReconstructSnapshot::create(
				reconstructable_features,
				rotation_model_argument,
				reconstruction_time.value(),
				anchor_plate_id);
	}

	/**
	 * This is called directly from Python via 'ReconstructSnapshot.get_reconstructed_features()'.
	 */
	bp::list
	reconstruct_snapshot_get_reconstructed_features(
			ReconstructSnapshot::non_null_ptr_type reconstruct_snapshot,
			ReconstructType::flags_type reconstruct_types)
	{
		// Reconstruct type flags must correspond to existing flags.
		if ((reconstruct_types & ~ReconstructType::ALL_RECONSTRUCT_TYPES) != 0)
		{
			PyErr_SetString(PyExc_ValueError, "Unknown bit flag specified in reconstruct types.");
			bp::throw_error_already_set();
		}

		bp::list reconstructed_features_list;

		// Group the reconstructed geometries by their feature.
		//
		// Note: The features are sorted in the order of the features in the reconstructable files (and the order across files).
		const std::list<ReconstructSnapshot::feature_geometry_group_type> reconstructed_features =
				reconstruct_snapshot->get_reconstructed_features(reconstruct_types);

		// Output the reconstructed geometries of each feature.
		for (const auto &reconstructed_feature : reconstructed_features)
		{
			// Create a Python feature.
			bp::object feature_object(
					GPlatesModel::FeatureHandle::non_null_ptr_to_const_type(
							reconstructed_feature.feature_ref.handle_ptr()));

			// Python list of reconstructed geometries of the current feature.
			bp::list reconstructed_geometries_list;
			for (auto reconstructed_geometry : reconstructed_feature.recon_geoms)
			{
				reconstructed_geometries_list.append(reconstructed_geometry->get_non_null_pointer_to_const());
			}

			reconstructed_features_list.append(
					bp::make_tuple(feature_object, reconstructed_geometries_list));
		}

		return reconstructed_features_list;
	}

	/**
	 * This is called directly from Python via 'ReconstructSnapshot.get_reconstructed_geometries()'.
	 */
	bp::list
	reconstruct_snapshot_get_reconstructed_geometries(
			ReconstructSnapshot::non_null_ptr_type reconstruct_snapshot,
			ReconstructType::flags_type reconstruct_types,
			bool same_order_as_reconstructable_features)
	{
		// Reconstruct type flags must correspond to existing flags.
		if ((reconstruct_types & ~ReconstructType::ALL_RECONSTRUCT_TYPES) != 0)
		{
			PyErr_SetString(PyExc_ValueError, "Unknown bit flag specified in reconstruct types.");
			bp::throw_error_already_set();
		}

		bp::list reconstructed_geometries_list;

		// Get all reconstructed geometries.
		const std::vector<GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_to_const_type>
				reconstructed_geometries = reconstruct_snapshot->get_reconstructed_geometries(
						reconstruct_types,
						same_order_as_reconstructable_features);

		// Output the reconstructed geometries.
		for (auto reconstructed_geometry : reconstructed_geometries)
		{
			reconstructed_geometries_list.append(reconstructed_geometry);
		}

		return reconstructed_geometries_list;
	}

	/**
	 * This is called directly from Python via 'ReconstructSnapshot.export_reconstructed_geometries()'.
	 */
	void
	reconstruct_snapshot_export_reconstructed_geometries(
			ReconstructSnapshot::non_null_ptr_type reconstruct_snapshot,
			const FilePathFunctionArgument &export_file_name,
			ReconstructType::Value reconstruct_type,
			bool wrap_to_dateline,
			boost::optional<GPlatesMaths::PolygonOrientation::Orientation> force_polygon_orientation)
	{
		// Reconstruct type flag must correspond to an existing flag.
		if ((reconstruct_type & ~ReconstructType::ALL_RECONSTRUCT_TYPES) != 0)
		{
			PyErr_SetString(PyExc_ValueError, "Unknown reconstruct type.");
			bp::throw_error_already_set();
		}

		// And the reconstruct type can be only one flag.
		if (reconstruct_type != ReconstructType::FEATURE_GEOMETRY &&
			reconstruct_type != ReconstructType::MOTION_PATH &&
			reconstruct_type != ReconstructType::FLOWLINE)
		{
			PyErr_SetString(PyExc_ValueError, "Must specify a single reconstruct type.");
			bp::throw_error_already_set();
		}

		reconstruct_snapshot->export_reconstructed_geometries(
				export_file_name,
				reconstruct_type,
				wrap_to_dateline,
				force_polygon_orientation);
	}

	bp::list
	reconstruct_snapshot_get_point_locations(
			ReconstructSnapshot::non_null_ptr_type reconstruct_snapshot,
			PointSequenceFunctionArgument point_seq,
			boost::optional<SortReconstructedStaticPolygons::Value> sort_reconstructed_static_polygons)
	{
		bp::list point_locations_list;

		const GPlatesAppLogic::GeometryCookieCutter partitioner(
				reconstruct_snapshot->get_reconstruction_time(),
				// Note: Since this is a *single* reconstruct type, the reconstructed geometries will already be
				//       in the order of the features in the reconstructable files (and the order across files).
				//       This will be the default search order if 'sort_reconstructed_static_polygons' is none...
				reconstruct_snapshot->get_reconstructed_feature_geometries(),
				boost::none/*resolved_topological_boundaries*/,
				boost::none/*resolved_topological_networks*/,
				SortReconstructedStaticPolygons::convert(sort_reconstructed_static_polygons));

		// Iterate over the sequence of points.
		for (const auto &point : point_seq.get_points())
		{
			// Find which reconstructed static polygons (if any) contains the point.
			//
			// Note: This is none if the point is not inside any reconstructed static polygons. 
			boost::optional<const GPlatesAppLogic::ReconstructionGeometry *> reconstructed_static_polygon =
					partitioner.partition_point(point);

			if (reconstructed_static_polygon)
			{
				point_locations_list.append(GPlatesUtils::get_non_null_pointer(reconstructed_static_polygon.get()));
			}
			else
			{
				point_locations_list.append(bp::object()/*Py_None*/);
			}
		}

		return point_locations_list;
	}

	bp::object
	reconstruct_snapshot_get_point_velocities(
			ReconstructSnapshot::non_null_ptr_type reconstruct_snapshot,
			PointSequenceFunctionArgument point_seq,
			const double &velocity_delta_time,
			GPlatesAppLogic::VelocityDeltaTime::Type velocity_delta_time_type,
			GPlatesAppLogic::VelocityUnits::Value velocity_units,
			const double &earth_radius_in_kms,
			boost::optional<SortReconstructedStaticPolygons::Value> sort_reconstructed_static_polygons,
			bool return_point_locations)
	{
		bp::list point_velocities_list;

		boost::optional<bp::list> point_locations_list;
		if (return_point_locations)
		{
			point_locations_list = bp::list();
		}

		const GPlatesAppLogic::GeometryCookieCutter partitioner(
				reconstruct_snapshot->get_reconstruction_time(),
				// Note: Since this is a *single* reconstruct type, the reconstructed geometries will already be
				//       in the order of the features in the reconstructable files (and the order across files).
				//       This will be the default search order if 'sort_reconstructed_static_polygons' is none...
				reconstruct_snapshot->get_reconstructed_feature_geometries(),
				boost::none/*resolved_topological_boundaries*/,
				boost::none/*resolved_topological_networks*/,
				SortReconstructedStaticPolygons::convert(sort_reconstructed_static_polygons));

		// Iterate over the sequence of points.
		for (const auto &point : point_seq.get_points())
		{
			// Find which reconstructed static polygons (if any) contains the point.
			//
			// Note: This is none if the point is not inside any reconstructed static polygons. 
			boost::optional<const GPlatesAppLogic::ReconstructionGeometry *> reconstruction_geometry =
					partitioner.partition_point(point);
			if (reconstruction_geometry)
			{
				// We only input ReconstructedFeatureGeometry's to the partitioner, so we should only get them as output.
				const GPlatesAppLogic::ReconstructedFeatureGeometry *reconstructed_static_polygon =
						dynamic_cast<const GPlatesAppLogic::ReconstructedFeatureGeometry *>(reconstruction_geometry.get());
				GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
						reconstructed_static_polygon,
						GPLATES_ASSERTION_SOURCE);

				// Get the plate ID from reconstructed static polygon.
				//
				// If we can't get a reconstruction plate ID then we'll just use plate id zero (spin axis)
				// which can still give a non-identity rotation if the anchor plate id is non-zero.
				boost::optional<GPlatesModel::integer_plate_id_type> reconstructed_static_polygon_plate_id =
						reconstructed_static_polygon->reconstruction_plate_id();
				if (!reconstructed_static_polygon_plate_id)
				{
					reconstructed_static_polygon_plate_id = 0;
				}

				const GPlatesMaths::Vector3D velocity = GPlatesAppLogic::PlateVelocityUtils::calculate_velocity_vector(
						point,
						reconstructed_static_polygon_plate_id.get(),
						reconstructed_static_polygon->get_reconstruction_tree_creator(),
						reconstructed_static_polygon->get_reconstruction_time(),
						velocity_delta_time,
						velocity_delta_time_type,
						velocity_units,
						earth_radius_in_kms);

				point_velocities_list.append(velocity);

				if (return_point_locations)
				{
					point_locations_list->append(reconstructed_static_polygon->get_non_null_pointer_to_const());
				}
			}
			else
			{
				// Point is not located inside any reconstructed static polygons.
				point_velocities_list.append(bp::object()/*Py_None*/);

				if (return_point_locations)
				{
					point_locations_list->append(bp::object()/*Py_None*/);
				}
			}
		}

		if (return_point_locations)
		{
			return bp::make_tuple(point_velocities_list, point_locations_list.get());
		}
		else
		{
			return point_velocities_list;
		}
	}


	ReconstructSnapshot::non_null_ptr_type
	ReconstructSnapshot::create(
			const FeatureCollectionSequenceFunctionArgument &reconstructable_features,
			const RotationModelFunctionArgument &rotation_model_argument,
			const double &reconstruction_time,
			boost::optional<GPlatesModel::integer_plate_id_type> anchor_plate_id)
	{
		// Extract the rotation model from the function argument and adapt it to a new one that has 'anchor_plate_id'
		// as its default (which if none, then uses default anchor plate of extracted rotation model instead).
		// This ensures we will reconstruct using the correct anchor plate.
		RotationModel::non_null_ptr_type rotation_model = RotationModel::create(
				rotation_model_argument.get_rotation_model(),
				1/*reconstruction_tree_cache_size*/,
				anchor_plate_id);

		// Get the reconstructable files.
		std::vector<GPlatesFileIO::File::non_null_ptr_type> reconstructable_files;
		reconstructable_features.get_files(reconstructable_files);

		return non_null_ptr_type(
				new ReconstructSnapshot(
						rotation_model,
						reconstructable_files,
						reconstruction_time));
	}

	ReconstructSnapshot::ReconstructSnapshot(
			const RotationModel::non_null_ptr_type &rotation_model,
			const std::vector<GPlatesFileIO::File::non_null_ptr_type> &reconstructable_files,
			const double &reconstruction_time) :
		d_rotation_model(rotation_model),
		d_reconstructable_files(reconstructable_files),
		d_reconstruction_time(reconstruction_time)
	{
		initialise_reconstructed_geometries();
	}

	void
	ReconstructSnapshot::initialise_reconstructed_geometries()
	{
		// Clear the data members we're about to initialise in case this function called during transcribing.
		d_reconstructed_feature_geometries.clear();
		d_reconstructed_motion_paths.clear();
		d_reconstructed_flowlines.clear();

		//
		// Reconstruct the features in the feature collection files.
		//

		const GPlatesAppLogic::ReconstructionTreeCreator reconstruction_tree_creator =
				d_rotation_model->get_reconstruction_tree_creator();

		// Create the context state in which to reconstruct.
		const GPlatesAppLogic::ReconstructMethodInterface::Context reconstruct_method_context(
				GPlatesAppLogic::ReconstructParams(),
				reconstruction_tree_creator);

		GPlatesAppLogic::ReconstructMethodRegistry reconstruct_method_registry;

		// Get the next global reconstruct handle - it'll be stored in each RFG.
		// It doesn't actually matter in our case though.
		const GPlatesAppLogic::ReconstructHandle::type reconstruct_handle =
				GPlatesAppLogic::ReconstructHandle::get_next_reconstruct_handle();

		// For motion paths and flowlines we reconstruct into ReconstructedFeatureGeometry arrays,
		// and later downcast to ReconstructedMotionPath and ReconstructedFlowline arrays.
		std::vector<GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_type> reconstructed_motion_paths;
		std::vector<GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_type> reconstructed_flowlines;

		// Iterate over the files and reconstruct their features.
		//
		// NOTE: Each reconstruct type (ie, FEATURE_GEOMETRY, MOTION_PATH and FLOWLINE) is naturally sorted
		//       in the order of the features in the reconstructable files (and the order across files).
		for (const auto &reconstruct_file : d_reconstructable_files)
		{
			const GPlatesModel::FeatureCollectionHandle::weak_ref feature_collection_ref =
					reconstruct_file->get_reference().get_feature_collection();

			// Iterate over the features in the current file's feature collection.
			for (const auto &feature : *feature_collection_ref)
			{
				const GPlatesModel::FeatureHandle::weak_ref feature_ref = feature->reference();

				// Determine what type of reconstructed output the current feature will produce (if any).
				boost::optional<GPlatesAppLogic::ReconstructMethod::Type> reconstruct_method_type =
						reconstruct_method_registry.get_reconstruct_method_type(feature_ref);
				if (!reconstruct_method_type)
				{
					continue;
				}

				GPlatesAppLogic::ReconstructMethodInterface::non_null_ptr_type reconstruct_method =
						reconstruct_method_registry.create_reconstruct_method(
								reconstruct_method_type.get(),
								feature_ref,
								reconstruct_method_context);

				// Reconstruct the current feature and append to the target reconstructed geometries array.
				//
				// Target reconstructed feature geometries or motion paths or flowlines depending on the reconstruct type.
				if (reconstruct_method_type.get() == GPlatesAppLogic::ReconstructMethod::MOTION_PATH)  // ReconstructType::MOTION_PATH
				{
					reconstruct_method->reconstruct_feature_geometries(
							reconstructed_motion_paths,
							reconstruct_handle,
							reconstruct_method_context,
							d_reconstruction_time);
				}
				else if (reconstruct_method_type.get() == GPlatesAppLogic::ReconstructMethod::FLOWLINE)  // ReconstructType::FLOWLINE
				{
					reconstruct_method->reconstruct_feature_geometries(
							reconstructed_flowlines,
							reconstruct_handle,
							reconstruct_method_context,
							d_reconstruction_time);
				}
				else  // ReconstructType::FEATURE_GEOMETRY
				{
					// Note we don't need to downcast like we do for reconstructed motion paths and flowlines.
					// So we reconstruct directly into the final ReconstructedFeatureGeometry array.
					reconstruct_method->reconstruct_feature_geometries(
							d_reconstructed_feature_geometries,
							reconstruct_handle,
							reconstruct_method_context,
							d_reconstruction_time);
				}
			}
		}

		// From the ReconstructedFeatureGeometry's generated when reconstructing motion paths,
		// downcast those that are ReconstructedMotionPath's.
		//
		// Note that, when motion paths are reconstructed, both ReconstructedMotionPath's and
		// ReconstructedFeatureGeometry's are generated. So this ensures that the concrete ReconstructedFeatureGeometry's
		// are ignored (noting that ReconstructedMotionPath is derived from ReconstructedFeatureGeometry).
		GPlatesAppLogic::ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type_sequence(
						reconstructed_motion_paths.begin(),
						reconstructed_motion_paths.end(),
						d_reconstructed_motion_paths);

		// Downcast the ReconstructedFeatureGeometry's generated when reconstructing flowlines to ReconstructedFlowline's.
		//
		// Note that, unlike motion paths, all ReconstructedFeatureGeometry's generated when reconstructing flowlines
		// are actually ReconstructedFlowline's.
		GPlatesAppLogic::ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type_sequence(
						reconstructed_flowlines.begin(),
						reconstructed_flowlines.end(),
						d_reconstructed_flowlines);
	}

	std::list<ReconstructSnapshot::feature_geometry_group_type>
	ReconstructSnapshot::get_reconstructed_features(
			ReconstructType::flags_type reconstruct_types) const
	{
		// Gather all the reconstructed geometries to output.
		const std::vector<GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_to_const_type>
				reconstructed_geometries = get_reconstructed_geometries(
						reconstruct_types,
						// Don't sort since we'll be doing that ourselves...
						false/*same_order_as_reconstructable_features*/);

		// Group features with their reconstructed geometries.
		//
		// Note: Features are sorted in the order of the features in the reconstructable files (and the order across files).
		std::list<ReconstructSnapshot::feature_geometry_group_type> grouped_reconstructed_geometries;
		find_feature_geometry_groups(grouped_reconstructed_geometries, reconstructed_geometries);

		return grouped_reconstructed_geometries;
	}

	void
	ReconstructSnapshot::find_feature_geometry_groups(
			std::list<ReconstructSnapshot::feature_geometry_group_type> &grouped_reconstructed_geometries,
			const std::vector<GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_to_const_type> &reconstructed_geometries) const
	{
		// Get the sequence of reconstructable files as File pointers.
		std::vector<const GPlatesFileIO::File::Reference *> reconstructable_file_ptrs;
		for (auto reconstructable_file : d_reconstructable_files)
		{
			reconstructable_file_ptrs.push_back(&reconstructable_file->get_reference());
		}

		// Converts reconstructed geometries to raw pointers.
		std::vector<const GPlatesAppLogic::ReconstructedFeatureGeometry *> reconstructed_geometry_ptrs;
		reconstructed_geometry_ptrs.reserve(reconstructed_geometries.size());
		for (auto reconstructed_geometry : reconstructed_geometries)
		{
			reconstructed_geometry_ptrs.push_back(reconstructed_geometry.get());
		}

		//
		// Order the reconstructed geometries according to the order of the features in the feature collections.
		//

		// Get the list of active reconstructable feature collection files that contain
		// the features referenced by the ReconstructedFeatureGeometry objects.
		GPlatesFileIO::ReconstructionGeometryExportImpl::feature_handle_to_collection_map_type feature_to_collection_map;
		GPlatesFileIO::ReconstructionGeometryExportImpl::populate_feature_handle_to_collection_map(
				feature_to_collection_map,
				reconstructable_file_ptrs);

		// Group the ReconstructedFeatureGeometry objects by their feature.
		GPlatesFileIO::ReconstructionGeometryExportImpl::group_reconstruction_geometries_with_their_feature(
				grouped_reconstructed_geometries,
				reconstructed_geometry_ptrs,
				feature_to_collection_map);
	}

	std::vector<GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_to_const_type>
	ReconstructSnapshot::get_reconstructed_geometries(
			ReconstructType::flags_type reconstruct_types,
			bool same_order_as_reconstructable_features) const
	{
		// Gather all the reconstructed geometries to output.
		std::vector<GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_to_const_type> reconstructed_geometries;

		if (same_order_as_reconstructable_features &&
			// If there's only one reconstruct type specified then it's already sorted (by reconstructable features)...
			!ReconstructType::only_one_reconstruct_type(reconstruct_types))
		{
			// Group the reconstructed geometries by their feature.
			//
			// Note: The features are sorted in the order of the features in the reconstructable files (and the order across files).
			const std::list<ReconstructSnapshot::feature_geometry_group_type> reconstructed_features =
					get_reconstructed_features(reconstruct_types);

			// Output the reconstructed geometries of each feature.
			for (const auto &reconstructed_feature : reconstructed_features)
			{
				for (auto reconstructed_geometry : reconstructed_feature.recon_geoms)
				{
					reconstructed_geometries.push_back(reconstructed_geometry->get_non_null_pointer_to_const());
				}
			}
		}
		else
		{
			if ((reconstruct_types & ReconstructType::FEATURE_GEOMETRY) != 0)
			{
				reconstructed_geometries.insert(
						reconstructed_geometries.end(),
						d_reconstructed_feature_geometries.begin(),
						d_reconstructed_feature_geometries.end());
			}

			if ((reconstruct_types & ReconstructType::MOTION_PATH) != 0)
			{
				reconstructed_geometries.insert(
						reconstructed_geometries.end(),
						d_reconstructed_motion_paths.begin(),
						d_reconstructed_motion_paths.end());
			}

			if ((reconstruct_types & ReconstructType::FLOWLINE) != 0)
			{
				reconstructed_geometries.insert(
						reconstructed_geometries.end(),
						d_reconstructed_flowlines.begin(),
						d_reconstructed_flowlines.end());
			}
		}

		return reconstructed_geometries;
	}

	void
	ReconstructSnapshot::export_reconstructed_geometries(
			const FilePathFunctionArgument &export_file_path,
			ReconstructType::Value reconstruct_type,
			bool wrap_to_dateline,
			boost::optional<GPlatesMaths::PolygonOrientation::Orientation> force_polygon_orientation) const
	{
		const QString export_file_name = export_file_path.get_file_path();

		// Get the sequence of reconstructable files as File pointers.
		std::vector<const GPlatesFileIO::File::Reference *> reconstructable_file_ptrs;
		for (const auto &reconstructable_file : d_reconstructable_files)
		{
			reconstructable_file_ptrs.push_back(&reconstructable_file->get_reference());
		}

		// Get the sequence of reconstruction files (if any) from the rotation model.
		std::vector<GPlatesFileIO::File::non_null_ptr_type> reconstruction_files;
		d_rotation_model->get_files(reconstruction_files);

		// Convert the sequence of reconstruction files to File pointers.
		std::vector<const GPlatesFileIO::File::Reference *> reconstruction_file_ptrs;
		for (const auto &reconstruction_file : reconstruction_files)
		{
			reconstruction_file_ptrs.push_back(&reconstruction_file->get_reference());
		}

		// Export based on the reconstructed type requested by the caller.
		//
		// Note: We don't need to sort the reconstructed geometries because the following exports will do that.
		switch (reconstruct_type)
		{
		case ReconstructType::FEATURE_GEOMETRY:
			export_reconstructed_feature_geometries(
					export_file_name,
					reconstructable_file_ptrs,
					reconstruction_file_ptrs,
					d_rotation_model->get_reconstruction_tree_creator().get_default_anchor_plate_id(),
					d_reconstruction_time,
					wrap_to_dateline,
					force_polygon_orientation);
			break;

		case ReconstructType::MOTION_PATH:
			export_reconstructed_motion_paths(
					export_file_name,
					reconstructable_file_ptrs,
					reconstruction_file_ptrs,
					d_rotation_model->get_reconstruction_tree_creator().get_default_anchor_plate_id(),
					d_reconstruction_time,
					wrap_to_dateline);
			break;

		case ReconstructType::FLOWLINE:
			export_reconstructed_flowlines(
					export_file_name,
					reconstructable_file_ptrs,
					reconstruction_file_ptrs,
					d_rotation_model->get_reconstruction_tree_creator().get_default_anchor_plate_id(),
					d_reconstruction_time,
					wrap_to_dateline);
			break;

		default:
			GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
			break;
		}
	}

	void
	ReconstructSnapshot::export_reconstructed_feature_geometries(
			const QString &export_file_name,
			const std::vector<const GPlatesFileIO::File::Reference *> &reconstructable_file_ptrs,
			const std::vector<const GPlatesFileIO::File::Reference *> &reconstruction_file_ptrs,
			const GPlatesModel::integer_plate_id_type &anchor_plate_id,
			const double &reconstruction_time,
			bool export_wrap_to_dateline,
			boost::optional<GPlatesMaths::PolygonOrientation::Orientation> force_polygon_orientation) const
	{
		// Converts to raw pointers.
		std::vector<const GPlatesAppLogic::ReconstructedFeatureGeometry *> reconstructed_feature_geometry_ptrs;
		reconstructed_feature_geometry_ptrs.reserve(d_reconstructed_feature_geometries.size());
		for (auto rfg : d_reconstructed_feature_geometries)
		{
			reconstructed_feature_geometry_ptrs.push_back(rfg.get());
		}

		GPlatesFileIO::FeatureCollectionFileFormat::Registry file_format_registry;
		const GPlatesFileIO::ReconstructedFeatureGeometryExport::Format format =
						GPlatesFileIO::ReconstructedFeatureGeometryExport::get_export_file_format(
										QFileInfo(export_file_name),
										file_format_registry);

		// The API docs state that dateline wrapping should be ignored except for Shapefile.
		//
		// For example, we don't want to pollute real-world data with dateline vertices when
		// using GMT software (since it can handle 3D globe data, whereas ESRI handles only 2D).
		if (format != GPlatesFileIO::ReconstructedFeatureGeometryExport::SHAPEFILE)
		{
			export_wrap_to_dateline = false;
		}

		// Export the reconstructed feature geometries.
		GPlatesFileIO::ReconstructedFeatureGeometryExport::export_reconstructed_feature_geometries(
					export_file_name,
					format,
					reconstructed_feature_geometry_ptrs,
					reconstructable_file_ptrs,
					reconstruction_file_ptrs,
					anchor_plate_id,
					reconstruction_time,
					// If exporting to Shapefile and there's only *one* input reconstructable file then
					// shapefile attributes in input reconstructable file will get copied to output...
					true/*export_single_output_file*/,
					false/*export_per_input_file*/, // We only generate a single output file.
					false/*export_output_directory_per_input_file*/, // We only generate a single output file.
					force_polygon_orientation,
					export_wrap_to_dateline);
	}

	void
	ReconstructSnapshot::export_reconstructed_motion_paths(
			const QString &export_file_name,
			const std::vector<const GPlatesFileIO::File::Reference *> &reconstructable_file_ptrs,
			const std::vector<const GPlatesFileIO::File::Reference *> &reconstruction_file_ptrs,
			const GPlatesModel::integer_plate_id_type &anchor_plate_id,
			const double &reconstruction_time,
			bool export_wrap_to_dateline) const
	{
		// Converts to raw pointers.
		std::vector<const GPlatesAppLogic::ReconstructedMotionPath *> reconstructed_motion_path_ptrs;
		reconstructed_motion_path_ptrs.reserve(d_reconstructed_motion_paths.size());
		for (auto rmp : d_reconstructed_motion_paths)
		{
			reconstructed_motion_path_ptrs.push_back(rmp.get());
		}

		GPlatesFileIO::FeatureCollectionFileFormat::Registry file_format_registry;
		const GPlatesFileIO::ReconstructedMotionPathExport::Format format =
						GPlatesFileIO::ReconstructedMotionPathExport::get_export_file_format(
										QFileInfo(export_file_name),
										file_format_registry);

		// The API docs state that dateline wrapping should be ignored except for Shapefile.
		//
		// For example, we don't want to pollute real-world data with dateline vertices when
		// using GMT software (since it can handle 3D globe data, whereas ESRI handles only 2D).
		if (format != GPlatesFileIO::ReconstructedMotionPathExport::SHAPEFILE)
		{
			export_wrap_to_dateline = false;
		}

		// Export the reconstructed motion paths.
		GPlatesFileIO::ReconstructedMotionPathExport::export_reconstructed_motion_paths(
					export_file_name,
					format,
					reconstructed_motion_path_ptrs,
					reconstructable_file_ptrs,
					reconstruction_file_ptrs,
					anchor_plate_id,
					reconstruction_time,
					true/*export_single_output_file*/,
					false/*export_per_input_file*/, // We only generate a single output file.
					false/*export_output_directory_per_input_file*/, // We only generate a single output file.
					export_wrap_to_dateline);
	}

	void
	ReconstructSnapshot::export_reconstructed_flowlines(
			const QString &export_file_name,
			const std::vector<const GPlatesFileIO::File::Reference *> &reconstructable_file_ptrs,
			const std::vector<const GPlatesFileIO::File::Reference *> &reconstruction_file_ptrs,
			const GPlatesModel::integer_plate_id_type &anchor_plate_id,
			const double &reconstruction_time,
			bool export_wrap_to_dateline) const
	{
		// Converts to raw pointers.
		std::vector<const GPlatesAppLogic::ReconstructedFlowline *> reconstructed_flowline_ptrs;
		reconstructed_flowline_ptrs.reserve(d_reconstructed_flowlines.size());
		for (auto rf : d_reconstructed_flowlines)
		{
			reconstructed_flowline_ptrs.push_back(rf.get());
		}

		GPlatesFileIO::FeatureCollectionFileFormat::Registry file_format_registry;
		const GPlatesFileIO::ReconstructedFlowlineExport::Format format =
						GPlatesFileIO::ReconstructedFlowlineExport::get_export_file_format(
										QFileInfo(export_file_name),
										file_format_registry);

		// The API docs state that dateline wrapping should be ignored except for Shapefile.
		//
		// For example, we don't want to pollute real-world data with dateline vertices when
		// using GMT software (since it can handle 3D globe data, whereas ESRI handles only 2D).
		if (format != GPlatesFileIO::ReconstructedFlowlineExport::SHAPEFILE)
		{
			export_wrap_to_dateline = false;
		}
			
		// Export the reconstructed flowlines.
		GPlatesFileIO::ReconstructedFlowlineExport::export_reconstructed_flowlines(
				export_file_name,
				format,
				reconstructed_flowline_ptrs,
				reconstructable_file_ptrs,
				reconstruction_file_ptrs,
				anchor_plate_id,
				reconstruction_time,
				true/*export_single_output_file*/,
				false/*export_per_input_file*/, // We only generate a single output file.
				false/*export_output_directory_per_input_file*/, // We only generate a single output file.
				export_wrap_to_dateline);
	}


	GPlatesScribe::TranscribeResult
	ReconstructSnapshot::transcribe_construct_data(
			GPlatesScribe::Scribe &scribe,
			GPlatesScribe::ConstructObject<ReconstructSnapshot> &reconstruct_snapshot)
	{
		if (scribe.is_saving())
		{
			save_construct_data(scribe, reconstruct_snapshot.get_object());
		}
		else // loading
		{
			GPlatesScribe::LoadRef<RotationModel::non_null_ptr_type> rotation_model;
			std::vector<GPlatesFileIO::File::non_null_ptr_type> reconstructable_files;
			double reconstruction_time;
			if (!load_construct_data(
					scribe,
					rotation_model,
					reconstructable_files,
					reconstruction_time))
			{
				return scribe.get_transcribe_result();
			}

			// Create the reconstruct model.
			reconstruct_snapshot.construct_object(
					rotation_model,
					reconstructable_files,
					reconstruction_time);
		}

		return GPlatesScribe::TRANSCRIBE_SUCCESS;
	}


	GPlatesScribe::TranscribeResult
	ReconstructSnapshot::transcribe(
			GPlatesScribe::Scribe &scribe,
			bool transcribed_construct_data)
	{
		if (!transcribed_construct_data)
		{
			if (scribe.is_saving())
			{
				save_construct_data(scribe, *this);
			}
			else // loading
			{
				GPlatesScribe::LoadRef<RotationModel::non_null_ptr_type> rotation_model;
				if (!load_construct_data(
						scribe,
						rotation_model,
						d_reconstructable_files,
						d_reconstruction_time))
				{
					return scribe.get_transcribe_result();
				}
				d_rotation_model = rotation_model.get();

				// Initialise reconstructed geometries (based on the construct parameters we just loaded).
				//
				// Note: The existing reconstructed geometries in 'this' reconstructable snapshot must be old data
				//       because 'transcribed_construct_data' is false (ie, it was not transcribed) and so 'this'
				//       object must've been created first (using unknown constructor arguments) and *then* transcribed.
				initialise_reconstructed_geometries();
			}
		}

		return GPlatesScribe::TRANSCRIBE_SUCCESS;
	}


	void
	ReconstructSnapshot::save_construct_data(
			GPlatesScribe::Scribe &scribe,
			const ReconstructSnapshot &reconstruct_snapshot)
	{
		// Save the rotation model.
		scribe.save(TRANSCRIBE_SOURCE, reconstruct_snapshot.d_rotation_model, "rotation_model");

		const GPlatesScribe::ObjectTag files_tag("files");

		// Save number of reconstructable files.
		const unsigned int num_files = reconstruct_snapshot.d_reconstructable_files.size();
		scribe.save(TRANSCRIBE_SOURCE, num_files, files_tag.sequence_size());

		// Save the reconstructable files (feature collections and their filenames).
		for (unsigned int file_index = 0; file_index < num_files; ++file_index)
		{
			const auto feature_collection_file = reconstruct_snapshot.d_reconstructable_files[file_index];

			const GPlatesModel::FeatureCollectionHandle::non_null_ptr_type feature_collection(
					feature_collection_file->get_reference().get_feature_collection().handle_ptr());
			const QString filename =
					feature_collection_file->get_reference().get_file_info().get_qfileinfo().absoluteFilePath();

			scribe.save(TRANSCRIBE_SOURCE, feature_collection, files_tag[file_index]("feature_collection"));
			scribe.save(TRANSCRIBE_SOURCE, filename, files_tag[file_index]("filename"));
		}

		// Save the reconstruction time.
		scribe.save(TRANSCRIBE_SOURCE, reconstruct_snapshot.d_reconstruction_time, "reconstruction_time");
	}


	bool
	ReconstructSnapshot::load_construct_data(
			GPlatesScribe::Scribe &scribe,
			GPlatesScribe::LoadRef<RotationModel::non_null_ptr_type> &rotation_model,
			std::vector<GPlatesFileIO::File::non_null_ptr_type> &reconstructable_files,
			double &reconstruction_time)
	{
		// Load the rotation model.
		rotation_model = scribe.load<RotationModel::non_null_ptr_type>(TRANSCRIBE_SOURCE, "rotation_model");
		if (!rotation_model.is_valid())
		{
			return false;
		}

		const GPlatesScribe::ObjectTag files_tag("files");

		// Number of reconstructable files.
		unsigned int num_files;
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, num_files, files_tag.sequence_size()))
		{
			return false;
		}

		// Load the reconstructable files (feature collections and their filenames).
		for (unsigned int file_index = 0; file_index < num_files; ++file_index)
		{
			GPlatesScribe::LoadRef<GPlatesModel::FeatureCollectionHandle::non_null_ptr_type> feature_collection =
					scribe.load<GPlatesModel::FeatureCollectionHandle::non_null_ptr_type>(
							TRANSCRIBE_SOURCE,
							files_tag[file_index]("feature_collection"));
			if (!feature_collection.is_valid())
			{
				return false;
			}

			QString filename;
			if (!scribe.transcribe(TRANSCRIBE_SOURCE, filename, files_tag[file_index]("filename")))
			{
				return false;
			}

			reconstructable_files.push_back(
					GPlatesFileIO::File::create_file(GPlatesFileIO::FileInfo(filename), feature_collection));
		}

		// Load the reconstruction time.
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, reconstruction_time, "reconstruction_time"))
		{
			return false;
		}

		return true;
	}
}

	
void
export_reconstruct_snapshot()
{
	// An enumeration nested within 'pygplates' (ie, current) module.
	bp::enum_<GPlatesApi::ReconstructType::Value>("ReconstructType")
			.value("feature_geometry", GPlatesApi::ReconstructType::FEATURE_GEOMETRY)
			.value("motion_path", GPlatesApi::ReconstructType::MOTION_PATH)
			.value("flowline", GPlatesApi::ReconstructType::FLOWLINE);


	// An enumeration nested within 'pygplates' (ie, current) module.
	bp::enum_<GPlatesApi::SortReconstructedStaticPolygons::Value>("SortReconstructedStaticPolygons")
			.value("by_plate_id", GPlatesApi::SortReconstructedStaticPolygons::BY_PLATE_ID)
			.value("by_plate_area", GPlatesApi::SortReconstructedStaticPolygons::BY_PLATE_AREA);

	// Enable boost::optional<GPlatesApi::SortReconstructedStaticPolygons::Value> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::register_optional_conversion<GPlatesApi::SortReconstructedStaticPolygons::Value>();


	//
	// ReconstructSnapshot - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesApi::ReconstructSnapshot,
			GPlatesApi::ReconstructSnapshot::non_null_ptr_type,
			boost::noncopyable>(
					"ReconstructSnapshot",
					"A snapshot of reconstructed regular features (including motion paths and flowlines) at a specific geological time.\n"
					"\n"
					"A *ReconstructSnapshot* can also be `pickled <https://docs.python.org/3/library/pickle.html>`_.\n"
					"\n"
					".. versionadded:: 0.48\n",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::reconstruct_snapshot_create,
						bp::default_call_policies(),
						(bp::arg("reconstructable_features"),
							bp::arg("rotation_model"),
							bp::arg("reconstruction_time"),
							bp::arg("anchor_plate_id") = boost::optional<GPlatesModel::integer_plate_id_type>())),
			"__init__(reconstructable_features, rotation_model, reconstruction_time, [anchor_plate_id])\n"
			"  Create from reconstructable features and a rotation model at a specific reconstruction time.\n"
			"\n"
			"  :param reconstructable_features: The reconstructable features as a feature collection, "
			"or filename, or feature, or sequence of features, or a sequence (eg, ``list`` or ``tuple``) "
			"of any combination of those four types.\n"
			"  :type reconstructable_features: FeatureCollection, or str, or os.PathLike, or Feature, "
			"or sequence of Feature, or sequence of any combination of those four types\n"
			"  :param rotation_model: A rotation model. Or a rotation feature collection, or a rotation filename, "
			"or a rotation feature, or a sequence of rotation features, or a sequence of any combination of those four types.\n"
			"  :type rotation_model: RotationModel. Or FeatureCollection, or str, or os.PathLike, "
			"or Feature, or sequence of Feature, or sequence of any combination of those four types\n"
			"  :param reconstruction_time: the specific geological time to reconstruct to\n"
			"  :type reconstruction_time: float or GeoTimeInstant\n"
			"  :param anchor_plate_id: The anchored plate id used for all reconstructions. "
			"Defaults to the default anchor plate of *rotation_model* (or zero if *rotation_model* is not a :class:`RotationModel`).\n"
			"  :type anchor_plate_id: int\n"
			"\n"
			"  Create a reconstruct snapshot by reconstructing features at a specific reconstruction time:\n"
			"  ::\n"
			"\n"
			"    reconstruction_time = 100\n"
			"    reconstructable_features = pygplates.FeatureCollection('reconstructable_features.gpml')\n"
			"    rotation_model = pygplates.RotationModel('rotations.rot')\n"
			"    reconstruct_snapshot = pygplates.ReconstructSnapshot(reconstructable_features, rotation_model, reconstruction_time)\n")
		// Pickle support...
		//
		// Note: This adds an __init__ method accepting a single argument (of type 'bytes') that supports pickling.
		//       So we define this *after* (higher priority) the other __init__ methods in case one of them accepts a single argument
		//       of type bp::object (which, being more general, would otherwise obscure the __init__ that supports pickling).
		.def(GPlatesApi::PythonPickle::PickleDefVisitor<GPlatesApi::ReconstructSnapshot::non_null_ptr_type>())
		.def("get_reconstructed_features",
				&GPlatesApi::reconstruct_snapshot_get_reconstructed_features,
				(bp::arg("reconstruct_types") = GPlatesApi::ReconstructType::DEFAULT_RECONSTRUCT_TYPES),
				"get_reconstructed_features([reconstruct_types=pygplates.ReconstructType.feature_geometry])\n"
				"  Returns the reconstructed geometries of the requested type(s) grouped by their feature.\n"
				"\n"
				"  :param reconstruct_types: specifies which types of features to reconstruct - defaults "
				"to reconstructing only regular features (not motion paths or flowlines)\n"
				"  :type reconstruct_types: a bitwise combination of any of pygplates.ReconstructType.feature_geometry, "
				"pygplates.ReconstructType.motion_path or pygplates.ReconstructType.flowline\n"
				"  :returns: a list of tuples, where each tuple contains a :class:`Feature` and a ``list`` of reconstructed geometries "
				"(each reconstructed geometry is a :class:`reconstructed feature geometry <ReconstructedFeatureGeometry>`, "
				":class:`reconstructed motion path <ReconstructedMotionPath>` or :class:`reconstructed flowline <ReconstructedFlowline>` - "
				"depending on the optional argument *reconstruct_types*)\n"
				"  :rtype: list[tuple[Feature, list[ReconstructedFeatureGeometry | ReconstructedMotionPath | ReconstructedFlowline]]]\n"
				"  :raises: ValueError if *reconstruct_types* (if specified) contains a flag that "
				"is not one of ``pygplates.ReconstructType.feature_geometry``, ``pygplates.ReconstructType.motion_path`` or "
				"``pygplates.ReconstructType.flowline``\n"
				"\n"
				"  This can be useful (compared to :meth:`get_reconstructed_geometries`) when a :class:`feature <Feature>` has "
				"more than one (present day) geometry and hence more than one reconstructed geometry.\n"
				"\n"
				"  .. note:: The returned features (and associated reconstructed geometries) are sorted in the order of their respective reconstructable "
				"features (see :meth:`constructor<__init__>`). This includes the order across any reconstructable feature collections/files.\n"
				"\n"
				"  .. note:: The *reconstruct_types* argument accepts more than one reconstruct type, unlike :meth:`export_reconstructed_geometries` and :func:`reconstruct`.\n"
				"\n"
				"  To get the :class:`reconstructed feature geometries <ReconstructedFeatureGeometry>` grouped by their :class:`Feature`:\n"
				"  ::\n"
				"\n"
				"    reconstructed_features = reconstruct_snapshot.get_reconstructed_features()\n"
				"    for feature, feature_reconstructed_geometries in reconstructed_features:\n"
				"        # Note that 'feature' is the same as 'feature_reconstructed_geometry.get_feature()'.\n"
				"        for feature_reconstructed_geometry in feature_reconstructed_geometries:\n"
				"            ...\n"
				"\n"
				"  .. seealso:: :meth:`get_reconstructed_geometries`\n")
		.def("get_reconstructed_geometries",
				&GPlatesApi::reconstruct_snapshot_get_reconstructed_geometries,
				(bp::arg("reconstruct_types") = GPlatesApi::ReconstructType::DEFAULT_RECONSTRUCT_TYPES,
					bp::arg("same_order_as_reconstructable_features") = false),
				"get_reconstructed_geometries([reconstruct_types=pygplates.ReconstructType.feature_geometry], [same_order_as_reconstructable_features=False])\n"
				"  Returns the reconstructed geometries of the requested type(s).\n"
				"\n"
				"  :param reconstruct_types: specifies which types of features to reconstruct - defaults "
				"to reconstructing only regular features (not motion paths or flowlines)\n"
				"  :type reconstruct_types: a bitwise combination of any of pygplates.ReconstructType.feature_geometry, "
				"pygplates.ReconstructType.motion_path or pygplates.ReconstructType.flowline\n"
				"  :param same_order_as_reconstructable_features: whether the returned reconstructed geometries are sorted in "
				"the order of the reconstructable features (including order across reconstructable files, if there were any) - "
				"defaults to ``False``\n"
				"  :type same_order_as_reconstructable_features: bool\n"
				"  :returns: the :class:`reconstructed feature geometries <ReconstructedFeatureGeometry>`, "
				":class:`reconstructed motion paths <ReconstructedMotionPath>` and :class:`reconstructed flowlines <ReconstructedFlowline>` "
				"(depending on the optional argument *reconstruct_types*) - by default :class:`reconstructed motion paths <ReconstructedMotionPath>` "
				"and :class:`reconstructed flowlines <ReconstructedFlowline>` are excluded\n"
				"  :rtype: list[ReconstructedFeatureGeometry | ReconstructedMotionPath | ReconstructedFlowline]\n"
				"  :raises: ValueError if *reconstruct_types* (if specified) contains a flag that "
				"is not one of ``pygplates.ReconstructType.feature_geometry``, ``pygplates.ReconstructType.motion_path`` or "
				"``pygplates.ReconstructType.flowline``\n"
				"\n"
				"  .. note:: If *same_order_as_reconstructable_features* is ``True`` then the returned reconstructed geometries are sorted in the order of their "
				"respective reconstructable features (see :meth:`constructor<__init__>`). This includes the order across any reconstructable feature collections/files.\n"
				"\n"
				"  .. note:: The *reconstruct_types* argument accepts more than one reconstruct type, unlike :meth:`export_reconstructed_geometries` and :func:`reconstruct`.\n"
				"\n"
				"  .. seealso:: :meth:`get_reconstructed_features`\n")
		.def("export_reconstructed_geometries",
				&GPlatesApi::reconstruct_snapshot_export_reconstructed_geometries,
				(bp::arg("export_filename"),
					bp::arg("reconstruct_type") = GPlatesApi::ReconstructType::DEFAULT_RECONSTRUCT_TYPE,
					bp::arg("wrap_to_dateline") = true,
					bp::arg("force_polygon_orientation") = boost::optional<GPlatesMaths::PolygonOrientation::Orientation>()),
				"export_reconstructed_geometries(export_filename, [reconstruct_type=pygplates.ReconstructType.feature_geometry], "
				"[wrap_to_dateline=True], [force_polygon_orientation])\n"
				"  Exports the reconstructed geometries of the requested type(s) to a file.\n"
				"\n"
				"  :param export_filename: the name of the export file\n"
				"  :type export_filename: str, or os.PathLike\n"
				"  :param reconstruct_type: specifies which type of features to export - defaults "
				"to exporting only regular features (not motion paths or flowlines)\n"
				"  :type reconstruct_type: pygplates.ReconstructType.feature_geometry, "
				"pygplates.ReconstructType.motion_path or pygplates.ReconstructType.flowline\n"
				"  :param wrap_to_dateline: Whether to wrap/clip reconstructed geometries to the dateline "
				"(currently ignored unless exporting to an ESRI Shapefile format *file*). Defaults to ``True``.\n"
				"  :type wrap_to_dateline: bool\n"
				"  :param force_polygon_orientation: Optionally force boundary orientation to "
				"clockwise (``PolygonOnSphere.Orientation.clockwise``) or "
				"counter-clockwise (``PolygonOnSphere.Orientation.counter_clockwise``). "
				"Only applies to reconstructed feature geometries (excludes *motion paths* and *flowlines*) that are polygons. "
				"Note that ESRI Shapefiles always use *clockwise* orientation (and so ignore this parameter).\n"
				"  :type force_polygon_orientation: int\n"
				"  :raises: ValueError if *reconstruct_type* (if specified) is not **one** of ``pygplates.ReconstructType.feature_geometry``, "
				"``pygplates.ReconstructType.motion_path`` or ``pygplates.ReconstructType.flowline``\n"
				"\n"
				"  .. note:: *reconstruct_type* must be a **single** reconstruct type.  This is different than "
				":meth:`get_reconstructed_geometries` and :meth:`get_reconstructed_features` which can specify multiple types.\n"
				"\n"
				"  The following *export* file formats are currently supported:\n"
				"\n"
				"  =============================== =======================\n"
				"  Export File Format              Filename Extension     \n"
				"  =============================== =======================\n"
				"  ESRI Shapefile                  '.shp'                 \n"
				"  GeoJSON                         '.geojson' or '.json'  \n"
				"  OGR GMT                         '.gmt'                 \n"
				"  GMT xy                          '.xy'                  \n"
				"  =============================== =======================\n"
				"\n"
				"  .. note:: Reconstructed geometries are exported in the order of their respective reconstructable features "
				"(see :meth:`constructor<__init__>`). This includes the order across any reconstructable feature collections/files.\n")
		.def("get_point_locations",
				&GPlatesApi::reconstruct_snapshot_get_point_locations,
				(bp::arg("points"),
					bp::arg("sort_reconstructed_static_polygons") = GPlatesApi::SortReconstructedStaticPolygons::BY_PLATE_ID),
				"get_point_locations(points, [sort_reconstructed_static_polygons=pygplates.SortReconstructedStaticPolygons.by_plate_id])\n"
				"  Returns the reconstructed static polygons that contain the specified points.\n"
				"\n"
				"  :param points: sequence of points at which to find containing reconstructed static polygons\n"
				"  :type points: any sequence of PointOnSphere or LatLonPoint or tuple (latitude,longitude), in degrees, or tuple (x,y,z)\n"
				"  :param sort_reconstructed_static_polygons: optional sort order of reconstructed static polygons "
				"(defaults to ``pygplates.SortReconstructedStaticPolygons.by_plate_id``)\n"
				"  :type sort_reconstructed_static_polygons: pygplates.SortReconstructedStaticPolygons.by_plate_id or "
				"pygplates.SortReconstructedStaticPolygons.by_plate_area or None\n"
				"  :returns: the reconstructed static polygon containing each point (``None`` for each point *outside* all "
				"reconstructed static polygons)\n"
				"  :rtype: list[ReconstructedFeatureGeometry | None]\n"
				"\n"
				"  Reconstructed static polygons are :class:`reconstructed feature geometries <ReconstructedFeatureGeometry>` that have "
				":class:`polygon <PolygonOnSphere>` geometries (other geometry types are ignored since only polygons can contain points). "
				"The reconstructed feature geometries are obtained from :meth:`get_reconstructed_geometries` with "
				"``reconstruct_types=pygplates.ReconstructType.feature_geometry`` and ``same_order_as_reconstructable_features=True``.\n"
				"\n"
				"  .. note:: Each point that is *outside* all reconstructed static polygons will have a point location (reconstructed static polygon) of ``None``.\n"
				"\n"
				"  Reconstructed static polygons can overlap each other at reconstruction times in the past (unlike :class:`resolved topological plates <TopologicalSnapshot>` "
				"which typically do not overlap). This means a point could be contained inside more than one reconstructed static polygon, but only the first one will be "
				"returned for that point. However, you can change the search order of reconstructed static polygons using *sort_reconstructed_static_polygons*:\n"
				"\n"
				"  - ``pygplates.SortReconstructedStaticPolygons.by_plate_id``: Search by *plate ID* (from highest to lowest).\n"
				"  - ``pygplates.SortReconstructedStaticPolygons.by_plate_area``: Search by *plate area* (from highest to lowest).\n"
				"  - ``None``: Search using the original order. This is the order of reconstructable features (see :meth:`constructor<__init__>`), "
				"and includes the order across any reconstructable feature collections/files.\n"
				"\n"
				"  .. note:: The default search order is ``pygplates.SortReconstructedStaticPolygons.by_plate_id`` to ensure the results are the same regardless "
				"of the order of reconstructable features (specified in the :meth:`constructor<__init__>`).\n"
				"\n"
				"  To associate each point with the reconstructed static polygon containing it:\n"
				"  ::\n"
				"\n"
				"    reconstructed_static_polygons = reconstruct_snapshot.get_point_locations(points)\n"
				"\n"
				"    for point_index in range(len(points)):\n"
				"        point = points[point_index]\n"
				"        reconstructed_static_polygon = reconstructed_static_polygons[point_index]\n"
				"\n"
				"        if reconstructed_static_polygon:  # if point is inside a reconstructed static polygon\n"
				"            ...\n"
				"\n"
				"  .. versionadded:: 0.50\n")
		.def("get_point_velocities",
				&GPlatesApi::reconstruct_snapshot_get_point_velocities,
				(bp::arg("points"),
					bp::arg("velocity_delta_time") = 1.0,
					bp::arg("velocity_delta_time_type") = GPlatesAppLogic::VelocityDeltaTime::T_PLUS_DELTA_T_TO_T,
					bp::arg("velocity_units") = GPlatesAppLogic::VelocityUnits::KMS_PER_MY,
					bp::arg("earth_radius_in_kms") = GPlatesUtils::Earth::MEAN_RADIUS_KMS,
					bp::arg("sort_reconstructed_static_polygons") = GPlatesApi::SortReconstructedStaticPolygons::BY_PLATE_ID,
					bp::arg("return_point_locations") = false),
				"get_point_velocities(points, "
				"[velocity_delta_time=1.0], [velocity_delta_time_type=pygplates.VelocityDeltaTimeType.t_plus_delta_t_to_t], "
				"[velocity_units=pygplates.VelocityUnits.kms_per_my], [earth_radius_in_kms=pygplates.Earth.mean_radius_in_kms], "
				"[sort_reconstructed_static_polygons=pygplates.SortReconstructedStaticPolygons.by_plate_id], [return_point_locations=False])\n"
				"  Returns the velocities of the specified points (as determined by the reconstructed static polygons that contain them).\n"
				"\n"
				"  :param points: sequence of points at which to calculate velocities\n"
				"  :type points: any sequence of PointOnSphere or LatLonPoint or tuple (latitude,longitude), in degrees, or tuple (x,y,z)\n"
				"  :param velocity_delta_time: The time delta used to calculate velocities (defaults to 1 Myr).\n"
				"  :type velocity_delta_time: float\n"
				"  :param velocity_delta_time_type: How the two velocity times are calculated relative to the reconstruction time. "
				"This includes [t+dt, t], [t, t-dt] and [t+dt/2, t-dt/2]. Defaults to [t+dt, t].\n"
				"  :type velocity_delta_time_type: VelocityDeltaTimeType.t_plus_delta_t_to_t, "
				"VelocityDeltaTimeType.t_to_t_minus_delta_t or VelocityDeltaTimeType.t_plus_minus_half_delta_t\n"
				"  :param velocity_units: whether to return velocities as *kilometres per million years* or "
				"*centimetres per year* (defaults to *kilometres per million years*)\n"
				"  :type velocity_units: VelocityUnits.kms_per_my or VelocityUnits.cms_per_yr\n"
				"  :param earth_radius_in_kms: the radius of the Earth in *kilometres* (defaults to ``pygplates.Earth.mean_radius_in_kms``)\n"
				"  :type earth_radius_in_kms: float\n"
				"  :param sort_reconstructed_static_polygons: optional sort order of reconstructed static polygons "
				"(defaults to ``pygplates.SortReconstructedStaticPolygons.by_plate_id``)\n"
				"  :type sort_reconstructed_static_polygons: pygplates.SortReconstructedStaticPolygons.by_plate_id or "
				"pygplates.SortReconstructedStaticPolygons.by_plate_area or None\n"
				"  :param return_point_locations: whether to also return the reconstructed static polygon that contains each point - defaults to ``False``\n"
				"  :type return_point_locations: bool\n"
				"  :returns: the velocity of each point (``None`` for each point *outside* all reconstructed static polygons), and "
				"(if *return_point_locations* is ``True``) also the reconstructed static polygon containing each point "
				"(``None`` for points outside)\n"
				"  :rtype: list[Vector3D | None], or tuple[list[Vector3D | None], list[ReconstructedFeatureGeometry | None]]\n"
				"\n"
				"  Reconstructed static polygons are :class:`reconstructed feature geometries <ReconstructedFeatureGeometry>` that have "
				":class:`polygon <PolygonOnSphere>` geometries (other geometry types are ignored since only polygons can contain points). "
				"The reconstructed feature geometries are obtained from :meth:`get_reconstructed_geometries` with "
				"``reconstruct_types=pygplates.ReconstructType.feature_geometry`` and ``same_order_as_reconstructable_features=True``.\n"
				"\n"
				"  .. note:: Each point that is *outside* all reconstructed static polygons will have a velocity of ``None``, and optionally "
				"(if *return_point_locations* is ``True``) have a point location (reconstructed static polygon) of ``None``.\n"
				"\n"
				"  Reconstructed static polygons can overlap each other at reconstruction times in the past (unlike :class:`resolved topological plates <TopologicalSnapshot>` "
				"which typically do not overlap). This means a point could be contained inside more than one reconstructed static polygon, but only the first one will be "
				"returned for that point. However, you can change the search order of reconstructed static polygons using *sort_reconstructed_static_polygons*:\n"
				"\n"
				"  - ``pygplates.SortReconstructedStaticPolygons.by_plate_id``: Search by *plate ID* (from highest to lowest).\n"
				"  - ``pygplates.SortReconstructedStaticPolygons.by_plate_area``: Search by *plate area* (from highest to lowest).\n"
				"  - ``None``: Search using the original order. This is the order of reconstructable features (see :meth:`constructor<__init__>`), "
				"and includes the order across any reconstructable feature collections/files.\n"
				"\n"
				"  .. note:: The default search order is ``pygplates.SortReconstructedStaticPolygons.by_plate_id`` to ensure the results are the same regardless "
				"of the order of reconstructable features (specified in the :meth:`constructor<__init__>`).\n"
				"\n"
				"  To associate each point with its velocity and the reconstructed static polygon containing it:\n"
				"  ::\n"
				"\n"
				"    velocities, reconstructed_static_polygons = reconstruct_snapshot.get_point_velocities(\n"
				"            points,\n"
				"            return_point_locations=True)\n"
				"\n"
				"    for point_index in range(len(points)):\n"
				"        point = points[point_index]\n"
				"        velocity = velocities[point_index]\n"
				"        reconstructed_static_polygon = reconstructed_static_polygons[point_index]\n"
				"\n"
				"        if velocity:  # if point is inside a reconstructed static polygon\n"
				"            ...\n"
				"\n"
				"  .. note:: It is more efficient to call ``reconstruct_snapshot.get_point_velocities(points, return_point_locations=True)`` to get both velocities and "
				"point locations than it is to call both ``reconstruct_snapshot.get_point_velocities(points)`` and ``reconstruct_snapshot.get_point_locations(points)``.\n"
				"\n"
				"  .. versionadded:: 0.50\n")
		.def("get_rotation_model",
				&GPlatesApi::ReconstructSnapshot::get_rotation_model,
				"get_rotation_model()\n"
				"  Return the rotation model used internally.\n"
				"\n"
				"  :rtype: RotationModel\n"
				"\n"
				"  .. note:: The :meth:`default anchor plate ID<RotationModel.get_default_anchor_plate_id>` of the returned rotation model "
				"may be different to that of the rotation model passed into the :meth:`constructor<__init__>` if an anchor plate ID was specified "
				"in the :meth:`constructor<__init__>`.\n")
		.def("get_reconstruction_time",
				&GPlatesApi::ReconstructSnapshot::get_reconstruction_time,
				"get_reconstruction_time()\n"
				"  Return the reconstruction time of this snapshot.\n"
				"\n"
				"  :rtype: float\n")
		.def("get_anchor_plate_id",
				&GPlatesApi::ReconstructSnapshot::get_anchor_plate_id,
				"get_anchor_plate_id()\n"
				"  Return the anchor plate ID (see :meth:`constructor<__init__>`).\n"
				"\n"
				"  :rtype: int\n"
				"\n"
				"  .. note:: This is the same as the :meth:`default anchor plate ID<RotationModel.get_default_anchor_plate_id>` "
				"of :meth:`get_rotation_model`.\n")
		// Make hash and comparisons based on C++ object identity (not python object identity)...
		.def(GPlatesApi::ObjectIdentityHashDefVisitor())
	;

	// Register to/from Python conversions of non_null_intrusive_ptr<> including const/non-const and boost::optional.
	GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<GPlatesApi::ReconstructSnapshot>();
}
