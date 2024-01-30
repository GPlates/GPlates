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
#include "PythonHashDefVisitor.h"
#include "PythonPickle.h"
#include "PythonVariableFunctionArguments.h"

#include "global/GPlatesAssert.h"

#include "property-values/GeoTimeInstant.h"

#include "scribe/Scribe.h"


namespace bp = boost::python;


namespace GPlatesApi
{
	/**
	 * This is called directly from Python via 'NetRotationSnapshot.__init__()'.
	 */
	NetRotationSnapshot::non_null_ptr_type
	net_rotation_snapshot_create(
			TopologicalSnapshot::non_null_ptr_type topological_snapshot,
			const double &velocity_delta_time,
			GPlatesAppLogic::VelocityDeltaTime::Type velocity_delta_time_type,
			int num_samples_along_meridian)
	{
		// Velocity delta time must be positive.
		if (velocity_delta_time <= 0)
		{
			PyErr_SetString(PyExc_ValueError, "Velocity delta time must be positive.");
			bp::throw_error_already_set();
		}

		// Num samples must be positive.
		if (num_samples_along_meridian <= 0)
		{
			PyErr_SetString(PyExc_ValueError, "Number of samples along meridian must be positive.");
			bp::throw_error_already_set();
		}

		return NetRotationSnapshot::create(
				topological_snapshot,
				velocity_delta_time,
				velocity_delta_time_type,
				num_samples_along_meridian);
	}

	GPlatesAppLogic::NetRotationUtils::NetRotationAccumulator
	net_rotation_snapshot_get_total_net_rotation(
			NetRotationSnapshot::non_null_ptr_type net_rotation_snapshot)
	{
		return net_rotation_snapshot->get_net_rotation_calculator().get_total_net_rotation();
	}

