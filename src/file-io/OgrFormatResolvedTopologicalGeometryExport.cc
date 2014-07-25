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
#include "OgrFormatResolvedTopologicalGeometryExport.h"
#include "OgrGeometryExporter.h"
#include "OgrUtils.h"

#include "app-logic/GeometryUtils.h"
#include "app-logic/ReconstructionGeometry.h"
#include "app-logic/ReconstructionGeometryUtils.h"

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
	typedef GPlatesFileIO::OgrFormatResolvedTopologicalGeometryExport::referenced_files_collection_type
			referenced_files_collection_type;

	//! Convenience typedef for a sequence of RTGs.
	typedef std::vector<const GPlatesAppLogic::ResolvedTopologicalGeometry *>
			resolved_topological_geom_seq_type;


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
GPlatesFileIO::OgrFormatResolvedTopologicalGeometryExport::export_geometries(
		const std::list<feature_geometry_group_type> &feature_geometry_group_seq,
		const QFileInfo& file_info,
		const referenced_files_collection_type &referenced_files,
		const referenced_files_collection_type &active_reconstruction_files,
		const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
		const double &reconstruction_time,
		boost::optional<GPlatesMaths::PolygonOrientation::Orientation> force_polygon_orientation,
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

		// Iterate through the resolved geometries of the current feature.
		resolved_topological_geom_seq_type::const_iterator rtg_iter;
		for (rtg_iter = feature_geom_group.recon_geoms.begin();
			rtg_iter != feature_geom_group.recon_geoms.end();
			++rtg_iter)
		{
			const GPlatesAppLogic::ResolvedTopologicalGeometry *rtg = *rtg_iter;
			rtg->resolved_topology_geometry()->accept_visitor(finder);
		}
	}

	// Set up the appropriate form of OgrGeometryExporter.
	QString file_path = file_info.filePath();
	GPlatesFileIO::OgrGeometryExporter geom_exporter(
		file_path,
		finder.has_found_multiple_geometry_types(),
		wrap_to_dateline);



	// Iterate through the resolved geometries and write to output.
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


		GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type kvd_for_export =
			GPlatesPropertyValues::GpmlKeyValueDictionary::create();

		OgrUtils::add_reconstruction_fields_to_kvd(kvd_for_export,
										reconstruction_anchor_plate_id,
										reconstruction_time);

		OgrUtils::add_filename_sequence_to_kvd(QString("FILE"),referenced_files,kvd_for_export);
		OgrUtils::add_filename_sequence_to_kvd(QString("RECONFILE"),active_reconstruction_files,kvd_for_export);

		OgrUtils::add_standard_properties_to_kvd(
					feature_ref,kvd_for_export);



		// Iterate through the resolved geometries of the current feature and write to output.
		// Note that this will export each geometry as a separate entry in the shapefile, even if they
		// come from the same feature. 
		resolved_topological_geom_seq_type::const_iterator rtg_iter;
		for (rtg_iter = feature_geom_group.recon_geoms.begin();
			rtg_iter != feature_geom_group.recon_geoms.end();
			++rtg_iter)
		{
			const GPlatesAppLogic::ResolvedTopologicalGeometry *rtg = *rtg_iter;

			// Orient polygon if forcing orientation and geometry is a polygon.
			//
			// NOTE: This only works for non-Shapefile OGR formats because the OGR Shapefile
			// driver stores exterior rings as clockwise and interior as counter-clockwise -
			// so whatever we do here could just get undone by the Shapefile driver.
			GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type resolved_geometry =
					force_polygon_orientation
					? GPlatesAppLogic::GeometryUtils::convert_geometry_to_oriented_geometry(
							rtg->resolved_topology_geometry(),
							force_polygon_orientation.get())
					: rtg->resolved_topology_geometry();

			// Write the resolved geometry.
			geom_exporter.export_geometry(resolved_geometry, kvd_for_export); 
		}
	}

}


