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
#include <Python.h> 
#endif

#include <utility>
#include <vector>
#include <boost/foreach.hpp>
#include <boost/optional.hpp>
#include <boost/variant.hpp>
#include <QString>

#include "PyFeatureCollection.h"
#include "PyRotationModel.h"
#include "PythonConverterUtils.h"
#include "PythonUtils.h"
#include "PythonVariableFunctionArguments.h"

#include "app-logic/ReconstructedFeatureGeometry.h"
#include "app-logic/ReconstructedFlowline.h"
#include "app-logic/ReconstructedMotionPath.h"
#include "app-logic/ReconstructHandle.h"
#include "app-logic/ReconstructionGeometryUtils.h"
#include "app-logic/ReconstructMethodInterface.h"
#include "app-logic/ReconstructMethodRegistry.h"

#include "feature-visitors/GeometrySetter.h"

#include "file-io/FeatureCollectionFileFormatRegistry.h"
#include "file-io/File.h"
#include "file-io/ReadErrorAccumulation.h"
#include "file-io/ReconstructedFeatureGeometryExport.h"
#include "file-io/ReconstructedFlowlineExport.h"
#include "file-io/ReconstructedMotionPathExport.h"
#include "file-io/ReconstructionGeometryExportImpl.h"

#include "global/python.h"
// This is not included by <boost/python.hpp>.
// Also we must include this after <boost/python.hpp> which means after "global/python.h".
#include <boost/python/raw_function.hpp>

#include "model/FeatureCollectionHandle.h"
#include "model/types.h"

#include "property-values/GeoTimeInstant.h"


#if !defined(GPLATES_NO_PYTHON)

namespace bp = boost::python;


namespace GPlatesApi
{
	namespace
	{
		/**
		 * Enumeration to determine which reconstructed feature geometry types to output.
		 */
		namespace ReconstructType
		{
			enum Value
			{
				FEATURE_GEOMETRY,
				MOTION_PATH,
				FLOWLINE
			};
		};


