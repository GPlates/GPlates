/* $Id: OGRFormatReconstructedFeatureGeometryExport.cc -1   $ */

/**
 * \file Exports reconstructed feature geometries to a GMT format file.
 * $Revision: -1 $
 * $Date: $
 * 
 * Copyright (C) 2009 Geological Survey of Norway
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
#include <QStringList>
#include <QString>
#include <QTextStream>

#include "ErrorOpeningFileForWritingException.h"
#include "FileInfo.h"
#include "OgrFormatReconstructedFeatureGeometryExport.h"
#include "OgrGeometryExporter.h"
#include "OgrUtils.h"

#include "app-logic/ReconstructedFeatureGeometry.h"
#include "feature-visitors/GeometryTypeFinder.h"
#include "feature-visitors/KeyValueDictionaryFinder.h"
#include "feature-visitors/PropertyValueFinder.h"
#include "property-values/GpmlKeyValueDictionary.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/XsDouble.h"
#include "property-values/XsInteger.h"


namespace
{
	//! Convenience typedef for referenced files.
	typedef GPlatesFileIO::OgrFormatReconstructedFeatureGeometryExport::referenced_files_collection_type
			referenced_files_collection_type;

	//! Convenience typedef for a sequence of RFGs.
	typedef std::vector<const GPlatesAppLogic::ReconstructedFeatureGeometry *>
			reconstructed_feature_geom_seq_type;

	/*!
	 * Returns true if the feature-type of @a feature_ref is either
	 * flowline or motion path.
	 */
	bool
	feature_is_of_type_to_exclude(
			const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref)
	{
		static const GPlatesModel::FeatureType flowline_feature_type =
				GPlatesModel::FeatureType::create_gpml("Flowline");
		static const GPlatesModel::FeatureType motion_path_feature_type =
				GPlatesModel::FeatureType::create_gpml("MotionPath");

		if ((feature_ref->feature_type() == flowline_feature_type) ||
				(feature_ref->feature_type() == motion_path_feature_type))
		{
			return true;
		}
		return false;
	}


	GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type
	create_kvd_from_feature(
		const GPlatesModel::FeatureHandle::const_weak_ref &feature,
		const referenced_files_collection_type &referenced_files,
		const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
		const double &reconstruction_time	)
	{

		// FIXME: Consider exporting fields from the original feature's kvd too. This could get
		// complicated if features came from shapefiles with different attribute fields. 
		// For now, I'm just adding plateID, anchor plate, time, and referenced files to the kvd. 
	
		std::vector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement> elements;

		static const GPlatesModel::PropertyName plate_id_property_name =
			GPlatesModel::PropertyName::create_gpml("reconstructionPlateId");

		const GPlatesPropertyValues::GpmlPlateId *recon_plate_id;

		// If we found a plate id, add it. 
		if (GPlatesFeatureVisitors::get_property_value(feature,plate_id_property_name,recon_plate_id))
		{
			// Shapefile attribute field names are limited to 10 characters in length 
			// and should not contain spaces.
			GPlatesPropertyValues::XsString::non_null_ptr_type key = 
				GPlatesPropertyValues::XsString::create("PLATE_ID");
			GPlatesPropertyValues::XsInteger::non_null_ptr_type plateid_value = 
				GPlatesPropertyValues::XsInteger::create(recon_plate_id->get_value());	

			GPlatesPropertyValues::GpmlKeyValueDictionaryElement element(
				key,
				plateid_value,
				GPlatesPropertyValues::StructuralType::create_xsi("integer"));
			elements.push_back(element);
		}

		// Anchor plate.

		// (Shapefile attribute fields are limited to 10 characters in length)
		GPlatesPropertyValues::XsString::non_null_ptr_type key = 
			GPlatesPropertyValues::XsString::create("ANCHOR");
		GPlatesPropertyValues::XsInteger::non_null_ptr_type anchor_value = 
			GPlatesPropertyValues::XsInteger::create(reconstruction_anchor_plate_id);	

		GPlatesPropertyValues::GpmlKeyValueDictionaryElement anchor_element(
			key,
			anchor_value,
			GPlatesPropertyValues::StructuralType::create_xsi("integer"));
		elements.push_back(anchor_element);	

		// Reconstruction time.
		key = GPlatesPropertyValues::XsString::create("TIME");
		GPlatesPropertyValues::XsDouble::non_null_ptr_type time_value = 
			GPlatesPropertyValues::XsDouble::create(reconstruction_time);	

		GPlatesPropertyValues::GpmlKeyValueDictionaryElement time_element(
			key,
			time_value,
			GPlatesPropertyValues::StructuralType::create_xsi("double"));
		elements.push_back(time_element);	

		// Referenced files.
		// As this info is output on a geometry by geometry basis (there's no place in a shapefile 
		// for global attributes...) I could give each geometry its correct file, rather 
		// then write out the whole list. For now I'm going to write out the
		// whole list, so at least we're consistent with the GMT export. 

		// Attribute field names will have the form "FILE1", "FILE2" etc...
		QString file_string("FILE");

		int file_count = 1;
		referenced_files_collection_type::const_iterator file_iter;
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

			key = GPlatesPropertyValues::XsString::create(GPlatesUtils::make_icu_string_from_qstring(field_name));
			GPlatesPropertyValues::XsString::non_null_ptr_type file_value = 
				GPlatesPropertyValues::XsString::create(GPlatesUtils::make_icu_string_from_qstring(filename));

			GPlatesPropertyValues::GpmlKeyValueDictionaryElement element(
				key,
				file_value,
				GPlatesPropertyValues::StructuralType::create_xsi("string"));
			elements.push_back(element);	
		}


		return GPlatesPropertyValues::GpmlKeyValueDictionary::create(elements);

	}

	void
	add_feature_fields_to_kvd(
		GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type &output_kvd,
		const GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type &feature_kvd)
	{
		std::vector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement> output_elements =
				output_kvd->get_elements();

		std::vector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement>::const_iterator 
			it = feature_kvd->get_elements().begin(),
			end = feature_kvd->get_elements().end();
		for (; it != end ; ++it)
		{
			output_elements.push_back(*it);
		}

		output_kvd->set_elements(output_elements);
	}
		
}


