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

#include <QString>

#include "PythonUtils.h"

#include "app-logic/ReconstructUtils.h"

#include "data-mining/DataMiningUtils.h"

#include "feature-visitors/GeometrySetter.h"

#include "file-io/File.h"
#include "file-io/ReconstructedFeatureGeometryExport.h"
#include "file-io/FeatureCollectionFileFormatRegistry.h"

#include "global/python.h"

#include "model/Gpgim.h"

#include <boost/foreach.hpp>

#if !defined(GPLATES_NO_PYTHON)
namespace bp = boost::python;
namespace utils = GPlatesDataMining::DataMiningUtils;

namespace
{
	const std::vector<QString>
	to_str_vector(const bp::list& objs)
	{
		std::vector<QString> ret;
		bp::ssize_t n = bp::len(objs);
		for(bp::ssize_t i=0; i<n; i++) 
		{
			ret.push_back(QString(bp::extract<const char*>(objs[i])));
		}
		return ret;
	}

	GPlatesFileIO::ReconstructedFeatureGeometryExport::Format
	get_format(QString file_name)
	{
		//it seems currently only support xy and shp.
		QString f_type = file_name.right(file_name.lastIndexOf('.'));
		if(f_type.contains("xy"))
			return GPlatesFileIO::ReconstructedFeatureGeometryExport::GMT;
		if(f_type.contains("shp"))
			return GPlatesFileIO::ReconstructedFeatureGeometryExport::SHAPEFILE;
		if(f_type.contains("gmt"))
			return GPlatesFileIO::ReconstructedFeatureGeometryExport::OGRGMT;
		else
			return  GPlatesFileIO::ReconstructedFeatureGeometryExport::UNKNOWN;
	}

	void
	recontruct(
			bp::list recon_files,
			bp::list rot_files,
			bp::object time,
			bp::object anchor_plate_id,
			bp::object export_file_name)
	{
		using namespace GPlatesFileIO;

		std::vector<File::non_null_ptr_type> p_rot_files, p_recon_files;
		
		const std::vector<QString> s_rot_files = to_str_vector(rot_files);
		const std::vector<QString> s_recon_files = to_str_vector(recon_files);
	
		boost::scoped_ptr<FeatureCollectionFileFormat::Registry> registry(new FeatureCollectionFileFormat::Registry());
		
		GPlatesModel::ModelInterface  model;
		GPlatesModel::Gpgim::non_null_ptr_to_const_type gpgim = GPlatesModel::Gpgim::create();
		register_default_file_formats(*registry, model, *gpgim);
		
		std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> recon_fc = 
			utils::load_files(s_recon_files, p_recon_files,*registry);
		std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> rot_fc = 
			utils::load_files(s_rot_files, p_rot_files,*registry);

		double recon_time = bp::extract<double>(time);
		unsigned long anchor_pid = bp::extract<unsigned long>(anchor_plate_id);

		std::vector<GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_type> rfgs;
		GPlatesAppLogic::ReconstructUtils::reconstruct(
				rfgs,
				recon_time,
				anchor_pid,
				recon_fc,
				rot_fc);

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
		std::vector<const File::Reference *> reconstructable_file_ptrs;
		std::vector<File::non_null_ptr_type>::const_iterator file_iter = p_recon_files.begin();
		std::vector<File::non_null_ptr_type>::const_iterator file_end = p_recon_files.end();
		for ( ; file_iter != file_end; ++file_iter)
		{
			reconstructable_file_ptrs.push_back(&(*file_iter)->get_reference());
		}
		
		QString export_file_name_q = QString(bp::extract<const char *>(export_file_name));

		ReconstructedFeatureGeometryExport::Format format = get_format(export_file_name_q);

		// Export the reconstructed feature geometries.
		ReconstructedFeatureGeometryExport::export_reconstructed_feature_geometries(
					export_file_name_q,
					format,
					rfgs_p,
					reconstructable_file_ptrs,
					anchor_pid,
					recon_time,
					true/*export_single_output_file*/,
					false/*export_per_input_file*/,
					false/*export_separate_output_directory_per_input_file*/);
	}


