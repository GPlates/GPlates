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
				const double &max_total_strain_rate)
		{
			return ResolveTopologyParameters::create(enable_strain_rate_clamping, max_total_strain_rate);
		}
	}
}


GPlatesApi::ResolveTopologyParameters::ResolveTopologyParameters(
		bool enable_strain_rate_clamping,
		const double &max_total_strain_rate)
{
	GPlatesAppLogic::TopologyNetworkParams::StrainRateClamping strain_rate_clamping =
			d_topology_network_params.get_strain_rate_clamping();

	strain_rate_clamping.enable_clamping = enable_strain_rate_clamping;
	strain_rate_clamping.max_total_strain_rate = max_total_strain_rate;

	d_topology_network_params.set_strain_rate_clamping(strain_rate_clamping);
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
	// Docstring for constructor of pygplates.ResolveTopologyParameters.
	std::stringstream resolve_topology_parameters_constructor_docstring_stream;
	resolve_topology_parameters_constructor_docstring_stream <<
			// Specific overload signature...
			"__init__([enable_strain_rate_clamping="
			<< (GPlatesApi::ResolveTopologyParameters::DEFAULT_TOPOLOGY_NETWORK_PARAMS.get_strain_rate_clamping().enable_clamping ? "True" : "False")
			<< "], [max_clamped_strain_rate="
			<< GPlatesApi::ResolveTopologyParameters::DEFAULT_TOPOLOGY_NETWORK_PARAMS.get_strain_rate_clamping().max_total_strain_rate
			<< "])\n"
			"  Create the parameters used to resolve topologies.\n"
			"\n"
			"  :param enable_strain_rate_clamping: Whether to enable clamping of strain rate. "
			"This is useful to avoid excessive extension/compression in deforming networks "
			"(depending on how the deforming networks were built). Defaults to ``"
			<< (GPlatesApi::ResolveTopologyParameters::DEFAULT_TOPOLOGY_NETWORK_PARAMS.get_strain_rate_clamping().enable_clamping ? "True" : "False")
			<< "``.\n"
			"  :type enable_strain_rate_clamping: bool\n"
			"  :param max_clamped_strain_rate: Maximum total strain rate (in units of 1/second). "
			"This is only used if *enable_strain_rate_clamping* is true. "
			"Clamping strain rates also limits derived quantities such as crustal thinning and tectonic subsidence. "
			"The *total* strain rate includes both the normal and shear components of deformation. Default value is ``"
			<< GPlatesApi::ResolveTopologyParameters::DEFAULT_TOPOLOGY_NETWORK_PARAMS.get_strain_rate_clamping().max_total_strain_rate
			<< "`` 1/second.\n"
			"  :type max_clamped_strain_rate: float\n"
			"\n"
			"  Enable strain rate clamping for a topological model to avoid excessive crustal stretching factors:\n"
			"  ::\n"
			"\n"
			"    topological_model = pygplates.TopologicalModel(\n"
			"        topology_filenames,\n"
			"        rotation_filenames,\n"
			"        default_resolve_topology_parameters = pygplates.ResolveTopologyParameters(enable_strain_rate_clamping = True))\n"
			;

	//
	// ResolveTopologyParameters - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesApi::ResolveTopologyParameters,
			GPlatesApi::ResolveTopologyParameters::non_null_ptr_type,
			boost::noncopyable>(
					"ResolveTopologyParameters",
					"Specify parameters used to resolve topologies.\n"
					"\n"
					"A *ResolveTopologyParameters* can also be `pickled <https://docs.python.org/3/library/pickle.html>`_.\n"
					"\n"
					".. versionadded:: 0.31\n"
					"\n"
					".. versionchanged:: 0.42\n"
					"   Added pickle support.\n",
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
								GPlatesApi::ResolveTopologyParameters::DEFAULT_TOPOLOGY_NETWORK_PARAMS.get_strain_rate_clamping().max_total_strain_rate)),
			resolve_topology_parameters_constructor_docstring_stream.str().c_str())
		// Pickle support...
		.def(GPlatesApi::PythonPickle::PickleDefVisitor<GPlatesApi::ResolveTopologyParameters::non_null_ptr_type>())
	;

	// Register to/from Python conversions of non_null_intrusive_ptr<> including const/non-const and boost::optional.
	GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<GPlatesApi::ResolveTopologyParameters>();
}