void
GPlatesFileIO::OgrFormatReconstructedFeatureGeometryExport::export_geometries(
		const std::list<feature_geometry_group_type> &feature_geometry_group_seq,
		const QFileInfo& file_info,
		const referenced_files_collection_type &referenced_files,
		const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
		const double &reconstruction_time,
		bool wrap_to_dateline)
{

	// Iterate through the reconstructed geometries and check which geometry types we have.
	GPlatesFeatureVisitors::GeometryTypeFinder finder;

	std::list<feature_geometry_group_type>::const_iterator feature_iter;
	for (feature_iter = feature_geometry_group_seq.begin();
		feature_iter != feature_geometry_group_seq.end();
		++feature_iter)
	{
		const feature_geometry_group_type &feature_geom_group = *feature_iter;

		const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref =
			feature_geom_group.feature_ref;
		if (!feature_ref.is_valid())
		{
			continue;
		}


		// We will exclude export of flowline/motion-path seed points, so
		// don't include them in this geometry-type check either.
		if (feature_is_of_type_to_exclude(feature_ref))
		{
			continue;
		}

		// Iterate through the reconstructed geometries of the current feature.
		reconstructed_feature_geom_seq_type::const_iterator rfg_iter;
		for (rfg_iter = feature_geom_group.recon_geoms.begin();
			rfg_iter != feature_geom_group.recon_geoms.end();
			++rfg_iter)
		{
			const GPlatesAppLogic::ReconstructedFeatureGeometry *rfg = *rfg_iter;
			rfg->reconstructed_geometry()->accept_visitor(finder);
		}
	}

	// Set up the appropriate form of OgrGeometryExporter.
	QString file_path = file_info.filePath();
	GPlatesFileIO::OgrGeometryExporter geom_exporter(
		file_path,
		finder.has_found_multiple_geometry_types(),
		wrap_to_dateline);



	// Iterate through the reconstructed geometries and write to output.
	for (feature_iter = feature_geometry_group_seq.begin();
		feature_iter != feature_geometry_group_seq.end();
		++feature_iter)
	{

		const feature_geometry_group_type &feature_geom_group = *feature_iter;

		const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref =
				feature_geom_group.feature_ref;

		if (!feature_ref.is_valid())
		{
			continue;
		}

		// Prevents us from exporting flowline/motion-path seed points.
		if (feature_is_of_type_to_exclude(feature_ref))
		{
			continue;
		}


		GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type kvd_for_export =
			GPlatesPropertyValues::GpmlKeyValueDictionary::create();

		OgrUtils::add_reconstruction_fields_to_kvd(kvd_for_export,
										reconstruction_anchor_plate_id,
										reconstruction_time);

		OgrUtils::add_referenced_files_to_kvd(kvd_for_export,referenced_files);

		OgrUtils::add_standard_properties_to_kvd(
					feature_ref,kvd_for_export);



#if 0		
		GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type kvd =
			create_kvd_from_feature(feature_ref,
									referenced_files,
									reconstruction_anchor_plate_id,
									reconstruction_time);
#endif
		// Iterate through the reconstructed geometries of the current feature and write to output.
		// Note that this will export each geometry as a separate entry in the shapefile, even if they
		// come from the same feature. 
		reconstructed_feature_geom_seq_type::const_iterator rfg_iter;
		for (rfg_iter = feature_geom_group.recon_geoms.begin();
			rfg_iter != feature_geom_group.recon_geoms.end();
			++rfg_iter)
		{
			const GPlatesAppLogic::ReconstructedFeatureGeometry *rfg = *rfg_iter;

			// Write the reconstructed geometry.
			geom_exporter.export_geometry(rfg->reconstructed_geometry(),kvd_for_export); 
		}
	}

}

