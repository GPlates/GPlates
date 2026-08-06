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

#include <cmath>
#include <cstddef>
#include <iterator>
#include <limits>
#include <sstream>
#include <vector>
#include <boost/bind/bind.hpp>
#include <boost/cast.hpp>
#include <boost/foreach.hpp>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <boost/utility/in_place_factory.hpp>
#include <boost/variant.hpp>

#include "PyTopologicalModel.h"

#include "PyFeature.h"
#include "PyFeatureCollectionFunctionArgument.h"
#include "PyNetworkTriangulation.h"
#include "PyPropertyValues.h"
#include "PythonConverterUtils.h"
#include "PythonExtractUtils.h"
#include "PythonHashDefVisitor.h"
#include "PythonPickle.h"

#include "app-logic/GeometryUtils.h"
#include "app-logic/ScalarCoverageEvolution.h"
#include "app-logic/ReconstructParams.h"
#include "app-logic/TopologyInternalUtils.h"
#include "app-logic/TopologyPointLocation.h"
#include "app-logic/TopologyUtils.h"
#include "app-logic/VelocityDeltaTime.h"
#include "app-logic/VelocityUnits.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/python.h"

#include "maths/MathsUtils.h"
#include "maths/types.h"

#include "model/FeatureCollectionHandle.h"
#include "model/types.h"

#include "scribe/Scribe.h"

#include "utils/Earth.h"


namespace bp = boost::python;


namespace GPlatesApi
{
	/**
	 * This is called directly from Python via 'TopologicalModel.__init__()'.
	 */
	TopologicalModel::non_null_ptr_type
	topological_model_create(
			const TopologicalFeatureCollectionSequenceFunctionArgument &topological_features,
			const RotationModelFunctionArgument::function_argument_type &rotation_model_argument,
			boost::optional<GPlatesModel::integer_plate_id_type> anchor_plate_id,
			boost::optional<ResolveTopologyParameters::non_null_ptr_to_const_type> default_resolve_topology_parameters,
			boost::optional<unsigned int> topological_snapshot_cache_size)
	{
		return TopologicalModel::create(
				topological_features,
				rotation_model_argument,
				anchor_plate_id,
				default_resolve_topology_parameters,
				topological_snapshot_cache_size);
	}

	/**
	 * This is called directly from Python via 'TopologicalModel.get_topological_snapshot()'.
	 */
	TopologicalSnapshot::non_null_ptr_type
	topological_model_get_topological_snapshot(
			TopologicalModel::non_null_ptr_type topological_model,
			const GPlatesPropertyValues::GeoTimeInstant &reconstruction_time)
	{
		// Time must not be distant past/future.
		if (!reconstruction_time.is_real())
		{
			PyErr_SetString(PyExc_ValueError,
					"Time values cannot be distant-past (float('inf')) or distant-future (float('-inf')).");
			bp::throw_error_already_set();
		}

		return topological_model->get_topological_snapshot(reconstruction_time.value());
	}


	/**
	 * This is called directly from Python via 'ReconstructedGeometryTimeSpan.DefaultDeactivatePoints.__init__()'.
	 */
	GPlatesAppLogic::TopologyReconstruct::DefaultDeactivatePoint::non_null_ptr_type
	reconstructed_geometry_time_span_default_deactivate_points_create(
			const double &threshold_velocity_delta,
			const double &threshold_distance_to_boundary_in_kms_per_my,
			bool deactivate_points_that_fall_outside_a_network)
	{
		return GPlatesAppLogic::TopologyReconstruct::DefaultDeactivatePoint::create(
				threshold_velocity_delta,
				threshold_distance_to_boundary_in_kms_per_my,
				deactivate_points_that_fall_outside_a_network);
	}


	namespace
	{
		/**
		 * Extract the geometry.
		 *
		 * @a geometry_object can be either:
		 * (1) a PointOnSphere, or
		 * (2) a MultiPointOnSphere, or
		 * (3) a sequence of PointOnSphere (or anything convertible to PointOnSphere), returned as a MultiPointOnSphere.
		 *
		 * NOTE: Currently @a geometry_object is limited to a PointOnSphere, MultiPointOnSphere or sequence of points.
		 *       In future this will be extended to include polylines and polygons (with interior holes).
		 */
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
		get_geometry(
				bp::object geometry_object)
		{
			// See if it's a MultiPointOnSphere.
			bp::extract<GPlatesUtils::non_null_intrusive_ptr<GPlatesMaths::MultiPointOnSphere>> extract_multi_point(geometry_object);
			if (extract_multi_point.check())
			{
				GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point = extract_multi_point();
				return multi_point;
			}

			// See if it's a PointOnSphere.
			bp::extract<GPlatesUtils::non_null_intrusive_ptr<GPlatesMaths::PointGeometryOnSphere>> extract_point(geometry_object);
			if (extract_point.check())
			{
				GPlatesMaths::PointGeometryOnSphere::non_null_ptr_to_const_type point = extract_point();
				return point;
			}

			// Attempt to extract a sequence of points.
			std::vector<GPlatesMaths::PointOnSphere> points;
			PythonExtractUtils::extract_iterable(points, geometry_object,
					"Expected a point or a multipoint or a sequence of points");

			return GPlatesMaths::MultiPointOnSphere::create(points);
		}

		/**
		 * Extract reconstructed geometry points (at @a reconstruction_time) from geometry time span and
		 * return as a Python list.
		 */
		bp::list
		add_geometry_points_to_list(
				GPlatesAppLogic::TopologyReconstruct::GeometryTimeSpan::non_null_ptr_type geometry_time_span,
				const double &reconstruction_time,
				bool return_inactive_points)
		{
			// Put the geometry points in a Python list object.
			boost::python::list geometry_points_list_object;

			// Get the geometry points at the reconstruction time.
			if (return_inactive_points)
			{
				std::vector<boost::optional<GPlatesMaths::PointOnSphere>> all_geometry_points;
				if (!geometry_time_span->get_all_geometry_data(reconstruction_time, all_geometry_points))
				{
					all_geometry_points.resize(geometry_time_span->get_num_all_geometry_points());
				}

				for (auto geometry_point : all_geometry_points)
				{
					// Note that boost::none gets translated to Python 'None'.
					geometry_points_list_object.append(geometry_point);
				}
			}
			else // only active points...
			{
				std::vector<GPlatesMaths::PointOnSphere> geometry_points;
				geometry_time_span->get_geometry_data(reconstruction_time, geometry_points);

				for (auto geometry_point : geometry_points)
				{
					geometry_points_list_object.append(geometry_point);
				}
			}

			return geometry_points_list_object;
		}

		/**
		 * Extract the location in topologies of reconstructed geometry points (at @a reconstruction_time)
		 * from geometry time span and return as a Python list.
		 */
		bp::list
		add_topology_point_locations_to_list(
				GPlatesAppLogic::TopologyReconstruct::GeometryTimeSpan::non_null_ptr_type geometry_time_span,
				const double &reconstruction_time,
				bool return_inactive_points)
		{
			// Put the topology point locations in a Python list object.
			boost::python::list topology_point_locations_list_object;

			// Get the topology point locations at the reconstruction time.
			if (return_inactive_points)
			{
				std::vector<boost::optional<GPlatesAppLogic::TopologyPointLocation>> all_topology_point_locations;
				if (!geometry_time_span->get_all_geometry_data(
						reconstruction_time,
						boost::none/*points*/,
						all_topology_point_locations))
				{
					all_topology_point_locations.resize(geometry_time_span->get_num_all_geometry_points());
				}

				for (auto topology_point_location : all_topology_point_locations)
				{
					// Note that boost::none gets translated to Python 'None'.
					topology_point_locations_list_object.append(topology_point_location);
				}
			}
			else // only active points...
			{
				std::vector<GPlatesAppLogic::TopologyPointLocation> topology_point_locations;
				geometry_time_span->get_geometry_data(
						reconstruction_time,
						boost::none/*points*/,
						topology_point_locations);

				for (auto topology_point_location : topology_point_locations)
				{
					topology_point_locations_list_object.append(topology_point_location);
				}
			}

			return topology_point_locations_list_object;
		}

		/**
		 * Extract the strains of reconstructed geometry points (at @a reconstruction_time)
		 * from geometry time span and return as a Python list.
		 */
		bp::list
		add_strains_to_list(
				GPlatesAppLogic::TopologyReconstruct::GeometryTimeSpan::non_null_ptr_type geometry_time_span,
				const double &reconstruction_time,
				bool return_inactive_points)
		{
			// Put the strains in a Python list object.
			boost::python::list strains_list_object;

			// Get the strains at the reconstruction time.
			if (return_inactive_points)
			{
				std::vector<boost::optional<GPlatesAppLogic::DeformationStrain>> all_strains;
				if (!geometry_time_span->get_all_geometry_data(
						reconstruction_time,
						boost::none/*points*/,
						boost::none/*point_locations*/,
						boost::none/*strain_rates*/,
						all_strains))
				{
					all_strains.resize(geometry_time_span->get_num_all_geometry_points());
				}

				for (auto strain : all_strains)
				{
					// Note that boost::none gets translated to Python 'None'.
					strains_list_object.append(strain);
				}
			}
			else // only active points...
			{
				std::vector<GPlatesAppLogic::DeformationStrain> strains;
				geometry_time_span->get_geometry_data(
						reconstruction_time,
						boost::none/*points*/,
						boost::none/*point_locations*/,
						boost::none/*strain_rates*/,
						strains);

				for (auto strain : strains)
				{
					strains_list_object.append(strain);
				}
			}

			return strains_list_object;
		}

		/**
		 * Extract the strain rates of reconstructed geometry points (at @a reconstruction_time)
		 * from geometry time span and return as a Python list.
		 */
		bp::list
		add_strain_rates_to_list(
				GPlatesAppLogic::TopologyReconstruct::GeometryTimeSpan::non_null_ptr_type geometry_time_span,
				const double &reconstruction_time,
				bool return_inactive_points)
		{
			// Put the strain rates in a Python list object.
			boost::python::list strain_rates_list_object;

			// Get the strain rates at the reconstruction time.
			if (return_inactive_points)
			{
				std::vector<boost::optional<GPlatesAppLogic::DeformationStrainRate>> all_strain_rates;
				if (!geometry_time_span->get_all_geometry_data(
						reconstruction_time,
						boost::none/*points*/,
						boost::none/*point_locations*/,
						all_strain_rates))
				{
					all_strain_rates.resize(geometry_time_span->get_num_all_geometry_points());
				}

				for (auto strain_rate : all_strain_rates)
				{
					// Note that boost::none gets translated to Python 'None'.
					strain_rates_list_object.append(strain_rate);
				}
			}
			else // only active points...
			{
				std::vector<GPlatesAppLogic::DeformationStrainRate> strain_rates;
				geometry_time_span->get_geometry_data(
						reconstruction_time,
						boost::none/*points*/,
						boost::none/*point_locations*/,
						strain_rates);

				for (auto strain_rate : strain_rates)
				{
					strain_rates_list_object.append(strain_rate);
				}
			}

			return strain_rates_list_object;
		}

		/**
		 * Extract the velocities of reconstructed geometry points (at @a reconstruction_time)
		 * from geometry time span and return as a Python list.
		 */
		bp::list
		add_velocities_to_list(
				GPlatesAppLogic::TopologyReconstruct::GeometryTimeSpan::non_null_ptr_type geometry_time_span,
				const double &reconstruction_time,
				const double &velocity_delta_time,
				GPlatesAppLogic::VelocityDeltaTime::Type velocity_delta_time_type,
				GPlatesAppLogic::VelocityUnits::Value velocity_units,
				const double &earth_radius_in_kms,
				bool return_inactive_points)
		{
			// Put the velocities in a Python list object.
			boost::python::list velocities_list_object;

			// Get the velocities at the reconstruction time.
			if (return_inactive_points)
			{
				std::vector<boost::optional<GPlatesMaths::Vector3D>> all_velocities;
				if (!geometry_time_span->get_all_velocities(
						all_velocities,
						reconstruction_time,
						velocity_delta_time,
						velocity_delta_time_type,
						velocity_units,
						earth_radius_in_kms))
				{
					all_velocities.resize(geometry_time_span->get_num_all_geometry_points());
				}

				for (const auto &velocity : all_velocities)
				{
					// Note that boost::none gets translated to Python 'None'.
					velocities_list_object.append(velocity);
				}
			}
			else // only active points...
			{
				std::vector<GPlatesMaths::Vector3D> velocities;
				geometry_time_span->get_velocities(
						velocities,
						reconstruction_time,
						velocity_delta_time,
						velocity_delta_time_type,
						velocity_units,
						earth_radius_in_kms);

				for (const auto &velocity : velocities)
				{
					velocities_list_object.append(velocity);
				}
			}

			return velocities_list_object;
		}

