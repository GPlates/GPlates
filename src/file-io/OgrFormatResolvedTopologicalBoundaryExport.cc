/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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
#include "OgrFormatResolvedTopologicalBoundaryExport.h"
#include "OgrGeometryExporter.h"
#include "OgrUtils.h"

#include "app-logic/ReconstructionGeometry.h"
#include "app-logic/ReconstructionGeometryUtils.h"

#include "feature-visitors/GeometryTypeFinder.h"
#include "feature-visitors/KeyValueDictionaryFinder.h"
#include "feature-visitors/PropertyValueFinder.h"
#include "property-values/GpmlKeyValueDictionary.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/XsDouble.h"
#include "property-values/XsInteger.h"

using namespace GPlatesFileIO::ReconstructionGeometryExportImpl;
using namespace GPlatesFileIO::ResolvedTopologicalBoundaryExportImpl;


namespace
{
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
				GPlatesPropertyValues::StructuralType::create_xsi("integer"));
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
			GPlatesPropertyValues::StructuralType::create_xsi("integer"));
		dictionary->elements().push_back(anchor_element);	

		// Reconstruction time.
		key = GPlatesPropertyValues::XsString::create("TIME");
		GPlatesPropertyValues::XsDouble::non_null_ptr_type time_value = 
			GPlatesPropertyValues::XsDouble::create(reconstruction_time);	

		GPlatesPropertyValues::GpmlKeyValueDictionaryElement time_element(
			key,
			time_value,
			GPlatesPropertyValues::StructuralType::create_xsi("double"));
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
			dictionary->elements().push_back(element);	
		}


		return dictionary;

	}

	void
	add_feature_fields_to_kvd(
		GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type &output_kvd,
		const GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type &feature_kvd)
	{

		std::vector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement>::const_iterator 
			it = feature_kvd->elements().begin(),
			end = feature_kvd->elements().end();

		for (; it != end ; ++it)
		{
			output_kvd->elements().push_back(*it);
		}

	}
}


void
GPlatesFileIO::OgrFormatResolvedTopologicalBoundaryExport::export_resolved_topological_boundaries(
		const resolved_geom_seq_type &resolved_topological_geometries,
		const QFileInfo& file_info,
		const referenced_files_collection_type &referenced_files,
		const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
		const double &reconstruction_time,
		bool wrap_to_dateline)
{
	// Set up the appropriate form of ShapefileGeometryExporter.
	// All the geometries are resolved polygons so 'multiple_geometries' is false.
	QString file_path = file_info.filePath();
	GPlatesFileIO::OgrGeometryExporter geom_exporter(file_path, false/*multiple_geometries*/, wrap_to_dateline);

	// Iterate through the resolved topological geometries and write to output.
	resolved_geom_seq_type::const_iterator resolved_geom_iter;
	for (resolved_geom_iter = resolved_topological_geometries.begin();
		resolved_geom_iter != resolved_topological_geometries.end();
		++resolved_geom_iter)
	{
		const GPlatesAppLogic::ReconstructionGeometry *resolved_geom = *resolved_geom_iter;

		// Get the resolved boundary subsegments.
		boost::optional<const std::vector<GPlatesAppLogic::ResolvedTopologicalGeometrySubSegment> &> boundary_sub_segments =
				GPlatesAppLogic::ReconstructionGeometryUtils::get_resolved_topological_boundary_sub_segment_sequence(resolved_geom);
		// If not a ResolvedTopologicalGeometry (containing a polygon) or ResolvedTopologicalNetwork then skip.
		if (!boundary_sub_segments)
		{
			continue;
		}

		boost::optional<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type> boundary_polygon =
				GPlatesAppLogic::ReconstructionGeometryUtils::get_resolved_topological_boundary_polygon(resolved_geom);
		// If not a ResolvedTopologicalGeometry (containing a polygon) or ResolvedTopologicalNetwork then skip.
		if (!boundary_polygon)
		{
			continue;
		}

		boost::optional<GPlatesModel::FeatureHandle::weak_ref> feature_ref =
				GPlatesAppLogic::ReconstructionGeometryUtils::get_feature_ref(resolved_geom);
		if (!feature_ref || !feature_ref->is_valid())
		{
			continue;
		}

		GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type kvd_for_export =
			GPlatesPropertyValues::GpmlKeyValueDictionary::create();



		OgrUtils::add_reconstruction_fields_to_kvd(kvd_for_export,
										reconstruction_anchor_plate_id,
										reconstruction_time);

		OgrUtils::add_referenced_files_to_kvd(kvd_for_export, referenced_files);

        OgrUtils::add_standard_properties_to_kvd(feature_ref.get(), kvd_for_export);

#if 0		
		GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type kvd =
			create_kvd_from_feature(feature_ref,
									referenced_files,
									reconstruction_anchor_plate_id,
									reconstruction_time);
#endif
		// Write the resolved topological boundary.
		geom_exporter.export_geometry(boundary_polygon.get(), kvd_for_export); 
	}
}


