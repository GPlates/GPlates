/**
 * Copyright (C) 2023 The University of Sydney, Australia
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

#include <sstream>
#include <utility>
#include <vector>
#include <boost/foreach.hpp>
#include <boost/optional.hpp>
#include <boost/variant.hpp>
#include <QString>

#include "PyNetRotation.h"

#include "PythonConverterUtils.h"
#include "PythonExtractUtils.h"
#include "PythonHashDefVisitor.h"
#include "PythonPickle.h"
#include "PythonVariableFunctionArguments.h"

#include "app-logic/ReconstructionGeometryUtils.h"

#include "global/GPlatesAssert.h"

#include "property-values/GeoTimeInstant.h"

#include "scribe/Scribe.h"


namespace bp = boost::python;


namespace GPlatesApi
{
	GPlatesAppLogic::NetRotationUtils::NetRotationAccumulator
	net_rotation_create_sample_from_finite_rotation(
			const GPlatesMaths::PointOnSphere &point,
			const double &sample_area,
			const GPlatesMaths::FiniteRotation &finite_rotation,
			const double &time_interval)
	{
		return GPlatesAppLogic::NetRotationUtils::NetRotationAccumulator::create(
				point, sample_area, finite_rotation, time_interval);
	}

	GPlatesAppLogic::NetRotationUtils::NetRotationAccumulator
	net_rotation_create_sample_from_rotation_rate(
			const GPlatesMaths::PointOnSphere &point,
			const double &sample_area,
			const GPlatesMaths::Vector3D &rotation_rate_vector)
	{
		return GPlatesAppLogic::NetRotationUtils::NetRotationAccumulator::create(
				point, sample_area, rotation_rate_vector);
	}

	bp::object
	net_rotation_eq(
			const GPlatesAppLogic::NetRotationUtils::NetRotationAccumulator &net_rotation,
			bp::object other)
	{
		bp::extract<const GPlatesAppLogic::NetRotationUtils::NetRotationAccumulator &> extract_other_instance(other);
		// Prevent equality comparisons.
		if (extract_other_instance.check())
		{
			PyErr_SetString(PyExc_TypeError,
					"Cannot equality compare (==, !=) NetRotations since they could have equivalent finite rotations "
					"but cover different areas");
			bp::throw_error_already_set();
		}

		// Return NotImplemented so python can continue looking for a match
		// (eg, in case 'other' is a class that implements relational operators with NetRotation).
		//
		// NOTE: This will most likely fall back to python's default handling which uses 'id()'
		// and hence will compare based on *python* object address rather than *C++* object address.
		return bp::object(bp::handle<>(bp::borrowed(Py_NotImplemented)));
	}

	bp::object
	net_rotation_ne(
			const GPlatesAppLogic::NetRotationUtils::NetRotationAccumulator &net_rotation,
			bp::object other)
	{
		bp::object ne_result = net_rotation_eq(net_rotation, other);
		if (ne_result.ptr() == Py_NotImplemented)
		{
			// Return NotImplemented.
			return ne_result;
		}

		// Invert the result.
		return bp::object(!bp::extract<bool>(ne_result));
	}


	/**
	 * Extract a point and sample area from a 2-tuple (or sequence of size 2).
	 */
	std::pair<GPlatesMaths::PointOnSphere, double/*sample_area_steradians*/>
	extract_arbitrary_point_and_sample_area(
			bp::object arbitrary_point_and_sample_area,
			const char *type_error_string)
	{
		// Copy into a vector.
		std::vector<bp::object> point_and_sample_area_object;
		PythonExtractUtils::extract_iterable(point_and_sample_area_object, arbitrary_point_and_sample_area, type_error_string);

		if (point_and_sample_area_object.size() != 2)
		{
			PyErr_SetString(PyExc_TypeError, type_error_string);
			bp::throw_error_already_set();
		}

		// Extract point from 2-tuple.
		bp::extract<GPlatesMaths::PointOnSphere> extract_point(point_and_sample_area_object[0]);
		if (!extract_point.check())
		{
			PyErr_SetString(PyExc_TypeError, type_error_string);
			bp::throw_error_already_set();
		}
		const GPlatesMaths::PointOnSphere point = extract_point();

		// Extract sample area from 2-tuple.
		bp::extract<double> extract_sample_area(point_and_sample_area_object[1]);
		if (!extract_sample_area.check())
		{
			PyErr_SetString(PyExc_TypeError, type_error_string);
			bp::throw_error_already_set();
		}
		const double sample_area = extract_sample_area();

		return std::make_pair(point, sample_area);
	}

	/**
	 * Extract an integer or a sequence of (point, sample area).
	 */
	void
	extract_point_distribution(
			GPlatesAppLogic::NetRotationUtils::NetRotationCalculator::point_distribution_type &point_distribution,
			bp::object point_distribution_object)
	{
		const char *type_error_string = "Expected either an integer or a sequence of (point, sample_area)";

		//
		// First see if point distribution is an integer.
		//
		bp::extract<unsigned int> extract_num_samples_along_meridian(point_distribution_object);
		if (extract_num_samples_along_meridian.check())
		{
			const unsigned int num_samples_along_meridian = extract_num_samples_along_meridian();

			// Num samples must be positive.
			if (num_samples_along_meridian <= 0)
			{
				PyErr_SetString(PyExc_ValueError, "Number of samples along meridian must be positive.");
				bp::throw_error_already_set();
			}

			point_distribution = num_samples_along_meridian;
			return;
		}

		//
		// Point distribution must be an arbitrary sequence of (point, sample area).
		//

		// Copy into a vector.
		std::vector<bp::object> arbitrary_points_and_sample_areas;
		PythonExtractUtils::extract_iterable(
				arbitrary_points_and_sample_areas,
				point_distribution_object,
				type_error_string);

		// Assign an empty arbitrary point distribution.
		point_distribution = GPlatesAppLogic::NetRotationUtils::NetRotationCalculator::arbitrary_point_distribution_type();
		GPlatesAppLogic::NetRotationUtils::NetRotationCalculator::arbitrary_point_distribution_type &arbitrary_point_distribution =
				boost::get<GPlatesAppLogic::NetRotationUtils::NetRotationCalculator::arbitrary_point_distribution_type>(point_distribution);
		// Add (point, sample area) tuples directly into the caller's 'point_distribution'.
		for (bp::object arbitrary_point_and_sample_area : arbitrary_points_and_sample_areas)
		{
			arbitrary_point_distribution.push_back(
					extract_arbitrary_point_and_sample_area(arbitrary_point_and_sample_area, type_error_string));
		}
	}

	const unsigned int NetRotationSnapshot::DEFAULT_NUM_SAMPLES_ALONG_MERIDIAN = 180;

	/**
	 * This is called directly from Python via 'NetRotationSnapshot.__init__()'.
	 */
	NetRotationSnapshot::non_null_ptr_type
	net_rotation_snapshot_create(
			TopologicalSnapshot::non_null_ptr_type topological_snapshot,
			const double &velocity_delta_time,
			GPlatesAppLogic::VelocityDeltaTime::Type velocity_delta_time_type,
			bp::object point_distribution_object)
	{
		// Velocity delta time must be positive.
		if (velocity_delta_time <= 0)
		{
			PyErr_SetString(PyExc_ValueError, "Velocity delta time must be positive.");
			bp::throw_error_already_set();
		}

		GPlatesAppLogic::NetRotationUtils::NetRotationCalculator::point_distribution_type point_distribution;
		extract_point_distribution(point_distribution, point_distribution_object);

		return NetRotationSnapshot::create(
				topological_snapshot,
				velocity_delta_time,
				velocity_delta_time_type,
				point_distribution);
	}

	GPlatesAppLogic::NetRotationUtils::NetRotationAccumulator
	net_rotation_snapshot_get_total_net_rotation(
			NetRotationSnapshot::non_null_ptr_type net_rotation_snapshot)
	{
		return net_rotation_snapshot->get_net_rotation_calculator().get_total_net_rotation();
	}

	bp::object
	net_rotation_snapshot_get_net_rotation(
			NetRotationSnapshot::non_null_ptr_type net_rotation_snapshot,
			boost::optional<GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_type> resolved_topology_boundary_or_network)
	{
		const GPlatesAppLogic::NetRotationUtils::NetRotationCalculator &net_rotation_calculator =
				net_rotation_snapshot->get_net_rotation_calculator();

		// If user specified a resolved topology.
		if (resolved_topology_boundary_or_network)
		{
			// See if a resolved topological boundary.
			if (boost::optional<GPlatesAppLogic::ResolvedTopologicalBoundary *> rtb =
					GPlatesAppLogic::ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type<
							GPlatesAppLogic::ResolvedTopologicalBoundary *>(resolved_topology_boundary_or_network.get()))
			{
				// If resolved topological boundary exists in net rotation map (ie, sample points intersected it) then return its net rotation.
				auto rtb_iter = net_rotation_calculator.get_topological_boundary_net_rotation_map().find(rtb.get());
				if (rtb_iter != net_rotation_calculator.get_topological_boundary_net_rotation_map().end())
				{
					return bp::object(rtb_iter->second);  // net rotation
				}

				// Resolved topology does not contribute net rotation.
				return bp::object()/*Py_None*/;
			}
			// else should be a resolved topological network...
			else if (boost::optional<GPlatesAppLogic::ResolvedTopologicalNetwork *> rtn =
					GPlatesAppLogic::ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type<
							GPlatesAppLogic::ResolvedTopologicalNetwork*>(resolved_topology_boundary_or_network.get()))
			{
				// If resolved topological network exists in net rotation map (ie, sample points intersected it) then return its net rotation.
				auto rtn_iter = net_rotation_calculator.get_topological_network_net_rotation_map().find(rtn.get());
				if (rtn_iter != net_rotation_calculator.get_topological_network_net_rotation_map().end())
				{
					return bp::object(rtn_iter->second);  // net rotation
				}

				// Resolved topology does not contribute net rotation.
				return bp::object()/*Py_None*/;
			}
			else
			{
				PyErr_SetString(PyExc_ValueError, "Should be either a ResolvedTopologicalBoundary or ResolvedTopologicalNetwork.");
				bp::throw_error_already_set();
			}
		}

		//
		// Return net rotations for all resolved topological boundaries and networks that contribute net rotation (in a dict).
		//

		bp::dict net_rotation_dict;

		// Iterate over all resolved topological boundaries.
		for (const auto &rtb_entry : net_rotation_calculator.get_topological_boundary_net_rotation_map())
		{
			// Map the current resolved topology to its net rotation.
			net_rotation_dict[rtb_entry.first] = rtb_entry.second;
		}

		// Iterate over all resolved topological networks.
		for (const auto &rtn_entry : net_rotation_calculator.get_topological_network_net_rotation_map())
		{
			// Map the current resolved topology to its net rotation.
			net_rotation_dict[rtn_entry.first] = rtn_entry.second;
		}

		return net_rotation_dict;
	}

	NetRotationSnapshot::non_null_ptr_type
	NetRotationSnapshot::create(
			TopologicalSnapshot::non_null_ptr_type topological_snapshot,
			const double &velocity_delta_time,
			GPlatesAppLogic::VelocityDeltaTime::Type velocity_delta_time_type,
			const GPlatesAppLogic::NetRotationUtils::NetRotationCalculator::point_distribution_type &point_distribution)
	{
		GPlatesAppLogic::NetRotationUtils::NetRotationCalculator::resolved_topological_boundary_seq_type resolved_topological_boundaries;
		for (auto resolved_topological_boundary : topological_snapshot->get_resolved_topological_boundaries())
		{
			resolved_topological_boundaries.push_back(resolved_topological_boundary);
		}

		GPlatesAppLogic::NetRotationUtils::NetRotationCalculator::resolved_topological_network_seq_type resolved_topological_networks;
		for (auto resolved_topological_network : topological_snapshot->get_resolved_topological_networks())
		{
			resolved_topological_networks.push_back(resolved_topological_network);
		}

		return non_null_ptr_type(
				new NetRotationSnapshot(
						topological_snapshot,
						resolved_topological_boundaries,
						resolved_topological_networks,
						velocity_delta_time,
						velocity_delta_time_type,
						point_distribution));
	}

	NetRotationSnapshot::NetRotationSnapshot(
			TopologicalSnapshot::non_null_ptr_type topological_snapshot,
			const GPlatesAppLogic::NetRotationUtils::NetRotationCalculator::resolved_topological_boundary_seq_type &resolved_topological_boundaries,
			const GPlatesAppLogic::NetRotationUtils::NetRotationCalculator::resolved_topological_network_seq_type &resolved_topological_networks,
			const double &velocity_delta_time,
			GPlatesAppLogic::VelocityDeltaTime::Type velocity_delta_time_type,
			const GPlatesAppLogic::NetRotationUtils::NetRotationCalculator::point_distribution_type &point_distribution) :
		d_topological_snapshot(topological_snapshot),
		d_net_rotation_calculator(
					resolved_topological_boundaries,
					resolved_topological_networks,
					topological_snapshot->get_reconstruction_time(),
					velocity_delta_time,
					velocity_delta_time_type,
					point_distribution,
					d_topological_snapshot->get_anchor_plate_id())
	{
	}

	GPlatesScribe::TranscribeResult
	NetRotationSnapshot::transcribe_construct_data(
			GPlatesScribe::Scribe &scribe,
			GPlatesScribe::ConstructObject<NetRotationSnapshot> &net_rotation_snapshot)
	{
		if (scribe.is_saving())
		{
			save_construct_data(scribe, net_rotation_snapshot.get_object());
		}
		else // loading
		{
			GPlatesScribe::LoadRef<TopologicalSnapshot::non_null_ptr_type> topological_snapshot;
			GPlatesAppLogic::NetRotationUtils::NetRotationCalculator::resolved_topological_boundary_seq_type resolved_topological_boundaries;
			GPlatesAppLogic::NetRotationUtils::NetRotationCalculator::resolved_topological_network_seq_type resolved_topological_networks;
			double velocity_delta_time;
			GPlatesAppLogic::VelocityDeltaTime::Type velocity_delta_time_type;
			GPlatesAppLogic::NetRotationUtils::NetRotationCalculator::point_distribution_type point_distribution;
			if (!load_construct_data(
					scribe,
					topological_snapshot,
					resolved_topological_boundaries,
					resolved_topological_networks,
					velocity_delta_time,
					velocity_delta_time_type,
					point_distribution))
			{
				return scribe.get_transcribe_result();
			}

			// Create the net rotation snapshot.
			net_rotation_snapshot.construct_object(
					topological_snapshot,
					resolved_topological_boundaries,
					resolved_topological_networks,
					velocity_delta_time,
					velocity_delta_time_type,
					point_distribution);
		}

		return GPlatesScribe::TRANSCRIBE_SUCCESS;
	}

	GPlatesScribe::TranscribeResult
	NetRotationSnapshot::transcribe(
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
				GPlatesScribe::LoadRef<TopologicalSnapshot::non_null_ptr_type> topological_snapshot;
				GPlatesAppLogic::NetRotationUtils::NetRotationCalculator::resolved_topological_boundary_seq_type resolved_topological_boundaries;
				GPlatesAppLogic::NetRotationUtils::NetRotationCalculator::resolved_topological_network_seq_type resolved_topological_networks;
				double velocity_delta_time;
				GPlatesAppLogic::VelocityDeltaTime::Type velocity_delta_time_type;
				GPlatesAppLogic::NetRotationUtils::NetRotationCalculator::point_distribution_type point_distribution;
				if (!load_construct_data(
						scribe,
						topological_snapshot,
						resolved_topological_boundaries,
						resolved_topological_networks,
						velocity_delta_time,
						velocity_delta_time_type,
						point_distribution))
				{
					return scribe.get_transcribe_result();
				}

				d_topological_snapshot = topological_snapshot.get();

				d_net_rotation_calculator = GPlatesAppLogic::NetRotationUtils::NetRotationCalculator(
						resolved_topological_boundaries,
						resolved_topological_networks,
						d_topological_snapshot->get_reconstruction_time(),
						velocity_delta_time,
						velocity_delta_time_type,
						point_distribution,
						d_topological_snapshot->get_anchor_plate_id());
			}
		}

		return GPlatesScribe::TRANSCRIBE_SUCCESS;
	}

	void
	NetRotationSnapshot::save_construct_data(
			GPlatesScribe::Scribe &scribe,
			const NetRotationSnapshot &net_rotation_snapshot)
	{
		// Save the topological snapshot.
		scribe.save(TRANSCRIBE_SOURCE, net_rotation_snapshot.d_topological_snapshot, "topological_snapshot");

		// Save the velocity delta time.
		scribe.save(TRANSCRIBE_SOURCE, net_rotation_snapshot.d_net_rotation_calculator.get_velocity_delta_time(), "velocity_delta_time");

		// Save the velocity delta time type.
		scribe.save(TRANSCRIBE_SOURCE, net_rotation_snapshot.d_net_rotation_calculator.get_velocity_delta_time_type(), "velocity_delta_time_type");

		// Save the point distribution.
		scribe.save(TRANSCRIBE_SOURCE, net_rotation_snapshot.d_net_rotation_calculator.get_point_distribution(), "point_distribution");
	}

	bool
	NetRotationSnapshot::load_construct_data(
			GPlatesScribe::Scribe &scribe,
			GPlatesScribe::LoadRef<TopologicalSnapshot::non_null_ptr_type> &topological_snapshot,
			GPlatesAppLogic::NetRotationUtils::NetRotationCalculator::resolved_topological_boundary_seq_type &resolved_topological_boundaries,
			GPlatesAppLogic::NetRotationUtils::NetRotationCalculator::resolved_topological_network_seq_type &resolved_topological_networks,
			double &velocity_delta_time,
			GPlatesAppLogic::VelocityDeltaTime::Type &velocity_delta_time_type,
			GPlatesAppLogic::NetRotationUtils::NetRotationCalculator::point_distribution_type &point_distribution)
	{
		// Load the topological snapshot.
		topological_snapshot = scribe.load<TopologicalSnapshot::non_null_ptr_type>(TRANSCRIBE_SOURCE, "topological_snapshot");
		if (!topological_snapshot.is_valid())
		{
			return false;
		}

		for (auto resolved_topological_boundary : topological_snapshot.get()->get_resolved_topological_boundaries())
		{
			resolved_topological_boundaries.push_back(resolved_topological_boundary);
		}
		for (auto resolved_topological_network : topological_snapshot.get()->get_resolved_topological_networks())
		{
			resolved_topological_networks.push_back(resolved_topological_network);
		}

		// Load the velocity delta time.
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, velocity_delta_time, "velocity_delta_time"))
		{
			return false;
		}

		// Load the velocity delta time type.
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, velocity_delta_time_type, "velocity_delta_time_type"))
		{
			return false;
		}

		// Load the point distribution.
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, point_distribution, "point_distribution"))
		{
			return false;
		}

		return true;
	}


	/**
	 * This is called directly from Python via 'NetRotationModel.__init__()'.
	 */
	NetRotationModel::non_null_ptr_type
	net_rotation_model_create(
			TopologicalModel::non_null_ptr_type topological_model,
			const double &velocity_delta_time,
			GPlatesAppLogic::VelocityDeltaTime::Type velocity_delta_time_type,
			bp::object point_distribution_object)
	{
		// Velocity delta time must be positive.
		if (velocity_delta_time <= 0)
		{
			PyErr_SetString(PyExc_ValueError, "Velocity delta time must be positive.");
			bp::throw_error_already_set();
		}

		GPlatesAppLogic::NetRotationUtils::NetRotationCalculator::point_distribution_type point_distribution;
		extract_point_distribution(point_distribution, point_distribution_object);

		return NetRotationModel::create(topological_model, velocity_delta_time, velocity_delta_time_type, point_distribution);
	}

	/**
	 * This is called directly from Python via 'NetRotationModel.net_rotation_snapshot()'.
	 */
	NetRotationSnapshot::non_null_ptr_type
	net_rotation_model_create_net_rotation_snapshot(
			NetRotationModel::non_null_ptr_type net_rotation_model,
			const GPlatesPropertyValues::GeoTimeInstant &reconstruction_time)
	{
		// Time must not be distant past/future.
		if (!reconstruction_time.is_real())
		{
			PyErr_SetString(PyExc_ValueError,
					"Time values cannot be distant-past (float('inf')) or distant-future (float('-inf')).");
			bp::throw_error_already_set();
		}

		return net_rotation_model->create_net_rotation_snapshot(reconstruction_time.value());
	}

	NetRotationModel::non_null_ptr_type
	NetRotationModel::create(
			TopologicalModel::non_null_ptr_type topological_model,
			const double &velocity_delta_time,
			GPlatesAppLogic::VelocityDeltaTime::Type velocity_delta_time_type,
			const GPlatesAppLogic::NetRotationUtils::NetRotationCalculator::point_distribution_type &point_distribution)
	{
		return non_null_ptr_type(new NetRotationModel(topological_model, velocity_delta_time, velocity_delta_time_type, point_distribution));
	}

	NetRotationModel::NetRotationModel(
			TopologicalModel::non_null_ptr_type topological_model,
			const double &velocity_delta_time,
			GPlatesAppLogic::VelocityDeltaTime::Type velocity_delta_time_type,
			const GPlatesAppLogic::NetRotationUtils::NetRotationCalculator::point_distribution_type &point_distribution) :
		d_topological_model(topological_model),
		d_velocity_delta_time(velocity_delta_time),
		d_velocity_delta_time_type(velocity_delta_time_type),
		d_point_distribution(point_distribution)
	{  }
	
	NetRotationSnapshot::non_null_ptr_type
	NetRotationModel::create_net_rotation_snapshot(
			const double &reconstruction_time) const
	{
		TopologicalSnapshot::non_null_ptr_type topological_snapshot =
				d_topological_model->get_topological_snapshot(reconstruction_time);

		return NetRotationSnapshot::create(
				topological_snapshot,
				d_velocity_delta_time,
				d_velocity_delta_time_type,
				d_point_distribution);
	}

	GPlatesScribe::TranscribeResult
	NetRotationModel::transcribe_construct_data(
			GPlatesScribe::Scribe &scribe,
			GPlatesScribe::ConstructObject<NetRotationModel> &net_rotation_model)
	{
		if (scribe.is_saving())
		{
			save_construct_data(scribe, net_rotation_model.get_object());
		}
		else // loading
		{
			GPlatesScribe::LoadRef<TopologicalModel::non_null_ptr_type> topological_model;
			double velocity_delta_time;
			GPlatesAppLogic::VelocityDeltaTime::Type velocity_delta_time_type;
			GPlatesAppLogic::NetRotationUtils::NetRotationCalculator::point_distribution_type point_distribution;
			if (!load_construct_data(
					scribe,
					topological_model,
					velocity_delta_time,
					velocity_delta_time_type,
					point_distribution))
			{
				return scribe.get_transcribe_result();
			}

			// Create the net rotation model.
			net_rotation_model.construct_object(
					topological_model,
					velocity_delta_time,
					velocity_delta_time_type,
					point_distribution);
		}

		return GPlatesScribe::TRANSCRIBE_SUCCESS;
	}

	GPlatesScribe::TranscribeResult
	NetRotationModel::transcribe(
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
				GPlatesScribe::LoadRef<TopologicalModel::non_null_ptr_type> topological_model;
				if (!load_construct_data(
						scribe,
						topological_model,
						d_velocity_delta_time,
						d_velocity_delta_time_type,
						d_point_distribution))
				{
					return scribe.get_transcribe_result();
				}
				d_topological_model = topological_model.get();
			}
		}

		return GPlatesScribe::TRANSCRIBE_SUCCESS;
	}

	void
	NetRotationModel::save_construct_data(
			GPlatesScribe::Scribe &scribe,
			const NetRotationModel &net_rotation_model)
	{
		// Save the net rotation model.
		scribe.save(TRANSCRIBE_SOURCE, net_rotation_model.d_topological_model, "topological_model");

		// Save the velocity delta time.
		scribe.save(TRANSCRIBE_SOURCE, net_rotation_model.d_velocity_delta_time, "velocity_delta_time");

		// Save the velocity delta time type.
		scribe.save(TRANSCRIBE_SOURCE, net_rotation_model.d_velocity_delta_time_type, "velocity_delta_time_type");

		// Save the point distribution.
		scribe.save(TRANSCRIBE_SOURCE, net_rotation_model.d_point_distribution, "point_distribution");
	}

	bool
	NetRotationModel::load_construct_data(
			GPlatesScribe::Scribe &scribe,
			GPlatesScribe::LoadRef<TopologicalModel::non_null_ptr_type> &topological_model,
			double &velocity_delta_time,
			GPlatesAppLogic::VelocityDeltaTime::Type &velocity_delta_time_type,
			GPlatesAppLogic::NetRotationUtils::NetRotationCalculator::point_distribution_type &point_distribution)
	{
		// Load the net rotation model.
		topological_model = scribe.load<TopologicalModel::non_null_ptr_type>(TRANSCRIBE_SOURCE, "topological_model");
		if (!topological_model.is_valid())
		{
			return false;
		}

		// Load the velocity delta time.
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, velocity_delta_time, "velocity_delta_time"))
		{
			return false;
		}

		// Load the velocity delta time type.
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, velocity_delta_time_type, "velocity_delta_time_type"))
		{
			return false;
		}

		// Load the point distribution.
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, point_distribution, "point_distribution"))
		{
			return false;
		}

		return true;
	}
}

	
void
export_net_rotation()
{
	//
	// NetRotation - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesAppLogic::NetRotationUtils::NetRotationAccumulator,
			// A pointer holder is required by 'bp::make_constructor'...
			boost::shared_ptr<GPlatesAppLogic::NetRotationUtils::NetRotationAccumulator>>(
					"NetRotation",
					"Net rotation of regional or global crust.\n"
					"\n"
					"NetRotations support addition ``net_rotation = net_rotation1 + net_rotation2`` and ``net_rotation += other_net_rotation``.\n"
					"\n"
					"NetRotations are *not* equality (``==``, ``!=``) comparable (will raise ``TypeError`` when compared) and "
					"are not hashable (cannot be used as a key in a ``dict``). This stems from the fact that two NetRotations "
					"can have equivalent finite rotations but can cover different :meth:`areas<get_area>`.\n"
					"\n"
					"A *NetRotation* can also be `pickled <https://docs.python.org/3/library/pickle.html>`_.\n"
					"\n"
					".. versionadded:: 0.43\n",
					bp::init<>(
							"__init__()\n"
							"  Creates a zero net rotation.\n"))
		// Pickle support...
		//
		// Note: This adds an __init__ method accepting a single argument (of type 'bytes') that supports pickling.
		//       So we define this *after* (higher priority) the other __init__ methods in case one of them accepts a single argument
		//       of type bp::object (which, being more general, would otherwise obscure the __init__ that supports pickling).
		.def(GPlatesApi::PythonPickle::PickleDefVisitor<boost::shared_ptr<GPlatesAppLogic::NetRotationUtils::NetRotationAccumulator>>())
		.def("get_finite_rotation",
				&GPlatesAppLogic::NetRotationUtils::NetRotationAccumulator::get_net_finite_rotation,
				"get_finite_rotation()\n"
				"  Return the net rotation as a finite rotation (over a time interval of 1Myr).\n"
				"\n"
				"  :rtype: FiniteRotation\n"
				"\n"
				"  Returns :meth:`identity rotation<FiniteRotation.create_identity_rotation>` if the net rotation is zero.\n")
		.def("get_rotation_rate_vector",
				&GPlatesAppLogic::NetRotationUtils::NetRotationAccumulator::get_net_rotation_rate_vector,
				"get_rotation_rate_vector()\n"
				"  Return the net rotation as a rotation rate vector with a magnitude of radians per Myr.\n"
				"\n"
				"  :rtype: Vector3D\n"
				"\n"
				"  Returns :class:`zero vector<Vector3D>` if the net rotation is zero.\n")
		.def("get_area",
				&GPlatesAppLogic::NetRotationUtils::NetRotationAccumulator::get_area_in_steradians,
				"get_area()\n"
				"  Return the sample area covered by the :meth:`point samples<NetRotationSnapshot.__init__>` used to calculate this net rotation, in steradians (square radians).\n"
				"\n"
				"  :rtype: float\n"
				"\n"
				"  For example, if this is the net rotation for a single topological plate then the returned area would be the sum of the sample areas of those "
				"sample points in the :meth:`point distribution<NetRotationSnapshot.__init__>` that fell within the polygon boundary of the topological plate.\n"
				"\n"
				"  To convert from steradians (square radians) to square kms multiply, by the square of the :class:`Earth's radius<Earth>`:\n"
				"  ::\n"
				"\n"
				"    area_in_square_kms = net_rotation.get_area() * pygplates.Earth.mean_radius_in_kms**2\n"
				"\n"
				"  .. note:: The accuracy of this area depends on how many :meth:`point samples<NetRotationSnapshot.__init__>` were used to calculate net rotation. "
				"If you need an accurate area then it's better to explicitly calculate the :meth:`polygon area<PolygonOnSphere.get_area>` "
				"of the topology (or topologies) that contributed to this net rotation.\n")
		.def("create_sample_from_finite_rotation",
				&GPlatesApi::net_rotation_create_sample_from_finite_rotation,
				(bp::arg("point"),
					bp::arg("sample_area"),
					bp::arg("finite_rotation"),
					bp::arg("time_interval") = 1.0),
				"create_sample_from_finite_rotation(point, sample_area, finite_rotation, [time_interval=1.0])\n"
				"  Creates a net rotation contribution from a finite rotation at a point sample.\n"
				"\n"
				"  :param point: The point that contributes to net rotation.\n"
				"  :type point: PointOnSphere or LatLonPoint or tuple (latitude,longitude), in degrees, or tuple (x,y,z)\n"
				"  :param sample_area: The surface area around the point in steradians (square radians).\n"
				"  :type sample_area: float\n"
				"  :param finite_rotation: The finite rotation over the specified time interval.\n"
				"  :type finite_rotation: FiniteRotation\n"
				"  :param time_interval: The time interval of the specified finite rotation (defaults to 1Myr).\n"
				"  :type time_interval: float\n"
				"  :rtype: NetRotation\n"
				"\n"
				"  In this contrived example we calculate the net rotation of a single plate. "
				"This is just for demonstration purposes in case you wanted to do your own intersections of point samples with plates "
				"(otherwise it's easier to just use :class:`NetRotationSnapshot` which does all this for you).\n"
				"  ::\n"
				"\n"
				"    # Let's assume you have a plate and know its finite rotation (over 1Myr), and you have a list of\n"
				"    # (lat, lon) tuples which are those points on a uniform lat-lon grid that are inside the plate.\n"
				"    plate_finite_rotation = ...\n"
				"    lat_lon_tuples_inside_plate = [...]\n"
				"    lat_lon_grid_spacing_in_degrees = ...\n"
				"    lat_lon_grid_spacing_in_radians = math.radians(lat_lon_grid_spacing_in_degrees)\n"
				"\n"
				"    # We'll accumulate net rotation over the point samples inside the plate.\n"
				"    plate_net_rotation_accumulator = pygplates.NetRotation()  # start with zero net rotation\n"
				"\n"
				"    for lat, lon in lat_lon_tuples_inside_plate:\n"
				"        # The cosine is because points near the North/South poles are closer together\n"
				"        # (latitude parallel small circle radius).\n"
				"        sample_area_radians = math.cos(math.radians(lat)) * lat_lon_grid_spacing_in_radians * lat_lon_grid_spacing_in_radians\n"
				"        plate_net_rotation_accumulator += pygplates.NetRotation.create_sample_from_finite_rotation(\n"
				"                pygplates.LatLonPoint(lat, lon), sample_area_radians, plate_finite_rotation)\n"
				"\n"
				"    plate_net_rotation = plate_net_rotation_accumulator.get_finite_rotation()\n")
		.staticmethod("create_sample_from_finite_rotation")
		.def("create_sample_from_rotation_rate",
				&GPlatesApi::net_rotation_create_sample_from_rotation_rate,
				(bp::arg("point"),
					bp::arg("sample_area"),
					bp::arg("rotation_rate_vector")),
				"create_sample_from_rotation_rate(point, sample_area, rotation_rate_vector)\n"
				"  Creates a net rotation contribution from a rotation rate vector at a point sample.\n"
				"\n"
				"  :param point: The point that contributes to net rotation.\n"
				"  :type point: PointOnSphere or LatLonPoint or tuple (latitude,longitude), in degrees, or tuple (x,y,z)\n"
				"  :param sample_area: The surface area around the point in steradians (square radians).\n"
				"  :type sample_area: float\n"
				"  :param rotation_rate_vector: The rotation rate vector (with magnitude in radians per Myr).\n"
				"  :type rotation_rate_vector: Vector3D\n"
				"  :rtype: NetRotation\n"
				"\n"
				"  .. seealso:: :meth:`create_sample_from_finite_rotation`\n")
		.staticmethod("create_sample_from_rotation_rate")
		.def("convert_finite_rotation_to_rotation_rate_vector",
				&GPlatesAppLogic::NetRotationUtils::NetRotationAccumulator::convert_finite_rotation_to_rotation_rate_vector,
				(bp::arg("finite_rotation"),
					bp::arg("time_interval") = 1.0),
				"convert_finite_rotation_to_rotation_rate_vector(finite_rotation, [time_interval=1.0])\n"
				"  Convert a :class:`finite rotation<FiniteRotation>` over a time interval to a rotation rate vector (with magnitude in radians per Myr).\n"
				"\n"
				"  :param finite_rotation: The finite rotation over the specified time interval.\n"
				"  :type finite_rotation: FiniteRotation\n"
				"  :param time_interval: The time interval of the specified finite rotation (defaults to 1Myr).\n"
				"  :type time_interval: float\n"
				"  :rtype: Vector3D\n"
				"\n"
				"  To convert a finite rotation over 10Myr to a rotation rate vector (in radians per Myr):"
				"  ::\n"
				"\n"
				"    net_rotation_rate_vector = pygplates.NetRotation.convert_finite_rotation_to_rotation_rate_vector(net_finite_rotation_over_10myr, 10)\n")
		.staticmethod("convert_finite_rotation_to_rotation_rate_vector")
		.def("convert_rotation_rate_vector_to_finite_rotation",
				&GPlatesAppLogic::NetRotationUtils::NetRotationAccumulator::convert_rotation_rate_vector_to_finite_rotation,
				(bp::arg("rotation_rate_vector"),
					bp::arg("time_interval") = 1.0),
				"convert_rotation_rate_vector_to_finite_rotation(rotation_rate_vector, [time_interval=1.0])\n"
				"  Convert a rotation rate vector (with magnitude in radians per Myr) to a :class:`finite rotation<FiniteRotation>` over a time interval.\n"
				"\n"
				"  :param rotation_rate_vector: The rotation rate vector (with magnitude in radians per Myr).\n"
				"  :type rotation_rate_vector: Vector3D\n"
				"  :param time_interval: The time interval of the returned finite rotation (defaults to 1Myr).\n"
				"  :type time_interval: float\n"
				"  :rtype: FiniteRotation\n"
				"\n"
				"  To convert a rotation rate vector (in radians per Myr) to a finite rotation over 10Myr (ie, having the same pole but with an angle multiplied by 10):"
				"  ::\n"
				"\n"
				"    net_finite_rotation_over_10myr = pygplates.NetRotation.convert_rotation_rate_vector_to_finite_rotation(net_rotation_rate_vector, 10)\n")
		.staticmethod("convert_rotation_rate_vector_to_finite_rotation")
		.def(bp::self += bp::self) // modify a NetRotation by adding another
		.def(bp::self + bp::self)  // add two NetRotation's and return a new one
		// Comparisons...
		// Due to the fact that two NetRotations can have equivalent finite rotations but can cover different areas
		// we prevent equality comparisons and also make unhashable since user will expect hashing
		// to be based on object value and not object identity (address).
		// Make unhashable, with no *equality* comparison operators (we explicitly define them)...
		.def(GPlatesApi::NoHashDefVisitor(false, true))
		.def("__eq__", &GPlatesApi::net_rotation_eq)
		.def("__ne__", &GPlatesApi::net_rotation_ne)
	;

	// Enable boost::optional<FiniteRotation> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::register_optional_conversion<GPlatesAppLogic::NetRotationUtils::NetRotationAccumulator>();


	std::stringstream net_rotation_snapshot_create_docstring_stream;
	net_rotation_snapshot_create_docstring_stream <<
			"__init__(topological_snapshot, [velocity_delta_time=1.0], [velocity_delta_time_type=pygplates.VelocityDeltaTimeType.t_plus_delta_t_to_t], [point_distribution])\n"
			"  Create a net rotation snapshot from the specified topological snapshot, and using the requested parameters.\n"
			"\n"
			"  :param topological_snapshot: The topological snapshot to calculate net rotation with.\n"
			"  :type topological_snapshot: TopologicalSnapshot\n"
			"  :param velocity_delta_time: The time delta used to calculate velocities for net rotation (defaults to 1 Myr).\n"
			"  :type velocity_delta_time: float\n"
			"  :param velocity_delta_time_type: How the two velocity times are calculated relative to the reconstruction time. "
			"This includes [t+dt, t], [t, t-dt] and [t+dt/2, t-dt/2]. Defaults to [t+dt, t].\n"
			"  :type velocity_delta_time_type: VelocityDeltaTimeType.t_plus_delta_t_to_t, "
			"VelocityDeltaTimeType.t_to_t_minus_delta_t or VelocityDeltaTimeType.t_plus_minus_half_delta_t\n"
			"  :param point_distribution: Can be an integer `N` representing the number of uniformly spaced latitude-longitude grid points "
			"sampled along each *meridian* (ie, an `N x 2N` grid). Or can be a sequence of (point, sample_area) tuples where *point* is a "
			"point that contributes to net rotation and *sample_area* is the surface area around the point in steradians (square radians). "
			"If nothing specified then defaults to a `"
			<< GPlatesApi::NetRotationSnapshot::DEFAULT_NUM_SAMPLES_ALONG_MERIDIAN << " x " << 2 * GPlatesApi::NetRotationSnapshot::DEFAULT_NUM_SAMPLES_ALONG_MERIDIAN <<
			"` uniformly spaced latitude-longitude points.\n"
			"  :type point_distribution: int, or sequence of tuple (point, float) where point is a "
			"PointOnSphere or LatLonPoint or tuple (latitude,longitude), in degrees, or tuple (x,y,z)\n"
			"  :raises: ValueError if *velocity_delta_time* is negative or zero.\n"
			"\n"
			"  The `total net rotation <https://doi.org/10.1016/j.epsl.2009.12.055>`_ of all resolved topologies in this snapshot is:\n"
			"\n"
			"  .. math::\n"
			"\n"
			"     \\boldsymbol \\omega_{net} = \\frac{3}{8\\pi} \\sum_i \\int \\boldsymbol r \\times (\\boldsymbol \\omega_i(\\boldsymbol r) \\times \\boldsymbol r) \\, dS_i\n"
			"\n"
			"  ...where :math:`\\sum_i` is the summation over all resolved topologies, and :math:`\\int ... dS_i` is integration over the surface area of resolved topology :math:`i`, and "
			":math:`\\boldsymbol \\omega_i(\\boldsymbol r)` is the rotation rate vector for resolved topology :math:`i` at location :math:`\\boldsymbol r`. For a rigid plate this is "
			"just a constant :math:`\\boldsymbol \\omega_i(\\boldsymbol r) = \\boldsymbol \\omega_i`, but for a deforming network this varies spatially across the network "
			"(hence the dependency on :math:`\\boldsymbol r`). Note that if a deforming network overlaps a rigid plate then only the deforming network contributes to the "
			"total net rotation (in the overlap region).\n"
			"\n"
			"  The surface integrals are approximated by sampling at points distributed across the surface of the globe. So it's helpful to think of a *single* integral "
			"over the entire globe (rather than a separate integral for each topology) and then approximate that as a summation over points:\n"
			"\n"
			"  .. math::\n"
			"\n"
			"     \\boldsymbol \\omega_{net} &= \\frac{3}{8\\pi} \\int \\boldsymbol r \\times (\\boldsymbol \\omega(\\boldsymbol r) \\times \\boldsymbol r) \\, dS\\\\\n"
			"                                &\\approx \\frac{3}{8\\pi} \\sum_p (\\boldsymbol r_p \\times (\\boldsymbol \\omega(\\boldsymbol r_p) \\times \\boldsymbol r_p)) \\, dS_p\n"
			"\n"
			"  ...where the summation is over all locations :math:`\\boldsymbol r_p` in the point distribution :math:`p`, and :math:`dS_p` is the surface area contributed at "
			"location :math:`\\boldsymbol r_p` (where :math:`\\sum_p dS_p \\approx 4\\pi`), and :math:`\\boldsymbol \\omega(\\boldsymbol r_p)` is the rotation rate vector contributed "
			"by whichever resolved topology happens to intersect location :math:`\\boldsymbol r_p` (with preference given to deforming networks when they overlap rigid plates).\n"
			"\n"
			"  The point distribution used to calculate net rotation can be controlled with the *point_distribution* parameter. "
			"If *point_distribution* is an integer `N` then a uniform latitude-longitude grid of `N x 2N` points is used. "
			"Otherwise *point_distribution* can be a sequence of (point, sample area) representing an arbitrary user-specified distribution of points (and their sample areas). "
			"For example, if you have a distribution that is uniformly spaced on the surface of the sphere then the sample area will be "
			"the same for each point (ie, :math:`4\\pi` steradians divided by the total number of points). "
			"If nothing is specified for *point_distribution* then a uniform latitude-longitude grid of `"
			<< GPlatesApi::NetRotationSnapshot::DEFAULT_NUM_SAMPLES_ALONG_MERIDIAN << " x " << 2 * GPlatesApi::NetRotationSnapshot::DEFAULT_NUM_SAMPLES_ALONG_MERIDIAN <<
			"` points is used.\n"
			"\n"
			"  To create a net rotation snapshot (of a topological snapshot) at 0Ma calculated with a velocity delta from 1Ma (to 0Ma):"
			"  ::\n"
			"\n"
			"    net_rotation_snapshot = pygplates.NetRotationSnapshot(\n"
			"        topological_snapshot, 0, 1.0, pygplates.VelocityDeltaTimeType.t_plus_delta_t_to_t)\n"
			"\n"
			"  ...which is equivalent to the following code that explicitly specifies a point distribution:"
			"  ::\n"
			"\n"
			"    point_distribution = []\n"
			"    num_samples_along_meridian = "
			<< GPlatesApi::NetRotationSnapshot::DEFAULT_NUM_SAMPLES_ALONG_MERIDIAN <<
			"\n"
			"    delta_in_degrees = 180.0 / num_samples_along_meridian\n"
			"    delta_in_radians = math.radians(delta_in_degrees)\n"
			"    for lat_index in range(num_samples_along_meridian):\n"
			"        lat = -90.0 + (lat_index + 0.5) * delta_in_degrees\n"
			"        # The cosine is because points near the North/South poles are closer together (latitude parallel small circle radius).\n"
			"        sample_area_radians = math.cos(math.radians(lat)) * delta_in_radians * delta_in_radians\n"
			"        for lon_index in range(2*num_samples_along_meridian):\n"
			"            lon = -180.0 + (lon_index + 0.5) * delta_in_degrees\n"
			"            point_distribution.append(((lat, lon), sample_area_radians))\n"
			"\n"
			"    net_rotation_snapshot = pygplates.NetRotationSnapshot(\n"
			"        topological_snapshot, 0, 1.0, pygplates.VelocityDeltaTimeType.t_plus_delta_t_to_t, point_distribution)\n"
			"\n"
			"  .. note:: The anchor plate is that of the specified topological snapshot (see :meth:`TopologicalSnapshot.get_anchor_plate_id`).\n";

	//
	// NetRotationSnapshot - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesApi::NetRotationSnapshot,
			GPlatesApi::NetRotationSnapshot::non_null_ptr_type,
			boost::noncopyable>(
					"NetRotationSnapshot",
					"Net rotation snapshot of topological plates and deforming networks at a particular reconstruction time.\n"
					"\n"
					"A *NetRotationSnapshot* can also be `pickled <https://docs.python.org/3/library/pickle.html>`_.\n"
					"\n"
					".. versionadded:: 0.43\n",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::net_rotation_snapshot_create,
						bp::default_call_policies(),
						(bp::arg("topological_snapshot"),
							bp::arg("velocity_delta_time") = 1.0,
							bp::arg("velocity_delta_time_type") = GPlatesAppLogic::VelocityDeltaTime::T_PLUS_DELTA_T_TO_T,
							bp::arg("point_distribution") = GPlatesApi::NetRotationSnapshot::DEFAULT_NUM_SAMPLES_ALONG_MERIDIAN)),
				net_rotation_snapshot_create_docstring_stream.str().c_str())
		// Pickle support...
		//
		// Note: This adds an __init__ method accepting a single argument (of type 'bytes') that supports pickling.
		//       So we define this *after* (higher priority) the other __init__ methods in case one of them accepts a single argument
		//       of type bp::object (which, being more general, would otherwise obscure the __init__ that supports pickling).
		.def(GPlatesApi::PythonPickle::PickleDefVisitor<GPlatesApi::NetRotationSnapshot::non_null_ptr_type>())
		.def("get_topological_snapshot",
				&GPlatesApi::NetRotationSnapshot::get_topological_snapshot,
				"get_topological_snapshot()\n"
				"  Return the associated topological snapshot.\n"
				"\n"
				"  :rtype: TopologicalSnapshot\n"
				"\n"
				"  .. note:: Parameters such as reconstruction time, anchor plate ID and rotation model can be obtained from the topological snapshot.\n")
		.def("get_total_net_rotation",
				&GPlatesApi::net_rotation_snapshot_get_total_net_rotation,
				"get_total_net_rotation()\n"
				"  Return the total net rotation over all resolved topologies in this snapshot.\n"
				"\n"
				"  :rtype: NetRotation\n"
				"\n"
				"  The `total net rotation <https://doi.org/10.1016/j.epsl.2009.12.055>`_ of all resolved topologies in this snapshot is:\n"
				"\n"
				"  .. math::\n"
				"\n"
				"     \\boldsymbol \\omega_{net} = \\frac{3}{8\\pi} \\sum_i \\int \\boldsymbol r \\times (\\boldsymbol \\omega_i(\\boldsymbol r) \\times \\boldsymbol r) \\, dS_i\n"
				"\n"
				"  ...where :math:`\\sum_i` is the summation over all resolved topologies, and :math:`\\int ... dS_i` is integration over the surface area of resolved topology :math:`i`, and "
				":math:`\\boldsymbol \\omega_i(\\boldsymbol r)` is the rotation rate vector for resolved topology :math:`i` at location :math:`\\boldsymbol r`. For a rigid plate this is "
				"just a constant :math:`\\boldsymbol \\omega_i(\\boldsymbol r) = \\boldsymbol \\omega_i`, but for a deforming network this varies spatially across the network "
				"(hence the dependency on :math:`\\boldsymbol r`). Note that if a deforming network overlaps a rigid plate then only the deforming network contributes to the "
				"total net rotation (in the overlap region).\n"
				"\n"
				"  ::\n"
				"\n"
				"    total_net_rotation = net_rotation_snapshot.get_total_net_rotation()\n"
				"\n"
				"  This is equivalent to accumulating the net rotation for each resolved topological boundary and network that contributed to the total net rotation:\n"
				"  ::\n"
				"\n"
				"    total_net_rotation = pygplates.NetRotation()  # zero net rotation\n"
				"    # Get the net rotation of each resolved topology that contributed to the toal net rotation (extract *values* from dict).\n"
				"    for resolved_topology_net_rotation in net_rotation_snapshot.get_net_rotation().values():\n"
				"        total_net_rotation += resolved_topology_net_rotation\n"
				"\n"
				"  .. note:: If a :class:`resolved topological boundary<ResolvedTopologicalBoundary>` does not have a :meth:`reconstruction plate ID<Feature.get_reconstruction_plate_id>` "
				"then ``0`` will be used.\n"
				"\n"
				"  .. seealso:: :meth:`get_net_rotation`\n")
		.def("get_net_rotation",
				&GPlatesApi::net_rotation_snapshot_get_net_rotation,
				(bp::arg("resolved_topology") = boost::optional<GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_type>()),
				"get_net_rotation([resolved_topology])\n"
				"  Return the net rotation of the specified resolved topology (or a net rotation for each topology as a ``dict``).\n"
				"\n"
				"  :param resolved_topology: Optional resolved topology to retrieve net rotation for. If not specified then net rotations for all "
				"resolved boundaries and networks that **contribute net rotation** are returned (returned as a ``dict``).\n"
				"  :type resolved_topology: ResolvedTopologicalBoundary or ResolvedTopologicalNetwork\n"
				"  :returns: If *resolved_topology* is specified then returns the :class:`NetRotation` of that resolved topology boundary or network "
				"(or ``None`` if *resolved_topology* does **not contribute net rotation**). Otherwise returns a ``dict`` mapping each "
				":class:`ResolvedTopologicalBoundary` or :class:`ResolvedTopologicalNetwork` that **contributes net rotation** to its :class:`NetRotation`.\n"
				"  :rtype: NetRotation, or dict[ResolvedTopologicalBoundary | ResolvedTopologicalNetwork, NetRotation], or None\n"
				"  :raises: ValueError if *resolved_topology* is specified but is neither a :class:`ResolvedTopologicalBoundary` nor a :class:`ResolvedTopologicalNetwork`.\n"
				"\n"
				"  .. note:: Any resolved boundary or network that did not intersect any sample points (see *point_distribution* in :meth:`__init__`) will **not contribute net rotation**. "
				"And if a contributing :class:`resolved boundary<ResolvedTopologicalBoundary>` does not have a :meth:`reconstruction plate ID<Feature.get_reconstruction_plate_id>` "
				"then ``0`` will be used.\n"
				"\n"
				"  The net rotation of resolved topology :math:`i` (rigid plate or deforming network) is:\n"
				"\n"
				"  .. math::\n"
				"\n"
				"     \\boldsymbol \\omega_{i \\, net} = \\frac{\\int \\boldsymbol r \\times (\\boldsymbol \\omega_i(\\boldsymbol r) \\times \\boldsymbol r) \\, dS_i}{\\frac{2}{3} \\int \\, dS_i}\n"
				"\n"
				"  ...where :math:`\\int ... dS_i` is integration over the surface area of resolved topology :math:`i`, and :math:`\\int dS_i` is its surface area, and :math:`\\boldsymbol \\omega_i(\\boldsymbol r)` "
				"is its rotation rate vector at location :math:`\\boldsymbol r`. For a rigid plate this is just a constant :math:`\\boldsymbol \\omega_i(\\boldsymbol r) = \\boldsymbol \\omega_i`, "
				"but for a deforming network this varies spatially across the network (hence the dependency on :math:`\\boldsymbol r`).\n"
				"\n"
				"  .. note:: The net rotation for individual topologies (rigid plates and deforming networks) can differ from those exported by GPlates 2.4 and older. "
				"This is because GPlates <= 2.4 calculated a topology's normalisation factor as :math:`\\frac{1}{\\int \\cos(latitude)^2 \\, dS_i}` whereas pyGPlates (and GPlates > 2.4) "
				"calculate it as :math:`\\frac{1}{\\frac{2}{3} \\int \\, dS_i}` to avoid any variation with latitude. Both give the same total normalization of :math:`\\frac{3}{8\\pi}` when "
				"integrated over the *entire* globe and hence result in the same :meth:`total net rotation<NetRotationSnapshot.get_total_net_rotation>` over all topologies. However they will "
				"give different results for an individual topology (ie, integrated over only the rigid plate or deforming network). Equatorial topologies will now have a higher net rotation "
				"(and topologies nearer the poles will have a lower net rotation).\n"
				"\n"
				"  To iterate over the resolved topologies that **contribute net rotation** in this snapshot and print out their individual net rotations:\n"
				"  ::\n"
				"\n"
				"    net_rotation_dict = net_rotation_snapshot.get_net_rotation()\n"
				"    for resolved_topology, net_rotation in net_rotation_dict.items():\n"
				"        print('Topology {} has net rotation {}'.format(resolved_topology.get_feature().get_name(), net_rotation.get_finite_rotation()))\n"
				"\n"
				"  ...which is equivalent to the following:\n"
				"  ::\n"
				"\n"
				"    for resolved_topology in net_rotation_snapshot.get_topological_snapshot().get_resolved_topologies():\n"
				"        net_rotation = net_rotation_snapshot.get_net_rotation(resolved_topology)\n"
				"        # Not all resolved topologies in our topological snapshot will necessarily contribute net rotation.\n"
				"        if net_rotation:\n"
				"            print('Topology {} has net rotation {}'.format(resolved_topology.get_feature().get_name(), net_rotation.get_finite_rotation()))\n"
				"\n"
				"  .. seealso:: :meth:`get_total_net_rotation`\n")
		// Make hash and comparisons based on C++ object identity (not python object identity)...
		.def(GPlatesApi::ObjectIdentityHashDefVisitor())
	;

	// Register to/from Python conversions of non_null_intrusive_ptr<> including const/non-const and boost::optional.
	GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<GPlatesApi::NetRotationSnapshot>();


	std::stringstream net_rotation_model_create_docstring_stream;
	net_rotation_model_create_docstring_stream <<
			"__init__(topological_model, [velocity_delta_time=1.0], [velocity_delta_time_type=pygplates.VelocityDeltaTimeType.t_plus_delta_t_to_t], [point_distribution])\n"
			"  Net rotation snapshots will be calculated from the specified topological model, and using the requested parameters.\n"
			"\n"
			"  :param topological_model: The topological model to calculate net rotations with.\n"
			"  :type topological_model: TopologicalModel\n"
			"  :param velocity_delta_time: The time delta used to calculate velocities for net rotation (defaults to 1 Myr).\n"
			"  :type velocity_delta_time: float\n"
			"  :param velocity_delta_time_type: How the two velocity times are calculated relative to the reconstruction time. "
			"This includes [t+dt, t], [t, t-dt] and [t+dt/2, t-dt/2]. Defaults to [t+dt, t].\n"
			"  :type velocity_delta_time_type: VelocityDeltaTimeType.t_plus_delta_t_to_t, "
			"VelocityDeltaTimeType.t_to_t_minus_delta_t or VelocityDeltaTimeType.t_plus_minus_half_delta_t\n"
			"  :param point_distribution: Can be an integer `N` representing the number of uniformly spaced latitude-longitude grid points "
			"sampled along each *meridian* (ie, an `N x 2N` grid). Or can be a sequence of (point, sample_area) tuples where *point* is a "
			"point that contributes to net rotation and *sample_area* is the surface area around the point in steradians (square radians). "
			"If nothing specified then defaults to a `"
			<< GPlatesApi::NetRotationSnapshot::DEFAULT_NUM_SAMPLES_ALONG_MERIDIAN << " x " << 2 * GPlatesApi::NetRotationSnapshot::DEFAULT_NUM_SAMPLES_ALONG_MERIDIAN <<
			"` uniformly spaced latitude-longitude points.\n"
			"  :type point_distribution: int, or sequence of tuple (point, float) where point is a "
			"PointOnSphere or LatLonPoint or tuple (latitude,longitude), in degrees, or tuple (x,y,z)\n"
			"  :raises: ValueError if *velocity_delta_time* is negative or zero.\n"
			"\n"
			"  The `total net rotation <https://doi.org/10.1016/j.epsl.2009.12.055>`_ of all resolved topologies in a snapshot (:meth:`NetRotationModel.net_rotation_snapshot`) is:\n"
			"\n"
			"  .. math::\n"
			"\n"
			"     \\boldsymbol \\omega_{net} = \\frac{3}{8\\pi} \\sum_i \\int \\boldsymbol r \\times (\\boldsymbol \\omega_i(\\boldsymbol r) \\times \\boldsymbol r) \\, dS_i\n"
			"\n"
			"  ...where :math:`\\sum_i` is the summation over all resolved topologies, and :math:`\\int ... dS_i` is integration over the surface area of resolved topology :math:`i`, and "
			":math:`\\boldsymbol \\omega_i(\\boldsymbol r)` is the rotation rate vector for resolved topology :math:`i` at location :math:`\\boldsymbol r`. For a rigid plate this is "
			"just a constant :math:`\\boldsymbol \\omega_i(\\boldsymbol r) = \\boldsymbol \\omega_i`, but for a deforming network this varies spatially across the network "
			"(hence the dependency on :math:`\\boldsymbol r`). Note that if a deforming network overlaps a rigid plate then only the deforming network contributes to the "
			"total net rotation (in the overlap region).\n"
			"\n"
			"  The surface integrals are approximated by sampling at points distributed across the surface of the globe. So it's helpful to think of a *single* integral "
			"over the entire globe (rather than a separate integral for each topology) and then approximate that as a summation over points:\n"
			"\n"
			"  .. math::\n"
			"\n"
			"     \\boldsymbol \\omega_{net} &= \\frac{3}{8\\pi} \\int \\boldsymbol r \\times (\\boldsymbol \\omega(\\boldsymbol r) \\times \\boldsymbol r) \\, dS\\\\\n"
			"                                &\\approx \\frac{3}{8\\pi} \\sum_p (\\boldsymbol r_p \\times (\\boldsymbol \\omega(\\boldsymbol r_p) \\times \\boldsymbol r_p)) \\, dS_p\n"
			"\n"
			"  ...where the summation is over all locations :math:`\\boldsymbol r_p` in the point distribution :math:`p`, and :math:`dS_p` is the surface area contributed at "
			"location :math:`\\boldsymbol r_p` (where :math:`\\sum_p dS_p \\approx 4\\pi`), and :math:`\\boldsymbol \\omega(\\boldsymbol r_p)` is the rotation rate vector contributed "
			"by whichever resolved topology happens to intersect location :math:`\\boldsymbol r_p` (with preference given to deforming networks when they overlap rigid plates).\n"
			"\n"
			"  The point distribution used to calculate net rotation can be controlled with the *point_distribution* parameter. "
			"If *point_distribution* is an integer `N` then a uniform latitude-longitude grid of `N x 2N` points is used. "
			"Otherwise *point_distribution* can be a sequence of (point, sample area) representing an arbitrary user-specified distribution of points (and their sample areas). "
			"For example, if you have a distribution that is uniformly spaced on the surface of the sphere then the sample area will be "
			"the same for each point (ie, :math:`4\\pi` steradians divided by the total number of points). "
			"If nothing is specified for *point_distribution* then a uniform latitude-longitude grid of `"
			<< GPlatesApi::NetRotationSnapshot::DEFAULT_NUM_SAMPLES_ALONG_MERIDIAN << " x " << 2 * GPlatesApi::NetRotationSnapshot::DEFAULT_NUM_SAMPLES_ALONG_MERIDIAN <<
			"` points is used.\n"
			"\n"
			"  To create a net rotation snapshot at 0Ma calculated from a net rotation model with a velocity delta from 1Ma (to 0Ma):"
			"  ::\n"
			"\n"
			"    net_rotation_model = pygplates.NetRotationModel(\n"
			"        topological_model, 1.0, pygplates.VelocityDeltaTimeType.t_plus_delta_t_to_t)\n"
			"    net_rotation_snapshot = net_rotation_model.net_rotation_snapshot(0)\n"
			"\n"
			"  ...which is equivalent to the following code that explicitly specifies a point distribution:"
			"  ::\n"
			"\n"
			"    point_distribution = []\n"
			"    num_samples_along_meridian = "
			<< GPlatesApi::NetRotationSnapshot::DEFAULT_NUM_SAMPLES_ALONG_MERIDIAN <<
			"\n"
			"    delta_in_degrees = 180.0 / num_samples_along_meridian\n"
			"    delta_in_radians = math.radians(delta_in_degrees)\n"
			"    for lat_index in range(num_samples_along_meridian):\n"
			"        lat = -90.0 + (lat_index + 0.5) * delta_in_degrees\n"
			"        # The cosine is because points near the North/South poles are closer together (latitude parallel small circle radius).\n"
			"        sample_area_radians = math.cos(math.radians(lat)) * delta_in_radians * delta_in_radians\n"
			"        for lon_index in range(2*num_samples_along_meridian):\n"
			"            lon = -180.0 + (lon_index + 0.5) * delta_in_degrees\n"
			"            point_distribution.append(((lat, lon), sample_area_radians))\n"
			"\n"
			"    net_rotation_model = pygplates.NetRotationModel(\n"
			"        topological_model, 1.0, pygplates.VelocityDeltaTimeType.t_plus_delta_t_to_t, point_distribution)\n"
			"    net_rotation_snapshot = net_rotation_model.net_rotation_snapshot(0)\n"
			"\n"
			"  .. note:: The anchor plate is that of the specified topological model (see :meth:`TopologicalModel.get_anchor_plate_id`).\n";

	//
	// NetRotationModel - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesApi::NetRotationModel,
			GPlatesApi::NetRotationModel::non_null_ptr_type,
			boost::noncopyable>(
					"NetRotationModel",
					"Net rotation of topological plates and deforming networks.\n"
					"\n"
					"A *NetRotationModel* can also be `pickled <https://docs.python.org/3/library/pickle.html>`_.\n"
					"\n"
					".. versionadded:: 0.43\n",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::net_rotation_model_create,
						bp::default_call_policies(),
						(bp::arg("topological_model"),
							bp::arg("velocity_delta_time"),
							bp::arg("velocity_delta_time_type"),
							bp::arg("point_distribution") = GPlatesApi::NetRotationSnapshot::DEFAULT_NUM_SAMPLES_ALONG_MERIDIAN)),
				net_rotation_model_create_docstring_stream.str().c_str())
		// Pickle support...
		//
		// Note: This adds an __init__ method accepting a single argument (of type 'bytes') that supports pickling.
		//       So we define this *after* (higher priority) the other __init__ methods in case one of them accepts a single argument
		//       of type bp::object (which, being more general, would otherwise obscure the __init__ that supports pickling).
		.def(GPlatesApi::PythonPickle::PickleDefVisitor<GPlatesApi::NetRotationModel::non_null_ptr_type>())
		.def("net_rotation_snapshot",
				&GPlatesApi::net_rotation_model_create_net_rotation_snapshot,
				(bp::arg("reconstruction_time")),
				"net_rotation_snapshot(reconstruction_time)\n"
				"  Returns a snapshot of net rotation at the requested reconstruction time.\n"
				"\n"
				"  :param reconstruction_time: the geological time of the snapshot\n"
				"  :type reconstruction_time: float or GeoTimeInstant\n"
				"  :rtype: NetRotationSnapshot\n"
				"  :raises: ValueError if *reconstruction_time* is distant-past (``float('inf')``) or distant-future (``float('-inf')``).\n")
		.def("get_topological_model",
				&GPlatesApi::NetRotationModel::get_topological_model,
				"get_topological_model()\n"
				"  Return the topological model used internally.\n"
				"\n"
				"  :rtype: TopologicalModel\n")
		// Make hash and comparisons based on C++ object identity (not python object identity)...
		.def(GPlatesApi::ObjectIdentityHashDefVisitor())
	;

	// Register to/from Python conversions of non_null_intrusive_ptr<> including const/non-const and boost::optional.
	GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<GPlatesApi::NetRotationModel>();
}
