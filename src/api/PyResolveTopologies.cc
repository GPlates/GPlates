/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2015 The University of Sydney, Australia
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

// Workaround for compile error in <pyport.h> for Python versions less than 2.7.13 and 3.5.3.
// See https://bugs.python.org/issue10910
// Workaround involves including "global/python.h" at the top of some source files
// to ensure <Python.h> is included before <ctype.h>.
#include "global/python.h"

#include <utility>
#include <vector>
#include <boost/optional.hpp>
#include <boost/variant.hpp>
#include <QString>

#include "PyFeatureCollectionFunctionArgument.h"
#include "PyRotationModel.h"
#include "PyTopologicalSnapshot.h"
#include "PythonConverterUtils.h"
#include "PythonHashDefVisitor.h"
#include "PythonUtils.h"
#include "PythonVariableFunctionArguments.h"

#include "app-logic/ReconstructionGeometry.h"
#include "app-logic/ResolvedTopologicalSection.h"

// This is not included by <boost/python.hpp>.
// Also we must include this after <boost/python.hpp> which means after "global/python.h".
#include <boost/python/raw_function.hpp>

#include "maths/PolygonOrientation.h"

#include "model/FeatureCollectionHandle.h"
#include "model/types.h"

#include "property-values/GeoTimeInstant.h"


namespace bp = boost::python;


namespace GPlatesApi
{
	namespace
	{
		/**
		 * The argument types for 'resolved topologies'.
		 */
		typedef boost::variant<
				// Export filename...
				QString,
				// List of 'ResolvedTopologicalLine's, 'ResolvedTopologicalBoundary's and 'ResolvedTopologicalNetwork's...
				bp::list>
						resolved_topologies_argument_type;

		/**
		 * The argument types for 'resolved topological sections'.
		 */
		typedef boost::variant<
				// Export filename...
				QString,
				// List of 'ResolvedTopologicalSection's...
				bp::list>
						resolved_topological_sections_argument_type;


