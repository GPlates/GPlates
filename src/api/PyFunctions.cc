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
#include "boost/foreach.hpp"

#include "PythonUtils.h"

#include "app-logic/ReconstructUtils.h"
#include "data-mining/DataMiningUtils.h"
#include "file-io/File.h"
#include "file-io/ReconstructedFeatureGeometryExport.h"
#include "file-io/FeatureCollectionFileFormatRegistry.h"
#include "global/python.h"

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
		register_default_file_formats(*registry,model);
		
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
					false/*export_per_input_file*/);
		return;
	}
}

	
void
export_functions()
{
	bp::def("reconstruct",&recontruct);
}
#endif







