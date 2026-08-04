/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2021 The University of Sydney, Australia
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

#include "PyResolveTopologyParameters.h"

#include "PythonConverterUtils.h"
#include "PythonHashDefVisitor.h"
#include "PythonPickle.h"

#include "global/python.h"

#include "scribe/Scribe.h"


namespace bp = boost::python;


const GPlatesAppLogic::TopologyNetworkParams GPlatesApi::ResolveTopologyParameters::DEFAULT_TOPOLOGY_NETWORK_PARAMS;


namespace GPlatesApi
{
	namespace
	{
		/**
		 * This is called directly from Python via 'ResolveTopologyParameters.__init__()'.
		 */
		ResolveTopologyParameters::non_null_ptr_type
		resolve_topology_parameters_create(
				bool enable_strain_rate_clamping,
				const double &max_total_strain_rate,
				GPlatesAppLogic::TopologyNetworkParams::StrainRateSmoothing strain_rate_smoothing,
				const double &rift_exponential_stretching_constant,
				const double &rift_strain_rate_resolution,
				const double &rift_edge_length_threshold_degrees)
		{
			return ResolveTopologyParameters::create(
					enable_strain_rate_clamping,
					max_total_strain_rate,
					strain_rate_smoothing,
					rift_exponential_stretching_constant,
					rift_strain_rate_resolution,
					rift_edge_length_threshold_degrees);
		}
	}
}


GPlatesApi::ResolveTopologyParameters::ResolveTopologyParameters(
		bool enable_strain_rate_clamping,
		const double &max_total_strain_rate,
		GPlatesAppLogic::TopologyNetworkParams::StrainRateSmoothing strain_rate_smoothing,
		const double &rift_exponential_stretching_constant,
		const double &rift_strain_rate_resolution,
		const double &rift_edge_length_threshold_degrees)
{
	GPlatesAppLogic::TopologyNetworkParams::StrainRateClamping strain_rate_clamping =
			d_topology_network_params.get_strain_rate_clamping();
	strain_rate_clamping.enable_clamping = enable_strain_rate_clamping;
	strain_rate_clamping.max_total_strain_rate = max_total_strain_rate;
	d_topology_network_params.set_strain_rate_clamping(strain_rate_clamping);

	d_topology_network_params.set_strain_rate_smoothing(strain_rate_smoothing);

	GPlatesAppLogic::TopologyNetworkParams::RiftParams rift_params =
			d_topology_network_params.get_rift_params();
	rift_params.exponential_stretching_constant = rift_exponential_stretching_constant;
	rift_params.strain_rate_resolution = rift_strain_rate_resolution;
	rift_params.edge_length_threshold_degrees = rift_edge_length_threshold_degrees;
	d_topology_network_params.set_rift_params(rift_params);
}


GPlatesScribe::TranscribeResult
GPlatesApi::ResolveTopologyParameters::transcribe_construct_data(
		GPlatesScribe::Scribe &scribe,
		GPlatesScribe::ConstructObject<ResolveTopologyParameters> &resolved_topology_parameters)
{
	if (scribe.is_saving())
	{
		scribe.save(TRANSCRIBE_SOURCE, resolved_topology_parameters->d_topology_network_params, "topology_network_params");
	}
	else // loading
	{
		GPlatesAppLogic::TopologyNetworkParams topology_network_params;
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, topology_network_params, "topology_network_params"))
		{
			return scribe.get_transcribe_result();
		}

		resolved_topology_parameters.construct_object(topology_network_params);
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesScribe::TranscribeResult
GPlatesApi::ResolveTopologyParameters::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	if (!transcribed_construct_data)
	{
		if (scribe.is_saving())
		{
			scribe.save(TRANSCRIBE_SOURCE, d_topology_network_params, "topology_network_params");
		}
		else // loading
		{
			if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_topology_network_params, "topology_network_params"))
			{
				return scribe.get_transcribe_result();
			}
		}
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