void
GPlatesFileIO::OgrFormatResolvedTopologicalGeometryExport::export_geometries_per_collection(
		const std::list<feature_geometry_group_type> &feature_geometry_group_seq,
		const QFileInfo& file_info,
		const referenced_files_collection_type &referenced_files,
		const referenced_files_collection_type &active_reconstruction_files,
		const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
		const double &reconstruction_time,
		boost::optional<GPlatesMaths::PolygonOrientation::Orientation> force_polygon_orientation,
		bool wrap_to_dateline)
{

	// Iterate through the resolved geometries and check which geometry types we have.
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

		// Iterate through the resolved geometries of the current feature.
		resolved_topological_geom_seq_type::const_iterator rtg_iter;
		for (rtg_iter = feature_geom_group.recon_geoms.begin();
			rtg_iter != feature_geom_group.recon_geoms.end();
			++rtg_iter)
		{
			const GPlatesAppLogic::ResolvedTopologicalGeometry *rtg = *rtg_iter;
			rtg->resolved_topology_geometry()->accept_visitor(finder);
		}
	}

	// Set up the appropriate form of ShapefileGeometryExporter.
	QString file_path = file_info.filePath();
	GPlatesFileIO::OgrGeometryExporter geom_exporter(
		file_path,
		finder.has_found_multiple_geometry_types(),
		wrap_to_dateline);



	// Iterate through the resolved geometries and write to output.
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


		GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_type kvd_for_export =
			GPlatesPropertyValues::GpmlKeyValueDictionary::create();

        OgrUtils::add_reconstruction_fields_to_kvd(kvd_for_export,
                                        reconstruction_anchor_plate_id,
                                        reconstruction_time);

		OgrUtils::add_filename_sequence_to_kvd(QString("FILE"),referenced_files,kvd_for_export);
		OgrUtils::add_filename_sequence_to_kvd(QString("RECONFILE"),active_reconstruction_files,kvd_for_export);

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


		// Iterate through the resolved geometries of the current feature and write to output.
		// Note that this will export each geometry as a separate entry in the shapefile, even if they
		// come from the same feature. 
		resolved_topological_geom_seq_type::const_iterator rtg_iter;
		for (rtg_iter = feature_geom_group.recon_geoms.begin();
			rtg_iter != feature_geom_group.recon_geoms.end();
			++rtg_iter)
		{
			const GPlatesAppLogic::ResolvedTopologicalGeometry *rtg = *rtg_iter;

			// Orient polygon if forcing orientation and geometry is a polygon.
			//
			// NOTE: This only works for non-Shapefile OGR formats because the OGR Shapefile
			// driver stores exterior rings as clockwise and interior as counter-clockwise -
			// so whatever we do here could just get undone by the Shapefile driver.
			GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type resolved_geometry =
					force_polygon_orientation
					? GPlatesAppLogic::GeometryUtils::convert_geometry_to_oriented_geometry(
							rtg->resolved_topology_geometry(),
							force_polygon_orientation.get())
					: rtg->resolved_topology_geometry();

			// Write the resolved geometry.
			geom_exporter.export_geometry(resolved_geometry, kvd_for_export); 
		}
	}
}


void
GPlatesFileIO::OgrFormatResolvedTopologicalGeometryExport::export_citcoms_resolved_topological_boundaries(
		const CitcomsResolvedTopologicalBoundaryExportImpl::resolved_geom_seq_type &resolved_topological_geometries,
		const QFileInfo& file_info,
		const referenced_files_collection_type &referenced_files,
		const referenced_files_collection_type &active_reconstruction_files,
		const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
		const double &reconstruction_time,
		bool wrap_to_dateline)
{
	// Set up the appropriate form of ShapefileGeometryExporter.
	// All the geometries are resolved polygons so 'multiple_geometries' is false.
	QString file_path = file_info.filePath();
	GPlatesFileIO::OgrGeometryExporter geom_exporter(file_path, false/*multiple_geometries*/, wrap_to_dateline);

	// Iterate through the resolved topological geometries and write to output.
	CitcomsResolvedTopologicalBoundaryExportImpl::resolved_geom_seq_type::const_iterator resolved_geom_iter;
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

		OgrUtils::add_filename_sequence_to_kvd(QString("FILE"),referenced_files,kvd_for_export);
		OgrUtils::add_filename_sequence_to_kvd(QString("RECONFILE"),active_reconstruction_files,kvd_for_export);

        OgrUtils::add_standard_properties_to_kvd(feature_ref.get(), kvd_for_export);

		// Write the resolved topological boundary.
		geom_exporter.export_geometry(boundary_polygon.get(), kvd_for_export); 
	}
}


void
GPlatesFileIO::OgrFormatResolvedTopologicalGeometryExport::export_citcoms_sub_segments(
		const CitcomsResolvedTopologicalBoundaryExportImpl::sub_segment_group_seq_type &sub_segments,
		const QFileInfo& file_info,
		const referenced_files_collection_type &referenced_files,
		const referenced_files_collection_type &active_reconstruction_files,
		const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
		const double &reconstruction_time,
		bool wrap_to_dateline)
{
	// Iterate through the subsegment groups and check which geometry types we have.
	GPlatesFeatureVisitors::GeometryTypeFinder finder;
	CitcomsResolvedTopologicalBoundaryExportImpl::sub_segment_group_seq_type::const_iterator sub_segment_group_iter;
	for (sub_segment_group_iter = sub_segments.begin();
		sub_segment_group_iter != sub_segments.end();
		++sub_segment_group_iter)
	{
		const CitcomsResolvedTopologicalBoundaryExportImpl::SubSegmentGroup &sub_segment_group =
				*sub_segment_group_iter;

		boost::optional<GPlatesModel::FeatureHandle::weak_ref> feature_ref =
				GPlatesAppLogic::ReconstructionGeometryUtils::get_feature_ref(
						sub_segment_group.resolved_topological_geometry);
		if (!feature_ref || !feature_ref->is_valid())
		{
			continue;
		}

		// Iterate through the subsegment geometries of the current resolved topological boundary.
		CitcomsResolvedTopologicalBoundaryExportImpl::sub_segment_ptr_seq_type::const_iterator sub_segment_iter;
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
		const CitcomsResolvedTopologicalBoundaryExportImpl::SubSegmentGroup &sub_segment_group =
				*sub_segment_group_iter;

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
		CitcomsResolvedTopologicalBoundaryExportImpl::sub_segment_ptr_seq_type::const_iterator sub_segment_iter;
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

			OgrUtils::add_filename_sequence_to_kvd(QString("FILE"),referenced_files,kvd_for_export);
			OgrUtils::add_filename_sequence_to_kvd(QString("RECONFILE"),active_reconstruction_files,kvd_for_export);

            OgrUtils::add_standard_properties_to_kvd(subsegment_feature_ref, kvd_for_export);


			// Write the subsegment.
			geom_exporter.export_geometry(sub_segment->get_geometry(), kvd_for_export); 
		}
	}
}
