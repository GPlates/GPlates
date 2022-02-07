/* $Id$ */

/**
 * \file Exports flowlines to shapefile format.
 * $Revision$
 * $Date$
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
#include "OgrFormatMotionPathExport.h"
#include "OgrGeometryExporter.h"
#include "OgrUtils.h"

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
	//! Typedef for a sequence of referenced files.
	typedef GPlatesFileIO::OgrFormatMotionPathExport::referenced_files_collection_type
			referenced_files_collection_type;

	//! Convenience typedef for a sequence of @a ReconstructedMotionPath objects.
	typedef std::vector<const GPlatesAppLogic::ReconstructedMotionPath *>
			reconstructed_motion_path_seq_type;


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

	
	
	/**
	 * Fill a kvd with data describing how the motion_paths were generated.
	 *
	 * This will be exported as shapefile attributes.
	 */
	GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type
	create_kvd_from_feature(
		const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref,		
		const referenced_files_collection_type &referenced_files,	
		const referenced_files_collection_type &reconstruction_files,
		const double &reconstruction_time,
		const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
		const GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type &seed_point,
		bool should_add_referenced_files = true)
	{
		using namespace GPlatesPropertyValues;

		GPlatesAppLogic::MotionPathUtils::MotionPathPropertyFinder finder;
		finder.visit_feature(feature_ref);


		GpmlKeyValueDictionary::non_null_ptr_type dictionary = GpmlKeyValueDictionary::create();

		// (Shapefile attribute fields are limited to 10 characters in length).

		// Feature name
		XsString::non_null_ptr_type key = XsString::create("NAME");
		XsString::non_null_ptr_type name_value = 
			XsString::create(GPlatesUtils::make_icu_string_from_qstring(finder.get_name()));	

		GpmlKeyValueDictionaryElement::non_null_ptr_type name_element =
				GpmlKeyValueDictionaryElement::create(
						key,
						name_value,
						StructuralType::create_xsi("string"));
		dictionary->elements().push_back(name_element);	

		// Seed points.
		key = XsString::create("SEED");
		QString seed_string = make_seed_string(seed_point);
		XsString::non_null_ptr_type motion_path_seeds_value =
			XsString::create(GPlatesUtils::make_icu_string_from_qstring(seed_string));

		GpmlKeyValueDictionaryElement::non_null_ptr_type motion_path_seeds_element =
				GpmlKeyValueDictionaryElement::create(
						key,
						motion_path_seeds_value,
						StructuralType::create_xsi("string"));
		dictionary->elements().push_back(motion_path_seeds_element);

		// Anchor plate.
		key = XsString::create("ANCHOR");
		XsInteger::non_null_ptr_type anchor_value = 
			XsInteger::create(reconstruction_anchor_plate_id);	

		GpmlKeyValueDictionaryElement::non_null_ptr_type anchor_element =
				GpmlKeyValueDictionaryElement::create(
						key,
						anchor_value,
						StructuralType::create_xsi("integer"));
		dictionary->elements().push_back(anchor_element);	

		// Reconstruction time.
		key = XsString::create("TIME");
		XsDouble::non_null_ptr_type time_value = 
			XsDouble::create(reconstruction_time);	

		GpmlKeyValueDictionaryElement::non_null_ptr_type time_element =
				GpmlKeyValueDictionaryElement::create(
						key,
						time_value,
						StructuralType::create_xsi("double"));
		dictionary->elements().push_back(time_element);	

		if (should_add_referenced_files)
		{
			GPlatesFileIO::OgrUtils::add_filename_sequence_to_kvd(
						QString("FILE"),
						referenced_files,
						dictionary);

			GPlatesFileIO::OgrUtils::add_filename_sequence_to_kvd(
						QString("RECONFILE"),
						reconstruction_files,
						dictionary);
		}


		return dictionary;

	}
}


void
GPlatesFileIO::OgrFormatMotionPathExport::export_motion_paths(
		const std::list<feature_geometry_group_type> &feature_geometry_group_seq,
		const QFileInfo& file_info,
		const referenced_files_collection_type &referenced_files,
		const referenced_files_collection_type &active_reconstruction_files,
		const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
		const double &reconstruction_time,
		bool should_export_referenced_files,
		bool wrap_to_dateline)
{

	QString file_path = file_info.filePath();
	OgrGeometryExporter exporter(file_path,false /* single geometry types */,wrap_to_dateline);

	std::list<feature_geometry_group_type>::const_iterator
		iter = feature_geometry_group_seq.begin(),
		end = feature_geometry_group_seq.end();

	for (; iter != end ; ++iter)
	{

		// Get per-feature stuff: feature info, left/right plates, times.
		const feature_geometry_group_type &motion_path_group = *iter;

		const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref =
			motion_path_group.feature_ref;

		if (!feature_ref.is_valid())
		{
			continue;
		}

		reconstructed_motion_path_seq_type::const_iterator rmt_iter;
		for (rmt_iter = motion_path_group.recon_geoms.begin();
			rmt_iter != motion_path_group.recon_geoms.end();
			++rmt_iter)
		{
			const GPlatesAppLogic::ReconstructedMotionPath *rmt = *rmt_iter;

			GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type kvd
				= create_kvd_from_feature(
					feature_ref,
					referenced_files,
					active_reconstruction_files,
					reconstruction_time,
					reconstruction_anchor_plate_id,
					rmt->present_day_seed_point(),
					should_export_referenced_files);

			exporter.export_geometry(rmt->motion_path_points(),kvd);

		}
	}	
}