	NetRotationSnapshot::non_null_ptr_type
	NetRotationSnapshot::create(
			TopologicalSnapshot::non_null_ptr_type topological_snapshot,
			const double &velocity_delta_time,
			GPlatesAppLogic::VelocityDeltaTime::Type velocity_delta_time_type,
			unsigned int num_samples_along_meridian)
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
						num_samples_along_meridian));
	}

	NetRotationSnapshot::NetRotationSnapshot(
			TopologicalSnapshot::non_null_ptr_type topological_snapshot,
			const GPlatesAppLogic::NetRotationUtils::NetRotationCalculator::resolved_topological_boundary_seq_type &resolved_topological_boundaries,
			const GPlatesAppLogic::NetRotationUtils::NetRotationCalculator::resolved_topological_network_seq_type &resolved_topological_networks,
			const double &velocity_delta_time,
			GPlatesAppLogic::VelocityDeltaTime::Type velocity_delta_time_type,
			unsigned int num_samples_along_meridian) :
		d_topological_snapshot(topological_snapshot),
		d_net_rotation_calculator(
					resolved_topological_boundaries,
					resolved_topological_networks,
					topological_snapshot->get_reconstruction_time(),
					velocity_delta_time,
					velocity_delta_time_type,
					d_topological_snapshot->get_anchor_plate_id(),
					num_samples_along_meridian)
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
			unsigned int num_samples_along_meridian;
			if (!load_construct_data(
					scribe,
					topological_snapshot,
					resolved_topological_boundaries,
					resolved_topological_networks,
					velocity_delta_time,
					velocity_delta_time_type,
					num_samples_along_meridian))
			{
				return scribe.get_transcribe_result();
			}

			// Create the topological model.
			net_rotation_snapshot.construct_object(
					topological_snapshot,
					resolved_topological_boundaries,
					resolved_topological_networks,
					velocity_delta_time,
					velocity_delta_time_type,
					num_samples_along_meridian);
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
				unsigned int num_samples_along_meridian;
				if (!load_construct_data(
						scribe,
						topological_snapshot,
						resolved_topological_boundaries,
						resolved_topological_networks,
						velocity_delta_time,
						velocity_delta_time_type,
						num_samples_along_meridian))
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
						d_topological_snapshot->get_anchor_plate_id(),
						num_samples_along_meridian);
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

		// Save the number of samples along meridian.
		scribe.save(TRANSCRIBE_SOURCE, net_rotation_snapshot.d_net_rotation_calculator.get_num_samples_along_meridian(), "num_samples_along_meridian");
	}

	bool
	NetRotationSnapshot::load_construct_data(
			GPlatesScribe::Scribe &scribe,
			GPlatesScribe::LoadRef<TopologicalSnapshot::non_null_ptr_type> &topological_snapshot,
			GPlatesAppLogic::NetRotationUtils::NetRotationCalculator::resolved_topological_boundary_seq_type &resolved_topological_boundaries,
			GPlatesAppLogic::NetRotationUtils::NetRotationCalculator::resolved_topological_network_seq_type &resolved_topological_networks,
			double &velocity_delta_time,
			GPlatesAppLogic::VelocityDeltaTime::Type &velocity_delta_time_type,
			unsigned int &num_samples_along_meridian)
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

		// Load the number of samples along meridian.
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, num_samples_along_meridian, "num_samples_along_meridian"))
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
			TopologicalModel::non_null_ptr_type topological_model)
	{
		return NetRotationModel::create(topological_model);
	}

	/**
	 * This is called directly from Python via 'NetRotationModel.get_net_rotation_snapshot()'.
	 */
	NetRotationSnapshot::non_null_ptr_type
	net_rotation_model_create_topological_snapshot(
			NetRotationModel::non_null_ptr_type net_rotation_model,
			const GPlatesPropertyValues::GeoTimeInstant &reconstruction_time,
			const double &velocity_delta_time,
			GPlatesAppLogic::VelocityDeltaTime::Type velocity_delta_time_type,
			int num_samples_along_meridian)
	{
		// Time must not be distant past/future.
		if (!reconstruction_time.is_real())
		{
			PyErr_SetString(PyExc_ValueError,
					"Time values cannot be distant-past (float('inf')) or distant-future (float('-inf')).");
			bp::throw_error_already_set();
		}

		// Velocity delta time must be positive.
		if (velocity_delta_time <= 0)
		{
			PyErr_SetString(PyExc_ValueError, "Velocity delta time must be positive.");
			bp::throw_error_already_set();
		}

		// Num samples must be positive.
		if (num_samples_along_meridian <= 0)
		{
			PyErr_SetString(PyExc_ValueError, "Number of samples along meridian must be positive.");
			bp::throw_error_already_set();
		}

		return net_rotation_model->create_net_rotation_snapshot(
				reconstruction_time.value(),
				velocity_delta_time,
				velocity_delta_time_type,
				num_samples_along_meridian);
	}

	NetRotationModel::non_null_ptr_type
	NetRotationModel::create(
			TopologicalModel::non_null_ptr_type topological_model)
	{
		return non_null_ptr_type(new NetRotationModel(topological_model));
	}

	NetRotationModel::NetRotationModel(
			TopologicalModel::non_null_ptr_type topological_model) :
		d_topological_model(topological_model)
	{  }
	
	NetRotationSnapshot::non_null_ptr_type
	NetRotationModel::create_net_rotation_snapshot(
			const double &reconstruction_time,
			const double &velocity_delta_time,
			GPlatesAppLogic::VelocityDeltaTime::Type velocity_delta_time_type,
			unsigned int num_samples_along_meridian) const
	{
		TopologicalSnapshot::non_null_ptr_type topological_snapshot =
				d_topological_model->create_topological_snapshot(reconstruction_time);

		return NetRotationSnapshot::create(
				topological_snapshot,
				velocity_delta_time,
				velocity_delta_time_type,
				num_samples_along_meridian);
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
			if (!load_construct_data(scribe, topological_model))
			{
				return scribe.get_transcribe_result();
			}

			// Create the net rotation model.
			net_rotation_model.construct_object(topological_model);
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
				if (!load_construct_data(scribe, topological_model))
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
		// Save the topological model.
		scribe.save(TRANSCRIBE_SOURCE, net_rotation_model.d_topological_model, "topological_model");
	}

	bool
	NetRotationModel::load_construct_data(
			GPlatesScribe::Scribe &scribe,
			GPlatesScribe::LoadRef<TopologicalModel::non_null_ptr_type> &topological_model)
	{
		// Load the topological model.
		topological_model = scribe.load<TopologicalModel::non_null_ptr_type>(TRANSCRIBE_SOURCE, "topological_model");
		if (!topological_model.is_valid())
		{
			return false;
		}

		return true;
	}
}

	
void
export_net_rotation()
{
	// An enumeration nested within 'pygplates' (ie, current) module.
	bp::enum_<GPlatesAppLogic::VelocityDeltaTime::Type>("VelocityDeltaTimeType")
			.value("t_plus_delta_t_to_t", GPlatesAppLogic::VelocityDeltaTime::T_PLUS_DELTA_T_TO_T)
			.value("t_to_t_minus_delta_t", GPlatesAppLogic::VelocityDeltaTime::T_TO_T_MINUS_DELTA_T)
			.value("t_plus_minus_half_delta_t", GPlatesAppLogic::VelocityDeltaTime::T_PLUS_MINUS_HALF_DELTA_T);


	//
	// NetRotation - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<GPlatesAppLogic::NetRotationUtils::NetRotationAccumulator>(
			"NetRotation",
			"Net rotation of regional or global crust.\n"
			"\n"
			"A *NetRotation* can also be `pickled <https://docs.python.org/3/library/pickle.html>`_.\n"
			"\n"
			".. versionadded:: 0.43\n",
			bp::init<>("__init__()\n")) // Sphinx autosummary complains if signature not present in docstring.
		// Pickle support...
		//
		// Note: This adds an __init__ method accepting a single argument (of type 'bytes') that supports pickling.
		//       So we define this *after* (higher priority) the other __init__ methods in case one of them accepts a single argument
		//       of type bp::object (which, being more general, would otherwise obscure the __init__ that supports pickling).
		.def(GPlatesApi::PythonPickle::PickleDefVisitor<boost::shared_ptr<GPlatesAppLogic::NetRotationUtils::NetRotationAccumulator>>())
		.def("get_finite_rotation",
				&GPlatesAppLogic::NetRotationUtils::NetRotationAccumulator::get_net_rotation,
				"get_finite_rotation()\n"
				"  Return the accumulated net rotation as a finite rotation.\n"
				"\n"
				"  :rtype: :class:`FiniteRotation`\n")
		// Make unhashable, with no comparison operators...
		.def(GPlatesApi::NoHashDefVisitor(false, false))
	;

	// Enable boost::optional<FiniteRotation> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::register_optional_conversion<GPlatesAppLogic::NetRotationUtils::NetRotationAccumulator>();


	std::stringstream net_rotation_snapshot_create_docstring_stream;
	net_rotation_snapshot_create_docstring_stream <<
			"__init__(topological_snapshot, velocity_delta_time, velocity_delta_time_type, [num_samples_along_meridian="
			<< GPlatesApi::NetRotationSnapshot::DEFAULT_NUM_SAMPLES_ALONG_MERIDIAN <<
			"])\n"
			"  Net rotation of the specified topological snapshot, and using the requested parameters.\n"
			"\n"
			"  A uniform lat-lon grid of *num_samples_along_meridian* x *2*num_samples_along_meridian* points is used "
			"to sample velocities of resolved topologies of the topological snapshot.\n"
			"\n"
			"  :param topological_snapshot: The topological snapshot to calculate net rotation with.\n"
			"  :type topological_snapshot: :class:`TopologicalSnapshot`\n"
			"  :param velocity_delta_time: The time delta used to calculate velocities for net rotation.\n"
			"  :type velocity_delta_time: float\n"
			"  :param velocity_delta_time_type: How the two velocity times are calculated relative to the reconstruction time. "
			"This includes [t+dt, t, [t, t-dt] and [t+dt/2, t-dt/2].\n"
			"  :type velocity_delta_time_type: *VelocityDeltaTimeType.t_plus_delta_t_to_t*, "
			"*VelocityDeltaTimeType.t_to_t_minus_delta_t* or *VelocityDeltaTimeType.t_plus_minus_half_delta_t*\n"
			"  :param num_samples_along_meridian: The number of grid points sampled along each meridian. Defaults to "
			<< GPlatesApi::NetRotationSnapshot::DEFAULT_NUM_SAMPLES_ALONG_MERIDIAN <<
			".\n"
			"  :type num_samples_along_meridian: int\n"
			"  :raises: ValueError if *velocity_delta_time* is negative or zero.\n"
			"  :raises: ValueError if *num_samples_along_meridian* is negative or zero.\n"
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
							bp::arg("velocity_delta_time"),
							bp::arg("velocity_delta_time_type"),
							bp::arg("num_samples_along_meridian") = GPlatesApi::NetRotationSnapshot::DEFAULT_NUM_SAMPLES_ALONG_MERIDIAN)),
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
				"  :rtype: :class:`TopologicalSnapshot`\n"
				"\n"
				"  .. note:: Parameters such as reconstruction time, anchor plate ID and rotation model can be obtained from the topological snapshot.\n")
		.def("get_total_net_rotation",
				&GPlatesApi::net_rotation_snapshot_get_total_net_rotation,
				"get_total_net_rotation()\n"
				"  Return the accumulated net rotation over all resolved topologies in this snapshot.\n"
				"\n"
				"  :rtype: :class:`NetRotation`\n")
		.def("get_velocity_delta_time",
				&GPlatesApi::NetRotationSnapshot::get_velocity_delta_time,
				"get_velocity_delta_time()\n"
				"  Return the time delta used to calculate velocities for net rotation.\n"
				"\n"
				"  :rtype: float\n")
		.def("get_velocity_delta_time_type",
				&GPlatesApi::NetRotationSnapshot::get_velocity_delta_time_type,
				"get_velocity_delta_time_type()\n"
				"  Return how the two velocity times are calculated relative to the reconstruction time.\n"
				"\n"
				"  :rtype: *VelocityDeltaTimeType.t_plus_delta_t_to_t*, "
				"*VelocityDeltaTimeType.t_to_t_minus_delta_t* or *VelocityDeltaTimeType.t_plus_minus_half_delta_t*\n")
		.def("get_num_samples_along_meridian",
				&GPlatesApi::NetRotationSnapshot::get_num_samples_along_meridian,
				"get_num_samples_along_meridian()\n"
				"  Return the number of grid points sampled along each meridian.\n"
				"\n"
				"  :rtype: int\n")
		// Make hash and comparisons based on C++ object identity (not python object identity)...
		.def(GPlatesApi::ObjectIdentityHashDefVisitor())
	;

	// Register to/from Python conversions of non_null_intrusive_ptr<> including const/non-const and boost::optional.
	GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<GPlatesApi::NetRotationSnapshot>();


	std::stringstream net_rotation_model_create_topological_snapshot_docstring_stream;
	net_rotation_model_create_topological_snapshot_docstring_stream <<
			"net_rotation_snapshot(reconstruction_time, velocity_delta_time, velocity_delta_time_type, [num_samples_along_meridian="
			<< GPlatesApi::NetRotationSnapshot::DEFAULT_NUM_SAMPLES_ALONG_MERIDIAN <<
			"])\n"
			"  Returns a snapshot of net rotation at the requested reconstruction time, and using the requested parameters.\n"
			"\n"
			"  A uniform lat-lon grid of *num_samples_along_meridian* x *2*num_samples_along_meridian* points is used "
			"to sample velocities of resolved topologies at the specified reconstruction time.\n"
			"\n"
			"  :param reconstruction_time: the geological time of the snapshot\n"
			"  :type reconstruction_time: float or :class:`GeoTimeInstant`\n"
			"  :param velocity_delta_time: The time delta used to calculate velocities for net rotation.\n"
			"  :type velocity_delta_time: float\n"
			"  :param velocity_delta_time_type: How the two velocity times are calculated relative to the reconstruction time. "
			"This includes [t+dt, t, [t, t-dt] and [t+dt/2, t-dt/2].\n"
			"  :type velocity_delta_time_type: *VelocityDeltaTimeType.t_plus_delta_t_to_t*, "
			"*VelocityDeltaTimeType.t_to_t_minus_delta_t* or *VelocityDeltaTimeType.t_plus_minus_half_delta_t*\n"
			"  :param num_samples_along_meridian: The number of grid points sampled along each meridian. Defaults to "
			<< GPlatesApi::NetRotationSnapshot::DEFAULT_NUM_SAMPLES_ALONG_MERIDIAN <<
			".\n"
			"  :type num_samples_along_meridian: int\n"
			"  :rtype: :class:`NetRotationSnapshot`\n"
			"  :raises: ValueError if *reconstruction_time* is distant-past (``float('inf')``) or distant-future (``float('-inf')``).\n"
			"  :raises: ValueError if *velocity_delta_time* is negative or zero.\n"
			"  :raises: ValueError if *num_samples_along_meridian* is negative or zero.\n"
			"\n"
			"  .. note:: The anchor plate is that of the topological model specified in the :meth:`constructor<__init__>` "
			"(see :meth:`TopologicalModel.get_anchor_plate_id`).\n";

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
						(bp::arg("topological_model"))),
				"__init__(topological_model)\n"
				"  Net rotations will be calculated from the specified topological model.\n"
				"\n"
				"  :param topological_model: The topological model to calculate net rotations with.\n"
				"  :type topological_model: :class:`TopologicalModel`\n"
				"\n"
				"  .. note:: The anchor plate is that of the specified topological model (see :meth:`TopologicalModel.get_anchor_plate_id`).\n")
		// Pickle support...
		//
		// Note: This adds an __init__ method accepting a single argument (of type 'bytes') that supports pickling.
		//       So we define this *after* (higher priority) the other __init__ methods in case one of them accepts a single argument
		//       of type bp::object (which, being more general, would otherwise obscure the __init__ that supports pickling).
		.def(GPlatesApi::PythonPickle::PickleDefVisitor<GPlatesApi::NetRotationModel::non_null_ptr_type>())
		.def("net_rotation_snapshot",
				&GPlatesApi::net_rotation_model_create_topological_snapshot,
				(bp::arg("reconstruction_time"),
					bp::arg("velocity_delta_time"),
					bp::arg("velocity_delta_time_type"),
					bp::arg("num_samples_along_meridian") = GPlatesApi::NetRotationSnapshot::DEFAULT_NUM_SAMPLES_ALONG_MERIDIAN),
				net_rotation_model_create_topological_snapshot_docstring_stream.str().c_str())
		.def("get_topological_model",
				&GPlatesApi::NetRotationModel::get_topological_model,
				"get_topological_model()\n"
				"  Return the topological model used internally.\n"
				"\n"
				"  :rtype: :class:`TopologicalModel`\n")
		// Make hash and comparisons based on C++ object identity (not python object identity)...
		.def(GPlatesApi::ObjectIdentityHashDefVisitor())
	;

	// Register to/from Python conversions of non_null_intrusive_ptr<> including const/non-const and boost::optional.
	GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<GPlatesApi::NetRotationModel>();
}
