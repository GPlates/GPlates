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

#include <vector>
#include <boost/foreach.hpp>
#include <boost/optional.hpp>
#include <boost/variant.hpp>
#include <QString>

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
#include <boost/python/stl_iterator.hpp>

#include "model/FeatureCollectionHandle.h"
#include "model/types.h"


#if !defined(GPLATES_NO_PYTHON)

namespace bp = boost::python;


namespace GPlatesApi
{
	namespace
	{
		/**
		 * The reconstruct features are either a feature collection or a filename.
		 *
		 * For backwards compatibility (since the 'reconstruct()' has been around for so long)
		 * we also allow the possibility of a list of filenames. However this is not documented
		 * in the API documentation to discourage users from using it that way.
		 */
		typedef boost::variant<
				QString,
				GPlatesModel::FeatureCollectionHandle::non_null_ptr_type,
				bp::list> reconstructable_features_type;

		// The rotation features are either a RotationModel or a list of feature collections / filenames.
		typedef boost::variant<bp::object/*sequence*/, RotationModel::non_null_ptr_type> rotation_features_type;


		/**
		 * Extracts, or creates, a RotationModel from a python function argument.
		 */
		RotationModel::non_null_ptr_type
		get_rotation_model(
				const rotation_features_type &rotation_features)
		{
			// It's either a filename or a feature collection.
			if (const RotationModel::non_null_ptr_type *rotation_model =
				boost::get<RotationModel::non_null_ptr_type>(&rotation_features))
			{
				return *rotation_model;
			}

			return RotationModel::create(
					boost::get<bp::object>(rotation_features),
					// Use a reasonable size in case reconstructing features that require more than
					// one reconstruction tree/time such as mid-ocean ridges (multiple half-stages)...
					32/*reconstruction_tree_cache_size*/);
		}


		/**
		 * Returns python function argument as either a reconstructable feature collection, filename
		 * or (for undocumented backwards compatibility) a list of filenames.
		 */
		void
		get_reconstructable_features_collection(
				const reconstructable_features_type &reconstructable_features,
				std::vector<GPlatesFileIO::File::non_null_ptr_type> &reconstruct_files,
				std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &reconstructable_features_collection,
				GPlatesFileIO::ReadErrorAccumulation &read_errors)
		{
			// If it's a feature collection...
			if (const GPlatesModel::FeatureCollectionHandle::non_null_ptr_type *reconstruct_feature_collection =
				boost::get<GPlatesModel::FeatureCollectionHandle::non_null_ptr_type>(&reconstructable_features))
			{
				// Create a file with an empty filename - since we don't know if feature collection
				// came from a file or not.
				GPlatesFileIO::File::non_null_ptr_type file =
						GPlatesFileIO::File::create_file(
								GPlatesFileIO::FileInfo(),
								*reconstruct_feature_collection);

				reconstruct_files.push_back(file);
				reconstructable_features_collection.push_back(file->get_reference().get_feature_collection());
			}
			// If it's a filename...
			else if (const QString *reconstruct_filename = boost::get<QString>(&reconstructable_features))
			{
				GPlatesFileIO::FeatureCollectionFileFormat::Registry file_registry;

				// Create a file with an empty feature collection.
				GPlatesFileIO::File::non_null_ptr_type file =
						GPlatesFileIO::File::create_file(GPlatesFileIO::FileInfo(*reconstruct_filename));

				// Read new features from the file into the feature collection.
				file_registry.read_feature_collection(file->get_reference(), read_errors);

				reconstruct_files.push_back(file);
				reconstructable_features_collection.push_back(file->get_reference().get_feature_collection());
			}
			else
			{
				//
				// Undocumented, backwards compatible support for a list of filenames.
				//

				bp::list filenames = boost::get<bp::list>(reconstructable_features);

				GPlatesFileIO::FeatureCollectionFileFormat::Registry file_registry;

				// Read the feature collections from the files.
				// Begin/end iterators over the python filenames iterable.
				bp::stl_input_iterator<QString> filenames_iter(filenames);
				bp::stl_input_iterator<QString> filenames_end;
				for ( ; filenames_iter != filenames_end; ++filenames_iter)
				{
					const QString filename = *filenames_iter;

					// Create a file with an empty feature collection.
					GPlatesFileIO::File::non_null_ptr_type file =
							GPlatesFileIO::File::create_file(GPlatesFileIO::FileInfo(filename));

					// Read new features from the file into the feature collection.
					file_registry.read_feature_collection(file->get_reference(), read_errors);

					reconstruct_files.push_back(file);
					reconstructable_features_collection.push_back(file->get_reference().get_feature_collection());
				}
			}
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


		void
		read_feature_collection_files(
				std::vector<GPlatesFileIO::File::non_null_ptr_type> &files,
				std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &feature_collection_refs,
				bp::object filenames,
				const GPlatesFileIO::FeatureCollectionFileFormat::Registry &file_registry,
				GPlatesFileIO::ReadErrorAccumulation &read_errors)
		{
			// Read the feature collections from the files.
			// Begin/end iterators over the python filenames iterable.
			bp::stl_input_iterator<QString> filenames_iter(filenames);
			bp::stl_input_iterator<QString> filenames_end;
			for ( ; filenames_iter != filenames_end; ++filenames_iter)
			{
				const QString filename = *filenames_iter;

				// Create a file with an empty feature collection.
				GPlatesFileIO::File::non_null_ptr_type file =
						GPlatesFileIO::File::create_file(GPlatesFileIO::FileInfo(filename));

				// Read new features from the file into the feature collection.
				file_registry.read_feature_collection(file->get_reference(), read_errors);

				files.push_back(file);
				feature_collection_refs.push_back(file->get_reference().get_feature_collection());
			}
		}
	}