		/**
		 * Extract reconstructed scalar values (at @a reconstruction_time and associated with @a scalar_type)
		 * from a scalar coverage time span and return as a Python list.
		 */
		bp::list
		add_scalar_values_to_list(
				GPlatesAppLogic::ScalarCoverageTimeSpan::non_null_ptr_type scalar_coverage_time_span,
				const GPlatesPropertyValues::ValueObjectType &scalar_type,
				const double &reconstruction_time,
				bool return_inactive_points)
		{
			// Put the scalar values in a Python list object.
			boost::python::list scalar_values_list_object;

			// Get the scalar values at the reconstruction time.
			if (return_inactive_points)
			{
				std::vector<double> all_scalar_values;
				std::vector<bool> all_scalar_values_are_active;
				if (scalar_coverage_time_span->get_all_scalar_values(
						scalar_type, reconstruction_time, all_scalar_values, all_scalar_values_are_active))
				{
					const unsigned int num_all_scalar_values = scalar_coverage_time_span->get_num_all_scalar_values();
					for (unsigned int scalar_value_index = 0; scalar_value_index < num_all_scalar_values; ++scalar_value_index)
					{
						boost::optional<double> scalar_value;
						if (all_scalar_values_are_active[scalar_value_index])
						{
							scalar_value = all_scalar_values[scalar_value_index];
						}

						// Note that boost::none gets translated to Python 'None'.
						scalar_values_list_object.append(scalar_value);
					}
				}
				else
				{
					const unsigned int num_all_scalar_values = scalar_coverage_time_span->get_num_all_scalar_values();
					for (unsigned int scalar_value_index = 0; scalar_value_index < num_all_scalar_values; ++scalar_value_index)
					{
						scalar_values_list_object.append(bp::object()/*Py_None*/);
					}
				}
			}
			else // only active points...
			{
				std::vector<double> scalar_values;
				if (scalar_coverage_time_span->get_scalar_values(scalar_type, reconstruction_time, scalar_values))
				{
					for (auto scalar_value : scalar_values)
					{
						scalar_values_list_object.append(scalar_value);
					}
				}
			}

			return scalar_values_list_object;
		}
	}

	/**
	 * Returns the time span of the history of reconstructed geometry points.
	 */
	bp::tuple
	reconstructed_geometry_time_span_get_time_span(
			ReconstructedGeometryTimeSpan::non_null_ptr_type reconstructed_geometry_time_span)
	{
		const GPlatesAppLogic::TimeSpanUtils::TimeRange &time_range =
				reconstructed_geometry_time_span->get_geometry_time_span()->get_time_range();

		return bp::make_tuple(
				time_range.get_begin_time(),
				time_range.get_end_time(),
				time_range.get_time_increment(),
				time_range.get_num_time_slots());
	}

	/**
	 * Returns the list of reconstructed geometry points (at reconstruction time).
	 */
	bp::object
	reconstructed_geometry_time_span_get_geometry_points(
			ReconstructedGeometryTimeSpan::non_null_ptr_type reconstructed_geometry_time_span,
			const GPlatesPropertyValues::GeoTimeInstant &reconstruction_time,
			bool return_inactive_points)
	{
		// Reconstruction time must not be distant past/future.
		if (!reconstruction_time.is_real())
		{
			PyErr_SetString(PyExc_ValueError,
					"Reconstruction time cannot be distant-past (float('inf')) or distant-future (float('-inf')).");
			bp::throw_error_already_set();
		}

		// Return None if there are no active points at the reconstruction time.
		if (!reconstructed_geometry_time_span->get_geometry_time_span()->is_valid(reconstruction_time.value()))
		{
			return bp::object()/*Py_None*/;
		}

		GPlatesAppLogic::TopologyReconstruct::GeometryTimeSpan::non_null_ptr_type geometry_time_span =
				reconstructed_geometry_time_span->get_geometry_time_span();

		return add_geometry_points_to_list(geometry_time_span, reconstruction_time.value(), return_inactive_points);
	}

	/**
	 * Returns the list of locations of geometry points in resolved topologies (at reconstruction time).
	 */
	bp::object
	reconstructed_geometry_time_span_get_topology_point_locations(
			ReconstructedGeometryTimeSpan::non_null_ptr_type reconstructed_geometry_time_span,
			const GPlatesPropertyValues::GeoTimeInstant &reconstruction_time,
			bool return_inactive_points)
	{
		// Reconstruction time must not be distant past/future.
		if (!reconstruction_time.is_real())
		{
			PyErr_SetString(PyExc_ValueError,
					"Reconstruction time cannot be distant-past (float('inf')) or distant-future (float('-inf')).");
			bp::throw_error_already_set();
		}

		// Return None if there are no active points at the reconstruction time.
		if (!reconstructed_geometry_time_span->get_geometry_time_span()->is_valid(reconstruction_time.value()))
		{
			return bp::object()/*Py_None*/;
		}

		GPlatesAppLogic::TopologyReconstruct::GeometryTimeSpan::non_null_ptr_type geometry_time_span =
				reconstructed_geometry_time_span->get_geometry_time_span();

		return add_topology_point_locations_to_list(geometry_time_span, reconstruction_time.value(), return_inactive_points);
	}

	/**
	 * Returns the list of strains at geometry points in resolved topologies (at reconstruction time).
	 */
	bp::object
	reconstructed_geometry_time_span_get_strains(
			ReconstructedGeometryTimeSpan::non_null_ptr_type reconstructed_geometry_time_span,
			const GPlatesPropertyValues::GeoTimeInstant &reconstruction_time,
			bool return_inactive_points)
	{
		// Reconstruction time must not be distant past/future.
		if (!reconstruction_time.is_real())
		{
			PyErr_SetString(PyExc_ValueError,
					"Reconstruction time cannot be distant-past (float('inf')) or distant-future (float('-inf')).");
			bp::throw_error_already_set();
		}

		// Return None if there are no active points at the reconstruction time.
		if (!reconstructed_geometry_time_span->get_geometry_time_span()->is_valid(reconstruction_time.value()))
		{
			return bp::object()/*Py_None*/;
		}

		GPlatesAppLogic::TopologyReconstruct::GeometryTimeSpan::non_null_ptr_type geometry_time_span =
				reconstructed_geometry_time_span->get_geometry_time_span();

		return add_strains_to_list(geometry_time_span, reconstruction_time.value(), return_inactive_points);
	}

	/**
	 * Returns the list of strain rates at geometry points in resolved topologies (at reconstruction time).
	 */
	bp::object
	reconstructed_geometry_time_span_get_strain_rates(
			ReconstructedGeometryTimeSpan::non_null_ptr_type reconstructed_geometry_time_span,
			const GPlatesPropertyValues::GeoTimeInstant &reconstruction_time,
			bool return_inactive_points)
	{
		// Reconstruction time must not be distant past/future.
		if (!reconstruction_time.is_real())
		{
			PyErr_SetString(PyExc_ValueError,
					"Reconstruction time cannot be distant-past (float('inf')) or distant-future (float('-inf')).");
			bp::throw_error_already_set();
		}

		// Return None if there are no active points at the reconstruction time.
		if (!reconstructed_geometry_time_span->get_geometry_time_span()->is_valid(reconstruction_time.value()))
		{
			return bp::object()/*Py_None*/;
		}

		GPlatesAppLogic::TopologyReconstruct::GeometryTimeSpan::non_null_ptr_type geometry_time_span =
				reconstructed_geometry_time_span->get_geometry_time_span();

		return add_strain_rates_to_list(geometry_time_span, reconstruction_time.value(), return_inactive_points);
	}

	/**
	 * Returns the list of strain rates at geometry points in resolved topologies (at reconstruction time).
	 */
	bp::object
	reconstructed_geometry_time_span_get_velocities(
			ReconstructedGeometryTimeSpan::non_null_ptr_type reconstructed_geometry_time_span,
			const GPlatesPropertyValues::GeoTimeInstant &reconstruction_time,
			const double &velocity_delta_time,
			GPlatesAppLogic::VelocityDeltaTime::Type velocity_delta_time_type,
			GPlatesAppLogic::VelocityUnits::Value velocity_units,
			const double &earth_radius_in_kms,
			bool return_inactive_points)
	{
		// Reconstruction time must not be distant past/future.
		if (!reconstruction_time.is_real())
		{
			PyErr_SetString(PyExc_ValueError,
					"Reconstruction time cannot be distant-past (float('inf')) or distant-future (float('-inf')).");
			bp::throw_error_already_set();
		}

		// Velocity delta time must be positive.
		if (velocity_delta_time <= 0)
		{
			PyErr_SetString(PyExc_ValueError, "Velocity delta time must be positive.");
			bp::throw_error_already_set();
		}

		// Return None if there are no active points at the reconstruction time.
		if (!reconstructed_geometry_time_span->get_geometry_time_span()->is_valid(reconstruction_time.value()))
		{
			return bp::object()/*Py_None*/;
		}

		GPlatesAppLogic::TopologyReconstruct::GeometryTimeSpan::non_null_ptr_type geometry_time_span =
				reconstructed_geometry_time_span->get_geometry_time_span();

		return add_velocities_to_list(
				geometry_time_span,
				reconstruction_time.value(),
				velocity_delta_time,
				velocity_delta_time_type,
				velocity_units,
				earth_radius_in_kms,
				return_inactive_points);
	}

	/**
	 * Returns the list of reconstructed scalar values (at reconstruction time) associated with the specified scalar type (if specified),
	 * otherwise returns a dict mapping available scalar types to their reconstructed scalar values (at reconstruction time).
	 */
	bp::object
	reconstructed_geometry_time_span_get_scalar_values(
			ReconstructedGeometryTimeSpan::non_null_ptr_type reconstructed_geometry_time_span,
			const GPlatesPropertyValues::GeoTimeInstant &reconstruction_time,
			boost::optional<GPlatesPropertyValues::ValueObjectType> scalar_type,
			bool return_inactive_points)
	{
		// Reconstruction time must not be distant past/future.
		if (!reconstruction_time.is_real())
		{
			PyErr_SetString(PyExc_ValueError,
					"Reconstruction time cannot be distant-past (float('inf')) or distant-future (float('-inf')).");
			bp::throw_error_already_set();
		}

		// Return None if there are no active points at the reconstruction time.
		if (!reconstructed_geometry_time_span->get_geometry_time_span()->is_valid(reconstruction_time.value()))
		{
			return bp::object()/*Py_None*/;
		}

		GPlatesAppLogic::ScalarCoverageTimeSpan::non_null_ptr_type scalar_coverage_time_span =
				reconstructed_geometry_time_span->get_scalar_coverage_time_span();

		if (scalar_type)
		{
			// Look up the scalar type.
			if (!scalar_coverage_time_span->contains_scalar_type(scalar_type.get()))
			{
				// Not found.
				return bp::object()/*Py_None*/;
			}

			return add_scalar_values_to_list(
					scalar_coverage_time_span,
					scalar_type.get(),
					reconstruction_time.value(),
					return_inactive_points);
		}

		bp::dict scalar_values_dict;

		// Find all available scalar types contained in the scalar coverage.
		std::vector<GPlatesPropertyValues::ValueObjectType> available_scalar_types;
		scalar_coverage_time_span->get_scalar_types(available_scalar_types);

		// Iterate over scalar types.
		for (const auto &available_scalar_type : available_scalar_types)
		{
			boost::python::list curr_scalar_values_list_object = add_scalar_values_to_list(
					scalar_coverage_time_span,
					available_scalar_type,
					reconstruction_time.value(),
					return_inactive_points);

			// Map the current scalar type to its scalar values.
			scalar_values_dict[available_scalar_type] = curr_scalar_values_list_object;
		}

		return scalar_values_dict;
	}

	/**
	 * Returns the list of reconstructed crustal thicknesses (in kms) at reconstruction time.
	 */
	bp::object
	reconstructed_geometry_time_span_get_crustal_thicknesses(
			ReconstructedGeometryTimeSpan::non_null_ptr_type reconstructed_geometry_time_span,
			const GPlatesPropertyValues::GeoTimeInstant &reconstruction_time,
			bool return_inactive_points)
	{
		static const GPlatesPropertyValues::ValueObjectType GPML_CRUSTAL_THICKNESS =
				GPlatesPropertyValues::ValueObjectType::create_gpml("CrustalThickness");

		return reconstructed_geometry_time_span_get_scalar_values(
				reconstructed_geometry_time_span,
				reconstruction_time,
				GPML_CRUSTAL_THICKNESS,
				return_inactive_points);
	}

	/**
	 * Returns the list of reconstructed crustal stretching factors at reconstruction time.
	 *
	 * Stretching (beta) factor is 'beta = Ti/T'.
	 */
	bp::object
	reconstructed_geometry_time_span_get_crustal_stretching_factors(
			ReconstructedGeometryTimeSpan::non_null_ptr_type reconstructed_geometry_time_span,
			const GPlatesPropertyValues::GeoTimeInstant &reconstruction_time,
			bool return_inactive_points)
	{
		static const GPlatesPropertyValues::ValueObjectType GPML_CRUSTAL_STRETCHING_FACTOR =
				GPlatesPropertyValues::ValueObjectType::create_gpml("CrustalStretchingFactor");

		return reconstructed_geometry_time_span_get_scalar_values(
				reconstructed_geometry_time_span,
				reconstruction_time,
				GPML_CRUSTAL_STRETCHING_FACTOR,
				return_inactive_points);
	}

	/**
	 * Returns the list of reconstructed crustal thinning factors at reconstruction time.
	 *
	 * Thinning (gamma) factor is 'gamma = (1 - T/Ti)'.
	 */
	bp::object
	reconstructed_geometry_time_span_get_crustal_thinning_factors(
			ReconstructedGeometryTimeSpan::non_null_ptr_type reconstructed_geometry_time_span,
			const GPlatesPropertyValues::GeoTimeInstant &reconstruction_time,
			bool return_inactive_points)
	{
		static const GPlatesPropertyValues::ValueObjectType GPML_CRUSTAL_THINNING_FACTOR =
				GPlatesPropertyValues::ValueObjectType::create_gpml("CrustalThinningFactor");

		return reconstructed_geometry_time_span_get_scalar_values(
				reconstructed_geometry_time_span,
				reconstruction_time,
				GPML_CRUSTAL_THINNING_FACTOR,
				return_inactive_points);
	}

	/**
	 * Returns the list of reconstructed tectonic subsidences (at reconstruction time).
	 */
	bp::object
	reconstructed_geometry_time_span_get_tectonic_subsidences(
			ReconstructedGeometryTimeSpan::non_null_ptr_type reconstructed_geometry_time_span,
			const GPlatesPropertyValues::GeoTimeInstant &reconstruction_time,
			bool return_inactive_points)
	{
		static const GPlatesPropertyValues::ValueObjectType GPML_TECTONIC_SUBSIDENCE =
				GPlatesPropertyValues::ValueObjectType::create_gpml("TectonicSubsidence");

		return reconstructed_geometry_time_span_get_scalar_values(
				reconstructed_geometry_time_span,
				reconstruction_time,
				GPML_TECTONIC_SUBSIDENCE,
				return_inactive_points);
	}