void
GPlatesFileIO::OgrFormatReconstructedFeatureGeometryExport::export_geometries_per_collection(
		const std::list<feature_geometry_group_type> &feature_geometry_group_seq,
		const QFileInfo& file_info,
		const referenced_files_collection_type &referenced_files,
		const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
		const double &reconstruction_time,
		bool wrap_to_dateline)
{



	// Iterate through the reconstructed geometries and check which geometry types we have.
	GPlatesFeatureVisitors::GeometryTypeFinder finder;

	std::list<feature_geometry_group_type>::const_iterator feature_iter;
	for (feature_iter = feature_geometry_group_seq.begin();
		feature_iter != feature_geometry_group_seq.end();
		++feature_iter)
	{
		const feature_geometry_group_type &feature_geom_group = *feature_iter;

		const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref =
			feature_geom_group.feature_ref;
		if (!feature_ref.is_valid())
		{
			continue;
		}

		// We will exclude export of flowline/motion-path seed points, so
		// don't include them in this geometry-type check either.
		if (feature_is_of_type_to_exclude(feature_ref))
		{
			continue;
		}


		// Iterate through the reconstructed geometries of the current feature.
		reconstructed_feature_geom_seq_type::const_iterator rfg_iter;
		for (rfg_iter = feature_geom_group.recon_geoms.begin();
			rfg_iter != feature_geom_group.recon_geoms.end();
			++rfg_iter)
		{
			const GPlatesAppLogic::ReconstructedFeatureGeometry *rfg = *rfg_iter;
			rfg->reconstructed_geometry()->accept_visitor(finder);
		}
	}

	// Set up the appropriate form of ShapefileGeometryExporter.
	QString file_path = file_info.filePath();
	GPlatesFileIO::OgrGeometryExporter geom_exporter(
		file_path,
		finder.has_found_multiple_geometry_types(),
		wrap_to_dateline);



	// Iterate through the reconstructed geometries and write to output.
	for (feature_iter = feature_geometry_group_seq.begin();
		feature_iter != feature_geometry_group_seq.end();
		++feature_iter)
	{

		const feature_geometry_group_type &feature_geom_group = *feature_iter;

		const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref =
				feature_geom_group.feature_ref;

		if (!feature_ref.is_valid())
		{
			continue;
		}

		// Prevents us from exporting flowline/motion-path seed points.
		if (feature_is_of_type_to_exclude(feature_ref))
		{
			continue;
		}


		GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type kvd_for_export =
			GPlatesPropertyValues::GpmlKeyValueDictionary::create();

		OgrUtils::add_reconstruction_fields_to_kvd(kvd_for_export,
												   reconstruction_anchor_plate_id,
												   reconstruction_time);

		OgrUtils::add_referenced_files_to_kvd(kvd_for_export,referenced_files);

		GPlatesFeatureVisitors::KeyValueDictionaryFinder kvd_finder;
		kvd_finder.visit_feature(feature_ref);
		if (kvd_finder.number_of_found_dictionaries() != 0)
		{
			// FIXME: Model values which have been updated (e.g. plate id) won't have been copied to the
			// kvd, so these exported values might be "old". We should approach this in a way similar to the 
			// OgrFeatureCollectionWriter which updates the kvd (based on the attribute-to-model map) prior
			// to export.
			// And if we *don't* find a kvd, we should generate a default one using the standard set of mapped
			// attributes.
			GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type found_kvd =
				*(kvd_finder.found_key_value_dictionaries_begin());
			add_feature_fields_to_kvd(kvd_for_export,found_kvd);
		}
		else
		{
			//FIXME: if the features being exported don't all have the standard
			// set of properties, then we could end up with some gaps in the
			// kvds, and so the exported kvds could be out of sync with the
			// field names.
			// To fix this we should define a standard kvd first, fill with default values,
			// then replace the values as we find them in each feature.
			OgrUtils::add_standard_properties_to_kvd(feature_ref,kvd_for_export);
		}



#if 0		
		GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type kvd =
			create_kvd_from_feature(feature_ref,
									referenced_files,
									reconstruction_anchor_plate_id,
									reconstruction_time);
#endif
		// Iterate through the reconstructed geometries of the current feature and write to output.
		// Note that this will export each geometry as a separate entry in the shapefile, even if they
		// come from the same feature. 
		reconstructed_feature_geom_seq_type::const_iterator rfg_iter;
		for (rfg_iter = feature_geom_group.recon_geoms.begin();
			rfg_iter != feature_geom_group.recon_geoms.end();
			++rfg_iter)
		{
			const GPlatesAppLogic::ReconstructedFeatureGeometry *rfg = *rfg_iter;

			// Write the reconstructed geometry.
			geom_exporter.export_geometry(rfg->reconstructed_geometry(),kvd_for_export); 
		}
	}
}

