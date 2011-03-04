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

#include <boost/foreach.hpp>

#include "ResolvedTopologicalBoundaryExport.h"

#include "GMTFormatResolvedTopologicalBoundaryExport.h"
#include "FeatureCollectionFileFormat.h"
#include "FileFormatNotSupportedException.h"
#include "ReconstructionGeometryExportImpl.h"
#include "ResolvedTopologicalBoundaryExportImpl.h"
#include "ShapefileFormatResolvedTopologicalBoundaryExport.h"

using namespace GPlatesFileIO::ReconstructionGeometryExportImpl;
using namespace GPlatesFileIO::ResolvedTopologicalBoundaryExportImpl;


namespace GPlatesFileIO
{
	namespace ResolvedTopologicalBoundaryExport
	{
		namespace
		{
			QString
			append_suffix_to_template_filebasename(
					const QFileInfo &original_template_filename,
					QString suffix)
			{
				const QString ext = original_template_filename.suffix();
				if (ext.isEmpty())
				{
					// Shouldn't really happen.
					return original_template_filename.fileName() + suffix;
				}

				// Remove any known file suffix from the template filename.
				const QString template_filebasename = original_template_filename.completeBaseName();

				return template_filebasename + suffix + '.' + ext;
			}


			QString
			substitute_placeholder(
					const QString &output_filebasename,
					const QString &placeholder,
					const QString &placeholder_replacement)
			{
				return QString(output_filebasename).replace(placeholder, placeholder_replacement);
			}


			const QString
			get_full_output_filename(
					const QDir &target_dir,
					const QString &filebasename,
					const QString &placeholder_string,
					const QString &placeholder_replacement)
			{
				const QString output_basename = substitute_placeholder(filebasename,
						placeholder_string, placeholder_replacement);

				return target_dir.absoluteFilePath(output_basename);
			}


			/**
			 * Exports a sequence of @a ResolvedTopologicalBoundary objects
			 * to the specified export file format.
			 */
			void
			export_resolved_topological_boundaries_file(
					const QString &filename,
					Format export_format,
					const resolved_geom_seq_type &resolved_geoms,
					const std::vector<const File::Reference *> &referenced_files,
					const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
					const double &reconstruction_time)
			{
				if (resolved_geoms.empty())
				{
					return;
				}

				switch (export_format)
				{
				case GMT:
					GMTFormatResolvedTopologicalBoundaryExport::export_resolved_topological_boundaries(
						resolved_geoms,
						filename,
						referenced_files,
						reconstruction_anchor_plate_id,
						reconstruction_time);
					break;

				case SHAPEFILE:
					ShapefileFormatResolvedTopologicalBoundaryExport::export_resolved_topological_boundaries(
						resolved_geoms,
						filename,
						referenced_files,
						reconstruction_anchor_plate_id,
						reconstruction_time);		
					break;

				default:
					throw FileFormatNotSupportedException(GPLATES_EXCEPTION_SOURCE,
						"Chosen export format is not currently supported.");
				}
			}


			/**
			 * Exports a sequence of subsegments of resolved topological boundaries
			 * to the specified export file format.
			 */
			void
			export_sub_segments_file(
					const QString &filename,
					Format export_format,
					const sub_segment_group_seq_type &sub_segment_groups,
					const std::vector<const File::Reference *> &referenced_files,
					const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
					const double &reconstruction_time)
			{
				// Make sure we have at least one subsegment.
				unsigned int num_sub_segments = 0;
				BOOST_FOREACH(const SubSegmentGroup &sub_segment_group, sub_segment_groups)
				{
					num_sub_segments += sub_segment_group.sub_segments.size();
				}
				if (num_sub_segments == 0)
				{
					return;
				}

				switch (export_format)
				{
				case GMT:
					GMTFormatResolvedTopologicalBoundaryExport::export_sub_segments(
						sub_segment_groups,
						filename,
						referenced_files,
						reconstruction_anchor_plate_id,
						reconstruction_time);
					break;

				case SHAPEFILE:
					ShapefileFormatResolvedTopologicalBoundaryExport::export_sub_segments(
						sub_segment_groups,
						filename,
						referenced_files,
						reconstruction_anchor_plate_id,
						reconstruction_time);		
					break;

				default:
					throw FileFormatNotSupportedException(GPLATES_EXCEPTION_SOURCE,
						"Chosen export format is not currently supported.");
				}
			}
		}
	}
}