	/**
	 * Returns true if point is not located in any resolved topologies.
	 */
	bool
	topology_point_not_located_in_resolved_topology(
			const GPlatesAppLogic::TopologyPointLocation &topology_point_location)
	{
		return topology_point_location.not_located();
	}

	/**
	 * Returns resolved topological boundary containing point, otherwise boost::none.
	 */
	boost::optional<GPlatesAppLogic::ResolvedTopologicalBoundary::non_null_ptr_to_const_type>
	topology_point_located_in_resolved_boundary(
			const GPlatesAppLogic::TopologyPointLocation &topology_point_location)
	{
		return topology_point_location.located_in_resolved_boundary();
	}

	/**
	 * Returns resolved topological network if it contains point, otherwise None.
	 */
	boost::optional<GPlatesAppLogic::ResolvedTopologicalNetwork::non_null_ptr_to_const_type>
	topology_point_located_in_resolved_network(
			const GPlatesAppLogic::TopologyPointLocation &topology_point_location)
	{
		boost::optional<GPlatesAppLogic::TopologyPointLocation::network_location_type>
				network_location = topology_point_location.located_in_resolved_network();
		if (network_location)
		{
			GPlatesAppLogic::ResolvedTopologicalNetwork::non_null_ptr_to_const_type resolved_network = network_location->first;
			return resolved_network;
		}

		return boost::none;
	}

	/**
	 * Returns resolved topological network if its deforming region (excludes rigid blocks) contains point, otherwise None.
	 *
	 * Also returns network triangle (in a 2-tuple) if 'return_network_triangle' is true.
	 */
	bp::object
	topology_point_located_in_resolved_network_deforming_region(
			const GPlatesAppLogic::TopologyPointLocation &topology_point_location,
			bool return_network_triangle)
	{
		boost::optional<GPlatesAppLogic::TopologyPointLocation::network_location_type>
				network_location = topology_point_location.located_in_resolved_network();
		if (network_location)
		{
			GPlatesAppLogic::ResolvedTopologicalNetwork::non_null_ptr_to_const_type resolved_network = network_location->first;
			const GPlatesAppLogic::ResolvedTriangulation::Network::PointLocation &point_location = network_location->second;

			if (boost::optional<GPlatesAppLogic::ResolvedTriangulation::Delaunay_2::Face_handle> deforming_face =
				point_location.located_in_deforming_region())
			{
				if (return_network_triangle)
				{
					const GPlatesApi::NetworkTriangulation::Triangle network_triangle(resolved_network, deforming_face.get());

					return bp::make_tuple(resolved_network, network_triangle);
				}
				else
				{
					return bp::object(resolved_network);
				}
			}
		}

		return bp::object()/*Py_None*/;
	}

	/**
	 * Returns tuple (resolved topological network, rigid block RFG) containing point, otherwise None.
	 */
	bp::object
	topology_point_located_in_resolved_network_rigid_block(
			const GPlatesAppLogic::TopologyPointLocation &topology_point_location)
	{
		// Is located in a resolved network?
		boost::optional<GPlatesAppLogic::TopologyPointLocation::network_location_type>
				network_location = topology_point_location.located_in_resolved_network();
		if (network_location)
		{
			GPlatesAppLogic::ResolvedTopologicalNetwork::non_null_ptr_to_const_type resolved_network = network_location->first;
			const GPlatesAppLogic::ResolvedTriangulation::Network::PointLocation &point_location = network_location->second;

			// Is located in one of resolved network's rigid blocks?
			if (boost::optional<const GPlatesAppLogic::ResolvedTriangulation::Network::RigidBlock &> rigid_block =
				point_location.located_in_rigid_block())
			{
				return bp::make_tuple(resolved_network, rigid_block->get_reconstructed_feature_geometry());
			}
		}

		return bp::object()/*Py_None*/;
	}
}


GPlatesApi::TopologicalModel::non_null_ptr_type
GPlatesApi::TopologicalModel::create(
		const TopologicalFeatureCollectionSequenceFunctionArgument &topological_features,
		// Note we're using 'RotationModelFunctionArgument::function_argument_type' instead of
		// just 'RotationModelFunctionArgument' since we want to know if it's an existing RotationModel...
		const RotationModelFunctionArgument::function_argument_type &rotation_model_argument,
		boost::optional<GPlatesModel::integer_plate_id_type> anchor_plate_id,
		boost::optional<ResolveTopologyParameters::non_null_ptr_to_const_type> default_resolve_topology_parameters,
		boost::optional<unsigned int> topological_snapshot_cache_size)
{
	boost::optional<RotationModel::non_null_ptr_type> rotation_model;

	//
	// Adapt an existing rotation model, or create a new rotation model.
	//
	if (const RotationModel::non_null_ptr_type *existing_rotation_model =
		boost::get<RotationModel::non_null_ptr_type>(&rotation_model_argument))
	{
		// Adapt the existing rotation model.
		rotation_model = RotationModel::create(
				*existing_rotation_model,
				// Start off with a cache size of 1 (later we'll increase it as needed)...
				1/*reconstruction_tree_cache_size*/,
				// If anchor plate ID is none then defaults to the default anchor plate of existing rotation model...
				anchor_plate_id);
	}
	else
	{
		const FeatureCollectionSequenceFunctionArgument rotation_feature_collections_function_argument =
				boost::get<FeatureCollectionSequenceFunctionArgument>(rotation_model_argument);

		// Create a new rotation model (from rotation features).
		//
		// Note: We're creating our own RotationModel from scratch (as opposed to adapting an existing one)
		//       to avoid having two rotation models (each with their own cache) thus doubling the cache memory usage.
		rotation_model = RotationModel::create(
				rotation_feature_collections_function_argument,
				// Start off with a cache size of 1 (later we'll increase it as needed)...
				1/*reconstruction_tree_cache_size*/,
				false/*extend_total_reconstruction_poles_to_distant_past*/,
				// We're creating a new RotationModel from scratch (as opposed to adapting an existing one)
				// so the anchor plate ID defaults to zero if not specified...
				anchor_plate_id ? anchor_plate_id.get() : 0);
	}

	// Get the topological files.
	std::vector<GPlatesFileIO::File::non_null_ptr_type> topological_files;
	topological_features.get_files(topological_files);

	// Get the associated resolved topology parameters.
	std::vector<boost::optional<ResolveTopologyParameters::non_null_ptr_to_const_type>> resolve_topology_parameters;
	topological_features.get_resolve_topology_parameters(resolve_topology_parameters);

	// If no default resolve topology parameters specified then use default values.
	if (!default_resolve_topology_parameters)
	{
		default_resolve_topology_parameters = ResolveTopologyParameters::create();
	}

	return non_null_ptr_type(
			new TopologicalModel(
					rotation_model.get(),
					topological_files,
					resolve_topology_parameters,
					default_resolve_topology_parameters.get(),
					topological_snapshot_cache_size));
}


GPlatesApi::TopologicalModel::TopologicalModel(
		const RotationModel::non_null_ptr_type &rotation_model,
		const std::vector<GPlatesFileIO::File::non_null_ptr_type> &topological_files,
		const std::vector<boost::optional<ResolveTopologyParameters::non_null_ptr_to_const_type>> &resolve_topology_parameters,
		ResolveTopologyParameters::non_null_ptr_to_const_type default_resolve_topology_parameters,
		boost::optional<unsigned int> topological_snapshot_cache_size) :
	d_rotation_model(rotation_model),
	d_topological_files(topological_files),
	d_resolve_topology_parameters(resolve_topology_parameters),
	d_default_resolve_topology_parameters(default_resolve_topology_parameters),
	d_topological_section_reconstruct_context(d_reconstruct_method_registry),
	d_topological_snapshot_cache_size(topological_snapshot_cache_size),
	d_topological_snapshot_cache(
			// Function to create a topological snapshot given a reconstruction time...
			boost::bind(&TopologicalModel::create_topological_snapshot, this, boost::placeholders::_1),
			// Initially set cache size to 1 - we'll set it properly in 'initialise_topological_reconstruction()'...
			1)
{
	initialise_topological_reconstruction();
}


void
GPlatesApi::TopologicalModel::initialise_topological_reconstruction()
{
	// Clear the data members we're about to initialise in case this function called during transcribing.
	d_topological_feature_collections.clear();
	d_topological_line_features.clear();
	d_topological_boundary_features.clear();
	d_topological_network_features_map.clear();
	d_topological_section_regular_features.clear();
	// Also clear any cached topological snapshots.
	d_topological_snapshot_cache.clear();

	// Size of topological snapshot cache.
	const unsigned int topological_snapshot_cache_size = d_topological_snapshot_cache_size
			? d_topological_snapshot_cache_size.get()
			// If not specified then default to unlimited - set to a very large value.
			// But should be *less* than max value so that max value can compare greater than it...
			: (std::numeric_limits<unsigned int>::max)() - 2;
	// Set size of topological snapshot cache.
	d_topological_snapshot_cache.set_maximum_num_values_in_cache(topological_snapshot_cache_size);

	// Size of reconstruction tree cache.
	//
	// The +1 accounts for the extra time step used to generate deformed geometries (and velocities).
	const unsigned int reconstruction_tree_cache_size = topological_snapshot_cache_size + 1;
	d_rotation_model->get_cached_reconstruction_tree_creator_impl()->set_maximum_cache_size(reconstruction_tree_cache_size);

	// Extract a feature collection from each topological file.
	for (auto topological_file : d_topological_files)
	{
		const GPlatesModel::FeatureCollectionHandle::non_null_ptr_type topological_feature_collection(
				topological_file->get_reference().get_feature_collection().handle_ptr());

		d_topological_feature_collections.push_back(topological_feature_collection);
	}

	// Each feature collection has an optional associated resolve topology parameters.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			d_resolve_topology_parameters.size() == d_topological_feature_collections.size(),
			GPLATES_ASSERTION_SOURCE);

	// Separate into regular features (used as topological sections for topological lines/boundaries/networks),
	// topological lines (can also be used as topological sections for topological boundaries/networks),
	// topological boundaries and topological networks.
	//
	// This makes it faster to resolve topologies since resolving topological lines/boundaries/networks visits
	// only those topological features actually containing topological lines/boundaries/networks respectively
	// (because visiting, eg, a network feature when resolving boundary features requires visiting all the feature
	// properties of that network only to discard the network feature since it's not a topological boundary).
	const unsigned int num_feature_collections = d_topological_feature_collections.size();
	for (unsigned int feature_collection_index = 0; feature_collection_index < num_feature_collections; ++feature_collection_index)
	{
		auto feature_collection = d_topological_feature_collections[feature_collection_index];
		auto resolve_topology_parameters = d_resolve_topology_parameters[feature_collection_index];

		// If current feature collection did not specify resolve topology parameters then use the default parameters.
		if (!resolve_topology_parameters)
		{
			resolve_topology_parameters = d_default_resolve_topology_parameters;
		}

		for (auto feature : *feature_collection)
		{
			const GPlatesModel::FeatureHandle::weak_ref feature_ref = feature->reference();

			// Determine the topology geometry type.
			boost::optional<GPlatesAppLogic::TopologyGeometry::Type> topology_geometry_type =
					GPlatesAppLogic::TopologyUtils::get_topological_geometry_type(feature_ref);

			if (topology_geometry_type == GPlatesAppLogic::TopologyGeometry::LINE)
			{
				d_topological_line_features.push_back(feature_ref);
			}
			else if (topology_geometry_type == GPlatesAppLogic::TopologyGeometry::BOUNDARY)
			{
				d_topological_boundary_features.push_back(feature_ref);
			}
			else if (topology_geometry_type == GPlatesAppLogic::TopologyGeometry::NETWORK)
			{
				// Add the network feature to the group of network features associated with the
				// topology network params belonging to the current feature collection.
				//
				// If multiple feature collections have the same network parameters then all their
				// network features will end up in the same group.
				const auto &topology_network_params = resolve_topology_parameters.get()->get_topology_network_params();
				d_topological_network_features_map[topology_network_params].push_back(feature_ref);
			}
			else
			{
				d_topological_section_regular_features.push_back(feature_ref);
			}
		}
	}

	d_topological_section_reconstruct_context_state =
			d_topological_section_reconstruct_context.create_context_state(
					GPlatesAppLogic::ReconstructMethodInterface::Context(
							GPlatesAppLogic::ReconstructParams(),
							d_rotation_model->get_reconstruction_tree_creator()));

	// Set the topological section regular features in the reconstruct context.
	d_topological_section_reconstruct_context.set_features(d_topological_section_regular_features);
}


GPlatesApi::TopologicalSnapshot::non_null_ptr_type
GPlatesApi::TopologicalModel::get_topological_snapshot(
		const double &reconstruction_time)
{
	return d_topological_snapshot_cache.get_value(reconstruction_time);
}