		/**
		 * The argument types for 'reconstructed feature geometries'.
		 */
		typedef boost::variant<
				QString,  // export filename
				bp::list> // list of ReconstructedFeatureGeometry's
						reconstructed_feature_geometries_argument_type;


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
				reconstructed_feature_geometries_argument_type &reconstructed_feature_geometries,
				GPlatesPropertyValues::GeoTimeInstant &reconstruction_time,
				GPlatesModel::integer_plate_id_type &anchor_plate_id)
		{
			// Define the explicit function argument types...
			//
			// We're actually more generous than the original (deprecated) function since the original
			// only allowed a python 'list' of filenames (for reconstructable and rotation features).
			typedef boost::tuple<
					FeatureCollectionSequenceFunctionArgument,
					RotationModelFunctionArgument,
					double, // Note: This is not GPlatesPropertyValues::GeoTimeInstant.
					GPlatesModel::integer_plate_id_type,
					QString> // Only export filename supported (not a python list of RFG's).
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
			reconstruction_time = GPlatesPropertyValues::GeoTimeInstant(boost::get<2>(reconstruct_args));
			anchor_plate_id = boost::get<3>(reconstruct_args);
			reconstructed_feature_geometries = boost::get<4>(reconstruct_args);

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
				reconstructed_feature_geometries_argument_type &reconstructed_feature_geometries,
				GPlatesPropertyValues::GeoTimeInstant &reconstruction_time,
				GPlatesModel::integer_plate_id_type &anchor_plate_id,
				ReconstructType::Value &reconstruct_type,
				bool &export_wrap_to_dateline,
				bool &group_with_feature)
		{
			// First attempt to get arguments from deprecated version of 'reconstruct()'.
			if (get_deprecated_reconstruct_args(
					positional_args,
					keyword_args,
					reconstructable_files,
					rotation_model,
					reconstructed_feature_geometries,
					reconstruction_time,
					anchor_plate_id))
			{
				// Successfully obtained arguments from deprecated version of 'reconstruct()'.

				// Parameters not available in deprecated function - so set to default values...
				reconstruct_type = ReconstructType::FEATURE_GEOMETRY;
				export_wrap_to_dateline = true;
				group_with_feature = false;

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
					reconstructed_feature_geometries_argument_type,
					GPlatesPropertyValues::GeoTimeInstant,
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
			reconstructed_feature_geometries = boost::get<2>(reconstruct_args);
			reconstruction_time = boost::get<3>(reconstruct_args);
			anchor_plate_id = boost::get<4>(reconstruct_args);

			//
			// Get the optional non-explicit output parameters from the variable argument list.
			//

			reconstruct_type =
					VariableArguments::extract_and_remove_or_default<ReconstructType::Value>(
							unused_keyword_args,
							"reconstruct_type",
							ReconstructType::FEATURE_GEOMETRY);

			export_wrap_to_dateline =
					VariableArguments::extract_and_remove_or_default<bool>(
							unused_keyword_args,
							"export_wrap_to_dateline",
							true);

			group_with_feature =
					VariableArguments::extract_and_remove_or_default<bool>(
							unused_keyword_args,
							"group_with_feature",
							false);

			// Raise a python error if there are any unused keyword arguments remaining.
			// These will be keywords that we didn't recognise.
			VariableArguments::raise_python_error_if_unused(unused_keyword_args);
		}


		// Traits class to allow one 'get_format()' function to handle all three export namespaces
		// 'ReconstructedFeatureGeometryExport', 'ReconstructedFlowlineExport' and 'ReconstructedMotionPathExport'.
		template <class ReconstructionGeometryType>
		struct FormatTraits
		{  };

		template <>
		struct FormatTraits<GPlatesAppLogic::ReconstructedFeatureGeometry>
		{
			typedef GPlatesFileIO::ReconstructedFeatureGeometryExport::Format format_type;

			static const format_type UNKNOWN = GPlatesFileIO::ReconstructedFeatureGeometryExport::UNKNOWN;
			static const format_type GMT = GPlatesFileIO::ReconstructedFeatureGeometryExport::GMT;
			static const format_type SHAPEFILE = GPlatesFileIO::ReconstructedFeatureGeometryExport::SHAPEFILE;
			static const format_type OGRGMT = GPlatesFileIO::ReconstructedFeatureGeometryExport::OGRGMT;
		};

		template <>
		struct FormatTraits<GPlatesAppLogic::ReconstructedMotionPath>
		{
			typedef GPlatesFileIO::ReconstructedMotionPathExport::Format format_type;

			static const format_type UNKNOWN = GPlatesFileIO::ReconstructedMotionPathExport::UNKNOWN;
			static const format_type GMT = GPlatesFileIO::ReconstructedMotionPathExport::GMT;
			static const format_type SHAPEFILE = GPlatesFileIO::ReconstructedMotionPathExport::SHAPEFILE;
			static const format_type OGRGMT = GPlatesFileIO::ReconstructedMotionPathExport::OGRGMT;
		};

		template <>
		struct FormatTraits<GPlatesAppLogic::ReconstructedFlowline>
		{
			typedef GPlatesFileIO::ReconstructedFlowlineExport::Format format_type;

			static const format_type UNKNOWN = GPlatesFileIO::ReconstructedFlowlineExport::UNKNOWN;
			static const format_type GMT = GPlatesFileIO::ReconstructedFlowlineExport::GMT;
			static const format_type SHAPEFILE = GPlatesFileIO::ReconstructedFlowlineExport::SHAPEFILE;
			static const format_type OGRGMT = GPlatesFileIO::ReconstructedFlowlineExport::OGRGMT;
		};


		/**
		 * Template function to handles format retrieval for all three export namespaces
		 * 'ReconstructedFeatureGeometryExport', 'ReconstructedFlowlineExport' and 'ReconstructedMotionPathExport'.
		 */
		template <class ReconstructionGeometryType>
		typename FormatTraits<ReconstructionGeometryType>::format_type
		get_format(
				QString file_name)
		{
			static const QString GMT_EXT = ".xy";
			static const QString SHP_EXT = ".shp";
			static const QString OGRGMT_EXT = ".gmt";

			if (file_name.endsWith(GMT_EXT))
			{
				return FormatTraits<ReconstructionGeometryType>::GMT;
			}
			if (file_name.endsWith(SHP_EXT))
			{
				return FormatTraits<ReconstructionGeometryType>::SHAPEFILE;
			}
			if (file_name.endsWith(OGRGMT_EXT))
			{
				return FormatTraits<ReconstructionGeometryType>::OGRGMT;
			}

			return FormatTraits<ReconstructionGeometryType>::UNKNOWN;
		}


		void
		export_reconstructed_feature_geometries(
				const std::vector<GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_type> &rfgs,
				const QString &export_file_name,
				const std::vector<const GPlatesFileIO::File::Reference *> &reconstructable_file_ptrs,
				const std::vector<const GPlatesFileIO::File::Reference *> &reconstruction_file_ptrs,
				const GPlatesModel::integer_plate_id_type &anchor_plate_id,
				const double &reconstruction_time,
				bool export_wrap_to_dateline)
		{
			// Converts to raw pointers.
			std::vector<const GPlatesAppLogic::ReconstructedFeatureGeometry *> reconstructed_feature_geometries;
			reconstructed_feature_geometries.reserve(rfgs.size());
			BOOST_FOREACH(
					const GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_type &rfg,
					rfgs)
			{
				reconstructed_feature_geometries.push_back(rfg.get());
			}

			const GPlatesFileIO::ReconstructedFeatureGeometryExport::Format format =
					get_format<GPlatesAppLogic::ReconstructedFeatureGeometry>(export_file_name);

			// Export the reconstructed feature geometries.
			GPlatesFileIO::ReconstructedFeatureGeometryExport::export_reconstructed_feature_geometries(
						export_file_name,
						format,
						reconstructed_feature_geometries,
						reconstructable_file_ptrs,
						reconstruction_file_ptrs,
						anchor_plate_id,
						reconstruction_time,
						// If exporting to Shapefile and there's only *one* input reconstructable file then
						// shapefile attributes in input reconstructable file will get copied to output...
						true/*export_single_output_file*/,
						false/*export_per_input_file*/, // We only generate a single output file.
						false/*export_output_directory_per_input_file*/, // We only generate a single output file.
						export_wrap_to_dateline);
		}


		void
		export_reconstructed_motion_paths(
				const std::vector<GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_type> &rfgs,
				const QString &export_file_name,
				const std::vector<const GPlatesFileIO::File::Reference *> &reconstructable_file_ptrs,
				const std::vector<const GPlatesFileIO::File::Reference *> &reconstruction_file_ptrs,
				const GPlatesModel::integer_plate_id_type &anchor_plate_id,
				const double &reconstruction_time,
				bool export_wrap_to_dateline)
		{
			// Get any ReconstructedFeatureGeometry objects that are of type ReconstructedMotionPath.
			//
			// Note that, when motion paths are reconstructed, both ReconstructedMotionPath's and
			// ReconstructedFeatureGeometry's are generated - so this also ensures that the
			// ReconstructedFeatureGeometry's are ignored when outputting reconstructed motion paths.
			std::vector<const GPlatesAppLogic::ReconstructedMotionPath *> reconstructed_motion_paths;
			GPlatesAppLogic::ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type_sequence(
					rfgs.begin(),
					rfgs.end(),
					reconstructed_motion_paths);

			const GPlatesFileIO::ReconstructedMotionPathExport::Format format =
					get_format<GPlatesAppLogic::ReconstructedMotionPath>(export_file_name);

			// Export the reconstructed motion paths.
			GPlatesFileIO::ReconstructedMotionPathExport::export_reconstructed_motion_paths(
						export_file_name,
						format,
						reconstructed_motion_paths,
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
		export_reconstructed_flowlines(
				const std::vector<GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_type> &rfgs,
				const QString &export_file_name,
				const std::vector<const GPlatesFileIO::File::Reference *> &reconstructable_file_ptrs,
				const std::vector<const GPlatesFileIO::File::Reference *> &reconstruction_file_ptrs,
				const GPlatesModel::integer_plate_id_type &anchor_plate_id,
				const double &reconstruction_time,
				bool export_wrap_to_dateline)
		{
			// Get any ReconstructedFeatureGeometry objects that are of type ReconstructedFlowline.
			// In fact they should all be ReconstructedFlowlines.
			std::vector<const GPlatesAppLogic::ReconstructedFlowline *> reconstructed_flowlines;
			GPlatesAppLogic::ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type_sequence(
					rfgs.begin(),
					rfgs.end(),
					reconstructed_flowlines);

			const GPlatesFileIO::ReconstructedFlowlineExport::Format format =
					get_format<GPlatesAppLogic::ReconstructedFlowline>(export_file_name);

			// Export the reconstructed flowlines.
			GPlatesFileIO::ReconstructedFlowlineExport::export_reconstructed_flowlines(
						export_file_name,
						format,
						reconstructed_flowlines,
						reconstructable_file_ptrs,
						reconstruction_file_ptrs,
						anchor_plate_id,
						reconstruction_time,
						true/*export_single_output_file*/,
						false/*export_per_input_file*/, // We only generate a single output file.
						false/*export_output_directory_per_input_file*/, // We only generate a single output file.
						export_wrap_to_dateline);
		}


		/**
		 * Append the reconstruction geometries, as type 'ReconstructionGeometryType', to the
		 * python list @a output_reconstruction_geometries_list.
		 *
		 * If @a group_with_feature is true then @a output_reconstruction_geometries_list contains
		 * tuples of (feature, list of reconstruction geometries).
		 */
		template <class ReconstructionGeometryType>
		void
		output_reconstruction_geometries(
				bp::list output_reconstruction_geometries_list,
				const std::vector<GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_type> &rfgs,
				const std::vector<const GPlatesFileIO::File::Reference *> &reconstructable_file_ptrs,
				bool group_with_feature)
		{
			// Get any ReconstructedFeatureGeometry objects that are of type ReconstructionGeometryType.
			//
			// Note that, when motion paths are reconstructed, both ReconstructedMotionPath's and
			// ReconstructedFeatureGeometry's are generated - so this also ensures that the
			// ReconstructedFeatureGeometry's are ignored when outputting reconstructed motion paths.
			std::vector<const ReconstructionGeometryType *> reconstruction_geometries;
			GPlatesAppLogic::ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type_sequence(
					rfgs.begin(),
					rfgs.end(),
					reconstruction_geometries);

			//
			// Order the reconstruction geometries according to the order of the features in the feature collections.
			//

			// Get the list of active reconstructable feature collection files that contain
			// the features referenced by the ReconstructionGeometry objects.
			GPlatesFileIO::ReconstructionGeometryExportImpl::feature_handle_to_collection_map_type feature_to_collection_map;
			GPlatesFileIO::ReconstructionGeometryExportImpl::populate_feature_handle_to_collection_map(
					feature_to_collection_map,
					reconstructable_file_ptrs);

			// Group the ReconstructionGeometry objects by their feature.
			typedef GPlatesFileIO::ReconstructionGeometryExportImpl::FeatureGeometryGroup<
					ReconstructionGeometryType> feature_geometry_group_type;
			std::list<feature_geometry_group_type> grouped_recon_geoms_seq;
			GPlatesFileIO::ReconstructionGeometryExportImpl::group_reconstruction_geometries_with_their_feature(
					grouped_recon_geoms_seq,
					reconstruction_geometries,
					feature_to_collection_map);

			//
			// Append the ordered RFG's to the output list.
			//

			typename std::list<feature_geometry_group_type>::const_iterator feature_iter;
			for (feature_iter = grouped_recon_geoms_seq.begin();
				feature_iter != grouped_recon_geoms_seq.end();
				++feature_iter)
			{
				const feature_geometry_group_type &feature_geom_group = *feature_iter;

				const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref =
						feature_geom_group.feature_ref;
				if (!feature_ref.is_valid())
				{
					continue;
				}

				// Group reconstruction geometries with their feature if requested.
				boost::optional<bp::list> feature_reconstruction_geometries_list;
				if (group_with_feature)
				{
					// Create a Python feature.
					bp::object feature_object(
							GPlatesModel::FeatureHandle::non_null_ptr_to_const_type(feature_ref.handle_ptr()));
					// Create a Python list (of reconstruction geometries).
					feature_reconstruction_geometries_list = bp::list();

					// Add a tuple containing the feature and its list of reconstruction geometries.
					output_reconstruction_geometries_list.append(
							bp::make_tuple(
									feature_object,
									feature_reconstruction_geometries_list.get()));
				}

				// Iterate through the reconstruction geometries of the current feature and write to output.
				typename std::vector<const ReconstructionGeometryType *>::const_iterator rg_iter;
				for (rg_iter = feature_geom_group.recon_geoms.begin();
					rg_iter != feature_geom_group.recon_geoms.end();
					++rg_iter)
				{
					const typename ReconstructionGeometryType::non_null_ptr_to_const_type rg(*rg_iter);

					// Add the reconstruction geometry to python list.
					if (group_with_feature)
					{
						feature_reconstruction_geometries_list->append(rg);
					}
					else
					{
						output_reconstruction_geometries_list.append(rg);
					}
				}
			}
		}
	}


	/**
	 * Reconstruct feature collections, optionally loaded from files, to a specific geological time and
	 * optionally export to file(s).
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
		reconstructed_feature_geometries_argument_type reconstructed_feature_geometries_argument;
		GPlatesPropertyValues::GeoTimeInstant reconstruction_time(0);
		GPlatesModel::integer_plate_id_type anchor_plate_id;
		ReconstructType::Value reconstruct_type;
		bool export_wrap_to_dateline;
		bool group_with_feature;

		get_reconstruct_args(
				positional_args,
				keyword_args,
				reconstructable_files,
				rotation_model,
				reconstructed_feature_geometries_argument,
				reconstruction_time,
				anchor_plate_id,
				reconstruct_type,
				export_wrap_to_dateline,
				group_with_feature);

		// Time must not be distant past/future.
		if (!reconstruction_time.is_real())
		{
			PyErr_SetString(PyExc_ValueError,
					"Time values cannot be distant-past (float('inf')) or distant-future (float('-inf')).");
			bp::throw_error_already_set();
		}

		//
		// Reconstruct the features in the feature collection files.
		//

		// Adapt the reconstruction tree creator to a new one that has 'anchor_plate_id' as its default.
		// This ensures 'ReconstructMethodInterface' will reconstruct using the correct anchor plate.
		GPlatesAppLogic::ReconstructionTreeCreator reconstruction_tree_creator =
				GPlatesAppLogic::create_cached_reconstruction_tree_adaptor(
						rotation_model.get()->get_reconstruction_tree_creator(),
						anchor_plate_id);

		// Create the context state in which to reconstruct.
		const GPlatesAppLogic::ReconstructMethodInterface::Context reconstruct_method_context(
				GPlatesAppLogic::ReconstructParams(),
				reconstruction_tree_creator);

		std::vector<GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_type> rfgs;
		GPlatesAppLogic::ReconstructMethodRegistry reconstruct_method_registry;

		// Get the next global reconstruct handle - it'll be stored in each RFG.
		// It doesn't actually matter in our case though.
		const GPlatesAppLogic::ReconstructHandle::type reconstruct_handle =
				GPlatesAppLogic::ReconstructHandle::get_next_reconstruct_handle();

		// Iterate over the files and reconstruct their features.
		BOOST_FOREACH(GPlatesFileIO::File::non_null_ptr_type reconstruct_file, reconstructable_files)
		{
			const GPlatesModel::FeatureCollectionHandle::weak_ref feature_collection_ref =
					reconstruct_file->get_reference().get_feature_collection();

			// Iterate over the features in the current file's feature collection.
			GPlatesModel::FeatureCollectionHandle::iterator features_iter = feature_collection_ref->begin();
			GPlatesModel::FeatureCollectionHandle::iterator features_end = feature_collection_ref->end();
			for ( ; features_iter != features_end; ++features_iter)
			{
				const GPlatesModel::FeatureHandle::weak_ref feature_ref = (*features_iter)->reference();

				// Determine what type of reconstructed output the current feature will produce (if any).
				boost::optional<GPlatesAppLogic::ReconstructMethod::Type> reconstruct_method_type =
						reconstruct_method_registry.get_reconstruct_method_type(feature_ref);
				if (!reconstruct_method_type)
				{
					continue;
				}

				// Check that the reconstructed type matches that requested by the caller.
				switch (reconstruct_type)
				{
				case ReconstructType::FEATURE_GEOMETRY:
					// Skip flowlines and motion paths.
					if (reconstruct_method_type.get() == GPlatesAppLogic::ReconstructMethod::FLOWLINE ||
						reconstruct_method_type.get() == GPlatesAppLogic::ReconstructMethod::MOTION_PATH)
					{
						continue;
					}
					break;

				case ReconstructType::MOTION_PATH:
					// Skip anything but motion paths.
					if (reconstruct_method_type.get() != GPlatesAppLogic::ReconstructMethod::MOTION_PATH)
					{
						continue;
					}
					break;

				case ReconstructType::FLOWLINE:
					// Skip anything but flowlines.
					if (reconstruct_method_type.get() != GPlatesAppLogic::ReconstructMethod::FLOWLINE)
					{
						continue;
					}
					break;

				default:
					GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
					break;
				}

				GPlatesAppLogic::ReconstructMethodInterface::non_null_ptr_type reconstruct_method =
						reconstruct_method_registry.create_reconstruct_method(
								reconstruct_method_type.get(),
								feature_ref,
								reconstruct_method_context);

				// Reconstruct the current feature and append the reconstructed feature geoms to 'rfgs'.
				reconstruct_method->reconstruct_feature_geometries(
						rfgs,
						reconstruct_handle,
						reconstruct_method_context,
						reconstruction_time.value());
			}
		}

		//
		// Either export the reconstructed geometries to a file or append them to a python list.
		//

		if (const QString *export_file_name = boost::get<QString>(&reconstructed_feature_geometries_argument))
		{
			// Get the sequence of reconstructable files as File pointers.
			std::vector<const GPlatesFileIO::File::Reference *> reconstructable_file_ptrs;
			BOOST_FOREACH(GPlatesFileIO::File::non_null_ptr_type reconstructable_file, reconstructable_files)
			{
				reconstructable_file_ptrs.push_back(&reconstructable_file->get_reference());
			}

			// Get the sequence of reconstruction files (if any) from the rotation model.
			std::vector<GPlatesFileIO::File::non_null_ptr_type> reconstruction_files;
			rotation_model.get()->get_files(reconstruction_files);
			std::vector<const GPlatesFileIO::File::Reference *> reconstruction_file_ptrs;
			BOOST_FOREACH(GPlatesFileIO::File::non_null_ptr_type reconstruction_file, reconstruction_files)
			{
				reconstruction_file_ptrs.push_back(&reconstruction_file->get_reference());
			}

			// Export based on the reconstructed type requested by the caller.
			switch (reconstruct_type)
			{
			case ReconstructType::FEATURE_GEOMETRY:
				export_reconstructed_feature_geometries(
						rfgs,
						*export_file_name,
						reconstructable_file_ptrs,
						reconstruction_file_ptrs,
						anchor_plate_id,
						reconstruction_time.value(),
						export_wrap_to_dateline);
				break;

			case ReconstructType::MOTION_PATH:
				export_reconstructed_motion_paths(
						rfgs,
						*export_file_name,
						reconstructable_file_ptrs,
						reconstruction_file_ptrs,
						anchor_plate_id,
						reconstruction_time.value(),
						export_wrap_to_dateline);
				break;

			case ReconstructType::FLOWLINE:
				export_reconstructed_flowlines(
						rfgs,
						*export_file_name,
						reconstructable_file_ptrs,
						reconstruction_file_ptrs,
						anchor_plate_id,
						reconstruction_time.value(),
						export_wrap_to_dateline);
				break;

			default:
				GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
				break;
			}
		}
		else // list of ReconstructedFeatureGeometry's...
		{
			bp::list output_reconstruction_geometries_list =
					boost::get<bp::list>(reconstructed_feature_geometries_argument);

			// Get the sequence of reconstructable files as File pointers.
			std::vector<const GPlatesFileIO::File::Reference *> reconstructable_file_ptrs;
			BOOST_FOREACH(GPlatesFileIO::File::non_null_ptr_type reconstructable_file, reconstructable_files)
			{
				reconstructable_file_ptrs.push_back(&reconstructable_file->get_reference());
			}

			// Output based on the reconstructed type requested by the caller.
			switch (reconstruct_type)
			{
			case ReconstructType::FEATURE_GEOMETRY:
				output_reconstruction_geometries<GPlatesAppLogic::ReconstructedFeatureGeometry>(
						output_reconstruction_geometries_list,
						rfgs,
						reconstructable_file_ptrs,
						group_with_feature);
				break;

			case ReconstructType::MOTION_PATH:
				output_reconstruction_geometries<GPlatesAppLogic::ReconstructedMotionPath>(
						output_reconstruction_geometries_list,
						rfgs,
						reconstructable_file_ptrs,
						group_with_feature);
				break;

			case ReconstructType::FLOWLINE:
				output_reconstruction_geometries<GPlatesAppLogic::ReconstructedFlowline>(
						output_reconstruction_geometries_list,
						rfgs,
						reconstructable_file_ptrs,
						group_with_feature);
				break;

			default:
				GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
				break;
			}
		}

		// We must return a value (required by 'bp::raw_function') so just return Py_None.
		return bp::object();
	}


	/**
	 * Loads one or more reconstructable feature collections (optionally from a files) @a reconstructable_features
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
			FeatureCollectionSequenceFunctionArgument reconstructable_features,
			RotationModelFunctionArgument rotation_model,
			const GPlatesPropertyValues::GeoTimeInstant &reconstruction_time,
			GPlatesModel::integer_plate_id_type anchor_plate_id)
	{
		// Time must not be distant past/future.
		if (!reconstruction_time.is_real())
		{
			PyErr_SetString(PyExc_ValueError,
					"Time values cannot be distant-past (float('inf')) or distant-future (float('-inf')).");
			bp::throw_error_already_set();
		}

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

		std::vector<GPlatesFileIO::File::non_null_ptr_type> reconstructable_files;
		reconstructable_features.get_files(reconstructable_files);

		// Iterate over the files.
		std::vector<GPlatesFileIO::File::non_null_ptr_type>::const_iterator reconstructable_files_iter =
				reconstructable_files.begin();
		std::vector<GPlatesFileIO::File::non_null_ptr_type>::const_iterator reconstructable_files_end =
				reconstructable_files.end();
		for ( ; reconstructable_files_iter != reconstructable_files_end; ++reconstructable_files_iter)
		{
			GPlatesFileIO::File::non_null_ptr_type reconstructable_file = *reconstructable_files_iter;

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
									reconstruction_time.value()/*reconstruction_time*/,
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
}

	
void
export_reconstruct()
{
	// An enumeration nested within 'pygplates (ie, current) module.
	bp::enum_<GPlatesApi::ReconstructType::Value>("ReconstructType")
			.value("feature_geometry", GPlatesApi::ReconstructType::FEATURE_GEOMETRY)
			.value("motion_path", GPlatesApi::ReconstructType::MOTION_PATH)
			.value("flowline", GPlatesApi::ReconstructType::FLOWLINE);

	const char *reconstruct_function_name = "reconstruct";
	bp::def(reconstruct_function_name, bp::raw_function(&GPlatesApi::reconstruct));

	// Seems we cannot set a docstring using non-class bp::def combined with bp::raw_function,
	// or any case where the second argument of bp::def is a bp::object,
	// so we set the docstring the old-fashioned way.
	bp::scope().attr(reconstruct_function_name).attr("__doc__") =
			"reconstruct(reconstructable_features, rotation_model, reconstructed_geometries, "
			"reconstruction_time, [anchor_plate_id=0], [\\*\\*output_parameters])\n"
			"  Reconstruct regular geological features, motion paths or flowlines to a specific geological time.\n"
			"\n"
			"  :param reconstructable_features: the features to reconstruct as a feature collection, or filename, or "
			"feature, or sequence of features, or a sequence (eg, ``list`` or ``tuple``) of any "
			"combination of those four types\n"
			"  :type reconstructable_features: :class:`FeatureCollection`, or string, or :class:`Feature`, "
			"or sequence of :class:`Feature`, or sequence of any combination of those four types\n"
			"  :param rotation_model: A rotation model or a rotation feature collection or a rotation "
			"filename or a sequence of rotation feature collections and/or rotation filenames\n"
			"  :type rotation_model: :class:`RotationModel` or :class:`FeatureCollection` or string "
			"or sequence of :class:`FeatureCollection` instances and/or strings\n"
			"  :param reconstructed_geometries: the "
			":class:`reconstructed feature geometries<ReconstructedFeatureGeometry>` (default) or "
			":class:`reconstructed motion paths<ReconstructedMotionPath>` or "
			":class:`reconstructed flowlines<ReconstructedFlowline>` (depending on the optional "
			"keyword argument *reconstruct_type* - see *output_parameters* table) are either exported to a "
			"file (with specified filename) or *appended* to a Python ``list`` (note that the list is *not* "
			"cleared first and note that the list contents are affected by *group_with_feature* - see "
			"*output_parameters* table)\n"
			"  :type reconstructed_geometries: string or ``list``\n"
			"  :param reconstruction_time: the specific geological time to reconstruct to\n"
			"  :type reconstruction_time: float or :class:`GeoTimeInstant`\n"
			"  :param anchor_plate_id: the anchored plate id used during reconstruction\n"
			"  :type anchor_plate_id: int\n"
			"  :param output_parameters: variable number of keyword arguments specifying output "
			"parameters (see table below)\n"
			"  :raises: OpenFileForReadingError if any input file is not readable (when filenames specified)\n"
			"  :raises: OpenFileForWritingError if *reconstructed_geometries* is a filename and it is not writeable\n"
			"  :raises: FileFormatNotSupportedError if any input file format (identified by any "
			"reconstructable and rotation filename extensions) does not support reading "
			"(when filenames specified)\n"
			"  :raises: ValueError if *reconstruction_time* is "
			":meth:`distant past<GeoTimeInstant.is_distant_past>` or "
			":meth:`distant future<GeoTimeInstant.is_distant_future>`\n"
			"\n"
			"  The following optional keyword arguments are supported by *output_parameters*:\n"
			"\n"
			"  +-------------------------+-----------------+----------------------------------+---------------------------------------------------------------------------+\n"
			"  | Name                    | Type            | Default                          | Description                                                               |\n"
			"  +=========================+=================+==================================+===========================================================================+\n"
			"  | reconstruct_type        | ReconstructType | ReconstructType.feature_geometry | - *ReconstructType.feature_geometry*:                                     |\n"
			"  |                         |                 |                                  |   only reconstruct regular features (not motion paths or                  |\n"
			"  |                         |                 |                                  |   flowlines), this generates                                              |\n"
			"  |                         |                 |                                  |   :class:`reconstructed feature geometries<ReconstructedFeatureGeometry>` |\n"
			"  |                         |                 |                                  | - *ReconstructType.motion_path*:                                          |\n"
			"  |                         |                 |                                  |   only reconstruct motion path features, this generates                   |\n"
			"  |                         |                 |                                  |   :class:`reconstructed motion paths<ReconstructedMotionPath>`            |\n"
			"  |                         |                 |                                  | - *ReconstructType.flowline*:                                             |\n"
			"  |                         |                 |                                  |   only reconstruct flowline features, this generates                      |\n"
			"  |                         |                 |                                  |   :class:`reconstructed flowlines<ReconstructedFlowline>`                 |\n"
			"  +-------------------------+-----------------+----------------------------------+---------------------------------------------------------------------------+\n"
			"  | group_with_feature      | bool            | False                            | | Group reconstructed geometries with their feature.                      |\n"
			"  |                         |                 |                                  | | This can be useful when a feature has more than one geometry and hence  |\n"
			"  |                         |                 |                                  |   more than one reconstructed geometry.                                   |\n"
			"  |                         |                 |                                  | | *reconstructed_geometries* then becomes a list of tuples where each     |\n"
			"  |                         |                 |                                  |   tuple contains a :class:`feature<Feature>` and a ``list`` of            |\n"
			"  |                         |                 |                                  |   reconstructed geometries.                                               |\n"
			"  |                         |                 |                                  |                                                                           |\n"
			"  |                         |                 |                                  | .. note:: Only applies when *reconstructed_geometries* is a ``list``      |\n"
			"  |                         |                 |                                  |    because exported files are always grouped with feature.                |\n"
			"  |                         |                 |                                  |                                                                           |\n"
			"  |                         |                 |                                  | .. note:: Any *ReconstructType* can be grouped.                           |\n"
			"  +-------------------------+-----------------+----------------------------------+---------------------------------------------------------------------------+\n"
			"  | export_wrap_to_dateline | bool            | True                             | | Wrap/clip reconstructed geometries to the dateline (currently           |\n"
			"  |                         |                 |                                  |   ignored unless exporting to an ESRI Shapefile format *file*).           |\n"
			"  |                         |                 |                                  | | Only applies when exporting to a file (ESRI Shapefile).                 |\n"
			"  +-------------------------+-----------------+----------------------------------+---------------------------------------------------------------------------+\n"
			"\n"
			"  Only the :class:`features<Feature>`, in *reconstructable_features*, that match the "
			"optional keyword argument *reconstruct_type* (see *output_parameters* table) are reconstructed. "
			"This also determines the type of reconstructed geometries output in *reconstructed_geometries* "
			"which are either :class:`reconstructed feature geometries<ReconstructedFeatureGeometry>` (default) or "
			":class:`reconstructed motion paths<ReconstructedMotionPath>` or "
			":class:`reconstructed flowlines<ReconstructedFlowline>`.\n"
			"\n"
			"  .. note:: | *reconstructed_geometries* can be either an export filename or a Python ``list``.\n"
			"            | In the latter case the reconstructed geometries (generated by the reconstruction) are "
			"appended to the Python ``list`` (instead of exported to a file).\n"
			"            | And if *group_with_feature* (see *output_parameters* table) is ``True`` then the list "
			"contains tuples that group each :class:`feature<Feature>` with a list of its reconstructed geometries.\n"
			"\n"
			"  The *reconstructed_geometries* are output in the same order as that of their "
			"respective features in *reconstructable_features* (the order across feature collections "
			"is also retained). This happens regardless of whether *reconstructable_features* "
			"and *reconstructed_geometries* include files or not.\n"
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
			"  .. note:: | When exporting to a file, the filename extension of *reconstructed_geometries* "
			"determines the export file format.\n"
			"            | If the export format is ESRI Shapefile then the shapefile attributes from "
			"*reconstructable_features* will only be retained in the exported shapefile if there "
			"is a single reconstructable feature collection (where *reconstructable_features* is a "
			"single feature collection or file, or sequence containing a single feature collection "
			"or file). This is because shapefile attributes from multiple input feature collections are "
			"not easily combined into a single output shapefile (due to different attribute field names).\n"
			"\n"
			"  .. note:: *reconstructable_features* can be a :class:`FeatureCollection` or a filename "
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
			"  Reconstructing a file containing regular reconstructable features to a shapefile at 10Ma:\n"
			"  ::\n"
			"\n"
            "    pygplates.reconstruct('volcanoes.gpml', 'rotations.rot', 'reconstructed_volcanoes_10Ma.shp', 10)\n"
			"\n"
			"  Reconstructing multiple files containing regular reconstructable features to a list of reconstructed "
			"feature geometries at 10Ma:\n"
			"  ::\n"
			"\n"
			"    reconstructed_feature_geometries = []\n"
			"    pygplates.reconstruct(['continent_ocean_boundaries.gpml', 'isochrons.gpml'], "
			"rotation_model, reconstructed_feature_geometries, 10)\n"
			"\n"
			"  ...and the same but also grouping the :class:`reconstructed feature geometries<ReconstructedFeatureGeometry>` "
			"with their :class:`feature<Feature>`:\n"
			"  ::\n"
			"\n"
			"    reconstructed_features = []\n"
			"    pygplates.reconstruct(['continent_ocean_boundaries.gpml', 'isochrons.gpml'], "
			"rotation_model, reconstructed_features, 10, group_with_feature=True)\n"
			"    for feature, feature_reconstructed_geometries in reconstructed_features:\n"
			"        # Note that 'feature' is the same as 'feature_reconstructed_geometry.get_feature()'.\n"
			"        for feature_reconstructed_geometry in feature_reconstructed_geometries:\n"
			"            ...\n"
			"\n"
			"  Reconstructing a file containing flowline features to a shapefile at 10Ma:\n"
			"  ::\n"
			"\n"
            "    pygplates.reconstruct('flowlines.gpml', rotation_model, 'reconstructed_flowlines_10Ma.shp', 10, "
			"reconstruct_type=pygplates.ReconstructType.flowline)\n"
			"\n"
			"  Reconstructing a file containing flowline features to a list of reconstructed flowlines at 10Ma:\n"
			"  ::\n"
			"\n"
			"    reconstructed_flowlines = []\n"
            "    pygplates.reconstruct('flowlines.gpml', rotation_model, reconstructed_flowlines, 10, "
			"reconstruct_type=pygplates.ReconstructType.flowline)\n"
			"\n"
			"  Reconstructing regular reconstructable features to a shapefile at 10Ma:\n"
			"  ::\n"
			"\n"
			"    pygplates.reconstruct(pygplates.FeatureCollection([feature1, feature2]), rotation_model, "
			"'reconstructed_features_10Ma.shp', 10)\n"
			"\n"
			"  Reconstructing a list of regular reconstructable features to a shapefile at 10Ma:\n"
			"  ::\n"
			"\n"
			"    pygplates.reconstruct([feature1, feature2], rotation_model, 'reconstructed_features_10Ma.shp', 10)\n"
			"\n"
			"  Reconstructing a single regular reconstructable feature to a list of reconstructed feature geometries at 10Ma:\n"
			"  ::\n"
			"\n"
			"    reconstructed_feature_geometries = []\n"
            "    pygplates.reconstruct(feature, rotation_model, reconstructed_feature_geometries, 10)\n"
			"    assert(reconstructed_feature_geometries[0].get_feature().get_feature_id() == feature.get_feature_id())\n";

	// Register 'reconstructed feature geometries' variant.
	GPlatesApi::PythonConverterUtils::register_variant_conversion<
			GPlatesApi::reconstructed_feature_geometries_argument_type>();


	bp::def("reverse_reconstruct",
			&GPlatesApi::reverse_reconstruct,
			(bp::arg("reconstructable_features"),
				bp::arg("rotation_model"),
				bp::arg("reconstruction_time"),
				bp::arg("anchor_plate_id")=0),
			"reverse_reconstruct(reconstructable_features, rotation_model, reconstruction_time, [anchor_plate_id=0])\n"
			"  Reverse reconstruct geological features from a specific geological time.\n"
			"\n"
			"  :param reconstructable_features: A reconstructable feature collection, or filename, or "
			"feature, or sequence of features, or a sequence (eg, ``list`` or ``tuple``) of any "
			"combination of those four types - all features used as input and output\n"
			"  :type reconstructable_features: :class:`FeatureCollection`, or string, or :class:`Feature`, "
			"or sequence of :class:`Feature`, or sequence of any combination of those four types\n"
			"  :param rotation_model: A rotation model or a rotation feature collection or a rotation "
			"filename or a sequence of rotation feature collections and/or rotation filenames\n"
			"  :type rotation_model: :class:`RotationModel` or :class:`FeatureCollection` or string "
			"or sequence of :class:`FeatureCollection` instances and/or strings\n"
			"  :param reconstruction_time: the specific geological time to reverse reconstruct from\n"
			"  :type reconstruction_time: float or :class:`GeoTimeInstant`\n"
			"  :param anchor_plate_id: the anchored plate id used during reverse reconstruction\n"
			"  :type anchor_plate_id: int\n"
			"  :raises: OpenFileForReadingError if any input file is not readable (when filenames specified)\n"
			"  :raises: OpenFileForWritingError if *reconstructable_features* specifies any filename that "
			"is not writeable (if any filenames are specified)\n"
			"  :raises: FileFormatNotSupportedError if any input file format (identified by any "
			"reconstructable and rotation filename extensions) does not support reading "
			"(when filenames specified)\n"
			"  :raises: ValueError if *reconstruction_time* is "
			":meth:`distant past<GeoTimeInstant.is_distant_past>` or "
			":meth:`distant future<GeoTimeInstant.is_distant_future>`\n"
			"\n"
			"  The effect of this function is to replace the present day geometries in each feature in "
			"*reconstructable_features* with reverse reconstructed versions of those geometries. "
			"This assumes that the original geometries, stored in *reconstructable_features*, are not "
			"in fact present day geometries (as they normally should be) but instead the "
			"already-reconstructed geometries corresponding to geological time *reconstruction_time*. "
			"This function reverses that reconstruction process to ensure present day geometries are "
			"stored in the features.\n"
			"\n"
			"  Note that *reconstructable_features* can be a :class:`FeatureCollection` or a filename "
			"or a feature or a sequence of features, or a sequence (eg, ``list`` or ``tuple``) of any "
			"combination of those four types.\n"
			"\n"
			"  If any filenames are specified in *reconstructable_features* then the modified feature "
			"collection(s) (containing reverse reconstructed geometries) that are associated with those "
			"files are written back out to those same files. :class:`FeatureCollectionFileFormatRegistry` "
			"is used internally to read/write feature collections from/to those files.\n"
			"\n"
			"  Note that *rotation_model* can be either a :class:`RotationModel` or a "
			"rotation :class:`FeatureCollection` or a rotation filename or a sequence "
			"(eg, ``list`` or ``tuple``) containing rotation :class:`FeatureCollection` instances "
			"or filenames (or a mixture of both). When a :class:`RotationModel` is not specified "
			"then a temporary one is created internally (and hence is less efficient if this "
			"function is called multiple times with the same rotation data).\n"
			"\n"
			"  Reverse reconstructing a file containing a feature collection from 10Ma:\n"
			"  ::\n"
			"\n"
            "    pygplates.reverse_reconstruct('volcanoes.gpml', rotation_model, 10)\n"
			"\n"
			"  Reverse reconstructing a feature collection from 10Ma:\n"
			"  ::\n"
			"\n"
			"    pygplates.reverse_reconstruct(pygplates.FeatureCollection([feature1, feature2]), rotation_model, 10)\n"
			"\n"
			"  Reverse reconstructing a list of features from 10Ma:\n"
			"  ::\n"
			"\n"
			"    pygplates.reverse_reconstruct([feature1, feature2], rotation_model, 10)\n"
			"\n"
			"  Reconstructing a single feature from 10Ma:\n"
			"  ::\n"
			"\n"
            "    pygplates.reconstruct(feature, rotation_model, 10)\n");
}

#endif