		/**
		 * Retrieve the function arguments from the python 'resolve_topologies()' function.
		 */
		void
		get_resolve_topologies_args(
				bp::tuple positional_args,
				bp::dict keyword_args,
				boost::optional<TopologicalFeatureCollectionSequenceFunctionArgument> &topological_features,
				boost::optional<RotationModelFunctionArgument> &rotation_model,
				resolved_topologies_argument_type &resolved_topologies,
				GPlatesPropertyValues::GeoTimeInstant &reconstruction_time,
				boost::optional<resolved_topological_sections_argument_type> &resolved_topological_sections,
				boost::optional<GPlatesModel::integer_plate_id_type> &anchor_plate_id,
				boost::optional<ResolveTopologyParameters::non_null_ptr_to_const_type> default_resolve_topology_parameters,
				ResolveTopologyType::flags_type &resolve_topology_types,
				ResolveTopologyType::flags_type &resolve_topological_section_types,
				bool &export_wrap_to_dateline,
				boost::optional<GPlatesMaths::PolygonOrientation::Orientation> &export_force_boundary_orientation)
		{
			//
			// Get arguments for 'resolve_topologies().
			// If this fails then a python exception will be generated.
			//

			// The non-explicit function arguments.
			// These are our variable number of export parameters.
			VariableArguments::keyword_arguments_type unused_keyword_args;

			// Define the explicit function argument types...
			typedef boost::tuple<
					TopologicalFeatureCollectionSequenceFunctionArgument,
					RotationModelFunctionArgument,
					resolved_topologies_argument_type,
					GPlatesPropertyValues::GeoTimeInstant,
					boost::optional<resolved_topological_sections_argument_type>,
					boost::optional<GPlatesModel::integer_plate_id_type>,
					boost::optional<ResolveTopologyParameters::non_null_ptr_to_const_type>>
							resolve_topologies_args_type;

			// Define the explicit function argument names...
			const boost::tuple<const char *, const char *, const char *, const char *, const char *, const char *, const char *>
					explicit_arg_names(
							"topological_features",
							"rotation_model",
							"resolved_topologies",
							"reconstruction_time",
							"resolved_topological_sections",
							"anchor_plate_id",
							"default_resolve_topology_parameters");

			// Define the default function arguments...
			typedef boost::tuple<
					boost::optional<resolved_topological_sections_argument_type>,
					boost::optional<GPlatesModel::integer_plate_id_type>,
					boost::optional<ResolveTopologyParameters::non_null_ptr_to_const_type>>
							default_args_type;
			default_args_type defaults_args(boost::none, boost::none/*anchor_plate_id*/, boost::none/*default_resolve_topology_parameters*/);

			const resolve_topologies_args_type resolve_topologies_args =
					VariableArguments::get_explicit_args<resolve_topologies_args_type>(
							positional_args,
							keyword_args,
							explicit_arg_names,
							defaults_args,
							boost::none/*unused_positional_args*/,
							unused_keyword_args);

			topological_features = boost::get<0>(resolve_topologies_args);
			rotation_model = boost::get<1>(resolve_topologies_args);
			resolved_topologies = boost::get<2>(resolve_topologies_args);
			reconstruction_time = boost::get<3>(resolve_topologies_args);
			resolved_topological_sections = boost::get<4>(resolve_topologies_args);
			anchor_plate_id = boost::get<5>(resolve_topologies_args);
			default_resolve_topology_parameters = boost::get<6>(resolve_topologies_args);

			//
			// Get the optional non-explicit output parameters from the variable argument list.
			//

			resolve_topology_types =
					VariableArguments::extract_and_remove_or_default<ResolveTopologyType::flags_type>(
							unused_keyword_args,
							"resolve_topology_types",
							ResolveTopologyType::BOUNDARY | ResolveTopologyType::NETWORK);

			resolve_topological_section_types =
					VariableArguments::extract_and_remove_or_default<ResolveTopologyType::flags_type>(
							unused_keyword_args,
							"resolve_topological_section_types",
							// Defaults to the value of 'resolve_topology_types'...
							resolve_topology_types);

			export_wrap_to_dateline =
					VariableArguments::extract_and_remove_or_default<bool>(
							unused_keyword_args,
							"export_wrap_to_dateline",
							true);

			export_force_boundary_orientation =
					VariableArguments::extract_and_remove_or_default<
									boost::optional<GPlatesMaths::PolygonOrientation::Orientation> >(
							unused_keyword_args,
							"export_force_boundary_orientation",
							boost::none);

			// Raise a python error if there are any unused keyword arguments remaining.
			// These will be keywords that we didn't recognise.
			VariableArguments::raise_python_error_if_unused(unused_keyword_args);
		}
	}


