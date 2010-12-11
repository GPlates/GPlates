/* $Id: ShapefileFormatReconstructedFeatureGeometryExport.cc 8194 2010-04-26 16:24:42Z rwatson $ */

/**
 * \file Exports flowlines to shapefile format.
 * $Revision: 8194 $
 * $Date: 2010-04-26 18:24:42 +0200 (ma, 26 apr 2010) $
 * 
 * Copyright (C) 2010 Geological Survey of Norway
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

#include <QDebug>
#include <QFile>
#include <QString>

#include "File.h"
#include "FileInfo.h"
#include "ShapefileFormatMotionPathExport.h"
#include "ShapefileGeometryExporter.h"

#include "app-logic/MotionPathUtils.h"
#include "app-logic/ReconstructedMotionPath.h"
#include "maths/PolylineOnSphere.h"
#include "property-values/GpmlIrregularSampling.h"
#include "property-values/GpmlKeyValueDictionary.h"
#include "property-values/GpmlTimeSample.h"
#include "property-values/XsDouble.h"
#include "property-values/XsInteger.h"



namespace
{
	QString
	make_seed_string(
		GPlatesAppLogic::ReconstructedMotionPath::seed_point_geom_ptr_type seed_point)
	{

		QString output;

		GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(*seed_point);		

		output += "(";
		output += QString::number(llp.latitude());
		output += ",";
		output += QString::number(llp.longitude());
		output += "),";

		output.chop(1);
		return output;
	}

	void
	get_export_times(
		std::vector<double> &export_times,
		const std::vector<double> &times,
		const double &reconstruction_time)
	{
		std::vector<double>::const_iterator 
			it = times.begin(),
			end = times.end();
			
		while ((it != end) && (*it <= reconstruction_time))
		{
			++it;
		}	
		export_times.push_back(reconstruction_time);
		while (it != end)
		{
			export_times.push_back(*it);
			++it;
		}
	}


	
	
	/**
	 * Fill a kvd with data describing how the motion_paths were generated.
	 *
	 * This will be exported as shapefile attributes.
	 */
	GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type
	create_kvd_from_feature(
		const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref,		
		const GPlatesFileIO::ReconstructedMotionPathExportImpl::referenced_files_collection_type &referenced_files,	
		const double &reconstruction_time,
		const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
		const GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type &seed_point,
		bool should_add_referenced_files = true)
	{
		using namespace GPlatesPropertyValues;

		GPlatesAppLogic::MotionPathUtils::MotionPathPropertyFinder finder;
		finder.visit_feature(feature_ref);


		GpmlKeyValueDictionary::non_null_ptr_type dictionary = 
			GpmlKeyValueDictionary::create();

		// (Shapefile attribute fields are limited to 10 characters in length).

		// Feature name
		XsString::non_null_ptr_type key = XsString::create("NAME");
		XsString::non_null_ptr_type name_value = 
			XsString::create(GPlatesUtils::make_icu_string_from_qstring(finder.get_name()));	

		GpmlKeyValueDictionaryElement name_element(
			key,
			name_value,
			TemplateTypeParameterType::create_xsi("string"));
		dictionary->elements().push_back(name_element);	

		// Seed points.
		key = XsString::create("SEED");
		QString seed_string = make_seed_string(seed_point);
		XsString::non_null_ptr_type motion_path_seeds_value =
			XsString::create(GPlatesUtils::make_icu_string_from_qstring(seed_string));

		GpmlKeyValueDictionaryElement motion_path_seeds_element(
			key,
			motion_path_seeds_value,
			TemplateTypeParameterType::create_xsi("string"));
		dictionary->elements().push_back(motion_path_seeds_element);

		// Anchor plate.
		key = XsString::create("ANCHOR");
		XsInteger::non_null_ptr_type anchor_value = 
			XsInteger::create(reconstruction_anchor_plate_id);	

		GpmlKeyValueDictionaryElement anchor_element(
			key,
			anchor_value,
			TemplateTypeParameterType::create_xsi("integer"));
		dictionary->elements().push_back(anchor_element);	

		// Reconstruction time.
		key = XsString::create("TIME");
		XsDouble::non_null_ptr_type time_value = 
			XsDouble::create(reconstruction_time);	

		GpmlKeyValueDictionaryElement time_element(
			key,
			time_value,
			TemplateTypeParameterType::create_xsi("double"));
		dictionary->elements().push_back(time_element);	

		if (should_add_referenced_files)
		{
			// Referenced file(s).
			// As this info is output on a geometry by geometry basis (there's no place in a shapefile 
			// for global attributes...) I could give each geometry its correct file, rather 
			// then write out the whole list. For now at least I'll just export the entire list for
			// each geometry. 

			// Attribute field names will have the form "FILE1", "FILE2" etc...
			QString file_string("FILE");

			int file_count = 1;
			GPlatesFileIO::ShapefileFormatMotionPathExport::referenced_files_collection_type::const_iterator file_iter;
			for (file_iter = referenced_files.begin();
				file_iter != referenced_files.end();
				++file_iter, ++file_count)
			{
				const GPlatesFileIO::File::Reference *file = *file_iter;

				QString count_string = QString("%1").arg(file_count);
				QString field_name = file_string + count_string;

				// Some files might not actually exist yet if the user created a new
				// feature collection internally and hasn't saved it to file yet.
				if (!GPlatesFileIO::file_exists(file->get_file_info()))
				{
					continue;
				}

				QString filename = file->get_file_info().get_display_name(false/*use_absolute_path_name*/);

				key = XsString::create(GPlatesUtils::make_icu_string_from_qstring(field_name));
				XsString::non_null_ptr_type file_value = 
					XsString::create(GPlatesUtils::make_icu_string_from_qstring(filename));

				GpmlKeyValueDictionaryElement element(
					key,
					file_value,
					TemplateTypeParameterType::create_xsi("string"));
				dictionary->elements().push_back(element);	
			}
		}

		return dictionary;

	}
}


void
GPlatesFileIO::ShapefileFormatMotionPathExport::export_motion_paths(
		const motion_path_group_seq_type &motion_path_group_seq,
		const QFileInfo& file_info,
		const referenced_files_collection_type &referenced_files,
		const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
		const double &reconstruction_time,
		bool should_export_referenced_files)
{

	QString file_path = file_info.filePath();
	ShapefileGeometryExporter exporter(file_path,false /* single geometry types */);

	motion_path_group_seq_type::const_iterator
		iter = motion_path_group_seq.begin(),
		end = motion_path_group_seq.end();

	for (; iter != end ; ++iter)
	{

		// Get per-feature stuff: feature info, left/right plates, times.
		const ReconstructedMotionPathExportImpl::MotionPathGroup &motion_path_group =
			*iter;

		const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref =
			motion_path_group.feature_ref;

		if (!feature_ref.is_valid())
		{
			continue;
		}

		ReconstructedMotionPathExportImpl::reconstructed_motion_path_seq_type::const_iterator rmt_iter;
		for (rmt_iter = motion_path_group.recon_motion_paths.begin();
			rmt_iter != motion_path_group.recon_motion_paths.end();
			++rmt_iter)
		{
			const GPlatesAppLogic::ReconstructedMotionPath *rmt = *rmt_iter;

			GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type kvd
				= create_kvd_from_feature(
					feature_ref,
					referenced_files,
					reconstruction_anchor_plate_id,
					reconstruction_time,
					rmt->seed_point(),
					should_export_referenced_files);

			exporter.export_geometry(rmt->motion_path_points(),kvd);

		}
	}	
}