void
export_resolve_topology_parameters()
{
	// An enumeration nested within 'pygplates' (ie, current) module.
	bp::enum_<GPlatesAppLogic::TopologyNetworkParams::StrainRateSmoothing>("StrainRateSmoothing")
			.value("none", GPlatesAppLogic::TopologyNetworkParams::NO_SMOOTHING)
			.value("barycentric", GPlatesAppLogic::TopologyNetworkParams::BARYCENTRIC_SMOOTHING)
			.value("natural_neighbour", GPlatesAppLogic::TopologyNetworkParams::NATURAL_NEIGHBOUR_SMOOTHING);


	const std::string default_strain_rate_smoothing_string =
			GPlatesApi::ResolveTopologyParameters::DEFAULT_TOPOLOGY_NETWORK_PARAMS.get_strain_rate_smoothing() == GPlatesAppLogic::TopologyNetworkParams::NO_SMOOTHING
				? "pygplates.StrainRateSmoothing.none"
				: (GPlatesApi::ResolveTopologyParameters::DEFAULT_TOPOLOGY_NETWORK_PARAMS.get_strain_rate_smoothing() == GPlatesAppLogic::TopologyNetworkParams::BARYCENTRIC_SMOOTHING
					? "pygplates.StrainRateSmoothing.barycentric" : "pygplates.StrainRateSmoothing.natural_neighbour");

	// Docstring for constructor of pygplates.ResolveTopologyParameters.
	std::stringstream resolve_topology_parameters_constructor_docstring_stream;
	resolve_topology_parameters_constructor_docstring_stream <<
			// Specific overload signature...
			"__init__([enable_strain_rate_clamping="
			<< (GPlatesApi::ResolveTopologyParameters::DEFAULT_TOPOLOGY_NETWORK_PARAMS.get_strain_rate_clamping().enable_clamping ? "True" : "False")
			<< "], [max_clamped_strain_rate="
			<< GPlatesApi::ResolveTopologyParameters::DEFAULT_TOPOLOGY_NETWORK_PARAMS.get_strain_rate_clamping().max_total_strain_rate
			<< "], [strain_rate_smoothing=" << default_strain_rate_smoothing_string
			<< "], [rift_exponential_stretching_constant="
			<< GPlatesApi::ResolveTopologyParameters::DEFAULT_TOPOLOGY_NETWORK_PARAMS.get_rift_params().exponential_stretching_constant
			<< "], [rift_strain_rate_resolution="
			<< GPlatesApi::ResolveTopologyParameters::DEFAULT_TOPOLOGY_NETWORK_PARAMS.get_rift_params().strain_rate_resolution
			<< "], [rift_edge_length_threshold_degrees="
			<< GPlatesApi::ResolveTopologyParameters::DEFAULT_TOPOLOGY_NETWORK_PARAMS.get_rift_params().edge_length_threshold_degrees
			<< "])\n"
			"  Specify the parameters used to resolve topologies.\n"
			"\n"
			"  :param enable_strain_rate_clamping: Whether to enable clamping of strain rate. Defaults to ``"
			<< (GPlatesApi::ResolveTopologyParameters::DEFAULT_TOPOLOGY_NETWORK_PARAMS.get_strain_rate_clamping().enable_clamping ? "True" : "False")
			<< "``. See :attr:`enable_strain_rate_clamping`. \n"
			"  :type enable_strain_rate_clamping: bool\n"
			"  :param max_clamped_strain_rate: Maximum :meth:`total strain rate <StrainRate.get_total_strain_rate>` (in units of :math:`second^{-1}`). "
			"This is only used if *enable_strain_rate_clamping* is true. Default value is ``"
			<< GPlatesApi::ResolveTopologyParameters::DEFAULT_TOPOLOGY_NETWORK_PARAMS.get_strain_rate_clamping().max_total_strain_rate
			<< "`` :math:`second^{-1}`. See :attr:`max_clamped_strain_rate`.\n"
			"  :type max_clamped_strain_rate: float\n"
			"  :param strain_rate_smoothing: How deformation strain rates are smoothed (if at all). "
			"This can be no smoothing, barycentric smoothing or natural neighbour smoothing. Default value is ``"
			<< default_strain_rate_smoothing_string << "``. See :attr:`strain_rate_smoothing`.\n"
			"  :type strain_rate_smoothing: pygplates.StrainRateSmoothing.none, "
			"pygplates.StrainRateSmoothing.barycentric or pygplates.StrainRateSmoothing.natural_neighbour\n"
			"  :param rift_exponential_stretching_constant: Controls the curvature of the exponential variation of stretching across a rift profile in a network triangulation. "
			"Default value is ``"
			<< GPlatesApi::ResolveTopologyParameters::DEFAULT_TOPOLOGY_NETWORK_PARAMS.get_rift_params().exponential_stretching_constant
			<< "``. See :attr:`rift_exponential_stretching_constant`.\n"
			"  :type rift_exponential_stretching_constant: float\n"
			"  :param rift_strain_rate_resolution: Controls how accurately the strain rate curve (across rift profile) matches exponential curve (in units of :math:`second^{-1}`). "
			"Default value is ``"
			<< GPlatesApi::ResolveTopologyParameters::DEFAULT_TOPOLOGY_NETWORK_PARAMS.get_rift_params().strain_rate_resolution
			<< "``. See :attr:`rift_strain_rate_resolution`.\n"
			"  :type rift_strain_rate_resolution: float\n"
			"  :param rift_edge_length_threshold_degrees: Rift edges in network triangulation shorter than this length (in degrees) will not be further sub-divided. "
			"Default value is ``"
			<< GPlatesApi::ResolveTopologyParameters::DEFAULT_TOPOLOGY_NETWORK_PARAMS.get_rift_params().edge_length_threshold_degrees
			<< "``. See :attr:`rift_edge_length_threshold_degrees`.\n"
			"  :type rift_edge_length_threshold_degrees: float\n"
			"\n"
			"  .. seealso:: :ref:`pygplates_primer_strain_rate_clamping`, :ref:`pygplates_primer_strain_rate_smoothing` and "
			":ref:`pygplates_primer_exponential_rift_stretching_profile` in the *Primer* documentation.\n"
			"\n"
			"  .. versionchanged:: 0.49\n"
			"     Added arguments *strain_rate_smoothing*, *rift_exponential_stretching_constant*, "
			"*rift_strain_rate_resolution* and *rift_edge_length_threshold*\n"
			;

	//
	// ResolveTopologyParameters - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesApi::ResolveTopologyParameters,
			GPlatesApi::ResolveTopologyParameters::non_null_ptr_type,
			boost::noncopyable>(
					"ResolveTopologyParameters",
					"Parameters used to resolve topologies.\n"
					"\n"
					"These parameters affect how topologies are resolved (when using :class:`TopologicalModel`, :class:`TopologicalSnapshot` or :func:`resolve_topologies`).\n"
					"\n"
					"Currently these parameters only affect :class:`resolved topological networks <ResolvedTopologicalNetwork>`. These parameters include:\n"
					"\n"
					"* *strain rate clamping* (see :attr:`enable_strain_rate_clamping` and :attr:`max_clamped_strain_rate`)\n"
					"* *strain rate smoothing* (see :attr:`strain_rate_smoothing`)\n"
					"* *rift exponential strain rate profiles* (see :attr:`rift_exponential_stretching_constant`, :attr:`rift_strain_rate_resolution` and :attr:`rift_edge_length_threshold_degrees`).\n"
					"\n"
					".. seealso:: :ref:`pygplates_primer_strain_rate_clamping`, :ref:`pygplates_primer_strain_rate_smoothing` and "
					":ref:`pygplates_primer_exponential_rift_stretching_profile` in the *Primer* documentation.\n"
					"\n"
					"ResolveTopologyParameters are equality (``==``, ``!=``) comparable (but not hashable - cannot be used as a key in a ``dict``).\n"
					"\n"
					"A *ResolveTopologyParameters* can also be `pickled <https://docs.python.org/3/library/pickle.html>`_.\n"
					"\n"
					".. versionadded:: 0.31\n"
					"\n"
					".. versionchanged:: 0.42\n"
					"   Added pickle support.\n"
					"\n"
					".. versionchanged:: 0.49\n"
					"   Added a class attribute for each parameter.\n",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::resolve_topology_parameters_create,
						bp::default_call_policies(),
						(bp::arg("enable_strain_rate_clamping") =
								GPlatesApi::ResolveTopologyParameters::DEFAULT_TOPOLOGY_NETWORK_PARAMS.get_strain_rate_clamping().enable_clamping,
							bp::arg("max_clamped_strain_rate") =
								GPlatesApi::ResolveTopologyParameters::DEFAULT_TOPOLOGY_NETWORK_PARAMS.get_strain_rate_clamping().max_total_strain_rate,
							bp::arg("strain_rate_smoothing") =
								GPlatesApi::ResolveTopologyParameters::DEFAULT_TOPOLOGY_NETWORK_PARAMS.get_strain_rate_smoothing(),
							bp::arg("rift_exponential_stretching_constant") =
								GPlatesApi::ResolveTopologyParameters::DEFAULT_TOPOLOGY_NETWORK_PARAMS.get_rift_params().exponential_stretching_constant,
							bp::arg("rift_strain_rate_resolution") =
								GPlatesApi::ResolveTopologyParameters::DEFAULT_TOPOLOGY_NETWORK_PARAMS.get_rift_params().strain_rate_resolution,
							bp::arg("rift_edge_length_threshold_degrees") =
								GPlatesApi::ResolveTopologyParameters::DEFAULT_TOPOLOGY_NETWORK_PARAMS.get_rift_params().edge_length_threshold_degrees)),
				resolve_topology_parameters_constructor_docstring_stream.str().c_str())
		// Pickle support...
		//
		// Note: This adds an __init__ method accepting a single argument (of type 'bytes') that supports pickling.
		//       So we define this *after* (higher priority) the other __init__ methods in case one of them accepts a single argument
		//       of type bp::object (which, being more general, would otherwise obscure the __init__ that supports pickling).
		.def(GPlatesApi::PythonPickle::PickleDefVisitor<GPlatesApi::ResolveTopologyParameters::non_null_ptr_type>())
		.add_property("enable_strain_rate_clamping",
				&GPlatesApi::ResolveTopologyParameters::get_enable_strain_rate_clamping,
				"Whether deformation strain rates are clamped.\n"
				"\n"
				"  :type: bool\n"
				"\n"
				"  This is useful to avoid excessive extension/compression in deforming networks (depending on how the deforming networks were built).\n"
				"\n"
				"  .. seealso:: :ref:`pygplates_primer_strain_rate_clamping` in the *Primer* documentation.\n"
				"\n"
				"  .. versionadded:: 0.49\n")
		.add_property("max_clamped_strain_rate",
				&GPlatesApi::ResolveTopologyParameters::get_max_clamped_strain_rate,
				"The maximum value that the :meth:`total strain rate <StrainRate.get_total_strain_rate>` is clamped to (in units of :math:`second^{-1}`).\n"
				"\n"
				"  :type: float\n"
				"\n"
				"  .. note:: This only applies if :attr:`enable_strain_rate_clamping` is ``True``.\n"
				"\n"
				"  .. seealso:: :ref:`pygplates_primer_strain_rate_clamping` in the *Primer* documentation.\n"
				"\n"
				"  .. versionadded:: 0.49\n")
		.add_property("strain_rate_smoothing",
				&GPlatesApi::ResolveTopologyParameters::get_strain_rate_smoothing,
				"How deformation strain rates are smoothed (if at all) when queried at arbitrary locations (in deforming network).\n"
				"\n"
				"  :type: pygplates.StrainRateSmoothing.none, pygplates.StrainRateSmoothing.barycentric or pygplates.StrainRateSmoothing.natural_neighbour\n"
				"\n"
				"  This can be no smoothing, barycentric smoothing or natural neighbour smoothing.\n"
				"\n"
				"  .. seealso:: :ref:`pygplates_primer_strain_rate_smoothing` in the *Primer* documentation.\n"
				"\n"
				"  .. versionadded:: 0.49\n")
		//! An edge should not be subdivided if it is shorter than this length.
		.add_property("rift_exponential_stretching_constant",
				&GPlatesApi::ResolveTopologyParameters::get_rift_exponential_stretching_constant,
				"Controls the curvature of the exponential variation of stretching across a rift profile in a network triangulation.\n"
				"\n"
				"  :type: float\n"
				"\n"
				"  .. seealso:: :ref:`pygplates_primer_exponential_rift_stretching_profile` in the *Primer* documentation.\n"
				"\n"
				"  .. versionadded:: 0.49\n")
		//! Default stretching profile is exp(exponential_stretching_constant * x).
		.add_property("rift_strain_rate_resolution",
				&GPlatesApi::ResolveTopologyParameters::get_rift_strain_rate_resolution,
				"Controls how accurately the strain rate curve (across rift profile) matches exponential curve (in units of :math:`second^{-1}`).\n"
				"\n"
				"  :type: float\n"
				"\n"
				"  Rift edges in the network triangulation are sub-divided until the strain rate matches the exponential curve (within this tolerance).\n"
				"\n"
				"  .. Note:: Sub-division is also limited by :attr:`rift_edge_length_threshold_degrees`.\n"
				"\n"
				"  .. seealso:: :ref:`pygplates_primer_exponential_rift_stretching_profile` in the *Primer* documentation.\n"
				"\n"
				"  .. versionadded:: 0.49\n")
		//! Adjacent strain rates samples should resolved within this tolerance (in units 1/sec).
		.add_property("rift_edge_length_threshold_degrees",
				&GPlatesApi::ResolveTopologyParameters::get_rift_edge_length_threshold_degrees,
				"Rift edges in network triangulation shorter than this length (in degrees) will not be further sub-divided.\n"
				"\n"
				"  :type: float\n"
				"\n"
				"  Rifts edges in network triangulation are sub-divided to fit an exponential strain rate profile in the rift stretching direction.\n"
				"\n"
				"  .. note:: Sub-division is also limited by :attr:`rift_strain_rate_resolution`.\n"
				"\n"
				"  .. seealso:: :ref:`pygplates_primer_exponential_rift_stretching_profile` in the *Primer* documentation.\n"
				"\n"
				"  .. versionadded:: 0.49\n")
		// Due to the numerical tolerance in comparisons we cannot make hashable.
		// Make unhashable, with no *equality* comparison operators (we explicitly define them)...
		.def(GPlatesApi::NoHashDefVisitor(false, true))
		.def(bp::self == bp::self)
		.def(bp::self != bp::self)
	;

	// Register to/from Python conversions of non_null_intrusive_ptr<> including const/non-const and boost::optional.
	GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<GPlatesApi::ResolveTopologyParameters>();
}