	/**
	 * Loads reconstructable features from files @a python_reconstructable_filenames and assumes
	 * each feature geometry is *not* present day geometry but instead is the reconstructed geometry
	 * for the specified time @a python_time.
	 *
	 * The reconstructed geometries of each reconstructable feature are reverse reconstructed to
	 * present day, stored back in the features and saved to files (one output file per input
	 * reconstructable file) with...
	 *    <python_output_file_basename_suffix> + '.' + <python_output_file_format>
	 * ...appended to each corresponding input reconstructable file basename.
	 *
	 * @a python_time is the time representing the reconstructed geometries in each feature.
	 * @a python_reconstruction_filenames contains the reconstruction/rotation features used to
	 * perform the reverse reconstruction.
	 */
	void
	reverse_recontruct(
			bp::list python_reconstructable_filenames,
			bp::list python_reconstruction_filenames,
			bp::object python_time,
			bp::object python_anchor_plate_id,
			bp::object python_output_file_basename_suffix,
			bp::object python_output_file_format)
	{
		const std::vector<QString> reconstruction_filenames = to_str_vector(python_reconstruction_filenames);
		const std::vector<QString> reconstructable_filenames = to_str_vector(python_reconstructable_filenames);

		const double time = bp::extract<double>(python_time);
		const unsigned long anchor_plate_id = bp::extract<unsigned long>(python_anchor_plate_id);
		const QString output_file_format = QString(bp::extract<const char *>(python_output_file_format));
		const QString output_file_basename_suffix = QString(bp::extract<const char *>(python_output_file_basename_suffix));

		GPlatesFileIO::FeatureCollectionFileFormat::Registry feature_collection_file_format_registry;
		GPlatesModel::ModelInterface model;
		GPlatesModel::Gpgim::non_null_ptr_to_const_type gpgim = GPlatesModel::Gpgim::create();
		register_default_file_formats(feature_collection_file_format_registry, model, *gpgim);

		std::vector<GPlatesFileIO::File::non_null_ptr_type> reconstruction_files;
		std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> reconstruction_feature_collections = 
				utils::load_files(reconstruction_filenames, reconstruction_files, feature_collection_file_format_registry);

		std::vector<GPlatesFileIO::File::non_null_ptr_type> reconstructable_files;
		std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> reconstructable_feature_collections = 
				utils::load_files(reconstructable_filenames, reconstructable_files, feature_collection_file_format_registry);

		GPlatesAppLogic::ReconstructMethodRegistry reconstruct_method_registry;
		register_default_reconstruct_method_types(reconstruct_method_registry);

		const GPlatesAppLogic::ReconstructionTreeCreator reconstruction_tree_creator =
				GPlatesAppLogic::get_cached_reconstruction_tree_creator(
						reconstruction_feature_collections,
						time/*default_reconstruction_time*/,
						anchor_plate_id/*default_anchor_plate_id*/);

		// Iterate over the reconstructable files.
		for (unsigned int reconstructable_file_index = 0;
			reconstructable_file_index < reconstructable_filenames.size();
			++reconstructable_file_index)
		{
			const GPlatesModel::FeatureCollectionHandle::weak_ref &reconstructable_feature_collection =
					reconstructable_feature_collections[reconstructable_file_index];

			// Iterate over the features in the current feature collection.
			GPlatesModel::FeatureCollectionHandle::iterator reconstructable_features_iter =
					reconstructable_feature_collection->begin();
			GPlatesModel::FeatureCollectionHandle::iterator reconstructable_features_end =
					reconstructable_feature_collection->end();
			for ( ; reconstructable_features_iter != reconstructable_features_end; ++reconstructable_features_iter)
			{
				const GPlatesModel::FeatureHandle::weak_ref reconstructable_feature_ref =
						(*reconstructable_features_iter)->reference();

				// Find out how to reconstruct each geometry in a feature based on the feature's other properties.
				const GPlatesAppLogic::ReconstructMethod::Type reconstruct_method_type =
						reconstruct_method_registry.get_reconstruct_method_type_or_default(reconstructable_feature_ref);

				// Get the reconstruct method so we can reverse reconstruct the geometry.
				GPlatesAppLogic::ReconstructMethodInterface::non_null_ptr_type reconstruct_method =
						reconstruct_method_registry.get_reconstruct_method(reconstruct_method_type);

				// Get the (reconstructed - not present day) geometries for the current feature.
				//
				// NOTE: We are actually going to treat these geometries *not* as present day
				// but as geometries at time 'time' - we're going to reverse reconstruct to get
				// the present day geometries.
				// Note: There should be one geometry for each geometry property that can be reconstructed.
				std::vector<GPlatesAppLogic::ReconstructMethodInterface::Geometry> feature_reconstructed_geometries;
				reconstruct_method->get_present_day_geometries(
						feature_reconstructed_geometries,
						reconstructable_feature_ref);

				// Iterate over the reconstructed geometries for the current feature.
				BOOST_FOREACH(
						const GPlatesAppLogic::ReconstructMethodInterface::Geometry &feature_reconstructed_geometry,
						feature_reconstructed_geometries)
				{
					// Reverse reconstruct the current feature geometry from time 'time' to present day.
					GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type present_day_geometry =
							reconstruct_method->reconstruct_geometry(
									feature_reconstructed_geometry.geometry,
									reconstructable_feature_ref,
									reconstruction_tree_creator,
									time/*reconstruction_time*/, // The time of the reconstructed feature geometry.
									true/*reverse_reconstruct*/);

					// Set the reverse reconstructed (present day) geometry back onto the feature's geometry property.
					GPlatesModel::TopLevelProperty::non_null_ptr_type geom_top_level_prop_clone = 
							(*feature_reconstructed_geometry.property_iterator)->deep_clone();
					GPlatesFeatureVisitors::GeometrySetter(present_day_geometry).set_geometry(
							geom_top_level_prop_clone.get());
					*feature_reconstructed_geometry.property_iterator = geom_top_level_prop_clone;
				}
			}

			const QString &reconstructable_input_filename =
					reconstructable_filenames[reconstructable_file_index];
			const QString reconstructable_input_file_basename =
					reconstructable_input_filename.left(reconstructable_input_filename.lastIndexOf('.'));
			const QString reconstructable_output_filename =
					reconstructable_input_file_basename + output_file_basename_suffix + '.' + output_file_format;

			// Create an output file to write back out the current modified feature collection.
			GPlatesFileIO::File::Reference::non_null_ptr_type output_file_ref =
					GPlatesFileIO::File::create_file_reference(
							GPlatesFileIO::FileInfo(reconstructable_output_filename),
							reconstructable_feature_collection);

			// Save the modified feature collection to file.
			feature_collection_file_format_registry.write_feature_collection(*output_file_ref);
		}
	}
}

	
void
export_functions()
{
	bp::def("reconstruct", &recontruct);
	bp::def("reverse_reconstruct", &reverse_recontruct);
}
#endif







