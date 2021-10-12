/* $Id$ */

/**
 * \file Exports reconstructed feature geometries to a GMT format file.
 * $Revision$
 * $Date$
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
#include "ShapefileFormatReconstructedFeatureGeometryExport.h"
#include "ShapefileGeometryExporter.h"

#include "feature-visitors/GeometryTypeFinder.h"
#include "feature-visitors/PropertyValueFinder.h"
#include "model/ReconstructedFeatureGeometry.h"
#include "property-values/GpmlKeyValueDictionary.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/XsDouble.h"
#include "property-values/XsInteger.h"


namespace
{
	//! Convenience typedef for referenced files.
	typedef GPlatesFileIO::ShapefileFormatReconstructedFeatureGeometryExport::referenced_files_collection_type
			referenced_files_collection_type;

	//! Convenience typedef for reconstructed geometries.
	typedef GPlatesFileIO::ReconstructedFeatureGeometryExportImpl::reconstructed_feature_geom_seq_type
		reconstructed_feature_geom_seq_type;

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
	
		GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type dictionary = 
			GPlatesPropertyValues::GpmlKeyValueDictionary::create();

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
				GPlatesPropertyValues::XsInteger::create(recon_plate_id->value());	

			GPlatesPropertyValues::GpmlKeyValueDictionaryElement element(
				key,
				plateid_value,
				GPlatesPropertyValues::TemplateTypeParameterType::create_xsi("integer"));
			dictionary->elements().push_back(element);
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
			GPlatesPropertyValues::TemplateTypeParameterType::create_xsi("integer"));
		dictionary->elements().push_back(anchor_element);	

		// Reconstruction time.
		key = GPlatesPropertyValues::XsString::create("TIME");
		GPlatesPropertyValues::XsDouble::non_null_ptr_type time_value = 
			GPlatesPropertyValues::XsDouble::create(reconstruction_time);	

		GPlatesPropertyValues::GpmlKeyValueDictionaryElement time_element(
			key,
			time_value,
			GPlatesPropertyValues::TemplateTypeParameterType::create_xsi("double"));
		dictionary->elements().push_back(time_element);	

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
			const GPlatesFileIO::File *file = *file_iter;

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
				GPlatesPropertyValues::TemplateTypeParameterType::create_xsi("string"));
			dictionary->elements().push_back(element);	
		}


		return dictionary;

	}
}


void
GPlatesFileIO::ShapefileFormatReconstructedFeatureGeometryExport::export_geometries(
		const feature_geometry_group_seq_type &feature_geometry_group_seq,
		const QFileInfo& file_info,
		const referenced_files_collection_type &referenced_files,
		const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
		const double &reconstruction_time)
{

	// Iterate through the reconstructed geometries and check which geometry types we have.
	GPlatesFeatureVisitors::GeometryTypeFinder finder;

	feature_geometry_group_seq_type::const_iterator feature_iter;
	for (feature_iter = feature_geometry_group_seq.begin();
		feature_iter != feature_geometry_group_seq.end();
		++feature_iter)
	{
		const ReconstructedFeatureGeometryExportImpl::FeatureGeometryGroup &feature_geom_group =
			*feature_iter;

		const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref =
			feature_geom_group.feature_ref;
		if (!feature_ref.is_valid())
		{
			continue;
		}

		// Iterate through the reconstructed geometries of the current feature.
		reconstructed_feature_geom_seq_type::const_iterator rfg_iter;
		for (rfg_iter = feature_geom_group.recon_feature_geoms.begin();
			rfg_iter != feature_geom_group.recon_feature_geoms.end();
			++rfg_iter)
		{
			const GPlatesModel::ReconstructedFeatureGeometry *rfg = *rfg_iter;
			rfg->geometry()->accept_visitor(finder);
		}
	}

	// Set up the appropriate form of ShapefileGeometryExporter.
	QString file_path = file_info.filePath();
	GPlatesFileIO::ShapefileGeometryExporter geom_exporter(
		file_path,
		finder.has_found_multiple_geometries());


	// Iterate through the reconstructed geometries and write to output.
	for (feature_iter = feature_geometry_group_seq.begin();
		feature_iter != feature_geometry_group_seq.end();
		++feature_iter)
	{

		const ReconstructedFeatureGeometryExportImpl::FeatureGeometryGroup &feature_geom_group =
				*feature_iter;

		const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref =
				feature_geom_group.feature_ref;

		if (!feature_ref.is_valid())
		{
			continue;
		}

		GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type kvd =
			create_kvd_from_feature(feature_ref,
									referenced_files,
									reconstruction_anchor_plate_id,
									reconstruction_time);

		// Iterate through the reconstructed geometries of the current feature and write to output.
		reconstructed_feature_geom_seq_type::const_iterator rfg_iter;
		for (rfg_iter = feature_geom_group.recon_feature_geoms.begin();
			rfg_iter != feature_geom_group.recon_feature_geoms.end();
			++rfg_iter)
		{
			const GPlatesModel::ReconstructedFeatureGeometry *rfg = *rfg_iter;

			// Write the reconstructed geometry.
			geom_exporter.export_geometry(rfg->geometry(),kvd); 
		}
	}

}