	/**
	 * Reconstruct feature collections, loaded from files, to a specific geological time and export to file(s).
	 *
	 * The function signature enables us to use bp::raw_function to get variable keyword arguments.
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

		// The non-explicit function arguments.
		// These are our variable number of export parameters.
		VariableArguments::keyword_arguments_type unused_keyword_args;

		reconstructable_features_type reconstructable_features;
		rotation_features_type rotation_features;
		QString export_file_name;
		double reconstruction_time;
		GPlatesModel::integer_plate_id_type anchor_plate_id;

		boost::tie(
				reconstructable_features,
				rotation_features,
				export_file_name,
				reconstruction_time,
				anchor_plate_id) =
						VariableArguments::get_explicit_args<
								// Define the number of explicit arguments and their types...
								boost::tuple<
										reconstructable_features_type,
										rotation_features_type,
										QString,
										double,
										GPlatesModel::integer_plate_id_type> >(
								positional_args,
								keyword_args,
								boost::make_tuple(
										"reconstructable_features",
										"rotation_features",
										"reconstructed_feature_geometries",
										"reconstruction_time",
										"anchor_plate_id"),
								boost::tuples::make_tuple(),
								boost::none/*unused_positional_args*/,
								unused_keyword_args);

		//
		// Get the optional non-explicit output parameters from the variable argument list.
		//

		bool export_wrap_to_dateline =
				VariableArguments::extract_and_remove_or_default<bool>(
						unused_keyword_args,
						"export_wrap_to_dateline",
						true);

		// Raise a python error if there are any unused keyword arguments remaining.
		// These will be keywords that we didn't recognise.
		VariableArguments::raise_python_error_if_unused(unused_keyword_args);

		// TODO: Return the read errors to the python caller.
		GPlatesFileIO::ReadErrorAccumulation read_errors;

		// Get the reconstructable feature collection(s).
		//
		// The 'File's keep the feature collections alive (since 'reconstructable_features_collection'
		// contains only weak references).
		std::vector<GPlatesFileIO::File::non_null_ptr_type> reconstruct_files;
		std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> reconstructable_features_collection;
		get_reconstructable_features_collection(
				reconstructable_features,
				reconstruct_files,
				reconstructable_features_collection,
				read_errors);

		// Extract, or create, the rotation model.
		const RotationModel::non_null_ptr_type rotation_model = get_rotation_model(rotation_features);

		// Adapt the reconstruction tree creator to a new one that has 'anchor_plate_id' as its default.
		// This ensures 'ReconstructUtils::reconstruct()' will reconstruct using the correct anchor plate.
		GPlatesAppLogic::ReconstructionTreeCreator reconstruction_tree_creator =
				GPlatesAppLogic::create_cached_reconstruction_tree_adaptor(
						rotation_model->get_reconstruction_tree_creator(),
						anchor_plate_id);

		// Reconstruct.
		std::vector<GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_type> rfgs;
		GPlatesAppLogic::ReconstructMethodRegistry reconstruct_method_registry;
		GPlatesAppLogic::ReconstructUtils::reconstruct(
				rfgs,
				reconstruction_time,
				reconstruct_method_registry,
				reconstructable_features_collection,
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
		std::vector<GPlatesFileIO::File::non_null_ptr_type>::const_iterator file_iter = reconstruct_files.begin();
		std::vector<GPlatesFileIO::File::non_null_ptr_type>::const_iterator file_end = reconstruct_files.end();
		for ( ; file_iter != file_end; ++file_iter)
		{
			reconstructable_file_ptrs.push_back(&(*file_iter)->get_reference());
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
					true/*export_single_output_file*/,
					false/*export_per_input_file*/,
					false/*export_output_directory_per_input_file*/,
					export_wrap_to_dateline);

		// We must return a value (required by 'bp::raw_function') so just return Py_None.
		return bp::object();
	}