GPlatesFileIO::ResolvedTopologicalBoundaryExport::Format
GPlatesFileIO::ResolvedTopologicalBoundaryExport::get_export_file_format(
		const QFileInfo& file_info)
{
	// Since we're using a feature collection file format to export
	// our RFGs we'll use the feature collection file format code.
	const FeatureCollectionFileFormat::Format feature_collection_file_format =
			get_feature_collection_file_format(file_info);

	// Only some feature collection file formats are used for exporting
	// reconstructed feature geometries because most file formats only
	// make sense for unreconstructed geometry (since they provide the
	// information required to do the reconstructions).
	switch (feature_collection_file_format)
	{
	case FeatureCollectionFileFormat::GMT:
		return GMT;
	case FeatureCollectionFileFormat::SHAPEFILE:
		return SHAPEFILE;
	default:
		break;
	}

	return UNKNOWN;
}


void
GPlatesFileIO::ResolvedTopologicalBoundaryExport::export_resolved_topological_boundaries(
		const QDir &target_dir,
		const QString &file_basename,
		const QString &placeholder_format_string,
		const OutputOptions &output_options,
		Format export_format,
		const resolved_geom_seq_type &resolved_geom_seq,
		const std::vector<const File::Reference *> &active_files,
		const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
		const double &reconstruction_time)
{
	// Get the list of active feature collection files that contain
	// the features referenced by the ResolvedTopologicalBoundary objects.
	feature_handle_to_collection_map_type feature_to_collection_map;
	std::vector<const File::Reference *> referenced_files;
	get_files_referenced_by_geometries(
			referenced_files, resolved_geom_seq, active_files, feature_to_collection_map);

	// Information to get exported by the file format exporters.
	resolved_geom_seq_type platepolygons;
	resolved_geom_seq_type slab_polygons;
	sub_segment_group_seq_type lines;
	sub_segment_group_seq_type ridge_transforms;
	sub_segment_group_seq_type subductions;
	sub_segment_group_seq_type left_subductions;
	sub_segment_group_seq_type right_subductions;

	// Iterate over the ResolvedTopologicalBoundary objects and
	// collect information for the file format exporter.
	resolved_geom_seq_type::const_iterator resolved_seq_iter = resolved_geom_seq.begin();
	resolved_geom_seq_type::const_iterator resolved_seq_end = resolved_geom_seq.end();
	for ( ; resolved_seq_iter != resolved_seq_end; ++resolved_seq_iter)
	{
		const GPlatesAppLogic::ResolvedTopologicalBoundary *resolved_geom = *resolved_seq_iter;

		// Feature handle reference to topology feature.
		const GPlatesModel::FeatureHandle::const_weak_ref feature_ref =
				resolved_geom->feature_handle_ptr()->reference();


		static const GPlatesModel::FeatureType plate_type = 
			GPlatesModel::FeatureType::create_gpml("TopologicalClosedPlateBoundary");

		if (feature_ref->feature_type() == plate_type)
		{
			if (output_options.placeholder_platepolygons)
			{
				platepolygons.push_back(resolved_geom);
			}

			// The export file with all subsegments (regardless of type).
			if (output_options.placeholder_lines)
			{
				lines.push_back(SubSegmentGroup(resolved_geom));
			}
			// The export file for all subductions.
			if (output_options.placeholder_subductions)
			{
				subductions.push_back(SubSegmentGroup(resolved_geom));
			}
			// The export file for all left subductions.
			if (output_options.placeholder_left_subductions)
			{
				left_subductions.push_back(SubSegmentGroup(resolved_geom));
			}
			// The export file for all right subductions.
			if (output_options.placeholder_right_subductions)
			{
				right_subductions.push_back(SubSegmentGroup(resolved_geom));
			}
			// The export file for all ridge transforms.
			if (output_options.placeholder_ridge_transforms)
			{
				ridge_transforms.push_back(SubSegmentGroup(resolved_geom));
			}

			// Iterate over the subsegments contained in the current resolved topological geometry.
			GPlatesAppLogic::ResolvedTopologicalBoundary::sub_segment_const_iterator sub_segment_iter =
					resolved_geom->sub_segment_begin();
			GPlatesAppLogic::ResolvedTopologicalBoundary::sub_segment_const_iterator sub_segment_end =
					resolved_geom->sub_segment_end();
			for ( ; sub_segment_iter != sub_segment_end; ++sub_segment_iter)
			{
				const GPlatesAppLogic::ResolvedTopologicalBoundary::SubSegment &sub_segment =
						*sub_segment_iter;

				// The export file with all subsegments (regardless of type).
				if (output_options.placeholder_lines)
				{
					lines.back().sub_segments.push_back(&sub_segment);
				}

				// Also export the sub segment to another file based on its feature type.

				// Determine the feature type of subsegment.
				const SubSegmentType sub_segment_type =
						get_sub_segment_type(sub_segment, reconstruction_time);

				// Export subsegment depending on its feature type.
				switch (sub_segment_type)
				{
				case SUB_SEGMENT_TYPE_SUBDUCTION_ZONE_LEFT:
					if (output_options.placeholder_subductions)
					{
						subductions.back().sub_segments.push_back(&sub_segment);
					}
					if (output_options.placeholder_left_subductions)
					{
						left_subductions.back().sub_segments.push_back(&sub_segment);
					}
					break;

				case SUB_SEGMENT_TYPE_SUBDUCTION_ZONE_RIGHT:
					if (output_options.placeholder_subductions)
					{
						subductions.back().sub_segments.push_back(&sub_segment);
					}
					if (output_options.placeholder_right_subductions)
					{
						right_subductions.back().sub_segments.push_back(&sub_segment);
					}
					break;

				case SUB_SEGMENT_TYPE_SUBDUCTION_ZONE_UNKNOWN:
					// We know it's a subduction zone but don't know if left or right
					// so export to the subduction zone file only.
					if (output_options.placeholder_subductions)
					{
						subductions.back().sub_segments.push_back(&sub_segment);
					}
					break;

				case SUB_SEGMENT_TYPE_OTHER:
				default:
					if (output_options.placeholder_ridge_transforms)
					{
						ridge_transforms.back().sub_segments.push_back(&sub_segment);
					}
					break;
				}
			}
		}


		static const GPlatesModel::FeatureType slab_type = 
			GPlatesModel::FeatureType::create_gpml("TopologicalSlabBoundary");
	
		if (feature_ref->feature_type() == slab_type)
		{	
			if (output_options.placeholder_slab_polygons)
			{
				slab_polygons.push_back(resolved_geom);
			}
		}
	}

	if (output_options.placeholder_platepolygons)
	{
		const QString filename = get_full_output_filename(
			target_dir,
			file_basename,
			placeholder_format_string,
			output_options.placeholder_platepolygons.get());

		export_resolved_topological_boundaries_file(
				filename,
				export_format,
				platepolygons,
				referenced_files,
				reconstruction_anchor_plate_id,
				reconstruction_time);
	}

	if (output_options.placeholder_lines)
	{
		const QString filename = get_full_output_filename(
			target_dir,
			file_basename,
			placeholder_format_string,
			output_options.placeholder_lines.get());

		export_sub_segments_file(
				filename,
				export_format,
				lines,
				referenced_files,
				reconstruction_anchor_plate_id,
				reconstruction_time);
	}

	if (output_options.placeholder_slab_polygons)
	{
		const QString filename = get_full_output_filename(
			target_dir,
			file_basename,
			placeholder_format_string,
			output_options.placeholder_slab_polygons.get());

		export_resolved_topological_boundaries_file(
				filename,
				export_format,
				slab_polygons,
				referenced_files,
				reconstruction_anchor_plate_id,
				reconstruction_time);
	}
}
