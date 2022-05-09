/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2020 The University of Sydney, Australia
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

#ifndef GPLATES_API_PYTOPOLOGICALMODEL_H
#define GPLATES_API_PYTOPOLOGICALMODEL_H

#include <vector>
#include <map>
#include <boost/optional.hpp>

#include "PyResolveTopologyParameters.h"
#include "PyRotationModel.h"
#include "PyTopologicalFeatureCollectionFunctionArgument.h"
#include "PyTopologicalSnapshot.h"

#include "app-logic/ReconstructContext.h"
#include "app-logic/ReconstructMethodRegistry.h"
#include "app-logic/ResolvedTopologicalBoundary.h"
#include "app-logic/ResolvedTopologicalLine.h"
#include "app-logic/ResolvedTopologicalNetwork.h"
#include "app-logic/ScalarCoverageTimeSpan.h"
#include "app-logic/TimeSpanUtils.h"
#include "app-logic/TopologyNetworkParams.h"
#include "app-logic/TopologyReconstruct.h"

#include "file-io/File.h"

#include "global/python.h"

#include "maths/types.h"

#include "model/FeatureCollectionHandle.h"
#include "model/types.h"

#include "property-values/GeoTimeInstant.h"
#include "property-values/ValueObjectType.h"

#include "utils/ReferenceCount.h"


namespace GPlatesApi
{
	/**
	 * Contains topology-reconstructed history of a geometry (and optional scalars) over a time span.
	 */
	class ReconstructedGeometryTimeSpan :
			public GPlatesUtils::ReferenceCount<ReconstructedGeometryTimeSpan>
	{
	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<ReconstructedGeometryTimeSpan> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const ReconstructedGeometryTimeSpan> non_null_ptr_to_const_type;


		/**
		 * Create a reconstructed geometry time span (with optional reconstructed scalars).
		 */
		static
		non_null_ptr_type
		create(
				GPlatesAppLogic::TopologyReconstruct::GeometryTimeSpan::non_null_ptr_type geometry_time_span,
				GPlatesAppLogic::ScalarCoverageTimeSpan::non_null_ptr_type scalar_coverage_time_span)
		{
			return non_null_ptr_type(new ReconstructedGeometryTimeSpan(geometry_time_span, scalar_coverage_time_span));
		}


		/**
		 * Return time span of reconstructed geometries.
		 */
		GPlatesAppLogic::TopologyReconstruct::GeometryTimeSpan::non_null_ptr_type
		get_geometry_time_span() const
		{
			return d_geometry_time_span;
		}


		/**
		 * Return time span of reconstructed scalar values (and their associated scalar types).
		 */
		GPlatesAppLogic::ScalarCoverageTimeSpan::non_null_ptr_type
		get_scalar_coverage_time_span() const
		{
			return d_scalar_coverage_time_span;
		}


		/**
		 * The base class interface for deactivating geometry points as they are reconstructed forward and/or backward in time.
		 *
		 * This is a wrapper class (around 'GPlatesAppLogic::TopologyReconstruct::DeactivatePoint') so that it can be inherited
		 * by a Python class (if user decides to implement their own derivation) and invoked from C++ (through base class pointer).
		 *
		 * Note that we didn't use the traditional documented method of inheriting from both Base and bp::wrapper<Base>.
		 * This is because we are using smart pointers (GPlatesUtils::non_null_intrusive_ptr) and just cannot seem to get
		 * it to work with bp::wrapper (probably because bp::wrapper not designed to be used as a smart pointer and trying to convert
		 * a non-smart pointer HeldType to smart pointer using bp::register_ptr_to_python doesn't appear to work).
		 *
		 * Note that there's also a default C++ derived class (inheriting from 'GPlatesAppLogic::TopologyReconstruct::DeactivatePoint').
		 * That's implemented on the C++ side (exposed to Python though).
		 */
		struct DeactivatePointWrapper :
				public GPlatesAppLogic::TopologyReconstruct::DeactivatePoint
		{
			typedef GPlatesUtils::non_null_intrusive_ptr<DeactivatePointWrapper> non_null_ptr_type;
			typedef GPlatesUtils::non_null_intrusive_ptr<const DeactivatePointWrapper> non_null_ptr_to_const_type;

			/**
			 * Our class wrapper is:
			 *
			 *   bp::class_<
			 *       GPlatesAppLogic::TopologyReconstruct::DeactivatePoint,
			 *       GPlatesApi::ReconstructedGeometryTimeSpan::DeactivatePointWrapper::non_null_ptr_type,
			 *       boost::noncopyable>
			 *
			 * The documentation for bp::class_ dictates that:
			 * 
			 *     when HeldType is a smart pointer to a class derived from T, the initial PyObject*
			 *     argument must be supplied by all of HeldType's exposed constructors
			 * 
			 * ...and our HeldType is 'DeactivatePointWrapper::non_null_ptr_type' which is a smart pointer
			 * to a class derived from 'GPlatesAppLogic::TopologyReconstruct::DeactivatePoint'.
			 *
			 * Note that we don't also need to provide a 'PyObject *' version of the copy constructor
			 * (because of 'boost::noncopyable').
			 *
			 * In any case we need the Python object pointer so that we can later call "deactivate" on it
			 * (again, because the traditional approach of inheriting bp::wrapper, which handles all that
			 * for us, does not work in our smart pointer situation).
			 */
			explicit
			DeactivatePointWrapper(
					PyObject *self) :
				d_self(self)
			{  }

			/**
			 * Return true if the point should be deactivated.
			 */
			virtual
			bool
			deactivate(
					const GPlatesMaths::PointOnSphere &prev_point,
					const GPlatesAppLogic::TopologyPointLocation &prev_location,
					const double &prev_time,
					const GPlatesMaths::PointOnSphere &current_point,
					const GPlatesAppLogic::TopologyPointLocation &current_location,
					const double &current_time) const override
			{
				// Call the "deactivate" method on the derived Python object.
				return boost::python::call_method<bool>(d_self, "deactivate",
						prev_point,
						prev_location,
						prev_time,
						current_point,
						current_location,
						current_time);
			}

		private:
			PyObject *d_self;
		};

	private:

		ReconstructedGeometryTimeSpan(
				GPlatesAppLogic::TopologyReconstruct::GeometryTimeSpan::non_null_ptr_type geometry_time_span,
				GPlatesAppLogic::ScalarCoverageTimeSpan::non_null_ptr_type scalar_coverage_time_span) :
			d_geometry_time_span(geometry_time_span),
			d_scalar_coverage_time_span(scalar_coverage_time_span)
		{  }


		/**
		 * Reconstructed geometry time span.
		 */
		GPlatesAppLogic::TopologyReconstruct::GeometryTimeSpan::non_null_ptr_type d_geometry_time_span;

		/**
		 * Time span of reconstructed scalar values (and their associated scalar types).
		 */
		GPlatesAppLogic::ScalarCoverageTimeSpan::non_null_ptr_type d_scalar_coverage_time_span;
	};