	/**
	 * Loads reconstructable features from files @a python_reconstructable_filenames and assumes
	 * each feature geometry is *not* present day geometry but instead is the reconstructed geometry
	 * for the specified reconstruction_time @a reconstruction_time.
	 *
	 * The reconstructed geometries of each reconstructable feature are reverse reconstructed to
	 * present day, stored back in the features and saved to files (one output file per input
	 * reconstructable file) with...
	 *    <python_output_file_basename_suffix> + '.' + <python_output_file_format>
	 * ...appended to each corresponding input reconstructable file basename.
	 *
	 * @a reconstruction_time is the reconstruction_time representing the reconstructed geometries in each feature.
	 * @a python_reconstruction_filenames contains the reconstruction/rotation features used to
	 * perform the reverse reconstruction.
	 */
	void
	reverse_recontruct(
			bp::object reconstruct_filenames,
			bp::object rotation_filenames,
			const double &reconstruction_time,
			GPlatesModel::integer_plate_id_type anchor_plate_id,
			QString output_file_basename_suffix,
			QString output_file_format)
	{
		GPlatesFileIO::FeatureCollectionFileFormat::Registry file_registry;

		// TODO: Return the read errors to the python caller.
		GPlatesFileIO::ReadErrorAccumulation read_errors;

		// Read the feature collections from the reconstruct files.
		std::vector<GPlatesFileIO::File::non_null_ptr_type> reconstruct_files;
		std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> reconstruct_feature_collection_refs;
		read_feature_collection_files(
				reconstruct_files,
				reconstruct_feature_collection_refs,
				reconstruct_filenames,
				file_registry,
				read_errors);

		// Read the feature collections from the rotation files.
		std::vector<GPlatesFileIO::File::non_null_ptr_type> rotation_files;
		std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> rotation_feature_collection_refs;
		read_feature_collection_files(
				rotation_files,
				rotation_feature_collection_refs,
				rotation_filenames,
				file_registry,
				read_errors);

		GPlatesAppLogic::ReconstructMethodRegistry reconstruct_method_registry;

		const GPlatesAppLogic::ReconstructionTreeCreator reconstruction_tree_creator =
				GPlatesAppLogic::create_cached_reconstruction_tree_creator(
						rotation_feature_collection_refs,
						anchor_plate_id/*default_anchor_plate_id*/);

		// Create the context in which to reconstruct.
		const GPlatesAppLogic::ReconstructMethodInterface::Context reconstruct_method_context(
				GPlatesAppLogic::ReconstructParams(),
				reconstruction_tree_creator);

		// Iterate over the reconstruct files.
		bp::stl_input_iterator<QString> reconstruct_filenames_iter(reconstruct_filenames);
		bp::stl_input_iterator<QString> reconstruct_filenames_end;
		for (unsigned int reconstruct_file_index = 0;
			reconstruct_filenames_iter != reconstruct_filenames_end;
			++reconstruct_filenames_iter, ++reconstruct_file_index)
		{
			const GPlatesModel::FeatureCollectionHandle::weak_ref &reconstruct_feature_collection =
					reconstruct_feature_collection_refs[reconstruct_file_index];

			// Iterate over the features in the current feature collection.
			GPlatesModel::FeatureCollectionHandle::iterator reconstruct_features_iter =
					reconstruct_feature_collection->begin();
			GPlatesModel::FeatureCollectionHandle::iterator reconstruct_features_end =
					reconstruct_feature_collection->end();
			for ( ; reconstruct_features_iter != reconstruct_features_end; ++reconstruct_features_iter)
			{
				const GPlatesModel::FeatureHandle::weak_ref reconstruct_feature_ref =
						(*reconstruct_features_iter)->reference();

				// Find out how to reconstruct each geometry in a feature based on the feature's other properties.
				// Get the reconstruct method so we can reverse reconstruct the geometry.
				GPlatesAppLogic::ReconstructMethodInterface::non_null_ptr_type reconstruct_method =
						reconstruct_method_registry.create_reconstruct_method_or_default(
								reconstruct_feature_ref,
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
					GPlatesModel::TopLevelProperty::non_null_ptr_type geom_top_level_prop_clone = 
							(*feature_reconstructed_geometry.property_iterator)->clone();
					GPlatesFeatureVisitors::GeometrySetter(present_day_geometry).set_geometry(
							geom_top_level_prop_clone.get());
					*feature_reconstructed_geometry.property_iterator = geom_top_level_prop_clone;
				}
			}

			const QString reconstruct_input_filename = *reconstruct_filenames_iter;
			const QString reconstruct_input_file_basename =
					reconstruct_input_filename.left(reconstruct_input_filename.lastIndexOf('.'));
			const QString reconstruct_output_filename =
					reconstruct_input_file_basename + output_file_basename_suffix + '.' + output_file_format;

			// Create an output file to write back out the current modified feature collection.
			GPlatesFileIO::File::Reference::non_null_ptr_type output_file_ref =
					GPlatesFileIO::File::create_file_reference(
							GPlatesFileIO::FileInfo(reconstruct_output_filename),
							reconstruct_feature_collection);

			// Save the modified feature collection to file.
			file_registry.write_feature_collection(*output_file_ref);
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
			"reconstruct(reconstructable_features, rotation_features, reconstructed_feature_geometries, "
			"reconstruction_time, [anchor_plate_id=0], \\*\\*output_parameters)\n"
			"  Reconstruct geological features to a specific geological time.\n"
			"\n"
			"  :param reconstructable_features: The geological feature collection, or filename string "
			"containing a feature collection, to reconstruct\n"
			"  :type reconstructable_features: A :class:`FeatureCollection` or a string\n"
			"  :param rotation_features: A rotation model or a sequence of rotation feature collections "
			"and/or rotation filenames\n"
			"  :type rotation_features: A :class:`RotationModel` or a sequence containing "
			":class:`FeatureCollection` instances and/or strings\n"
			"  :param reconstructed_feature_geometries: the name of the file to export to (see supported formats below)\n"
			"  :type reconstructed_feature_geometries: string\n"
			"  :param reconstruction_time: the specific geological time to reconstruct to\n"
			"  :type reconstruction_time: float\n"
			"  :param anchor_plate_id: the anchored plate id of the :class:`ReconstructionTree` "
			"used to reconstruct\n"
			"  :type anchor_plate_id: int\n"
			"  :param output_parameters: variable number of keyword arguments specifying output "
			"parameters (see table below)\n"
			"  :raises: OpenFileForReadingError if any input file is not readable (when filenames specified)\n"
			"  :raises: FileFormatNotSupportedError if any input file format (identified by the "
			"reconstructable and rotation filename extensions) does not support reading "
			"(when filenames specified)\n"
			"\n"
			"  The following optional keyword arguments are supported by *output_parameters*:\n"
			"\n"
			"  ======================================= ===== ======== ==============\n"
			"  Name                                    Type  Default  Description\n"
			"  ======================================= ===== ======== ==============\n"
			"  export_wrap_to_dateline                 bool  True     Wrap/clip reconstructed "
			"geometries to the dateline (currently ignored unless exporting to ESRI shapefile format).\n"
			"  ======================================= ===== ======== ==============\n"
			"\n"
			"  The following *export* file formats are currently supported by GPlates:\n"
			"\n"
			"  =============================== =======================\n"
			"  Export File Format              Filename Extension     \n"
			"  =============================== =======================\n"
			"  ESRI shapefile                  '.shp'                 \n"
			"  OGR GMT                         '.gmt'                 \n"
			"  GMT xy                          '.xy'                  \n"
			"  =============================== =======================\n"
			"\n"
			"  Note that the filename extension of *reconstructed_feature_geometries* determines "
			"the export file format.\n"
			"\n"
			"  Note that *rotation_features* can be either a :class:`RotationModel` or a "
			"sequence (eg, ``list`` or ``tuple``) containing :class:`FeatureCollection` instances "
			"or filenames (or a mixture of both). In the latter case a temporary :class:`RotationModel` is "
			"created internally (and hence is less efficient if this function is called multiple times).\n"
			"\n"
			"  If any filenames are specified then :class:`FeatureCollectionFileFormatRegistry` is "
			"used internally to read feature collections from those files.\n";

	bp::def("reverse_reconstruct", &GPlatesApi::reverse_recontruct);

	// Enable the rotation features boost::variant to be initialised from python.
	// This is used in 'reconstruct()'.
	GPlatesApi::PythonConverterUtils::register_variant_conversion<GPlatesApi::rotation_features_type>();
}

#endif