GPlatesApi::TopologicalSnapshot::non_null_ptr_type
GPlatesApi::TopologicalModel::create_topological_snapshot(
		const GPlatesMaths::real_t &reconstruction_time)
{
	// Find the topological section feature IDs referenced by any topological features at current reconstruction time.
	//
	// This is an optimisation that avoids unnecessary reconstructions. Only those topological sections referenced
	// by topologies that exist at the reconstruction time are reconstructed.
	std::set<GPlatesModel::FeatureId> topological_sections_referenced;
	GPlatesAppLogic::TopologyInternalUtils::find_topological_sections_referenced(
			topological_sections_referenced,
			d_topological_line_features,
			GPlatesAppLogic::TopologyGeometry::LINE,
			reconstruction_time.dval());
	GPlatesAppLogic::TopologyInternalUtils::find_topological_sections_referenced(
			topological_sections_referenced,
			d_topological_boundary_features,
			GPlatesAppLogic::TopologyGeometry::BOUNDARY,
			reconstruction_time.dval());
	for (const auto &topological_network_features_map_entry : d_topological_network_features_map)
	{
		const topological_features_seq_type &topological_network_features = topological_network_features_map_entry.second;

		GPlatesAppLogic::TopologyInternalUtils::find_topological_sections_referenced(
				topological_sections_referenced,
				topological_network_features,
				GPlatesAppLogic::TopologyGeometry::NETWORK,
				reconstruction_time.dval());
	}

	// Contains the topological section regular geometries referenced by topologies.
	std::vector<GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_type> reconstructed_feature_geometries;

	// Generate RFGs only for the referenced topological sections.
	const GPlatesAppLogic::ReconstructHandle::type reconstruct_handle =
			d_topological_section_reconstruct_context.get_reconstructed_topological_sections(
					reconstructed_feature_geometries,
					topological_sections_referenced,
					d_topological_section_reconstruct_context_state,
					reconstruction_time.dval());

	// All reconstruct handles used to find topological sections (referenced by topological boundaries/networks).
	std::vector<GPlatesAppLogic::ReconstructHandle::type> topological_sections_reconstruct_handles(1, reconstruct_handle);

	// Resolving topological lines generates its own reconstruct handle that will be used by
	// topological boundaries and networks to find this group of resolved lines.
	//
	// So we always resolve topological *lines* regardless of whether the user requested it or not.
	std::vector<GPlatesAppLogic::ResolvedTopologicalLine::non_null_ptr_type> resolved_lines;
	const GPlatesAppLogic::ReconstructHandle::type resolved_topological_lines_handle =
			GPlatesAppLogic::TopologyUtils::resolve_topological_lines(
					// Contains the resolved topological line sections referenced by topological boundaries and networks...
					resolved_lines,
					d_topological_line_features,
					d_rotation_model->get_reconstruction_tree_creator(), 
					reconstruction_time.dval(),
					// Resolved topo lines use the reconstructed non-topo geometries...
					topological_sections_reconstruct_handles
	// NOTE: We need to generate all resolved topological lines, not just those referenced by resolved boundaries/networks,
	//       because the user may later explicitly request the resolved topological lines (or explicitly export them)...
#if 0
					// Only those topo lines referenced by resolved boundaries/networks...
					, topological_sections_referenced
#endif
			);

	topological_sections_reconstruct_handles.push_back(resolved_topological_lines_handle);

	// Resolve topological boundaries.
	std::vector<GPlatesAppLogic::ResolvedTopologicalBoundary::non_null_ptr_type> resolved_boundaries;
	GPlatesAppLogic::TopologyUtils::resolve_topological_boundaries(
			resolved_boundaries,
			d_topological_boundary_features,
			d_rotation_model->get_reconstruction_tree_creator(), 
			reconstruction_time.dval(),
			// Resolved topo boundaries use the resolved topo lines *and* the reconstructed non-topo geometries...
			topological_sections_reconstruct_handles);

	// Resolve topological networks.
	//
	// Different network features can have a different resolve topology parameters so resolve them separately.
	std::vector<GPlatesAppLogic::ResolvedTopologicalNetwork::non_null_ptr_type> resolved_networks;
	for (const auto &topological_network_features_map_entry : d_topological_network_features_map)
	{
		const GPlatesAppLogic::TopologyNetworkParams &topology_network_params = topological_network_features_map_entry.first;
		const topological_features_seq_type &topological_network_features = topological_network_features_map_entry.second;

		GPlatesAppLogic::TopologyUtils::resolve_topological_networks(
				resolved_networks,
				reconstruction_time.dval(),
				topological_network_features,
				// Resolved topo networks use the resolved topo lines *and* the reconstructed non-topo geometries...
				topological_sections_reconstruct_handles,
				topology_network_params);
	}

	return TopologicalSnapshot::create(
			resolved_lines, resolved_boundaries, resolved_networks,
			d_rotation_model, d_topological_files, d_resolve_topology_parameters, d_default_resolve_topology_parameters,
			reconstruction_time.dval());
}


GPlatesApi::ReconstructedGeometryTimeSpan::non_null_ptr_type
GPlatesApi::TopologicalModel::reconstruct_geometry(
		bp::object geometry_object,
		const GPlatesPropertyValues::GeoTimeInstant &initial_time,
		boost::optional<GPlatesPropertyValues::GeoTimeInstant> oldest_time_arg,
		const GPlatesPropertyValues::GeoTimeInstant &youngest_time,
		const double &time_increment,
		boost::optional<GPlatesModel::integer_plate_id_type> reconstruction_plate_id,
		bp::object scalar_type_to_initial_scalar_values_mapping_object,
		boost::optional<GPlatesAppLogic::TopologyReconstruct::DeactivatePoint::non_null_ptr_to_const_type> deactivate_points,
		bool deformation_uses_natural_neighbour_interpolation)
{
	// Initial reconstruction time must not be distant past/future.
	if (!initial_time.is_real())
	{
		PyErr_SetString(PyExc_ValueError,
				"Initial reconstruction time cannot be distant-past (float('inf')) or distant-future (float('-inf')).");
		bp::throw_error_already_set();
	}

	// Oldest time defaults to initial reconstruction time if not specified.
	const GPlatesPropertyValues::GeoTimeInstant oldest_time = oldest_time_arg ? oldest_time_arg.get() : initial_time;

	if (!oldest_time.is_real() ||
		!youngest_time.is_real())
	{
		PyErr_SetString(PyExc_ValueError,
				"Oldest/youngest times cannot be distant-past (float('inf')) or distant-future (float('-inf')).");
		bp::throw_error_already_set();
	}

	if (oldest_time >= youngest_time)  // note: using GeoTimeInstant comparison where '>' means later (not earlier)
	{
		PyErr_SetString(PyExc_ValueError, "Oldest time cannot be later than (or same as) youngest time.");
		bp::throw_error_already_set();
	}

	if (time_increment <= 0)
	{
		PyErr_SetString(PyExc_ValueError, "Time increment must be positive.");
		bp::throw_error_already_set();
	}

	const GPlatesAppLogic::TimeSpanUtils::TimeRange time_range(
			oldest_time.value()/*begin_time*/, youngest_time.value()/*end_time*/, time_increment,
			// If time increment was specified correctly then it shouldn't need to be adjusted...
			GPlatesAppLogic::TimeSpanUtils::TimeRange::ADJUST_TIME_INCREMENT);
	if (!GPlatesMaths::are_almost_exactly_equal(time_range.get_time_increment(), time_increment))
	{
		PyErr_SetString(PyExc_ValueError,
				"Oldest to youngest time period must be an integer multiple of the time increment.");
		bp::throw_error_already_set();
	}

	// Create our resolved topology (boundary/network) time spans.
	GPlatesAppLogic::TopologyReconstruct::resolved_boundary_time_span_type::non_null_ptr_type resolved_boundary_time_span =
			GPlatesAppLogic::TopologyReconstruct::resolved_boundary_time_span_type::create(time_range);
	GPlatesAppLogic::TopologyReconstruct::resolved_network_time_span_type::non_null_ptr_type resolved_network_time_span =
			GPlatesAppLogic::TopologyReconstruct::resolved_network_time_span_type::create(time_range);

	const unsigned int num_time_slots = time_range.get_num_time_slots();

	// Iterate over the time slots and fill in the resolved topological boundaries/networks.
	for (unsigned int time_slot = 0; time_slot < num_time_slots; ++time_slot)
	{
		const double time = time_range.get_time(time_slot);

		// Get topological snapshot (it'll either be cached or generated on demand).
		TopologicalSnapshot::non_null_ptr_type topological_snapshot = get_topological_snapshot(time);

		resolved_boundary_time_span->set_sample_in_time_slot(topological_snapshot->get_resolved_topological_boundaries(), time_slot);
		resolved_network_time_span->set_sample_in_time_slot(topological_snapshot->get_resolved_topological_networks(), time_slot);
	}

	GPlatesAppLogic::TopologyReconstruct::non_null_ptr_type topology_reconstruct =
			GPlatesAppLogic::TopologyReconstruct::create(
					time_range,
					resolved_boundary_time_span,
					resolved_network_time_span,
					d_rotation_model->get_reconstruction_tree_creator());

	// Extract the geometry.
	GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry = get_geometry(geometry_object);

	// Create time span of reconstructed geometry.
	GPlatesAppLogic::TopologyReconstruct::GeometryTimeSpan::non_null_ptr_type geometry_time_span =
			topology_reconstruct->create_geometry_time_span(
					geometry,
					// If a reconstruction plate ID is not specified then use the default anchor plate ID
					// of our rotation model...
					reconstruction_plate_id
							? reconstruction_plate_id.get()
							: d_rotation_model->get_reconstruction_tree_creator().get_default_anchor_plate_id(),
					initial_time.value(),
					deactivate_points,
					deformation_uses_natural_neighbour_interpolation);

	// Extract the optional initial scalar values.
	GPlatesAppLogic::ScalarCoverageTimeSpan::initial_scalar_coverage_type initial_scalar_coverage;
	if (scalar_type_to_initial_scalar_values_mapping_object != bp::object()/*Py_None*/)
	{
		// Extract the mapping of scalar types to scalar values.
		initial_scalar_coverage = create_scalar_type_to_values_map(
				scalar_type_to_initial_scalar_values_mapping_object);

		// Number of points in domain must match number of scalar values in range.
		const unsigned int num_domain_geometry_points =
				GPlatesAppLogic::GeometryUtils::get_num_geometry_points(*geometry);
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				!initial_scalar_coverage.empty(),
				GPLATES_ASSERTION_SOURCE);
		// Just test the scalar values length for the first scalar type (all types should already have the same length).
		if (num_domain_geometry_points != initial_scalar_coverage.begin()->second.size())
		{
			PyErr_SetString(PyExc_ValueError, "Number of scalar values must match number of points in geometry");
			bp::throw_error_already_set();
		}
	}

	// Create the scalar coverage from the initial scalar values and the geometry time span.
	GPlatesAppLogic::ScalarCoverageTimeSpan::non_null_ptr_type scalar_coverage_time_span =
			GPlatesAppLogic::ScalarCoverageTimeSpan::create(initial_scalar_coverage, geometry_time_span);

	return ReconstructedGeometryTimeSpan::create(geometry_time_span, scalar_coverage_time_span);
}


