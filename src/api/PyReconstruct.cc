/* $Id:  $ */

/**
 * \file 
 * $Revision: 7584 $
 * $Date: 2010-02-10 19:29:36 +1100 (Wed, 10 Feb 2010) $
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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

#if defined(__APPLE__)
//On mac, this header file must be included here to workaround a boost python bug.
//If you don't do this, you will get strange error messages when compiling on mac.
#include <python.h> 
#endif

#include <utility>
#include <vector>
#include <boost/foreach.hpp>
#include <boost/optional.hpp>
#include <QString>

#include "PyFeatureCollection.h"
#include "PyRotationModel.h"
#include "PythonConverterUtils.h"
#include "PythonUtils.h"
#include "PythonVariableFunctionArguments.h"

#include "app-logic/ReconstructMethodRegistry.h"
#include "app-logic/ReconstructUtils.h"

#include "feature-visitors/GeometrySetter.h"

#include "file-io/FeatureCollectionFileFormatRegistry.h"
#include "file-io/File.h"
#include "file-io/ReadErrorAccumulation.h"
#include "file-io/ReconstructedFeatureGeometryExport.h"

#include "global/python.h"
// This is not included by <boost/python.hpp>.
// Also we must include this after <boost/python.hpp> which means after "global/python.h".
#include <boost/python/raw_function.hpp>

#include "model/FeatureCollectionHandle.h"
#include "model/types.h"


#if !defined(GPLATES_NO_PYTHON)

namespace bp = boost::python;


namespace GPlatesApi
{
	namespace
	{
		/**
		 * Retrieve the function arguments from the *deprecated* python 'reconstruct()' function.
		 *
		 * This version of the 'reconstruct()' function is not documented. However we still
		 * support it since it was one of the few python API functions that's been around since
		 * the dawn of time and is currently used in some web applications.
		 *
		 * Returns true if this version of the 'reconstruct()' function was detected via the
		 * specified positional and keyword arguments.
		 */
		bool
		get_deprecated_reconstruct_args(
				bp::tuple positional_args,
				bp::dict keyword_args,
				std::vector<GPlatesFileIO::File::non_null_ptr_type> &reconstructable_files,
				boost::optional<RotationModel::non_null_ptr_type> &rotation_model,
				QString &export_file_name,
				double &reconstruction_time,
				GPlatesModel::integer_plate_id_type &anchor_plate_id,
				bool &export_wrap_to_dateline)
		{
			// Define the explicit function argument types...
			//
			// We're actually more generous than the original (deprecated) function since the original
			// only allowed a python 'list' of filenames (for reconstructable and rotation features).
			typedef boost::tuple<
					FeatureCollectionSequenceFunctionArgument,
					RotationModelFunctionArgument,
					double,
					GPlatesModel::integer_plate_id_type,
					QString>
							reconstruct_args_type;

			// Define the explicit function argument names...
			const boost::tuple<const char *, const char *, const char *, const char *, const char *>
					explicit_arg_names(
							"recon_files",
							"rot_files",
							"time",
							"anchor_plate_id",
							"export_file_name");

			// Define the default function arguments...
			typedef boost::tuple<> default_args_type;
			default_args_type defaults_args;

			// If this deprecated version of 'reconstruct()' does not match the actual function
			// arguments then return false.
			if (!VariableArguments::check_explicit_args<reconstruct_args_type>(
				positional_args,
				keyword_args,
				explicit_arg_names,
				defaults_args,
				boost::none/*unused_positional_args*/,
				boost::none/*unused_keyword_args*/))
			{
				return false;
			}

			const reconstruct_args_type reconstruct_args =
					VariableArguments::get_explicit_args<reconstruct_args_type>(
							positional_args,
							keyword_args,
							explicit_arg_names,
							defaults_args,
							boost::none/*unused_positional_args*/,
							boost::none/*unused_keyword_args*/);

			boost::get<0>(reconstruct_args).get_files(reconstructable_files);
			rotation_model = boost::get<1>(reconstruct_args).get_rotation_model();
			reconstruction_time = boost::get<2>(reconstruct_args);
			anchor_plate_id = boost::get<3>(reconstruct_args);
			export_file_name = boost::get<4>(reconstruct_args);

			// Parameters not available in deprecated function - so set to default values...
			export_wrap_to_dateline = true;

			return true;
		}


		/**
		 * Retrieve the function arguments from the python 'reconstruct()' function.
		 */
		void
		get_reconstruct_args(
				bp::tuple positional_args,
				bp::dict keyword_args,
				std::vector<GPlatesFileIO::File::non_null_ptr_type> &reconstructable_files,
				boost::optional<RotationModel::non_null_ptr_type> &rotation_model,
				QString &export_file_name,
				double &reconstruction_time,
				GPlatesModel::integer_plate_id_type &anchor_plate_id,
				bool &export_wrap_to_dateline)
		{
			// First attempt to get arguments from deprecated version of 'reconstruct()'.
			if (get_deprecated_reconstruct_args(
					positional_args,
					keyword_args,
					reconstructable_files,
					rotation_model,
					export_file_name,
					reconstruction_time,
					anchor_plate_id,
					export_wrap_to_dateline))
			{
				// Successfully obtained arguments from deprecated version of 'reconstruct()'.
				return;
			}

			//
			// Now get arguments from official version of 'reconstruct().
			// If this fails then a python exception will be generated.
			//

			// The non-explicit function arguments.
			// These are our variable number of export parameters.
			VariableArguments::keyword_arguments_type unused_keyword_args;

			// Define the explicit function argument types...
			typedef boost::tuple<
					FeatureCollectionSequenceFunctionArgument,
					RotationModelFunctionArgument,
					QString,
					double,
					GPlatesModel::integer_plate_id_type>
							reconstruct_args_type;

			// Define the explicit function argument names...
			const boost::tuple<const char *, const char *, const char *, const char *, const char *>
					explicit_arg_names(
							"reconstructable_features",
							"rotation_model",
							"reconstructed_feature_geometries",
							"reconstruction_time",
							"anchor_plate_id");

			// Define the default function arguments...
			typedef boost::tuple<GPlatesModel::integer_plate_id_type> default_args_type;
			default_args_type defaults_args(0/*anchor_plate_id*/);

			const reconstruct_args_type reconstruct_args =
					VariableArguments::get_explicit_args<reconstruct_args_type>(
							positional_args,
							keyword_args,
							explicit_arg_names,
							defaults_args,
							boost::none/*unused_positional_args*/,
							unused_keyword_args);

			boost::get<0>(reconstruct_args).get_files(reconstructable_files);
			rotation_model = boost::get<1>(reconstruct_args).get_rotation_model();
			export_file_name = boost::get<2>(reconstruct_args);
			reconstruction_time = boost::get<3>(reconstruct_args);
			anchor_plate_id = boost::get<4>(reconstruct_args);

			//
			// Get the optional non-explicit output parameters from the variable argument list.
			//

			export_wrap_to_dateline =
					VariableArguments::extract_and_remove_or_default<bool>(
							unused_keyword_args,
							"export_wrap_to_dateline",
							true);

			// Raise a python error if there are any unused keyword arguments remaining.
			// These will be keywords that we didn't recognise.
			VariableArguments::raise_python_error_if_unused(unused_keyword_args);
		}


		GPlatesFileIO::ReconstructedFeatureGeometryExport::Format
		get_format(
				QString file_name)
		{
			static const QString GMT_EXT = ".xy";
			static const QString SHP_EXT = ".shp";
			static const QString OGRGMT_EXT = ".gmt";

			if (file_name.endsWith(GMT_EXT))
			{
				return GPlatesFileIO::ReconstructedFeatureGeometryExport::GMT;
			}
			if (file_name.endsWith(SHP_EXT))
			{
				return GPlatesFileIO::ReconstructedFeatureGeometryExport::SHAPEFILE;
			}
			if (file_name.endsWith(OGRGMT_EXT))
			{
				return GPlatesFileIO::ReconstructedFeatureGeometryExport::OGRGMT;
			}

			return GPlatesFileIO::ReconstructedFeatureGeometryExport::UNKNOWN;
		}
	}


	/**
	 * Reconstruct feature collections, optionally loaded from files, to a specific geological time and export to file(s).
	 *
	 * The function signature enables us to use bp::raw_function to get variable keyword arguments and
	 * also more flexibility in function overloading.
	 *
	 * We must return a value (required by 'bp::raw_function') so we just return Py_None.
	 */
	bp::object
	reconstruct(
			bp::tuple positional_args,
			bp::dict keyword_args)
	{
		//
		// Get the explicit function arguments from the variable argument list.
		//

		std::vector<GPlatesFileIO::File::non_null_ptr_type> reconstructable_files;
		boost::optional<RotationModel::non_null_ptr_type> rotation_model;
		QString export_file_name;
		double reconstruction_time;
		GPlatesModel::integer_plate_id_type anchor_plate_id;
		bool export_wrap_to_dateline;

		get_reconstruct_args(
				positional_args,
				keyword_args,
				reconstructable_files,
				rotation_model,
				export_file_name,
				reconstruction_time,
				anchor_plate_id,
				export_wrap_to_dateline);

		// Extract reconstructable feature collection weak refs from their files.
		std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> reconstructable_feature_collections;
		BOOST_FOREACH(GPlatesFileIO::File::non_null_ptr_type reconstruct_file, reconstructable_files)
		{
			reconstructable_feature_collections.push_back(
					reconstruct_file->get_reference().get_feature_collection());
		}

		// Adapt the reconstruction tree creator to a new one that has 'anchor_plate_id' as its default.
		// This ensures 'ReconstructUtils::reconstruct()' will reconstruct using the correct anchor plate.
		GPlatesAppLogic::ReconstructionTreeCreator reconstruction_tree_creator =
				GPlatesAppLogic::create_cached_reconstruction_tree_adaptor(
						rotation_model.get()->get_reconstruction_tree_creator(),
						anchor_plate_id);

		// Reconstruct.
		std::vector<GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_type> rfgs;
		GPlatesAppLogic::ReconstructMethodRegistry reconstruct_method_registry;
		GPlatesAppLogic::ReconstructUtils::reconstruct(
				rfgs,
				reconstruction_time,
				reconstruct_method_registry,
				reconstructable_feature_collections,
				reconstruction_tree_creator);

		// Converts to raw pointers.
		std::vector<const GPlatesAppLogic::ReconstructedFeatureGeometry *> rfgs_p;
		rfgs_p.reserve(rfgs.size());
		BOOST_FOREACH(
				const GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_type &rfg,
				rfgs)
		{
			rfgs_p.push_back(rfg.get());
		}

		// Get the sequence of reconstructable files as File pointers.
		std::vector<const GPlatesFileIO::File::Reference *> reconstructable_file_ptrs;
		BOOST_FOREACH(GPlatesFileIO::File::non_null_ptr_type reconstructable_file, reconstructable_files)
		{
			reconstructable_file_ptrs.push_back(&reconstructable_file->get_reference());
		}

		GPlatesFileIO::ReconstructedFeatureGeometryExport::Format format = get_format(export_file_name);

		// Export the reconstructed feature geometries.
		GPlatesFileIO::ReconstructedFeatureGeometryExport::export_reconstructed_feature_geometries(
					export_file_name,
					format,
					rfgs_p,
					reconstructable_file_ptrs,
					anchor_plate_id,
					reconstruction_time,
					// If exporting to Shapefile and there's only *one* input reconstructable file then
					// shapefile attributes in input reconstructable file will get copied to output...
					true/*export_single_output_file*/,
					false/*export_per_input_file*/, // We only generate a single output file.
					false/*export_output_directory_per_input_file*/, // We only generate a single output file.
					export_wrap_to_dateline);

		// We must return a value (required by 'bp::raw_function') so just return Py_None.
		return bp::object();
	}


	/**
	 * Loads a reconstructable feature collection (optionally from a file) @a reconstructable_features
	 * and assumes each feature geometry is *not* present day geometry but instead is the
	 * reconstructed geometry for the specified reconstruction time @a reconstruction_time.
	 *
	 * The reconstructed geometries of each reconstructable feature are reverse reconstructed to
	 * present day, stored back in the same features (and saved back out to the same file if the
	 * features were initially read from a file).
	 *
	 * @a reconstruction_time is the reconstruction_time representing the reconstructed geometries in each feature.
	 * @a rotation_model contains the rotation model (or reconstruction/rotation features) used to
	 * perform the reverse reconstruction.
	 */
	void
	reverse_reconstruct(
			FeatureCollectionFunctionArgument reconstructable_features,
			RotationModelFunctionArgument rotation_model,
			const double &reconstruction_time,
			GPlatesModel::integer_plate_id_type anchor_plate_id)
	{
		// Adapt the reconstruction tree creator to a new one that has 'anchor_plate_id' as its default.
		// This ensures we will reverse reconstruct using the correct anchor plate.
		GPlatesAppLogic::ReconstructionTreeCreator reconstruction_tree_creator =
				GPlatesAppLogic::create_cached_reconstruction_tree_adaptor(
						rotation_model.get_rotation_model()->get_reconstruction_tree_creator(),
						anchor_plate_id);

		// Create the context in which to reconstruct.
		const GPlatesAppLogic::ReconstructMethodInterface::Context reconstruct_method_context(
				GPlatesAppLogic::ReconstructParams(),
				reconstruction_tree_creator);

		GPlatesFileIO::File::non_null_ptr_type reconstructable_file = reconstructable_features.get_file();

		const GPlatesModel::FeatureCollectionHandle::weak_ref &reconstructable_feature_collection =
				reconstructable_file->get_reference().get_feature_collection();

		GPlatesAppLogic::ReconstructMethodRegistry reconstruct_method_registry;

		// Iterate over the features in the reconstructable feature collection.
		GPlatesModel::FeatureCollectionHandle::iterator reconstructable_features_iter =
				reconstructable_feature_collection->begin();
		GPlatesModel::FeatureCollectionHandle::iterator reconstructable_features_end =
				reconstructable_feature_collection->end();
		for ( ; reconstructable_features_iter != reconstructable_features_end; ++reconstructable_features_iter)
		{
			const GPlatesModel::FeatureHandle::weak_ref reconstructable_feature =
					(*reconstructable_features_iter)->reference();

			// Find out how to reconstruct each geometry in a feature based on the feature's other properties.
			// Get the reconstruct method so we can reverse reconstruct the geometry.
			GPlatesAppLogic::ReconstructMethodInterface::non_null_ptr_type reconstruct_method =
					reconstruct_method_registry.create_reconstruct_method_or_default(
							reconstructable_feature,
							reconstruct_method_context);

			// Get the (reconstructed - not present day) geometries for the current feature.
			//
			// NOTE: We are actually going to treat these geometries *not* as present day
			// but as geometries at time 'reconstruction_time' - we're going to reverse reconstruct to get
			// the present day geometries.
			// Note: There should be one geometry for each geometry property that can be reconstructed.
			std::vector<GPlatesAppLogic::ReconstructMethodInterface::Geometry> feature_reconstructed_geometries;
			reconstruct_method->get_present_day_feature_geometries(feature_reconstructed_geometries);

			// Iterate over the reconstructed geometries for the current feature.
			BOOST_FOREACH(
					const GPlatesAppLogic::ReconstructMethodInterface::Geometry &feature_reconstructed_geometry,
					feature_reconstructed_geometries)
			{
				// Reverse reconstruct the current feature geometry from time 'reconstruction_time' to present day.
				GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type present_day_geometry =
						reconstruct_method->reconstruct_geometry(
								feature_reconstructed_geometry.geometry,
								reconstruct_method_context,
								// The reconstruction_time of the reconstructed feature geometry...
								reconstruction_time/*reconstruction_time*/,
								true/*reverse_reconstruct*/);

				// Set the reverse reconstructed (present day) geometry back onto the feature's geometry property.
				GPlatesFeatureVisitors::GeometrySetter(present_day_geometry).set_geometry(
						(*feature_reconstructed_geometry.property_iterator).get());
			}
		}

		// If the feature collection came from a file (as opposed to being passed in directly as a
		// feature collection) then write the current modified feature collection back out to the
		// same file it came from.
		if (reconstructable_file->get_reference().get_file_info().get_qfileinfo().exists())
		{
			GPlatesFileIO::FeatureCollectionFileFormat::Registry file_registry;
			file_registry.write_feature_collection(reconstructable_file->get_reference());
		}
	}
}

	
void
export_reconstruct()
{
	const char *reconstruct_function_name = "reconstruct";
	bp::def(reconstruct_function_name, bp::raw_function(&GPlatesApi::reconstruct));

	// Seems we cannot set a docstring using non-class bp::def combined with bp::raw_function,
	// or any case where the second argument of bp::def is a bp::object,
	// so we set the docstring the old-fashioned way.
	bp::scope().attr(reconstruct_function_name).attr("__doc__") =
			"reconstruct(reconstructable_features, rotation_model, reconstructed_feature_geometries, "
			"reconstruction_time, [anchor_plate_id=0], \\*\\*output_parameters)\n"
			"  Reconstruct geological features to a specific geological time.\n"
			"\n"
			"  :param reconstructable_features: A reconstructable feature collection or a filename or a "
			"sequence of feature collections and/or filenames\n"
			"  :type reconstructable_features: :class:`FeatureCollection` or string or sequence of "
			":class:`FeatureCollection` instances and/or strings\n"
			"  :param rotation_model: A rotation model or a rotation feature collection or a rotation "
			"filename or a sequence of rotation feature collections and/or rotation filenames\n"
			"  :type rotation_model: :class:`RotationModel` or :class:`FeatureCollection` or string "
			"or sequence of :class:`FeatureCollection` instances and/or strings\n"
			"  :param reconstructed_feature_geometries: the name of the file to export to (see supported formats below)\n"
			"  :type reconstructed_feature_geometries: string\n"
			"  :param reconstruction_time: the specific geological time to reconstruct to\n"
			"  :type reconstruction_time: float\n"
			"  :param anchor_plate_id: the anchored plate id used during reconstruction\n"
			"  :type anchor_plate_id: int\n"
			"  :param output_parameters: variable number of keyword arguments specifying output "
			"parameters (see table below)\n"
			"  :raises: OpenFileForReadingError if any input file is not readable (when filenames specified)\n"
			"  :raises: FileFormatNotSupportedError if any input file format (identified by any "
			"reconstructable and rotation filename extensions) does not support reading "
			"(when filenames specified)\n"
			"\n"
			"  The following optional keyword arguments are supported by *output_parameters*:\n"
			"\n"
			"  ======================================= ===== ======== ==============\n"
			"  Name                                    Type  Default  Description\n"
			"  ======================================= ===== ======== ==============\n"
			"  export_wrap_to_dateline                 bool  True     Wrap/clip reconstructed "
			"geometries to the dateline (currently ignored unless exporting to ESRI Shapefile format).\n"
			"  ======================================= ===== ======== ==============\n"
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
			"  Note that the filename extension of *reconstructed_feature_geometries* determines "
			"the export file format.\n"
			"\n"
			"  Note that if the export format is ESRI Shapefile then the shapefile attributes from "
			"*reconstructable_features* will only be retained in the exported shapefile if there "
			"is a single reconstructable feature collection (where *reconstructable_features* is a "
			"single feature collection or file, or sequence containing a single feature collection "
			"or file). This is because shapefile attributes from multiple input feature collections are "
			"not easily combined into a single output shapefile (due to different attribute field names).\n"
			"\n"
			"  Note that *reconstructable_features* can be a :class:`FeatureCollection` or a "
			"filename or a sequence (eg, ``list`` or ``tuple``) containing :class:`FeatureCollection` "
			"instances or filenames (or a mixture of both).\n"
			"\n"
			"  Note that *rotation_model* can be either a :class:`RotationModel` or a "
			"rotation :class:`FeatureCollection` or a rotation filename or a sequence "
			"(eg, ``list`` or ``tuple``) containing rotation :class:`FeatureCollection` instances "
			"or filenames (or a mixture of both). When a :class:`RotationModel` is not specified "
			"then a temporary one is created internally (and hence is less efficient if this "
			"function is called multiple times with the same rotation data).\n"
			"\n"
			"  If any filenames are specified then :class:`FeatureCollectionFileFormatRegistry` is "
			"used internally to read feature collections from those files.\n";

	bp::def("reverse_reconstruct",
			&GPlatesApi::reverse_reconstruct,
			(bp::arg("reconstructable_features"),
				bp::arg("rotation_model"),
				bp::arg("reconstruction_time"),
				bp::arg("anchor_plate_id")=0),
			"reverse_reconstruct(reconstructable_features, rotation_model, reconstruction_time, [anchor_plate_id=0])\n"
			"  Reverse reconstruct geological features from a specific geological time.\n"
			"\n"
			"  :param reconstructable_features: A reconstructable feature collection or a filename "
			"(used as input and output)\n"
			"  :type reconstructable_features: :class:`FeatureCollection` or string\n"
			"  :param rotation_model: A rotation model or a rotation feature collection or a rotation "
			"filename or a sequence of rotation feature collections and/or rotation filenames\n"
			"  :type rotation_model: :class:`RotationModel` or :class:`FeatureCollection` or string "
			"or sequence of :class:`FeatureCollection` instances and/or strings\n"
			"  :param reconstruction_time: the specific geological time to reverse reconstruct from\n"
			"  :type reconstruction_time: float\n"
			"  :param anchor_plate_id: the anchored plate id used during reverse reconstruction\n"
			"  :type anchor_plate_id: int\n"
			"  :raises: OpenFileForReadingError if any input file is not readable (when filenames specified)\n"
			"  :raises: FileFormatNotSupportedError if any input file format (identified by any "
			"reconstructable and rotation filename extensions) does not support reading "
			"(when filenames specified)\n"
			"\n"
			"  The effect of this function is to replace the present day geometries in each feature with "
			"reverse reconstructed versions of those geometries. This assumes that the original geometries, "
			"stored in *reconstructable_features*, are not in fact present day geometries (as they "
			"normally should be) but instead the already-reconstructed geometries corresponding to "
			"geological time *reconstruction_time*. This function reverses that reconstruction process "
			"to ensure present day geometries are stored in the features.\n"
			"\n"
			"  Note that *reconstructable_features* can be a :class:`FeatureCollection` or a "
			"filename. If loaded from a file then the modified feature collection (containing reverse "
			"reconstructed geometries) is written back out to the same file.\n"
			"\n"
			"  Note that *rotation_model* can be either a :class:`RotationModel` or a "
			"rotation :class:`FeatureCollection` or a rotation filename or a sequence "
			"(eg, ``list`` or ``tuple``) containing rotation :class:`FeatureCollection` instances "
			"or filenames (or a mixture of both). When a :class:`RotationModel` is not specified "
			"then a temporary one is created internally (and hence is less efficient if this "
			"function is called multiple times with the same rotation data).\n"
			"\n"
			"  If any filenames are specified then :class:`FeatureCollectionFileFormatRegistry` is "
			"used internally to read feature collections from those files.\n");
}

#endif