	/**
	 * Resolve topological feature collections, optionally loaded from files, to a specific geological time
	 * and optionally export to file(s).
	 *
	 * The function signature enables us to use bp::raw_function to get variable keyword arguments and
	 * also more flexibility in function overloading.
	 *
	 * We must return a value (required by 'bp::raw_function') so we just return Py_None.
	 */
	bp::object
	resolve_topologies(
			bp::tuple positional_args,
			bp::dict keyword_args)
	{
		//
		// Get the explicit function arguments from the variable argument list.
		//

		boost::optional<TopologicalFeatureCollectionSequenceFunctionArgument> topological_features_argument;
		boost::optional<RotationModelFunctionArgument> rotation_model_argument;
		resolved_topologies_argument_type resolved_topologies_argument;
		GPlatesPropertyValues::GeoTimeInstant reconstruction_time(0);
		boost::optional<resolved_topological_sections_argument_type> resolved_topological_sections_argument;
		boost::optional<GPlatesModel::integer_plate_id_type> anchor_plate_id;
		boost::optional<ResolveTopologyParameters::non_null_ptr_to_const_type> default_resolve_topology_parameters;
		ResolveTopologyType::flags_type resolve_topology_types;
		ResolveTopologyType::flags_type resolve_topological_section_types;
		bool export_wrap_to_dateline;
		boost::optional<GPlatesMaths::PolygonOrientation::Orientation> export_force_boundary_orientation;

		get_resolve_topologies_args(
				positional_args,
				keyword_args,
				topological_features_argument,
				rotation_model_argument,
				resolved_topologies_argument,
				reconstruction_time,
				resolved_topological_sections_argument,
				anchor_plate_id,
				default_resolve_topology_parameters,
				resolve_topology_types,
				resolve_topological_section_types,
				export_wrap_to_dateline,
				export_force_boundary_orientation);

		// Time must not be distant past/future.
		if (!reconstruction_time.is_real())
		{
			PyErr_SetString(PyExc_ValueError,
				"Time values cannot be distant-past (float('inf')) or distant-future (float('-inf')).");
			bp::throw_error_already_set();
		}

		// Resolved topology type flags must correspond to existing flags.
		if ((resolve_topology_types & ~ResolveTopologyType::ALL_RESOLVE_TOPOLOGY_TYPES) != 0)
		{
			PyErr_SetString(PyExc_ValueError, "Unknown bit flag specified in resolve topology types.");
			bp::throw_error_already_set();
		}

		//
		// Resolve the topologies (as a topological snapshot).
		//

		TopologicalSnapshot::non_null_ptr_type topological_snapshot = TopologicalSnapshot::create(
				topological_features_argument.get(),
				rotation_model_argument.get(),
				reconstruction_time.value(),
				anchor_plate_id,
				default_resolve_topology_parameters);

		//
		// Either export the resolved topologies to a file or append them to a python list.
		//

		if (const QString *resolved_topologies_export_file_name =
			boost::get<QString>(&resolved_topologies_argument))
		{
			// Export resolved topologies.
			topological_snapshot->export_resolved_topologies(
					*resolved_topologies_export_file_name,
					resolve_topology_types,
					export_wrap_to_dateline,
					export_force_boundary_orientation);
		}
		else // list of resolved topologies...
		{
			// Gather all the resolved topologies to output (limited to the resolve types requested).
			std::vector<GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_type> resolved_topologies =
					topological_snapshot->get_resolved_topologies(
							resolve_topology_types,
							// Sort the resolved topologies in the order of the features in the
							// topological files (and the order across files) since we promise this in the docs...
							true/*same_order_as_topological_features*/);

			// The caller's Python list.
			bp::list output_resolved_topologies_list = boost::get<bp::list>(resolved_topologies_argument);

			// Add to the list.
			for (auto resolved_topology : resolved_topologies)
			{
				output_resolved_topologies_list.append(resolved_topology);
			}
		}

		if (resolved_topological_sections_argument)
		{
			//
			// Either export the resolved topological sections to a file or append them to a python list.
			//

			if (const QString *resolved_topological_sections_export_file_name =
				boost::get<QString>(&resolved_topological_sections_argument.get()))
			{
				// Export resolved topological sections.
				topological_snapshot->export_resolved_topological_sections(
						*resolved_topological_sections_export_file_name,
						resolve_topological_section_types,
						export_wrap_to_dateline);
			}
			else // list of resolved topologies...
			{
				// Gather all the resolved topological sections to output (limited to the resolve types requested).
				std::vector<GPlatesAppLogic::ResolvedTopologicalSection::non_null_ptr_type> resolved_topological_sections =
						topological_snapshot->get_resolved_topological_sections(
								resolve_topological_section_types,
								// Sort the resolved topological sections in the order of the features in the
								// topological files (and the order across files) since we promise this in the docs...
								true/*same_order_as_topological_features*/);

				// The caller's Python list.
				bp::list output_resolved_topological_sections_list = boost::get<bp::list>(resolved_topological_sections_argument.get());

				// Add to the list.
				for (auto resolved_topological_section : resolved_topological_sections)
				{
					output_resolved_topological_sections_list.append(resolved_topological_section);
				}
			}
		}

		// We must return a value (required by 'bp::raw_function') so just return Py_None.
		return bp::object();
	}
}

	
void
export_resolve_topologies()
{
	const char *resolve_topologies_function_name = "resolve_topologies";
	bp::def(resolve_topologies_function_name, bp::raw_function(&GPlatesApi::resolve_topologies));

	// Seems we cannot set a docstring using non-class bp::def combined with bp::raw_function,
	// or any case where the second argument of bp::def is a bp::object,
	// so we set the docstring the old-fashioned way.
	bp::scope().attr(resolve_topologies_function_name).attr("__doc__") =
			"resolve_topologies(topological_features, rotation_model, resolved_topologies, "
			"reconstruction_time, [resolved_topological_sections], [anchor_plate_id], [default_resolve_topology_parameters], [**output_parameters])\n"
			"  Resolve topological features (lines, boundaries and networks) to a specific geological time.\n"
			"\n"
			"  :param topological_features: The topological boundary and network features and the "
			"topological section features they reference (regular and topological lines) as a feature collection, "
			"or filename, or feature, or sequence of features, or a sequence (eg, ``list`` or ``tuple``) "
			"of any combination of those four types. Note: Each sequence entry can optionally be a 2-tuple "
			"(entry, :class:`ResolveTopologyParameters`) to override *default_resolve_topology_parameters* for that entry.\n"
			"  :type topological_features: :class:`FeatureCollection`, or string, or :class:`Feature`, "
			"or sequence of :class:`Feature`, or sequence of any combination of those four types\n"
			"  :param rotation_model: A rotation model or a rotation feature collection or a rotation "
			"filename or a sequence of rotation feature collections and/or rotation filenames\n"
			"  :type rotation_model: :class:`RotationModel` or :class:`FeatureCollection` or string "
			"or sequence of :class:`FeatureCollection` instances and/or strings\n"
			"  :param resolved_topologies: the "
			":class:`resolved topological lines<ResolvedTopologicalLine>`, "
			":class:`resolved topological boundaries<ResolvedTopologicalBoundary>` and "
			":class:`resolved topological networks<ResolvedTopologicalNetwork>` (depending on the optional "
			"keyword argument *resolve_topology_types* - see *output_parameters* table) are either exported "
			"to a file (with specified filename) or *appended* to a python ``list`` (note that the list is "
			"*not* cleared first)\n"
			"  :type resolved_topologies: string or ``list``\n"
			"  :param reconstruction_time: the specific geological time to resolve to\n"
			"  :type reconstruction_time: float or :class:`GeoTimeInstant`\n"
			"  :param resolved_topological_sections: The :class:`resolved topological sections<ResolvedTopologicalSection>` "
			" are either exported to a file (with specified filename) or *appended* to a python ``list`` "
			"(note that the list is *not* cleared first). Default is to do neither.\n"
			"  :type resolved_topological_sections: string or ``list``\n"
			"  :param anchor_plate_id: The anchored plate id used during reconstruction. "
			"Defaults to the default anchor plate of *rotation_model*.\n"
			"  :type anchor_plate_id: int\n"
			"  :param default_resolve_topology_parameters: Default parameters used to resolve topologies. "
			"Note that these can optionally be overridden in *topological_features*. "
			"Defaults to :meth:`default-constructed ResolveTopologyParameters<ResolveTopologyParameters.__init__>`).\n"
			"  :type default_resolve_topology_parameters: :class:`ResolveTopologyParameters`\n"
			"  :param output_parameters: Variable number of keyword arguments specifying output "
			"parameters (see table below). Default is no keyword arguments.\n"
			"  :raises: OpenFileForReadingError if any input file is not readable (when filenames specified)\n"
			"  :raises: OpenFileForWritingError if *resolved_topologies* is a filename and it is not writeable\n"
			"  :raises: FileFormatNotSupportedError if any input file format (identified by any "
			"topological and rotation filename extensions) does not support reading "
			"(when filenames specified)\n"
			"  :raises: ValueError if *reconstruction_time* is "
			":meth:`distant past<GeoTimeInstant.is_distant_past>` or "
			":meth:`distant future<GeoTimeInstant.is_distant_future>`\n"
			"  :raises: ValueError if *resolve_topology_types* (if specified) contains a flag that "
			"is not one of ``pygplates.ResolveTopologyType.line``, ``pygplates.ResolveTopologyType.boundary`` or "
			"``pygplates.ResolveTopologyType.network``\n"
			"  :raises: ValueError if *resolve_topological_section_types* (if specified) contains a flag that "
			"is not one of ``pygplates.ResolveTopologyType.boundary`` or ``pygplates.ResolveTopologyType.network``\n"
			"\n"
			"  The following optional keyword arguments are supported by *output_parameters*:\n"
			"\n"
			"  +-----------------------------------+------+-----------------------------------------------------------------+----------------------------------------------------------------------------------+\n"
			"  | Name                              | Type | Default                                                         | Description                                                                      |\n"
			"  +===================================+======+=================================================================+==================================================================================+\n"
			"  | resolve_topology_types            | int  | ``ResolveTopologyType.boundary | ResolveTopologyType.network``  | A bitwise combination of any of the following:                                   |\n"
			"  |                                   |      |                                                                 |                                                                                  |\n"
			"  |                                   |      |                                                                 | - ``ResolveTopologyType.line``:                                                  |\n"
			"  |                                   |      |                                                                 | - ``ResolveTopologyType.boundary``:                                              |\n"
			"  |                                   |      |                                                                 | - ``ResolveTopologyType.network``:                                               |\n"
			"  |                                   |      |                                                                 |                                                                                  |\n"
			"  |                                   |      |                                                                 | Determines whether to output :class:`ResolvedTopologicalLine`,                   |\n"
			"  |                                   |      |                                                                 | :class:`ResolvedTopologicalBoundary` and :class:`ResolvedTopologicalNetwork`.    |\n"
			"  +-----------------------------------+------+-----------------------------------------------------------------+----------------------------------------------------------------------------------+\n"
			"  | resolve_topological_section_types | int  | Same value as *resolve_topology_types* (if specified),          | A bitwise combination of any of the following:                                   |\n"
			"  |                                   |      | otherwise its default value                                     |                                                                                  |\n"
			"  |                                   |      | ``ResolveTopologyType.boundary | ResolveTopologyType.network``  | - ``ResolveTopologyType.boundary``:                                              |\n"
			"  |                                   |      |                                                                 | - ``ResolveTopologyType.network``:                                               |\n"
			"  |                                   |      |                                                                 |                                                                                  |\n"
			"  |                                   |      |                                                                 | .. note:: ``ResolveTopologyType.line`` is excluded since only                    |\n"
			"  |                                   |      |                                                                 |    topologies with boundaries are considered.                                    |\n"
			"  |                                   |      |                                                                 |                                                                                  |\n"
			"  |                                   |      |                                                                 | Determines whether :class:`ResolvedTopologicalBoundary` or                       |\n"
			"  |                                   |      |                                                                 | :class:`ResolvedTopologicalNetwork` (or both types) are referenced in the        |\n"
			"  |                                   |      |                                                                 | :class:`resolved topological sections<pygplates.ResolvedTopologicalSection>`     |\n"
			"  |                                   |      |                                                                 | of *resolved_topological_sections*.                                              |\n"
			"  +-----------------------------------+------+-----------------------------------------------------------------+----------------------------------------------------------------------------------+\n"
			"  | export_wrap_to_dateline           | bool | True                                                            | | Wrap/clip resolved topologies to the dateline (currently                       |\n"
			"  |                                   |      |                                                                 |   ignored unless exporting to an ESRI Shapefile format *file*).                  |\n"
			"  |                                   |      |                                                                 |                                                                                  |\n"
			"  |                                   |      |                                                                 | .. note:: Only applies when exporting to a file (ESRI Shapefile).                |\n"
			"  +-----------------------------------+------+-----------------------------------------------------------------+----------------------------------------------------------------------------------+\n"
			"  | export_force_boundary_orientation | int  | ``None`` (don't force)                                          | Optionally force boundary orientation (clockwise or counter-clockwise):          |\n"
			"  |                                   |      |                                                                 |                                                                                  |\n"
			"  |                                   |      |                                                                 | - ``PolygonOnSphere.Orientation.clockwise``                                      |\n"
			"  |                                   |      |                                                                 | - ``PolygonOnSphere.Orientation.counter_clockwise``                              |\n"
			"  |                                   |      |                                                                 |                                                                                  |\n"
			"  |                                   |      |                                                                 | .. note:: Only applies to resolved topological *boundaries* and *networks*.      |\n"
			"  |                                   |      |                                                                 |                                                                                  |\n"
			"  |                                   |      |                                                                 | .. note:: ESRI Shapefiles always use *clockwise* orientation.                    |\n"
			"  |                                   |      |                                                                 |                                                                                  |\n"
			"  |                                   |      |                                                                 | .. warning:: Only applies when exporting to a **file** (except ESRI Shapefile).  |\n"
			"  +-----------------------------------+------+-----------------------------------------------------------------+----------------------------------------------------------------------------------+\n"
			"\n"
			"  | The argument *topological_features* consists of the *topological* :class:`features<Feature>` "
			"as well as the topological sections (also :class:`features<Feature>`) that are referenced by the "
			"*topological* features.\n"
			"  | They can all be mixed in a single :class:`feature collection<FeatureCollection>` or file, "
			"or they can be distributed across multiple :class:`feature collections<FeatureCollection>` or files.\n"
			"  | For example the dynamic polygons in the `GPlates sample data <http://www.gplates.org/download.html#download_data>`_ "
			"have everything in a single file.\n"
			"\n"
			"  .. note:: Topological *sections* can be regular features or topological *line* features. "
			"The latter are typically used for sections of a plate polygon (or network) boundary that are deforming.\n"
			"\n"
			"  | The argument *resolved_topologies* can be either an export filename or a python ``list``.\n"
			"  | In the latter case the resolved topologies generated by the reconstruction are appended "
			"to the python ``list`` (instead of exported to a file).\n"
			"\n"
			"  | A similar argument *resolved_topological_sections* can also be either an export filename or a python ``list``.\n"
			"  | In the latter case the :class:`resolved topological sections<pygplates.ResolvedTopologicalSection>` "
			"generated by the reconstruction are appended to the python ``list`` (instead of exported to a file).\n"
			"\n"
			"  | Both *resolved_topologies* and *resolved_topological_sections* are output in the same order as that of their "
			"respective features in *topological_features* (the order across feature collections is also retained). "
			"This happens regardless of whether *topological_features*, and *resolved_topologies* and *resolved_topological_sections*, "
			"include files or not.\n"
			"\n"
			"  .. note:: | :class:`Resolved topological sections<pygplates.ResolvedTopologicalSection>` can be used "
			"to find the unique (non-overlapping) set of boundary sub-segments that are shared by the resolved topologies.\n"
			"            | Each resolved topology also has a list of its boundary sub-segments but they overlap with the "
			"boundary sub-segments of neighbouring topologies.\n"
			"\n"
			"  | The optional keyword argument *resolve_topology_types* (see *output_parameters* table) determines "
			"the type of resolved topologies output to *resolved_topologies*.\n"
			"  | This can consist of "
			":class:`resolved topological lines<ResolvedTopologicalLine>`, "
			":class:`resolved topological boundaries<ResolvedTopologicalBoundary>` and "
			":class:`resolved topological networks<ResolvedTopologicalNetwork>` (and any combination of them).\n"
			"  | By default only "
			":class:`resolved topological boundaries<ResolvedTopologicalBoundary>` and "
			":class:`resolved topological networks<ResolvedTopologicalNetwork>` are output since "
			":class:`resolved topological lines<ResolvedTopologicalLine>` are typically only used as "
			"topological sections for resolved topological boundaries and networks.\n"
			"\n"
			"  | A similar optional keyword argument is *resolve_topological_section_types* (see *output_parameters* table).\n"
			"  | This determines which resolved topology types are referenced in the "
			":meth:`shared sub-segments<ResolvedTopologicalSharedSubSegment.get_sharing_resolved_topologies>` "
			"of the *resolved_topological_sections*.\n"
			"\n"
			"  The following *export* file formats are currently supported by GPlates:\n"
			"\n"
			"  =============================== =======================\n"
			"  Export File Format              Filename Extension     \n"
			"  =============================== =======================\n"
			"  ESRI Shapefile                  '.shp'                 \n"
			"  OGR GMT                         '.gmt'                 \n"
			"  GMT xy                          '.xy'                  \n"
			"  =============================== =======================\n"
			"\n"
			"  .. warning:: | Currently, resolved topological **networks** exported to *OGR GMT* or "
			"*ESRI Shapefile* will not be loaded if the exported file is subsequently loaded into "
			"`GPlates <http://www.gplates.org>`_.\n"
			"               | The resolved topological networks will still be in the exported file though.\n"
			"\n"
			"  .. note:: When exporting to a file, the filename extension of *resolved_topologies* "
			"determines the export file format.\n"
			"\n"
			"  .. note:: *topological_features* can be a :class:`FeatureCollection` or a filename "
			"or a :class:`Feature` or a sequence of :class:`features<Feature>`, or a sequence (eg, ``list`` "
			"or ``tuple``) of any combination of those four types.\n"
			"\n"
			"  .. note:: *rotation_model* can be either a :class:`RotationModel` or a "
			"rotation :class:`FeatureCollection` or a rotation filename or a sequence "
			"(eg, ``list`` or ``tuple``) containing rotation :class:`FeatureCollection` instances "
			"or filenames (or a mixture of both). When a :class:`RotationModel` is not specified "
			"then a temporary one is created internally (and hence is less efficient if this "
			"function is called multiple times with the same rotation data).\n"
			"\n"
			"  If any filenames are specified then :class:`FeatureCollectionFileFormatRegistry` is "
			"used internally to read feature collections from those files.\n"
			"\n"
			"  Resolving a file containing dynamic plate polygons to a shapefile at 10Ma:\n"
			"  ::\n"
			"\n"
            "    pygplates.resolve_topologies(\n"
			"        'dynamic_plate_polygons.gpml', 'rotations.rot', 'resolved_plate_polygons_10Ma.shp', 10)\n"
			"\n"
			"  | Resolving the same file but also exporting resolved topological sections.\n"
			"  | These are the unique (non-duplicated) segments (shared by neighbouring topology boundaries).\n"
			"\n"
			"  ::\n"
			"\n"
            "    pygplates.resolve_topologies(\n"
			"       'dynamic_plate_polygons.gpml', 'rotations.rot', 'resolved_plate_polygons_10Ma.shp', 10,\n"
			"       'resolved_plate_segments_10Ma.shp')\n"
			"\n"
			"  Resolving only topological networks in a file containing both dynamic plate polygons and deforming networks:\n"
			"  ::\n"
			"\n"
            "    pygplates.resolve_topologies(\n"
			"        'plate_polygons_and_networks.gpml', 'rotations.rot', 'resolved_networks_10Ma.shp', 10,\n"
			"        resolve_topology_types=pygplates.ResolveTopologyType.network)\n"
			"\n"
			"  Writing only resolved networks to ``resolved_networks_10Ma.shp`` but writing shared boundary segments "
			"between resolved plate polygons *and* networks to ``resolved_boundary_segments_10Ma.shp``:\n"
			"  ::\n"
			"\n"
            "    pygplates.resolve_topologies(\n"
			"        'plate_polygons_and_networks.gpml', 'rotations.rot', 'resolved_networks_10Ma.shp', 10,\n"
			"        'resolved_boundary_segments_10Ma.shp',\n"
			"        resolve_topology_types=pygplates.ResolveTopologyType.network,\n"
			"        resolve_topological_section_types=pygplates.ResolveTopologyType.boundary | pygplates.ResolveTopologyType.network)\n"
			"\n"
			"  Resolving to a list of topologies and a list of topological sections:\n"
			"  ::\n"
			"\n"
			"    resolved_topologies = []\n"
			"    resolved_topological_sections = []\n"
            "    pygplates.resolve_topologies(\n"
			"        'plate_polygons_and_networks.gpml', 'rotations.rot', resolved_topologies, 10,\n"
			"         resolved_topological_sections)\n"
			"\n"
			"  .. versionchanged:: 29\n"
			"     The output order of *resolved_topological_sections* is now same as that of their "
			"respective features in *topological_features* (the order across feature collections is also retained). "
			"Previously the order was only retained for *resolved_topologies*.\n"
			"\n"
			"  .. versionchanged:: 31\n"
			"     Added *default_resolve_topology_parameters* argument.\n";

	// Register 'resolved topologies' variant.
	GPlatesApi::PythonConverterUtils::register_variant_conversion<GPlatesApi::resolved_topologies_argument_type>();

	// Register 'resolved topological sections' variant.
	GPlatesApi::PythonConverterUtils::register_variant_conversion<GPlatesApi::resolved_topological_sections_argument_type>();
	// Enable boost::optional<resolved_topological_sections_argument_type> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::register_optional_conversion<GPlatesApi::resolved_topological_sections_argument_type>();
}