GPlatesScribe::TranscribeResult
GPlatesApi::TopologicalModel::transcribe_construct_data(
		GPlatesScribe::Scribe &scribe,
		GPlatesScribe::ConstructObject<TopologicalModel> &topological_model)
{
	if (scribe.is_saving())
	{
		save_construct_data(scribe, topological_model.get_object());
	}
	else // loading
	{
		GPlatesScribe::LoadRef<RotationModel::non_null_ptr_type> rotation_model;
		std::vector<GPlatesFileIO::File::non_null_ptr_type> topological_files;
		std::vector<boost::optional<ResolveTopologyParameters::non_null_ptr_to_const_type>> resolve_topology_parameters;
		GPlatesScribe::LoadRef<ResolveTopologyParameters::non_null_ptr_to_const_type> default_resolve_topology_parameters;
		boost::optional<unsigned int> topological_snapshot_cache_size;
		if (!load_construct_data(
				scribe,
				rotation_model,
				topological_files,
				resolve_topology_parameters,
				default_resolve_topology_parameters,
				topological_snapshot_cache_size))
		{
			return scribe.get_transcribe_result();
		}

		// Create the topological model.
		topological_model.construct_object(
				rotation_model,
				topological_files,
				resolve_topology_parameters,
				default_resolve_topology_parameters,
				topological_snapshot_cache_size);
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesScribe::TranscribeResult
GPlatesApi::TopologicalModel::transcribe(
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
			GPlatesScribe::LoadRef<ResolveTopologyParameters::non_null_ptr_to_const_type> default_resolve_topology_parameters;
			boost::optional<unsigned int> topological_snapshot_cache_size;
			if (!load_construct_data(
					scribe,
					rotation_model,
					d_topological_files,
					d_resolve_topology_parameters,
					default_resolve_topology_parameters,
					topological_snapshot_cache_size))
			{
				return scribe.get_transcribe_result();
			}
			d_rotation_model = rotation_model.get();
			d_default_resolve_topology_parameters = default_resolve_topology_parameters.get();
			d_topological_snapshot_cache_size = topological_snapshot_cache_size;

			// Initialise topological reconstruction (based on the construct parameters we just loaded).
			//
			// Note: The existing topological reconstruction in 'this' topological model must be old data
			//       because 'transcribed_construct_data' is false (ie, it was not transcribed) and so 'this'
			//       object must've been created first (using unknown constructor arguments) and *then* transcribed.
			initialise_topological_reconstruction();
		}
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


void
GPlatesApi::TopologicalModel::save_construct_data(
		GPlatesScribe::Scribe &scribe,
		const TopologicalModel &topological_model)
{
	// Save the rotation model.
	scribe.save(TRANSCRIBE_SOURCE, topological_model.d_rotation_model, "rotation_model");

	const GPlatesScribe::ObjectTag files_tag("files");

	// Save number of topological files.
	const unsigned int num_files = topological_model.d_topological_files.size();
	scribe.save(TRANSCRIBE_SOURCE, num_files, files_tag.sequence_size());

	// Save the topological files (feature collections and their filenames).
	for (unsigned int file_index = 0; file_index < num_files; ++file_index)
	{
		const auto feature_collection_file = topological_model.d_topological_files[file_index];

		const GPlatesModel::FeatureCollectionHandle::non_null_ptr_type feature_collection(
				feature_collection_file->get_reference().get_feature_collection().handle_ptr());
		const QString filename =
				feature_collection_file->get_reference().get_file_info().get_qfileinfo().absoluteFilePath();

		scribe.save(TRANSCRIBE_SOURCE, feature_collection, files_tag[file_index]("feature_collection"));
		scribe.save(TRANSCRIBE_SOURCE, filename, files_tag[file_index]("filename"));
	}

	// Save the resolved topology parameters.
	scribe.save(TRANSCRIBE_SOURCE, topological_model.d_resolve_topology_parameters, "resolve_topology_parameters");
	scribe.save(TRANSCRIBE_SOURCE, topological_model.d_default_resolve_topology_parameters, "default_resolve_topology_parameters");

	// Save the topological snapshot cache size.
	scribe.save(TRANSCRIBE_SOURCE, topological_model.d_topological_snapshot_cache_size, "topological_snapshot_cache_size");
}


bool
GPlatesApi::TopologicalModel::load_construct_data(
		GPlatesScribe::Scribe &scribe,
		GPlatesScribe::LoadRef<RotationModel::non_null_ptr_type> &rotation_model,
		std::vector<GPlatesFileIO::File::non_null_ptr_type> &topological_files,
		const std::vector<boost::optional<ResolveTopologyParameters::non_null_ptr_to_const_type>> &resolve_topology_parameters,
		GPlatesScribe::LoadRef<ResolveTopologyParameters::non_null_ptr_to_const_type> &default_resolve_topology_parameters,
		boost::optional<unsigned int> &topological_snapshot_cache_size)
{
	// Load the rotation model.
	rotation_model = scribe.load<RotationModel::non_null_ptr_type>(TRANSCRIBE_SOURCE, "rotation_model");
	if (!rotation_model.is_valid())
	{
		return false;
	}

	const GPlatesScribe::ObjectTag files_tag("files");

	// Number of topological files.
	unsigned int num_files;
	if (!scribe.transcribe(TRANSCRIBE_SOURCE, num_files, files_tag.sequence_size()))
	{
		return false;
	}

	// Load the topological files (feature collections and their filenames).
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

		topological_files.push_back(
				GPlatesFileIO::File::create_file(GPlatesFileIO::FileInfo(filename), feature_collection));
	}

	// Load the resolved topology parameters.
	//
	// Note: These must be loaded in the same order they were saved (see 'save_construct_data':
	//       per-file 'resolve_topology_parameters' first, then 'default_resolve_topology_parameters').
	//       The order matters for the "raw lane" (used by pickling) which is positional - there
	//       are no per-object tags to look up, so the load order must mirror the save order.
	if (!scribe.transcribe(TRANSCRIBE_SOURCE, resolve_topology_parameters, "resolve_topology_parameters"))
	{
		return false;
	}
	default_resolve_topology_parameters = scribe.load<ResolveTopologyParameters::non_null_ptr_to_const_type>(
			TRANSCRIBE_SOURCE, "default_resolve_topology_parameters");
	if (!default_resolve_topology_parameters.is_valid())
	{
		return false;
	}

	// Load the topological snapshot cache size.
	if (!scribe.transcribe(TRANSCRIBE_SOURCE, topological_snapshot_cache_size, "topological_snapshot_cache_size"))
	{
		return false;
	}

	return true;
}


void
export_topological_model()
{
	//
	// TopologyPointLocation - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<GPlatesAppLogic::TopologyPointLocation>(
					"TopologyPointLocation",
					"Locates a point in a specific resolved topological boundary or network (deforming region or interior rigid block).\n"
					"\n"
					"TopologyPointLocations are equality (``==``, ``!=``) comparable (but not hashable - cannot be used as a key in a ``dict``).\n"
					"\n"
					"  .. versionadded:: 0.29\n"
					"\n"
					"  .. versionchanged:: 0.47\n"
					"     Equality compares object *state* instead of object *identity*.\n",
					// Don't allow creation from python side...
					bp::no_init)
		.def("not_located_in_resolved_topology",
				&GPlatesApi::topology_point_not_located_in_resolved_topology,
				"not_located_in_resolved_topology()\n"
				"  Query if point is not located in any resolved topological boundaries or networks.\n"
				"\n"
				"  :returns: ``True`` if point is not located in any resolved topologies\n"
				"  :rtype: bool\n")
		.def("located_in_resolved_boundary",
				&GPlatesApi::topology_point_located_in_resolved_boundary,
				"located_in_resolved_boundary()\n"
				"  Query if point is located in a :class:`resolved topological boundary<ResolvedTopologicalBoundary>`.\n"
				"\n"
				"  :returns: the resolved topological boundary that contains the point, otherwise ``None``\n"
				"  :rtype: ResolvedTopologicalBoundary or None\n")
		.def("located_in_resolved_network",
				&GPlatesApi::topology_point_located_in_resolved_network,
				"located_in_resolved_network()\n"
				"  Query if point is located in a :class:`resolved topological network<ResolvedTopologicalNetwork>`.\n"
				"\n"
				"  :returns: the resolved topological network that contains the point, otherwise ``None``\n"
				"  :rtype: ResolvedTopologicalNetwork or None\n"
				"\n"
				"  .. note:: The point can be anywhere inside a resolved topological network - inside its deforming region "
				"or inside any one of its interior rigid blocks (if it has any).\n")
		.def("located_in_resolved_network_deforming_region",
				&GPlatesApi::topology_point_located_in_resolved_network_deforming_region,
				(bp::arg("return_network_triangle") = false),
				"located_in_resolved_network_deforming_region([return_network_triangle=False])\n"
				"  Query if point is located in the deforming region of a :class:`resolved topological network<ResolvedTopologicalNetwork>`.\n"
				"\n"
				"  :param return_network_triangle: Whether to also return the :class:`triangle <NetworkTriangulation.Triangle>` "
				"(in the network triangulation) containing the point. Defaults to ``False``.\n"
				"  :type return_network_triangle: bool\n"
				"  :returns: the resolved topological network whose deforming region contains the point (and the triangle in "
				"the network triangulation containing the point if *return_network_triangle* is ``True``), otherwise ``None``\n"
				"  :rtype: ResolvedTopologicalNetwork, or tuple[ResolvedTopologicalNetwork, NetworkTriangulation.Triangle], or None\n"
				"\n"
				"  .. note:: Returns ``None`` if point is inside a resolved topological network but is also inside one of "
				"its interior rigid blocks (and hence not inside its deforming region).\n"
				"\n"
				"  To locate the triangle (in the network triangulation) that contains the point:\n"
				"  ::\n"
				"\n"
				"    resolved_topological_network_and_triangle = topology_point_location.located_in_resolved_network_deforming_region(\n"
				"            return_network_triangle=True)\n"
				"    if resolved_topological_network_and_triangle:\n"
				"        resolved_topological_network, network_triangle_containing_point = resolved_topological_network_and_triangle\n"
				"\n"
				"  .. note:: | When a point location is queried, for example with :meth:`ResolvedTopologicalNetwork.get_point_location`, the point "
				"is first projected from 3D space into 2D projection space using the Lambert azimuthal equal-area projection (with projection centre "
				"at the centroid of the network's polygon boundary). Only then is the point tested against the triangles of the network's 2D Delaunay "
				"triangulation (also in the same 2D projection) to locate the containing 2D triangle.\n"
				"            | This means the original 3D point might not be contained by the 3D version of that triangle (ie, the 2D triangle with vertices unprojected back to 3D). "
				"For example, if you took the :class:`network triangle <NetworkTriangulation.Triangle>` (containing the 2D point) and created a :class:`polygon <PolygonOnSphere>` "
				"from its :attr:`3D vertices <NetworkTriangulation.Vertex.position>` and then did a :meth:`point-in-polygon test <PolygonOnSphere.is_point_in_polygon>` "
				"using the original 3D point then it could potentially be *outside* the polygon (3D triangle). This is because the triangle boundary lines in 2D space do not map "
				"to the triangle boundary lines in 3D space (since the projection is not gnomonic, ie, doesn't project 3D great circle arcs as *straight* 2D lines).\n"
				"            | This also means the network triangle containing the point might be :attr:`marked <NetworkTriangulation.Triangle.is_in_deforming_region>` "
				"as *outside* the deforming region. However the point is still considered to be *inside* the deforming region since it is inside the network's polygon boundary "
				"(and outside the network's interior rigid blocks, if any). And it can still have a *non-zero* :meth:`strain rate <ResolvedTopologicalNetwork.get_point_strain_rate>` "
				"due to :ref:`strain rate smoothing <pygplates_primer_strain_rate_smoothing>`.\n"
				"\n"
				"  .. versionchanged:: 0.50\n"
				"     Added *return_network_triangle* argument.\n")
		.def("located_in_resolved_network_rigid_block",
				&GPlatesApi::topology_point_located_in_resolved_network_rigid_block,
				"located_in_resolved_network_rigid_block()\n"
				"  Query if point is located in an interior rigid block of a :class:`resolved topological network<ResolvedTopologicalNetwork>`.\n"
				"\n"
				"  :returns: tuple of resolved topological network and its interior rigid block (that contains the point), otherwise ``None``\n"
				"  :rtype: tuple[ResolvedTopologicalNetwork, ReconstructedFeatureGeometry], or None\n"
				"\n"
				"  .. note:: Returns ``None`` if point is inside a resolved topological network but is *not* inside one of "
				"its interior rigid blocks.\n")
		// Due to the numerical tolerance in comparisons we cannot make hashable.
		// Make unhashable, with no *equality* comparison operators (we explicitly define them)...
		.def(GPlatesApi::NoHashDefVisitor(false, true))
		.def(bp::self == bp::self)
		.def(bp::self != bp::self)
	;

	// Enable boost::optional<TopologyPointLocation> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::register_optional_conversion<GPlatesAppLogic::TopologyPointLocation>();


	{
		//
		// ReconstructedGeometryTimeSpan - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
		//
		bp::scope reconstructed_geometry_time_span_class = bp::class_<
				GPlatesApi::ReconstructedGeometryTimeSpan,
				GPlatesApi::ReconstructedGeometryTimeSpan::non_null_ptr_type,
				boost::noncopyable>(
						"ReconstructedGeometryTimeSpan",
						"A history of geometries :meth:`reconstructed using topologies <TopologicalModel.reconstruct_geometry>` over geological time.\n"
						"\n"
						".. seealso:: :ref:`pygplates_primer_reconstructed_geometry_time_span` in the *Primer* documentation.\n"
						"\n"
						".. versionadded:: 0.29\n",
						// Don't allow creation from python side...
						// (Also there is no publicly-accessible default constructor).
						bp::no_init)
			.def("get_time_span",
					&GPlatesApi::reconstructed_geometry_time_span_get_time_span,
					"get_time_span()\n"
					"  Returns the time span of the history of reconstructed geometries.\n"
					"\n"
					"  :returns: the 4-tuple of (oldest time, youngest time, time increment, number of time slots)\n"
					"  :rtype: tuple[float, float, float, int]\n"
					"\n"
					"  The oldest time, youngest time and time increment are the same as were specified in :meth:`TopologicalModel.reconstruct_geometry`. "
					"And the number of time slots is :math:`\\frac{(oldest\\_time - youngest\\_time)}{time\\_increment}` which is an integer value "
					"(since :meth:`TopologicalModel.reconstruct_geometry` requires the oldest to youngest time period to be an integer multiple of the time increment).\n"
					"\n"
					"  .. seealso:: :ref:`pygplates_primer_reconstructed_geometry_time_span` in the *Primer* documentation.\n"
					"\n"
					"  .. versionadded:: 0.50\n")
			.def("get_geometry_points",
					&GPlatesApi::reconstructed_geometry_time_span_get_geometry_points,
					(bp::arg("reconstruction_time"),
						bp::arg("return_inactive_points") = false),
					"get_geometry_points(reconstruction_time, [return_inactive_points=False])\n"
					"  Returns geometry points at a specific reconstruction time.\n"
					"\n"
					"  :param reconstruction_time: Time to extract reconstructed geometry points. "
					"Can be any non-negative time (doesn't have to be an integer and can be outside the :meth:`time span <get_time_span>`).\n"
					"  :type reconstruction_time: float or GeoTimeInstant\n"
					"  :param return_inactive_points: Whether to return inactive geometry points. "
					"If ``True`` then each inactive point stores ``None`` instead of a point and hence the size of the ``list`` "
					"of points is equal to the number of points in the initial geometry (which are all initially active). "
					"By default only active points are returned.\n"
					"  :type return_inactive_points: bool\n"
					"  :returns: list of :class:`PointOnSphere`, or ``None`` if no points are active at *reconstruction_time*\n"
					"  :rtype: list[PointOnSphere], or None\n"
					"  :raises: ValueError if *reconstruction_time* is "
					":meth:`distant past<GeoTimeInstant.is_distant_past>` or "
					":meth:`distant future<GeoTimeInstant.is_distant_future>`\n"
					"\n"
					"  .. seealso:: :ref:`pygplates_primer_reconstructed_geometry_time_span_geometry_points` in the *Primer* documentation.\n")
			.def("get_topology_point_locations",
					&GPlatesApi::reconstructed_geometry_time_span_get_topology_point_locations,
					(bp::arg("reconstruction_time"),
						bp::arg("return_inactive_points") = false),
					"get_topology_point_locations(reconstruction_time, [return_inactive_points=False])\n"
					"  Returns the locations of geometry points in resolved topologies at a specific reconstruction time.\n"
					"\n"
					"  :param reconstruction_time: Time to extract topology point locations. "
					"Can be any non-negative time (doesn't have to be an integer and can be outside the :meth:`time span <get_time_span>`).\n"
					"  :type reconstruction_time: float or GeoTimeInstant\n"
					"  :param return_inactive_points: Whether to return topology locations associated with inactive points. "
					"If ``True`` then each topology location corresponding to an inactive point stores ``None`` instead of a "
					"topology location and hence the size of the ``list`` of topology locations is equal to the number of points "
					"in the initial geometry (which are all initially active). "
					"By default only topology locations for active points are returned.\n"
					"  :type return_inactive_points: bool\n"
					"  :returns: list of :class:`TopologyPointLocation`, or ``None`` if no points are active at *reconstruction_time*\n"
					"  :rtype: list[TopologyPointLocation], or None\n"
					"  :raises: ValueError if *reconstruction_time* is "
					":meth:`distant past<GeoTimeInstant.is_distant_past>` or "
					":meth:`distant future<GeoTimeInstant.is_distant_future>`\n"
					"\n"
					"  .. seealso:: :ref:`pygplates_primer_reconstructed_geometry_time_span_topology_locations` in the *Primer* documentation.\n")
			.def("get_strains",
					&GPlatesApi::reconstructed_geometry_time_span_get_strains,
					(bp::arg("reconstruction_time"),
						bp::arg("return_inactive_points") = false),
					"get_strains(reconstruction_time, [return_inactive_points=False])\n"
					"  Returns the strains accumulated at geometry points in resolved topologies at a specific reconstruction time.\n"
					"\n"
					"  :param reconstruction_time: Time to extract accumulated strains. "
					"Can be any non-negative time (doesn't have to be an integer and can be outside the :meth:`time span <get_time_span>`).\n"
					"  :type reconstruction_time: float or GeoTimeInstant\n"
					"  :param return_inactive_points: Whether to return strains associated with inactive points. "
					"If ``True`` then each strain corresponding to an inactive point stores ``None`` instead of a "
					"strain and hence the size of the ``list`` of strains is equal to the number of points "
					"in the initial geometry (which are all initially active). "
					"By default only strains for active points are returned.\n"
					"  :type return_inactive_points: bool\n"
					"  :returns: list of :class:`Strain`, or ``None`` if no points are active at *reconstruction_time*\n"
					"  :rtype: list[Strain], or None\n"
					"  :raises: ValueError if *reconstruction_time* is "
					":meth:`distant past<GeoTimeInstant.is_distant_past>` or "
					":meth:`distant future<GeoTimeInstant.is_distant_future>`\n"
					"\n"
					"  .. seealso:: :ref:`pygplates_primer_reconstructed_geometry_time_span_strains` in the *Primer* documentation.\n"
					"\n"
					"  .. versionadded:: 0.46\n")
			.def("get_strain_rates",
					&GPlatesApi::reconstructed_geometry_time_span_get_strain_rates,
					(bp::arg("reconstruction_time"),
						bp::arg("return_inactive_points") = false),
					"get_strain_rates(reconstruction_time, [return_inactive_points=False])\n"
					"  Returns the strain rates at geometry points in resolved topologies at a specific reconstruction time.\n"
					"\n"
					"  :param reconstruction_time: Time to extract strain rates. "
					"Can be any non-negative time (doesn't have to be an integer and can be outside the :meth:`time span <get_time_span>`).\n"
					"  :type reconstruction_time: float or GeoTimeInstant\n"
					"  :param return_inactive_points: Whether to return strain rates associated with inactive points. "
					"If ``True`` then each strain rate corresponding to an inactive point stores ``None`` instead of a "
					"strain rate and hence the size of the ``list`` of strain rates is equal to the number of points "
					"in the initial geometry (which are all initially active). "
					"By default only strain rates for active points are returned.\n"
					"  :type return_inactive_points: bool\n"
					"  :returns: list of :class:`StrainRate`, or ``None`` if no points are active at *reconstruction_time*\n"
					"  :rtype: list[StrainRate], or None\n"
					"  :raises: ValueError if *reconstruction_time* is "
					":meth:`distant past<GeoTimeInstant.is_distant_past>` or "
					":meth:`distant future<GeoTimeInstant.is_distant_future>`\n"
					"\n"
					"  .. seealso:: :ref:`pygplates_primer_reconstructed_geometry_time_span_strain_rates` in the *Primer* documentation.\n"
					"\n"
					"  .. note:: Strain rates in deforming networks are calculated from the spatial gradients of velocity where the velocities are calculated over "
					"a 1 Myr time interval and using the *equatorial* Earth radius :class:`pygplates.Earth.equatorial_radius_in_kms <Earth>`.\n"
					"\n"
					"  .. versionadded:: 0.46\n")
			.def("get_velocities",
					&GPlatesApi::reconstructed_geometry_time_span_get_velocities,
					(bp::arg("reconstruction_time"),
						bp::arg("velocity_delta_time") = 1.0,
						bp::arg("velocity_delta_time_type") = GPlatesAppLogic::VelocityDeltaTime::T_PLUS_DELTA_T_TO_T,
						bp::arg("velocity_units") = GPlatesAppLogic::VelocityUnits::KMS_PER_MY,
						bp::arg("earth_radius_in_kms") = GPlatesUtils::Earth::MEAN_RADIUS_KMS,
						bp::arg("return_inactive_points") = false),
					"get_velocities(reconstruction_time, [velocity_delta_time=1.0], [velocity_delta_time_type=pygplates.VelocityDeltaTimeType.t_plus_delta_t_to_t], "
					"[velocity_units=pygplates.VelocityUnits.kms_per_my], [earth_radius_in_kms=pygplates.Earth.mean_radius_in_kms], [return_inactive_points=False])\n"
					"  Returns the velocities at geometry points in resolved topologies at a specific reconstruction time.\n"
					"\n"
					"  :param reconstruction_time: Time to extract velocities. "
					"Can be any non-negative time (doesn't have to be an integer and can be outside the :meth:`time span <get_time_span>`).\n"
					"  :type reconstruction_time: float or GeoTimeInstant\n"
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
					"  :param return_inactive_points: Whether to return velocities associated with inactive points. "
					"If ``True`` then each velocity corresponding to an inactive point stores ``None`` instead of a "
					"velocity and hence the size of the ``list`` of velocities is equal to the number of points "
					"in the initial geometry (which are all initially active). "
					"By default only velocities for active points are returned.\n"
					"  :type return_inactive_points: bool\n"
					"  :returns: list of :class:`Vector3D`, or ``None`` if no points are active at *reconstruction_time*\n"
					"  :rtype: list[Vector3D], or None\n"
					"  :raises: ValueError if *reconstruction_time* is "
					":meth:`distant past<GeoTimeInstant.is_distant_past>` or "
					":meth:`distant future<GeoTimeInstant.is_distant_future>`\n"
					"  :raises: ValueError if *velocity_delta_time* is negative or zero.\n"
					"\n"
					"  .. seealso:: :ref:`pygplates_primer_reconstructed_geometry_time_span_velocities` in the *Primer* documentation.\n"
					"\n"
					"  .. versionadded:: 0.46\n"
					"\n"
					"  .. versionchanged:: 0.47\n"
					"     Added *earth_radius_in_kms* argument (that defaults to *pygplates.Earth.mean_radius_in_kms*). "
					"Previously *pygplates.Earth.equatorial_radius_in_kms* was hardwired internally).\n")
			.def("get_scalar_values",
					&GPlatesApi::reconstructed_geometry_time_span_get_scalar_values,
					(bp::arg("reconstruction_time"),
						bp::arg("scalar_type") = boost::optional<GPlatesPropertyValues::ValueObjectType>(),
						bp::arg("return_inactive_points") = false),
					"get_scalar_values(reconstruction_time, [scalar_type], [return_inactive_points=False])\n"
					"  Returns scalar values at a specific reconstruction time either for a single scalar type (as a ``list``) or "
					"for all scalar types (as a ``dict``).\n"
					"\n"
					"  :param reconstruction_time: Time to extract reconstructed scalar values. "
					"Can be any non-negative time (doesn't have to be an integer and can be outside the :meth:`time span <get_time_span>`).\n"
					"  :type reconstruction_time: float or GeoTimeInstant\n"
					"  :param scalar_type: Optional scalar type to retrieve scalar values for (returned as a ``list``). "
					"If not specified then all scalar values for all scalar types are returned (returned as a ``dict``).\n"
					"  :type scalar_type: ScalarType\n"
					"  :param return_inactive_points: Whether to return scalars associated with inactive points. "
					"If ``True`` then each scalar corresponding to an inactive point stores ``None`` instead of a scalar and hence "
					"the size of each ``list`` of scalars is equal to the number of points (and scalars) in the initial geometry "
					"(which are all initially active). "
					"By default only scalars for active points are returned.\n"
					"  :type return_inactive_points: bool\n"
					"  :returns: If *scalar_type* is specified then a ``list`` of scalar values associated with *scalar_type* "
					"at *reconstruction_time* (or ``None`` if no matching scalar type), otherwise a ``dict`` mapping available "
					"scalar types with their associated scalar values ``list`` at *reconstruction_time* (or ``None`` if no scalar types "
					"are available). Returns ``None`` if no points are active at *reconstruction_time*.\n"
					"  :rtype: list[float], or dict[ScalarType, list[float]], or None\n"
					"  :raises: ValueError if *reconstruction_time* is "
					":meth:`distant past<GeoTimeInstant.is_distant_past>` or "
					":meth:`distant future<GeoTimeInstant.is_distant_future>`\n"
					"\n"
					"  .. seealso:: :ref:`pygplates_primer_reconstructed_geometry_time_span_scalar_values` in the *Primer* documentation.\n")
			.def("get_tectonic_subsidences",
					&GPlatesApi::reconstructed_geometry_time_span_get_tectonic_subsidences,
					(bp::arg("reconstruction_time"),
						bp::arg("return_inactive_points") = false),
					"get_tectonic_subsidences(reconstruction_time, [return_inactive_points=False])\n"
					"  Returns tectonic subsidence values (in kms) at a specific reconstruction time.\n"
					"\n"
					"  :param reconstruction_time: Time to extract reconstructed tectonic subsidence values. "
					"Can be any non-negative time (doesn't have to be an integer and can be outside the :meth:`time span <get_time_span>`).\n"
					"  :type reconstruction_time: float or GeoTimeInstant\n"
					"  :param return_inactive_points: Whether to return tectonic subsidence values associated with inactive points. "
					"If ``True`` then each tectonic subsidence value corresponding to an inactive point stores ``None`` and hence "
					"the size of the returned ``list`` is equal to the number of points in the initial geometry (which are all initially active). "
					"By default only tectonic subsidence values for active points are returned.\n"
					"  :type return_inactive_points: bool\n"
					"  :returns: list of float, or ``None`` if no points are active at *reconstruction_time*\n"
					"  :rtype: list[float], or None\n"
					"  :raises: ValueError if *reconstruction_time* is "
					":meth:`distant past<GeoTimeInstant.is_distant_past>` or "
					":meth:`distant future<GeoTimeInstant.is_distant_future>`\n"
					"\n"
					"  .. seealso:: :ref:`pygplates_primer_reconstructed_geometry_time_span_tectonic_subsidence` in the *Primer* documentation.\n"
					"\n"
					"  .. versionadded:: 0.50\n")
			.def("get_crustal_thicknesses",
					&GPlatesApi::reconstructed_geometry_time_span_get_crustal_thicknesses,
					(bp::arg("reconstruction_time"),
						bp::arg("return_inactive_points") = false),
					"get_crustal_thicknesses(reconstruction_time, [return_inactive_points=False])\n"
					"  Returns crustal thicknesses (in kms) at a specific reconstruction time.\n"
					"\n"
					"  :param reconstruction_time: Time to extract reconstructed crustal thicknesses. "
					"Can be any non-negative time (doesn't have to be an integer and can be outside the :meth:`time span <get_time_span>`).\n"
					"  :type reconstruction_time: float or GeoTimeInstant\n"
					"  :param return_inactive_points: Whether to return crustal thicknesses associated with inactive points. "
					"If ``True`` then each crustal thickness corresponding to an inactive point stores ``None`` and hence "
					"the size of the returned ``list`` is equal to the number of points in the initial geometry (which are all initially active). "
					"By default only crustal thicknesses for active points are returned.\n"
					"  :type return_inactive_points: bool\n"
					"  :returns: list of float, or ``None`` if no points are active at *reconstruction_time*\n"
					"  :rtype: list[float], or None\n"
					"  :raises: ValueError if *reconstruction_time* is "
					":meth:`distant past<GeoTimeInstant.is_distant_past>` or "
					":meth:`distant future<GeoTimeInstant.is_distant_future>`\n"
					"\n"
					"  .. seealso:: :ref:`pygplates_primer_reconstructed_geometry_time_span_crustal_thickness_factors` in the *Primer* documentation.\n"
					"\n"
					"  .. versionadded:: 0.50\n")
			.def("get_crustal_stretching_factors",
					&GPlatesApi::reconstructed_geometry_time_span_get_crustal_stretching_factors,
					(bp::arg("reconstruction_time"),
						bp::arg("return_inactive_points") = false),
					"get_crustal_stretching_factors(reconstruction_time, [return_inactive_points=False])\n"
					"  Returns crustal stretching factors at a specific reconstruction time.\n"
					"\n"
					"  :param reconstruction_time: Time to extract reconstructed crustal stretching factors. "
					"Can be any non-negative time (doesn't have to be an integer and can be outside the :meth:`time span <get_time_span>`).\n"
					"  :type reconstruction_time: float or GeoTimeInstant\n"
					"  :param return_inactive_points: Whether to return crustal stretching factors associated with inactive points. "
					"If ``True`` then each crustal stretching factor corresponding to an inactive point stores ``None`` and hence "
					"the size of the returned ``list`` is equal to the number of points in the initial geometry (which are all initially active). "
					"By default only crustal stretching factors for active points are returned.\n"
					"  :type return_inactive_points: bool\n"
					"  :returns: list of float, or ``None`` if no points are active at *reconstruction_time*\n"
					"  :rtype: list[float], or None\n"
					"  :raises: ValueError if *reconstruction_time* is "
					":meth:`distant past<GeoTimeInstant.is_distant_past>` or "
					":meth:`distant future<GeoTimeInstant.is_distant_future>`\n"
					"\n"
					"  .. seealso:: :ref:`pygplates_primer_reconstructed_geometry_time_span_crustal_thickness_factors` in the *Primer* documentation.\n"
					"\n"
					"  .. versionadded:: 0.50\n")
			.def("get_crustal_thinning_factors",
					&GPlatesApi::reconstructed_geometry_time_span_get_crustal_thinning_factors,
					(bp::arg("reconstruction_time"),
						bp::arg("return_inactive_points") = false),
					"get_crustal_thinning_factors(reconstruction_time, [return_inactive_points=False])\n"
					"  Returns crustal thinning factors at a specific reconstruction time.\n"
					"\n"
					"  :param reconstruction_time: Time to extract reconstructed crustal thinning factors. "
					"Can be any non-negative time (doesn't have to be an integer and can be outside the :meth:`time span <get_time_span>`).\n"
					"  :type reconstruction_time: float or GeoTimeInstant\n"
					"  :param return_inactive_points: Whether to return crustal thinning factors associated with inactive points. "
					"If ``True`` then each crustal thinning factor corresponding to an inactive point stores ``None`` and hence "
					"the size of the returned ``list`` is equal to the number of points in the initial geometry (which are all initially active). "
					"By default only crustal thinning factors for active points are returned.\n"
					"  :type return_inactive_points: bool\n"
					"  :returns: list of float, or ``None`` if no points are active at *reconstruction_time*\n"
					"  :rtype: list[float], or None\n"
					"  :raises: ValueError if *reconstruction_time* is "
					":meth:`distant past<GeoTimeInstant.is_distant_past>` or "
					":meth:`distant future<GeoTimeInstant.is_distant_future>`\n"
					"\n"
					"  .. seealso:: :ref:`pygplates_primer_reconstructed_geometry_time_span_crustal_thickness_factors` in the *Primer* documentation.\n"
					"\n"
					"  .. versionadded:: 0.50\n")
			// Make hash and comparisons based on C++ object identity (not python object identity)...
			.def(GPlatesApi::ObjectIdentityHashDefVisitor())
		;

		// Register to/from Python conversions of non_null_intrusive_ptr<> including const/non-const and boost::optional.
		GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<GPlatesApi::ReconstructedGeometryTimeSpan>();


		//
		// ReconstructedGeometryTimeSpan.DeactivatePoints - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
		//
		// A class nested within python class ReconstructedGeometryTimeSpan (due to above 'bp::scope').
		bp::class_<
				GPlatesAppLogic::TopologyReconstruct::DeactivatePoint,
				GPlatesApi::ReconstructedGeometryTimeSpan::DeactivatePointWrapper::non_null_ptr_type,
				boost::noncopyable>(
						"DeactivatePoints",
						// NOTE: It seems Sphinx does not document '__init__' for nested classes (tested with Sphinx 3.4.3).
						//       Instead we'll document it in this *class* docstring.
						"The base class interface for deactivating geometry points as they are reconstructed forward and/or backward in time.\n"
						"\n"
						"To create your own class that inherits this base class and overrides its "
						":meth:`deactivate method <ReconstructedGeometryTimeSpan.DeactivatePoints.deactivate>` and then use that "
						"when :meth:`reconstructing a geometry using topologies <TopologicalModel.reconstruct_geometry>`:\n"
						"::\n"
						"\n"
						"  class MyDeactivatePoints(pygplates.ReconstructedGeometryTimeSpan.DeactivatePoints):\n"
						"      def __init__(self):\n"
						"          super(MyDeactivatePoints, self).__init__()\n"
						"          # Other initialisation you may want...\n"
						"          ...\n"
						"      def deactivate(self, prev_point, prev_location, prev_time, current_point, current_location, current_time):\n"
						"          # Implement your deactivation algorithm here...\n"
						"          ...\n"
						"          return ...\n"
						"  \n"
						"  # Reconstruct points in 'geometry' from 100Ma to present day using class MyDeactivatePoints to deactivate them (in this case subduct).\n"
						"  topological_model.reconstruct_geometry(geometry, 100, deactivate_points=MyDeactivatePoints())\n"
						"\n"
						".. warning:: If you create your own Python class that inherits this base class then you must call the base class *__init__* "
						"method otherwise you will get a *Boost.Python.ArgumentError* exception. Note that if you do not define an *__init__* method "
						"in your derived class then Python will call the base class *__init__* (so you don't have to do anything). "
						"However if you do define *__init__* in your derived class then it must explicitly call the base class *__init__*.\n"
						"\n"
						".. seealso:: :ref:`pygplates_primer_using_topological_reconstruction_deactivating_points` in the *Primer* documentation.\n"
						"\n"
						".. versionadded:: 0.31\n"
						"\n"
						"__init__()\n"
						"  Default constructor - must be explicitly called by derived class.\n"
						// NOTE: Must not define 'bp::no_init' because this base class is meant to be inherited by a python class.
					)
			.def("deactivate",
					bp::pure_virtual(&GPlatesAppLogic::TopologyReconstruct::DeactivatePoint::deactivate),
					(bp::arg("prev_point"),
						bp::arg("prev_location"),
						bp::arg("prev_time"),
						bp::arg("current_point"),
						bp::arg("current_location"),
						bp::arg("current_time")),
					// NOTE: It seems Sphinx does properly document parameters of methods of nested classes (tested with Sphinx 3.4.3).
					//       Instead we'll document the parameters using a list.
					//       (The stub generator understands this bullet convention too - see pygplates/stub/generate_stub.py.)
					//
					// The signature line must match 'DeactivatePoint::deactivate' (and the bp::arg list above).
					"deactivate(prev_point, prev_location, prev_time, current_point, current_location, current_time)\n"
					"  Return true if the point should be deactivated.\n"
					"\n"
					"  * **prev_point** (:class:`PointOnSphere`): the previous position of the point\n"
					"\n"
					"  * **prev_location** (:class:`TopologyPointLocation`): the previous location of the point in the topologies\n"
					"\n"
					"  * **prev_time** (float or :class:`GeoTimeInstant`): the time associated with the previous position of the point\n"
					"\n"
					"  * **current_point** (:class:`PointOnSphere`): the current position of the point\n"
					"\n"
					"  * **current_location** (:class:`TopologyPointLocation`): the current location of the point in the topologies\n"
					"\n"
					"  * **current_time** (float or :class:`GeoTimeInstant`): the time associated with the current position of the point\n"
					"\n"
					"  * **Return type**: bool\n"
					"\n"
					"  The above parameters represent the previous and current position/location-in-topologies/time of a single point in the "
					":meth:`geometry being reconstructed <TopologicalModel.reconstruct_geometry>`. If you return ``True`` then the point will be "
					"deactivated and will not have a position at the *next* time (where ``next_time = current_time + (current_time - prev_time)``).\n"
					"\n"
					"  .. note:: If the current time is *younger* than the previous time (``current_time < prev_time``) then we are reconstructing "
					"*forward* in time and the next time will be *younger* than the current time (``next_time < current_time``). Conversely, if "
					"the current time is *older* than the previous time (``current_time > prev_time``) then we are reconstructing "
					"*backward* in time and the next time will be *older* than the current time (``next_time > current_time``).\n"
					"\n"
					"  .. note:: This function is called for each point that is reconstructed using :meth:`TopologicalModel.reconstruct_geometry` "
					"at each time step.\n"
					"\n"
					// For some reason Sphinx (tested version 3.4.3) seems to repeat this docstring twice (not sure why, so we'll let users know)...
					"  .. note:: This function might be inadvertently documented twice.\n"
					"\n"
					"  .. seealso:: :ref:`pygplates_primer_using_topological_reconstruction_deactivating_points` in the *Primer* documentation.\n")
		;

		// Enable GPlatesAppLogic::TopologyReconstruct::DeactivatePoint::non_null_ptr_type to be stored in a Python object.
		// Normally the HeldType GPlatesApi::ReconstructedGeometryTimeSpan::DeactivatePointWrapper::non_null_ptr_type is stored.
		//
		// For example, this enables:
		//
		//   bp::arg("deactivate_points") = boost::optional<GPlatesAppLogic::TopologyReconstruct::DeactivatePoint::non_null_ptr_to_const_type>()
		//
		// ...as a default argument in 'TopologicalModel.reconstruct_geometry()' because bp::arg stores its default value in a Python object.
		//
		// Without 'bp::register_ptr_to_python<GPlatesAppLogic::TopologyReconstruct::DeactivatePoint::non_null_ptr_type>()'
		// (and also 'PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<GPlatesAppLogic::TopologyReconstruct::DeactivatePoint>()')
		// the above bp::arg statement would fail.
		// 
		bp::register_ptr_to_python<GPlatesAppLogic::TopologyReconstruct::DeactivatePoint::non_null_ptr_type>();

		GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<
				GPlatesAppLogic::TopologyReconstruct::DeactivatePoint>();


		// Docstring for class pygplates.ReconstructedGeometryTimeSpan.DefaultDeactivatePoints.
		//
		// NOTE: It seems Sphinx does not document '__init__' for nested classes (tested with Sphinx 3.4.3).
		//       Instead we'll document it in this *class* docstring.
		std::stringstream default_deactivate_points_class_docstring_stream;
		default_deactivate_points_class_docstring_stream <<
				"The default algorithm for deactivating geometry points as they are reconstructed forward and/or backward in time.\n"
				"\n"
				".. seealso:: :ref:`pygplates_primer_using_topological_reconstruction_deactivating_points` in the *Primer* documentation.\n"
				"\n"
				".. versionadded:: 0.31\n"
				"\n"
				"__init__([threshold_velocity_delta="
				<< GPlatesAppLogic::TopologyReconstruct::DefaultDeactivatePoint::DEFAULT_THRESHOLD_VELOCITY_DELTA
				<< "], [threshold_distance_to_boundary="
				<< GPlatesAppLogic::TopologyReconstruct::DefaultDeactivatePoint::DEFAULT_THRESHOLD_DISTANCE_TO_BOUNDARY_IN_KMS_PER_MY
				<< "], [deactivate_points_that_fall_outside_a_network="
				<< (GPlatesAppLogic::TopologyReconstruct::DefaultDeactivatePoint::DEFAULT_DEACTIVATE_POINTS_THAT_FALL_OUTSIDE_A_NETWORK ? "True" : "False")
				<< "])\n"
				"  Create default algorithm for deactivating points using the specified parameters.\n"
				"\n"
				"  * **threshold_velocity_delta** (float): A point that transitions from one plate/network to another can "
				"disappear if the change in velocity exceeds this threshold (in units of cms/yr). Defaults to ``"
				<< GPlatesAppLogic::TopologyReconstruct::DefaultDeactivatePoint::DEFAULT_THRESHOLD_VELOCITY_DELTA
				<< "`` cms/yr.\n"
				"\n"
				"  * **threshold_distance_to_boundary** (float): Only those transitioning points exceeding the "
				"*threshold velocity delta* **and** that are close enough to a plate/network boundary can disappear. "
				"The distance is proportional to the relative velocity (change in velocity), plus a constant offset based on the "
				"*threshold distance to boundary* (in units of kms/myr) to account for plate boundaries that change shape significantly "
				"from one time step to the next (note that some boundaries are meant to do this and others are a result of digitisation). "
				"The actual distance threshold used is ``(threshold_distance_to_boundary + relative_velocity) * time_increment``. Defaults to ``"
				<< GPlatesAppLogic::TopologyReconstruct::DefaultDeactivatePoint::DEFAULT_THRESHOLD_DISTANCE_TO_BOUNDARY_IN_KMS_PER_MY
				<< "`` kms/myr.\n"
				"\n"
				"  * **deactivate_points_that_fall_outside_a_network** (bool): Whether to have points inside a deforming network "
				"disappear as soon as they fall outside all deforming networks. This is useful for initial crustal thickness points that have "
				"been generated inside a deforming network and where subsequently deformed points should be limited to the deformed network regions. "
				"In this case sudden large changes to the deforming network boundary can progressively exclude points over time. "
				"However in the case where the topologies (deforming networks and rigid plates) have global coverage this option should "
				"generally be left disabled so that points falling outside deforming networks can then be reconstructed using rigid plates. "
				"And these rigidly reconstructed points may even re-enter a subsequent deforming network. Defaults to ``"
				<< (GPlatesAppLogic::TopologyReconstruct::DefaultDeactivatePoint::DEFAULT_DEACTIVATE_POINTS_THAT_FALL_OUTSIDE_A_NETWORK ? "True" : "False")
				<< "``.\n"
				"\n"
				"  .. seealso:: :ref:`pygplates_primer_using_topological_reconstruction_deactivating_points` in the *Primer* documentation.\n"
				;

		//
		// ReconstructedGeometryTimeSpan.DeactivatePoints - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
		//
		// A class nested within python class ReconstructedGeometryTimeSpan (due to above 'bp::scope').
		bp::class_<
				GPlatesAppLogic::TopologyReconstruct::DefaultDeactivatePoint,
				GPlatesAppLogic::TopologyReconstruct::DefaultDeactivatePoint::non_null_ptr_type,
				bp::bases<GPlatesAppLogic::TopologyReconstruct::DeactivatePoint>,
				boost::noncopyable>(
						"DefaultDeactivatePoints",
						default_deactivate_points_class_docstring_stream.str().c_str(),
						// There is no publicly-accessible default constructor...
						bp::no_init)
			.def("__init__",
					bp::make_constructor(
							&GPlatesApi::reconstructed_geometry_time_span_default_deactivate_points_create,
							bp::default_call_policies(),
							(bp::arg("threshold_velocity_delta") =
									GPlatesAppLogic::TopologyReconstruct::DefaultDeactivatePoint::DEFAULT_THRESHOLD_VELOCITY_DELTA,
								bp::arg("threshold_distance_to_boundary") =
									GPlatesAppLogic::TopologyReconstruct::DefaultDeactivatePoint::DEFAULT_THRESHOLD_DISTANCE_TO_BOUNDARY_IN_KMS_PER_MY,
								bp::arg("deactivate_points_that_fall_outside_a_network") =
									GPlatesAppLogic::TopologyReconstruct::DefaultDeactivatePoint::DEFAULT_DEACTIVATE_POINTS_THAT_FALL_OUTSIDE_A_NETWORK)))
		;

		// Register to/from Python conversions of non_null_intrusive_ptr<> including const/non-const and boost::optional.
		GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<
				GPlatesAppLogic::TopologyReconstruct::DefaultDeactivatePoint>();
	}


	//
	// TopologicalModel - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesApi::TopologicalModel,
			GPlatesApi::TopologicalModel::non_null_ptr_type,
			boost::noncopyable>(
					"TopologicalModel",
					"A history of topologies over geological time.\n"
					"\n"
					".. seealso:: :ref:`pygplates_primer_topological_model` in the *Primer* documentation.\n"
					"\n"
					"A *TopologicalModel* can also be `pickled <https://docs.python.org/3/library/pickle.html>`_.\n"
					"\n"
					".. versionadded:: 0.30\n"
					"\n"
					".. versionchanged:: 0.42\n"
					"   Added pickle support.\n",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::topological_model_create,
						bp::default_call_policies(),
						(bp::arg("topological_features"),
							bp::arg("rotation_model"),
							bp::arg("anchor_plate_id") = boost::optional<GPlatesModel::integer_plate_id_type>(),
							bp::arg("default_resolve_topology_parameters") =
								boost::optional<GPlatesApi::ResolveTopologyParameters::non_null_ptr_to_const_type>(),
							bp::arg("topological_snapshot_cache_size") = boost::optional<unsigned int>())),
			"__init__(topological_features, rotation_model, [anchor_plate_id], [default_resolve_topology_parameters], [topological_snapshot_cache_size])\n"
			"  Create from topological features and a rotation model.\n"
			"\n"
			"  :param topological_features: The topological boundary and/or network features and the "
			"topological section features they reference (regular and topological lines) as a feature collection, "
			"or filename, or feature, or sequence of features, or a sequence (eg, ``list`` or ``tuple``) "
			"of any combination of those four types. **Note**: Each entry can optionally be a 2-tuple "
			"(entry, :class:`ResolveTopologyParameters`) to override *default_resolve_topology_parameters* for that entry.\n"
			"  :type topological_features: FeatureCollection, or str, or os.PathLike, or Feature, "
			"or sequence of Feature, or sequence of any combination of those four types\n"
			"  :param rotation_model: A rotation model. Or a rotation feature collection, or a rotation filename, "
			"or a rotation feature, or a sequence of rotation features, or a sequence of any combination of those four types.\n"
			"  :type rotation_model: RotationModel. Or FeatureCollection, or str, or os.PathLike, "
			"or Feature, or sequence of Feature, or sequence of any combination of those four types\n"
			"  :param anchor_plate_id: The anchored plate id used for all reconstructions "
			"(resolving topologies, and reconstructing regular features and :meth:`geometries<reconstruct_geometry>`). "
			"Defaults to the default anchor plate of *rotation_model* (or zero if *rotation_model* is not a :class:`RotationModel`).\n"
			"  :type anchor_plate_id: int\n"
			"  :param default_resolve_topology_parameters: Default parameters used to resolve topologies. "
			"Note that these can optionally be overridden in *topological_features*. "
			"Defaults to :meth:`default-constructed ResolveTopologyParameters<ResolveTopologyParameters.__init__>`).\n"
			"  :type default_resolve_topology_parameters: ResolveTopologyParameters\n"
			"  :param topological_snapshot_cache_size: Number of topological snapshots to cache internally. Defaults to unlimited.\n"
			"  :type topological_snapshot_cache_size: int\n"
			"\n"
			"  .. seealso:: :ref:`pygplates_primer_topological_model` in the *Primer* documentation.\n"
			"\n"
			"  .. note:: All reconstructions (including resolving topologies and reconstructing regular features and "
			":meth:`geometries<reconstruct_geometry>`) use *anchor_plate_id*. So if you need to use a different "
			"anchor plate ID then you'll need to create a new :class:`TopologicalModel<__init__>`. However this should "
			"only be done if necessary since each :class:`TopologicalModel` created can consume a reasonable amount of "
			"CPU and memory (since it caches resolved topologies and reconstructed geometries over geological time).\n"
			"\n"
			"  .. note:: The *topological_snapshot_cache_size* parameter controls "
			"the size of an internal least-recently-used cache of topological snapshots "
			"(evicts least recently requested topological snapshot when a new reconstruction "
			"time is requested that does not currently exist in the cache). This enables "
			"topological snapshots associated with different reconstruction times to be re-used "
			"instead of re-creating them, provided they have not been evicted from the cache. "
			"This benefit also applies when reconstructing geometries with :meth:`reconstruct_geometry` "
			"since it, in turn, requests topological snapshots.\n"
			"\n"
			"  .. versionchanged:: 0.31\n"
			"     Added *default_resolve_topology_parameters* argument.\n"
			"\n"
			"  .. versionchanged:: 0.43\n"
			"     Added *topological_snapshot_cache_size* argument.\n"
			"\n"
			"  .. versionchanged:: 0.44\n"
			"     Filenames can be `os.PathLike <https://docs.python.org/3/library/os.html#os.PathLike>`_ "
			"(such as `pathlib.Path <https://docs.python.org/3/library/pathlib.html>`_) in addition to strings.\n")
		// Pickle support...
		//
		// Note: This adds an __init__ method accepting a single argument (of type 'bytes') that supports pickling.
		//       So we define this *after* (higher priority) the other __init__ methods in case one of them accepts a single argument
		//       of type bp::object (which, being more general, would otherwise obscure the __init__ that supports pickling).
		.def(GPlatesApi::PythonPickle::PickleDefVisitor<GPlatesApi::TopologicalModel::non_null_ptr_type>())
		.def("topological_snapshot",
				&GPlatesApi::topological_model_get_topological_snapshot,
				(bp::arg("reconstruction_time")),
				"topological_snapshot(reconstruction_time)\n"
				"  Returns a snapshot of resolved topologies at the requested reconstruction time.\n"
				"\n"
				"  :param reconstruction_time: the geological time of the snapshot\n"
				"  :type reconstruction_time: float or GeoTimeInstant\n"
				"  :rtype: TopologicalSnapshot\n"
				"  :raises: ValueError if *reconstruction_time* is distant-past (``float('inf')``) or distant-future (``float('-inf')``).\n"
				"\n"
				"  .. seealso:: :ref:`pygplates_primer_topological_snapshot` in the *Primer* documentation.\n"
				"\n"
				"  .. versionchanged:: 0.43\n"
				"     *reconstruction_time* no longer required to be integral.\n")
		.def("reconstruct_geometry",
				&GPlatesApi::TopologicalModel::reconstruct_geometry,
				(bp::arg("geometry"),
					bp::arg("initial_time"),
					bp::arg("oldest_time") = boost::optional<GPlatesPropertyValues::GeoTimeInstant>(),
					bp::arg("youngest_time") = GPlatesPropertyValues::GeoTimeInstant(0),
					bp::arg("time_increment") = GPlatesMaths::real_t(1),
					bp::arg("reconstruction_plate_id") = boost::optional<GPlatesModel::integer_plate_id_type>(),
					bp::arg("initial_scalars") = bp::object()/*Py_None*/,
					bp::arg("deactivate_points") = boost::optional<GPlatesAppLogic::TopologyReconstruct::DeactivatePoint::non_null_ptr_to_const_type>(
							GPlatesUtils::static_pointer_cast<const GPlatesAppLogic::TopologyReconstruct::DeactivatePoint>(
									GPlatesAppLogic::TopologyReconstruct::DefaultDeactivatePoint::create())),
					bp::arg("deformation_uses_natural_neighbour_interpolation") = true),
				"reconstruct_geometry(geometry, initial_time, [oldest_time], [youngest_time=0], [time_increment=1], "
				"[reconstruction_plate_id], [initial_scalars], [deactivate_points=ReconstructedGeometryTimeSpan.DefaultDeactivatePoints()], "
				"[deformation_uses_natural_neighbour_interpolation=True])\n"
				"  Reconstruct a geometry (and optional scalars) over a time span.\n"
				"\n"
				"  :param geometry: The geometry to reconstruct (using topologies). Currently limited to a "
				"multipoint, or a point or sequence of points. Polylines and polygons to be introduced in future.\n"
				"  :type geometry: MultiPointOnSphere, or PointOnSphere, or sequence of points "
				"(where a point can be PointOnSphere or (x,y,z) tuple or (latitude,longitude) tuple in degrees)\n"
				"  :param initial_time: The time that reconstruction by topologies starts at.\n"
				"  :type initial_time: float or GeoTimeInstant\n"
				"  :param oldest_time: Oldest time in the history of topologies. Defaults to *initial_time*.\n"
				"  :type oldest_time: float or GeoTimeInstant\n"
				"  :param youngest_time: Youngest time in the history of topologies. Defaults to present day.\n"
				"  :type youngest_time: float or GeoTimeInstant\n"
				"  :param time_increment: Time step in the history of topologies ("
				"``oldest_time - youngest_time`` must be an integer multiple of ``time_increment``). Defaults to 1My.\n"
				"  :type time_increment: float\n"
				"  :param reconstruction_plate_id: If specified then *geometry* is assumed to be a snapshot at present day, and this will "
				"rotate it to *initial_time*. If not specified then *geometry* is assumed to already be a snapshot at *initial_time* - this is the default.\n"
				"  :type reconstruction_plate_id: int\n"
				"  :param initial_scalars: optional mapping of scalar types to sequences of initial scalar values\n"
				"  :type initial_scalars: dict mapping each ScalarType to a sequence "
				"of float, or a sequence of (ScalarType, sequence of float) tuples\n"
				"  :param deactivate_points: Specify how points are deactivated when reconstructed forward and/or backward in time, or "
				"specify ``None`` to disable deactivation of points (which is useful if you know your points are on continental crust where "
				"they're typically always active, as opposed to oceanic crust that is produced at mid-ocean ridges and consumed at subduction zones). "
				"Note that you can use your own class derived from :class:`ReconstructedGeometryTimeSpan.DeactivatePoints` or "
				"use the provided class :class:`ReconstructedGeometryTimeSpan.DefaultDeactivatePoints`. "
				"Defaults to a default-constructed :class:`ReconstructedGeometryTimeSpan.DefaultDeactivatePoints`.\n"
				"  :type deactivate_points: ReconstructedGeometryTimeSpan.DeactivatePoints or None\n"
				"  :param deformation_uses_natural_neighbour_interpolation: If ``True`` then any point that lies (at any time) within a deforming region of a "
				"resolved topological network will be reconstructed using natural neighbour interpolation (otherwise barycentric interpolation will be used) - "
				"see :meth:`ResolvedTopologicalNetwork.reconstruct_point`. Defaults to ``True``.\n"
				"  :type deformation_uses_natural_neighbour_interpolation: bool\n"
				"  :rtype: ReconstructedGeometryTimeSpan\n"
				"  :raises: ValueError if initial time, oldest time or youngest time is "
				"distant-past (``float('inf')``) or distant-future (``float('-inf')``).\n"
				"  :raises: ValueError if oldest time is later than (or same as) youngest time.\n"
				"  :raises: ValueError if time increment is negative or zero.\n"
				"  :raises: ValueError if oldest to youngest time period is not an integer multiple of the time increment.\n"
				"  :raises: ValueError if *initial_scalars* is specified but: is empty, or each :class:`scalar type<ScalarType>` "
				"is not mapped to the same number of scalar values, or the number of scalars is not equal to the "
				"number of points in *geometry*\n"
				"\n"
				"  .. seealso:: :ref:`pygplates_primer_topologically_reconstruct_geometries` in the *Primer* documentation.\n"
				"\n"
				"  .. versionchanged:: 0.31\n"
				"     Added *deactivate_points* argument.\n"
				"\n"
				"  .. versionchanged:: 0.43\n"
				"     Oldest time, youngest time and time increment no longer required to be *integral* values.\n"
				"\n"
				"  .. versionchanged:: 0.50\n"
				"     Added *deformation_uses_natural_neighbour_interpolation* argument. Previously it was hardwired to ``True``.\n")
		.def("get_rotation_model",
				&GPlatesApi::TopologicalModel::get_rotation_model,
				"get_rotation_model()\n"
				"  Return the rotation model used internally.\n"
				"\n"
				"  :rtype: RotationModel\n"
				"\n"
				"  .. note:: The :meth:`default anchor plate ID<RotationModel.get_default_anchor_plate_id>` of the returned rotation model "
				"may be different to that of the rotation model passed into the :meth:`constructor<__init__>` if an anchor plate ID was specified "
				"in the :meth:`constructor<__init__>`.\n"
				"\n"
				"  .. note:: The reconstruction tree cache size of the returned rotation model is equal to the *topological_snapshot_cache_size* "
				"argument specified in the :meth:`constructor<__init__>` plus one (or unlimited if not specified).\n")
		.def("get_anchor_plate_id",
				&GPlatesApi::TopologicalModel::get_anchor_plate_id,
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
	GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<GPlatesApi::TopologicalModel>();
}