	/**
	 * Dynamic plates and deforming networks, and their associated rotation model.
	 */
	class TopologicalModel :
			public GPlatesUtils::ReferenceCount<TopologicalModel>
	{
	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<TopologicalModel> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const TopologicalModel> non_null_ptr_to_const_type;


		/**
		 * Create a topological model from topological features and associated rotation model.
		 */
		static
		non_null_ptr_type
		create(
				const TopologicalFeatureCollectionSequenceFunctionArgument &topological_features,
				// Note we're using 'RotationModelFunctionArgument::function_argument_type' instead of
				// just 'RotationModelFunctionArgument' since we want to know if it's an existing RotationModel...
				const RotationModelFunctionArgument::function_argument_type &rotation_model_argument,
				boost::optional<GPlatesModel::integer_plate_id_type> anchor_plate_id,
				boost::optional<ResolveTopologyParameters::non_null_ptr_to_const_type> default_resolve_topology_parameters);


		/**
		 * Returns the topological snapshot (resolved topologies) for the specified time (creating and caching them if necessary).
		 *
		 * Raises ValueError if @a reconstruction_time is not an integral value.
		 */
		TopologicalSnapshot::non_null_ptr_type
		get_topological_snapshot(
				const double &reconstruction_time);


		/**
		 * Reconstruct/deform a geometry or a coverage (geometry + scalars) over a time range.
		 *
		 * The time range is from @a oldest_time (defaults to @a initial_time) to
		 * @a youngest_time (defaults to present day) in increments of @a time_increment (defaults to 1My).
		 *
		 * Returns a reconstructed time span.
		 *
		 * @a scalar_type_to_values_mapping_object can be any of the following:
		 *   (1) a Python 'dict' mapping each 'ScalarType' to a sequence of float, or
		 *   (2) a Python sequence of ('ScalarType', sequence of float) tuples.
		 *   (3) boost::python::object()  // Py_None
		 *
		 * This will raise Python ValueError if @a scalar_type_to_values_mapping_object is not None but:
		 * is empty, or if each scalar type is not mapped to the same number of scalar values, or
		 * number of scalar values is not equal to number of points in @a geometry_object.
		 *
		 * NOTE: Currently @a geometry_object is limited to a PointOnSphere, MultiPointOnSphere or sequence of points.
		 *       In future this will be extended to include polylines and polygons (with interior holes).
		 */
		ReconstructedGeometryTimeSpan::non_null_ptr_type
		reconstruct_geometry(
				boost::python::object geometry_object,
				const GPlatesPropertyValues::GeoTimeInstant &initial_time,
				boost::optional<GPlatesPropertyValues::GeoTimeInstant> oldest_time = boost::none,
				const GPlatesPropertyValues::GeoTimeInstant &youngest_time = GPlatesPropertyValues::GeoTimeInstant(0),
				const double &time_increment = 1,
				boost::optional<GPlatesModel::integer_plate_id_type> reconstruction_plate_id = boost::none,
				boost::python::object scalar_type_to_values_mapping_object = boost::python::object()/*Py_None*/,
				boost::optional<GPlatesAppLogic::TopologyReconstruct::DeactivatePoint::non_null_ptr_to_const_type> deactivate_points =
						GPlatesUtils::static_pointer_cast<const GPlatesAppLogic::TopologyReconstruct::DeactivatePoint>(
								GPlatesAppLogic::TopologyReconstruct::DefaultDeactivatePoint::create()));


		/**
		 * Return the topological feature collections used to create this topological model.
		 */
		const std::vector<GPlatesModel::FeatureCollectionHandle::non_null_ptr_type> &
		get_topological_feature_collections() const
		{
			return d_topological_feature_collections;
		}


		/**
		 * Return the topological feature collection files used to create this topological model.
		 *
		 * NOTE: Any feature collections that did not come from files will have empty filenames.
		 */
		const std::vector<GPlatesFileIO::File::non_null_ptr_type> &
		get_files() const
		{
			return d_topological_files;
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

		//! Typedef for a mapping of (integral) times to topological snapshots (resolved topologies).
		typedef std::map<GPlatesMaths::real_t/*time*/, TopologicalSnapshot::non_null_ptr_type> topological_snapshots_type;

		//! Typedef for a sequence of topological features.
		typedef std::vector<GPlatesModel::FeatureHandle::weak_ref> topological_features_seq_type;

		/**
		 * Typedef for a mapping of topology network parameters to topological network features.
		 *
		 * Network features associated with different parameters need to be resolved separately.
		 */
		typedef std::map<GPlatesAppLogic::TopologyNetworkParams, topological_features_seq_type> topological_network_features_map_type;


		/**
		 * Rotation model associated with topological features.
		 */
		RotationModel::non_null_ptr_type d_rotation_model;

		/**
		 * Topological feature collections/files.
		 */
		std::vector<GPlatesModel::FeatureCollectionHandle::non_null_ptr_type> d_topological_feature_collections;
		std::vector<GPlatesFileIO::File::non_null_ptr_type> d_topological_files;

		// Separate the topological features into regular features (used as topological sections for
		// topological lines/boundaries/networks), topological lines (can also be used as topological
		// sections for topological boundaries/networks), topological boundaries and topological networks.
		topological_features_seq_type d_topological_section_regular_features;
		topological_features_seq_type d_topological_line_features;
		topological_features_seq_type d_topological_boundary_features;
		// Network features are a little different (they're grouped by their topology network parameters)
		// because they need to be resolved separately.
		topological_network_features_map_type d_topological_network_features_map;

		GPlatesAppLogic::ReconstructMethodRegistry d_reconstruct_method_registry;

		/**
		 * Used to reconstruct @a d_topological_section_regular_features so they can be used as
		 * topological sections for the topologies.
		 */
		GPlatesAppLogic::ReconstructContext d_topological_section_reconstruct_context;
		GPlatesAppLogic::ReconstructContext::context_state_reference_type d_topological_section_reconstruct_context_state;

		/**
		 * Cache of topological snapshots (resolved topologies) at various (integer) time instants.
		 */
		topological_snapshots_type d_cached_topological_snapshots;


		TopologicalModel(
				const TopologicalFeatureCollectionSequenceFunctionArgument &topological_features,
				const RotationModel::non_null_ptr_type &rotation_model,
				ResolveTopologyParameters::non_null_ptr_to_const_type default_resolve_topology_parameters);

		/**
		 * Resolves topologies for the specified time and returns them as a topological snapshot.
		 *
		 * @a reconstruction_time should be an integral value.
		 */
		TopologicalSnapshot::non_null_ptr_type
		create_topological_snapshot(
				const double &reconstruction_time);
	};
}

#endif // GPLATES_API_PYTOPOLOGICALMODEL_H