void
GPlatesFileIO::OgrFormatResolvedTopologicalBoundaryExport::export_sub_segments(
		const sub_segment_group_seq_type &sub_segments,
		const QFileInfo& file_info,
		const referenced_files_collection_type &referenced_files,
		const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
		const double &reconstruction_time,
		bool wrap_to_dateline)
{
	// Iterate through the subsegment groups and check which geometry types we have.
	GPlatesFeatureVisitors::GeometryTypeFinder finder;
	sub_segment_group_seq_type::const_iterator sub_segment_group_iter;
	for (sub_segment_group_iter = sub_segments.begin();
		sub_segment_group_iter != sub_segments.end();
		++sub_segment_group_iter)
	{
		const SubSegmentGroup &sub_segment_group = *sub_segment_group_iter;

		boost::optional<GPlatesModel::FeatureHandle::weak_ref> feature_ref =
				GPlatesAppLogic::ReconstructionGeometryUtils::get_feature_ref(
						sub_segment_group.resolved_topological_geometry);
		if (!feature_ref || !feature_ref->is_valid())
		{
			continue;
		}

		// Iterate through the subsegment geometries of the current resolved topological boundary.
		sub_segment_ptr_seq_type::const_iterator sub_segment_iter;
		for (sub_segment_iter = sub_segment_group.sub_segments.begin();
			sub_segment_iter != sub_segment_group.sub_segments.end();
			++sub_segment_iter)
		{
			const GPlatesAppLogic::ResolvedTopologicalGeometrySubSegment *sub_segment = *sub_segment_iter;
			sub_segment->get_geometry()->accept_visitor(finder);
		}
	}

	// Set up the appropriate form of ShapefileGeometryExporter.
	QString file_path = file_info.filePath();
	GPlatesFileIO::OgrGeometryExporter geom_exporter(
		file_path,
		finder.has_found_multiple_geometry_types(),
		wrap_to_dateline);

	// Iterate through the subsegment groups and write them out.
	for (sub_segment_group_iter = sub_segments.begin();
		sub_segment_group_iter != sub_segments.end();
		++sub_segment_group_iter)
	{
		const SubSegmentGroup &sub_segment_group = *sub_segment_group_iter;

#if 0
		// The topological plate polygon feature.
		const GPlatesModel::FeatureHandle::weak_ref resolved_geom_feature_ref =
				sub_segment_group.resolved_topological_boundary->get_feature_ref();
		if (!resolved_geom_feature_ref.is_valid())
		{
			continue;
		}
#endif

		// Iterate through the subsegment geometries of the current resolved topological boundary.
		sub_segment_ptr_seq_type::const_iterator sub_segment_iter;
		for (sub_segment_iter = sub_segment_group.sub_segments.begin();
			sub_segment_iter != sub_segment_group.sub_segments.end();
			++sub_segment_iter)
		{
			const GPlatesAppLogic::ResolvedTopologicalGeometrySubSegment *sub_segment = *sub_segment_iter;

			// The subsegment feature.
			const GPlatesModel::FeatureHandle::const_weak_ref subsegment_feature_ref =
					sub_segment->get_feature_ref();
			if (!subsegment_feature_ref.is_valid())
			{
				continue;
			}

			GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type kvd_for_export =
				GPlatesPropertyValues::GpmlKeyValueDictionary::create();

			//
			// NOTE: Do we want to use the plate id of the topological polygon feature or
			// the plate id of the topological section feature (the one we're exporting a subsegment of)?
			//
			// Perhaps both.
			//
			// For now just write the plate id of the topological section feature.
            //

			OgrUtils::add_reconstruction_fields_to_kvd(kvd_for_export,
											reconstruction_anchor_plate_id,
											reconstruction_time);

			OgrUtils::add_referenced_files_to_kvd(kvd_for_export, referenced_files);

            OgrUtils::add_standard_properties_to_kvd(subsegment_feature_ref, kvd_for_export);

#if 0		
			GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type kvd =
				create_kvd_from_feature(feature_ref,
										referenced_files,
										reconstruction_anchor_plate_id,
										reconstruction_time);
#endif

			// Write the subsegment.
			geom_exporter.export_geometry(sub_segment->get_geometry(), kvd_for_export); 
		}
	}
}
