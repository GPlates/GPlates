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
#include "global/CompilerWarnings.h"
PUSH_MSVC_WARNINGS
// Disable Visual Studio warning "qualifier applied to function type has no meaning; ignored" in boost 1.36.0
DISABLE_MSVC_WARNING(4181)
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
POP_MSVC_WARNINGS

#include "ResolvedTopologicalBoundaryExport.h"

#include "GMTFormatResolvedTopologicalBoundaryExport.h"
#include "FeatureCollectionFileFormat.h"
#include "FileFormatNotSupportedException.h"
#include "ReconstructionGeometryExportImpl.h"
#include "ResolvedTopologicalBoundaryExportImpl.h"
#include "OgrFormatResolvedTopologicalBoundaryExport.h"

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
			 * The output data to be exported.
			 */
			struct Output
			{
				resolved_geom_seq_type platepolygons;
				sub_segment_group_seq_type lines;
				sub_segment_group_seq_type ridge_transforms;
				sub_segment_group_seq_type subductions;
				sub_segment_group_seq_type left_subductions;
				sub_segment_group_seq_type right_subductions;

				resolved_geom_seq_type slab_polygons;
				sub_segment_group_seq_type slab_edge_leading;
				sub_segment_group_seq_type slab_edge_leading_left;
				sub_segment_group_seq_type slab_edge_leading_right;
				sub_segment_group_seq_type slab_edge_trench;
				sub_segment_group_seq_type slab_edge_side;
			};


			/**
			 * Returns a unique list of files that contain the subsegment features.
			 *
			 * Does *not* look for files that contain the topological closed plate polygon features.
			 */
			void
			get_unique_list_of_referenced_files(
					referenced_files_collection_type &referenced_files,
					const sub_segment_group_seq_type &sub_segment_groups,
					const feature_handle_to_collection_map_type &feature_handle_to_collection_map)
			{
				// Iterate through the list of subsegment groups and build up a unique list of
				// feature collection files referenced by them.
				sub_segment_group_seq_type::const_iterator sub_segment_group_iter;
				for (sub_segment_group_iter = sub_segment_groups.begin();
					sub_segment_group_iter != sub_segment_groups.end();
					++sub_segment_group_iter)
				{
					const SubSegmentGroup &sub_segment_group = *sub_segment_group_iter;

					// Iterate over the subsegments.
					sub_segment_seq_type::const_iterator sub_segment_iter;
					for (sub_segment_iter = sub_segment_group.sub_segments.begin();
						sub_segment_iter != sub_segment_group.sub_segments.end();
						++sub_segment_iter)
					{
						const GPlatesModel::FeatureHandle::const_weak_ref sub_segment_feature_ref =
								(*sub_segment_iter)->get_feature_ref();

						const feature_handle_to_collection_map_type::const_iterator map_iter =
								feature_handle_to_collection_map.find(sub_segment_feature_ref.handle_ptr());
						if (map_iter == feature_handle_to_collection_map.end())
						{
							continue;
						}

						const GPlatesFileIO::File::Reference *file = map_iter->second;
						referenced_files.push_back(file);
					}
				}

				// Sort in preparation for removing duplicates.
				// We end up sorting on 'const GPlatesFileIO::File::weak_ref' objects.
				std::sort(referenced_files.begin(), referenced_files.end(), boost::lambda::_1 < boost::lambda::_2);

				// Remove duplicates.
				referenced_files.erase(
						std::unique(referenced_files.begin(), referenced_files.end()),
						referenced_files.end());
			}


			void
			add_topological_closed_plate_boundary_sub_segment(
					const GPlatesAppLogic::ResolvedTopologicalBoundary::SubSegment &sub_segment,
					const double &reconstruction_time,
					const OutputOptions &output_options,
					Output &output)
			{
				// The export file with all subsegments (regardless of type).
				if (output_options.export_plate_polygon_subsegments_to_lines)
				{
					output.lines.back().sub_segments.push_back(&sub_segment);
				}

				// Also export the sub segment to another file based on its feature type.

				// Determine the feature type of subsegment.
				const SubSegmentType sub_segment_type =
						get_sub_segment_type(sub_segment, reconstruction_time);

				// Export subsegment depending on its feature type.
				switch (sub_segment_type)
				{
				case SUB_SEGMENT_TYPE_SUBDUCTION_ZONE_LEFT:
					if (output_options.export_subductions)
					{
						output.subductions.back().sub_segments.push_back(&sub_segment);
					}
					if (output_options.export_left_subductions)
					{
						output.left_subductions.back().sub_segments.push_back(&sub_segment);
					}
					break;

				case SUB_SEGMENT_TYPE_SUBDUCTION_ZONE_RIGHT:
					if (output_options.export_subductions)
					{
						output.subductions.back().sub_segments.push_back(&sub_segment);
					}
					if (output_options.export_right_subductions)
					{
						output.right_subductions.back().sub_segments.push_back(&sub_segment);
					}
					break;

				case SUB_SEGMENT_TYPE_SUBDUCTION_ZONE_UNKNOWN:
					// We know it's a subduction zone but don't know if left or right
					// so export to the subduction zone file only.
					if (output_options.export_subductions)
					{
						output.subductions.back().sub_segments.push_back(&sub_segment);
					}
					break;

				case SUB_SEGMENT_TYPE_OTHER:
				default:
					if (output_options.export_ridge_transforms)
					{
						output.ridge_transforms.back().sub_segments.push_back(&sub_segment);
					}
					break;
				}
			}


			void
			add_topological_closed_plate_boundary(
					const GPlatesAppLogic::ResolvedTopologicalBoundary *resolved_geom,
					const double &reconstruction_time,
					const OutputOptions &output_options,
					Output &output)
			{
				// Add the plate polygon if they are being exported.
				if (output_options.export_all_plate_polygons_to_a_single_file)
				{
					output.platepolygons.push_back(resolved_geom);
				}

				// The export file with all subsegments (regardless of type).
				if (output_options.export_plate_polygon_subsegments_to_lines)
				{
					output.lines.push_back(SubSegmentGroup(resolved_geom));
				}
				// The export file for all subductions.
				if (output_options.export_subductions)
				{
					output.subductions.push_back(SubSegmentGroup(resolved_geom));
				}
				// The export file for all left subductions.
				if (output_options.export_left_subductions)
				{
					output.left_subductions.push_back(SubSegmentGroup(resolved_geom));
				}
				// The export file for all right subductions.
				if (output_options.export_right_subductions)
				{
					output.right_subductions.push_back(SubSegmentGroup(resolved_geom));
				}
				// The export file for all ridge transforms.
				if (output_options.export_ridge_transforms)
				{
					output.ridge_transforms.push_back(SubSegmentGroup(resolved_geom));
				}

				// Iterate over the subsegments contained in the current resolved topological geometry.
				GPlatesAppLogic::ResolvedTopologicalBoundary::sub_segment_const_iterator sub_segment_iter =
						resolved_geom->sub_segment_begin();
				GPlatesAppLogic::ResolvedTopologicalBoundary::sub_segment_const_iterator sub_segment_end =
						resolved_geom->sub_segment_end();
				for ( ; sub_segment_iter != sub_segment_end; ++sub_segment_iter)
				{
					add_topological_closed_plate_boundary_sub_segment(
							*sub_segment_iter,
							reconstruction_time,
							output_options,
							output);
				}
			}


			void
			add_topological_slab_boundary_sub_segment(
					const GPlatesAppLogic::ResolvedTopologicalBoundary::SubSegment &sub_segment,
					const double &reconstruction_time,
					const OutputOptions &output_options,
					Output &output)
			{
				// The export file with all subsegments (regardless of type).
				if (output_options.export_slab_polygon_subsegments_to_lines)
				{
					output.lines.back().sub_segments.push_back(&sub_segment);
				}

				// Also export the sub segment to another file based on its feature type.

				// Determine the feature type of subsegment.
				const SubSegmentType sub_segment_type =
						get_slab_sub_segment_type(sub_segment, reconstruction_time);

				// Export subsegment depending on its feature type.
				switch (sub_segment_type)
				{
				case SUB_SEGMENT_TYPE_SLAB_EDGE_LEADING_LEFT:
					if (output_options.export_slab_edge_leading)
					{
						output.slab_edge_leading.back().sub_segments.push_back(&sub_segment);
					}
					if (output_options.export_slab_edge_leading_left)
					{
						output.slab_edge_leading_left.back().sub_segments.push_back(&sub_segment);
					}
					break;

				case SUB_SEGMENT_TYPE_SLAB_EDGE_LEADING_RIGHT:
					if (output_options.export_slab_edge_leading)
					{
						output.slab_edge_leading.back().sub_segments.push_back(&sub_segment);
					}
					if (output_options.export_slab_edge_leading_right)
					{
						output.slab_edge_leading_right.back().sub_segments.push_back(&sub_segment);
					}
					break;

				case SUB_SEGMENT_TYPE_SLAB_EDGE_TRENCH:
					if (output_options.export_slab_edge_trench)
					{
						output.slab_edge_trench.back().sub_segments.push_back(&sub_segment);
					}
					break;

				case SUB_SEGMENT_TYPE_SLAB_EDGE_SIDE:
				default:
					if (output_options.export_slab_edge_side)
					{
						output.slab_edge_side.back().sub_segments.push_back(&sub_segment);
					}
					break;
				}
			}


			void
			add_topological_slab_boundary(
					const GPlatesAppLogic::ResolvedTopologicalBoundary *resolved_geom,
					const double &reconstruction_time,
					const OutputOptions &output_options,
					Output &output)
			{
				// Add the slab polygon if they are being exported.
				if (output_options.export_all_slab_polygons_to_a_single_file)
				{
					output.slab_polygons.push_back(resolved_geom);
				}

				// The export file with all subsegments (regardless of type).
				if (output_options.export_slab_polygon_subsegments_to_lines)
				{
					output.lines.push_back(SubSegmentGroup(resolved_geom));
				}
				// The export file for all slab_edge_leading.
				if (output_options.export_slab_edge_leading)
				{
					output.slab_edge_leading.push_back(SubSegmentGroup(resolved_geom));
				}
				// The export file for all slab_edge_leading_left.
				if (output_options.export_slab_edge_leading_left)
				{
					output.slab_edge_leading_left.push_back(SubSegmentGroup(resolved_geom));
				}
				// The export file for all slab_edge_leading_right.
				if (output_options.export_slab_edge_leading_right)
				{
					output.slab_edge_leading_right.push_back(SubSegmentGroup(resolved_geom));
				}
				// The export file for all slab_edge_trench.
				if (output_options.export_slab_edge_trench)
				{
					output.slab_edge_trench.push_back(SubSegmentGroup(resolved_geom));
				}
				// The export file for all slab_edge_side.
				if (output_options.export_slab_edge_side)
				{
					output.slab_edge_side.push_back(SubSegmentGroup(resolved_geom));
				}

				// Iterate over the subsegments contained in the current resolved topological geometry.
				GPlatesAppLogic::ResolvedTopologicalBoundary::sub_segment_const_iterator sub_segment_iter =
						resolved_geom->sub_segment_begin();
				GPlatesAppLogic::ResolvedTopologicalBoundary::sub_segment_const_iterator sub_segment_end =
						resolved_geom->sub_segment_end();
				for ( ; sub_segment_iter != sub_segment_end; ++sub_segment_iter)
				{
					add_topological_slab_boundary_sub_segment(
							*sub_segment_iter,
							reconstruction_time,
							output_options,
							output);
				}
			}


			void
			collect_exports(
					const resolved_geom_seq_type &resolved_geom_seq,
					const double &reconstruction_time,
					const OutputOptions &output_options,
					Output &output)
			{
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
						add_topological_closed_plate_boundary(
								resolved_geom,
								reconstruction_time,
								output_options,
								output);
					}


					static const GPlatesModel::FeatureType slab_type = 
						GPlatesModel::FeatureType::create_gpml("TopologicalSlabBoundary");
				
					if (feature_ref->feature_type() == slab_type)
					{	
						add_topological_slab_boundary(
								resolved_geom,
								reconstruction_time,
								output_options,
								output);
					}
				}
			}


			/**
			 * Exports a sequence of @a ResolvedTopologicalBoundary objects
			 * to the specified export file format.
			 */
			void
			export_resolved_topological_boundaries_file(
					const QString &filename,
					Format export_format,
					ResolvedTopologicalBoundaryExportType export_type,
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
						export_type,
						filename,
						referenced_files,
						reconstruction_anchor_plate_id,
						reconstruction_time);
					break;

				case SHAPEFILE:
					OgrFormatResolvedTopologicalBoundaryExport::export_resolved_topological_boundaries(
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
					SubSegmentExportType export_type,
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
						export_type,
						filename,
						referenced_files,
						reconstruction_anchor_plate_id,
						reconstruction_time);
					break;

				case SHAPEFILE:
					OgrFormatResolvedTopologicalBoundaryExport::export_sub_segments(
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


			void
			export_resolved_topological_boundaries(
					const QDir &target_dir,
					const QString &file_basename,
					const QString &placeholder_format_string,
					Format export_format,
					ResolvedTopologicalBoundaryExportType export_type,
					const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
					const double &reconstruction_time,
					const QString &placeholder,
					const resolved_geom_seq_type &resolved_geoms,
					const feature_handle_to_collection_map_type &feature_to_collection_map)
			{
				const QString filename = get_full_output_filename(
					target_dir,
					file_basename,
					placeholder_format_string,
					placeholder);

				// Get the files containing the topological features that created
				// the resolved topological boundaries we're about to export.
				std::vector<const File::Reference *> referenced_files;
				ReconstructionGeometryExportImpl::get_unique_list_of_referenced_files(
						referenced_files,
						resolved_geoms,
						feature_to_collection_map);

				export_resolved_topological_boundaries_file(
						filename,
						export_format,
						export_type,
						resolved_geoms,
						referenced_files,
						reconstruction_anchor_plate_id,
						reconstruction_time);
			}


			void
			export_sub_segments(
					const QDir &target_dir,
					const QString &file_basename,
					const QString &placeholder_format_string,
					Format export_format,
					SubSegmentExportType export_type,
					const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
					const double &reconstruction_time,
					const QString &placeholder,
					const sub_segment_group_seq_type &sub_segment_groups,
					const feature_handle_to_collection_map_type &feature_to_collection_map)
			{
				const QString filename = get_full_output_filename(
					target_dir,
					file_basename,
					placeholder_format_string,
					placeholder);

				// Get the files containing the topological section features of
				// the subsegments we're about to export.
				std::vector<const File::Reference *> referenced_files;
				get_unique_list_of_referenced_files(
						referenced_files,
						sub_segment_groups,
						feature_to_collection_map);

				export_sub_segments_file(
						filename,
						export_format,
						export_type,
						sub_segment_groups,
						referenced_files,
						reconstruction_anchor_plate_id,
						reconstruction_time);
			}


			void
			output_exports(
					const QDir &target_dir,
					const QString &file_basename,
					const QString &placeholder_format_string,
					Format export_format,
					const std::vector<const File::Reference *> &loaded_files,
					const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
					const double &reconstruction_time,
					const OutputOptions &output_options,
					const Output &output)
			{
				// Map each loaded feature to the loaded file it belongs to.
				feature_handle_to_collection_map_type feature_to_collection_map;
				populate_feature_handle_to_collection_map(feature_to_collection_map, loaded_files);


				//
				// All subsegments.
				//

				if (output_options.export_plate_polygon_subsegments_to_lines ||
					output_options.export_slab_polygon_subsegments_to_lines)
				{
					export_sub_segments(
							target_dir,
							file_basename,
							placeholder_format_string,
							export_format,
							ALL_SUB_SEGMENTS_EXPORT_TYPE,
							reconstruction_anchor_plate_id,
							reconstruction_time,
							output_options.placeholder_lines,
							output.lines,
							feature_to_collection_map);
				}

				//
				// Plate polygons.
				//

				if (output_options.export_all_plate_polygons_to_a_single_file)
				{
					export_resolved_topological_boundaries(
							target_dir,
							file_basename,
							placeholder_format_string,
							export_format,
							PLATE_POLYGON_EXPORT_TYPE,
							reconstruction_anchor_plate_id,
							reconstruction_time,
							output_options.placeholder_platepolygons,
							output.platepolygons,
							feature_to_collection_map);
				}

				// If we're also exporting each plate polygon to its own file.
				if (output_options.export_individual_plate_polygon_files)
				{
					// Iterate over the plate polygons and export each separately.
					BOOST_FOREACH(
							const GPlatesAppLogic::ResolvedTopologicalBoundary *resolved_geom,
							output.platepolygons)
					{
						// We're expecting a plate id as that forms part of the filename.
						if (!resolved_geom->plate_id())
						{
							continue;
						}
						const QString place_holder_replacement =
								QString::number(*resolved_geom->plate_id());

						// We're exporting a sequence of one resolved geometry to its own file.
						resolved_geom_seq_type resolved_geoms(1, resolved_geom);

						export_resolved_topological_boundaries(
								target_dir,
								file_basename,
								placeholder_format_string,
								export_format,
								PLATE_POLYGON_EXPORT_TYPE,
								reconstruction_anchor_plate_id,
								reconstruction_time,
								place_holder_replacement,
								resolved_geoms,
								feature_to_collection_map);
					}
				}

				//
				// Plate polygon subsegments.
				//

				if (output_options.export_ridge_transforms)
				{
					export_sub_segments(
							target_dir,
							file_basename,
							placeholder_format_string,
							export_format,
							PLATE_POLYGON_SUB_SEGMENTS_EXPORT_TYPE,
							reconstruction_anchor_plate_id,
							reconstruction_time,
							output_options.placeholder_ridge_transforms,
							output.ridge_transforms,
							feature_to_collection_map);
				}

				if (output_options.export_subductions)
				{
					export_sub_segments(
							target_dir,
							file_basename,
							placeholder_format_string,
							export_format,
							PLATE_POLYGON_SUB_SEGMENTS_EXPORT_TYPE,
							reconstruction_anchor_plate_id,
							reconstruction_time,
							output_options.placeholder_subductions,
							output.subductions,
							feature_to_collection_map);
				}

				if (output_options.export_left_subductions)
				{
					export_sub_segments(
							target_dir,
							file_basename,
							placeholder_format_string,
							export_format,
							PLATE_POLYGON_SUB_SEGMENTS_EXPORT_TYPE,
							reconstruction_anchor_plate_id,
							reconstruction_time,
							output_options.placeholder_left_subductions,
							output.left_subductions,
							feature_to_collection_map);
				}

				if (output_options.export_right_subductions)
				{
					export_sub_segments(
							target_dir,
							file_basename,
							placeholder_format_string,
							export_format,
							PLATE_POLYGON_SUB_SEGMENTS_EXPORT_TYPE,
							reconstruction_anchor_plate_id,
							reconstruction_time,
							output_options.placeholder_right_subductions,
							output.right_subductions,
							feature_to_collection_map);
				}

				//
				// Slab polygons.
				//

				if (output_options.export_all_slab_polygons_to_a_single_file)
				{
					export_resolved_topological_boundaries(
							target_dir,
							file_basename,
							placeholder_format_string,
							export_format,
							SLAB_POLYGON_EXPORT_TYPE,
							reconstruction_anchor_plate_id,
							reconstruction_time,
							output_options.placeholder_slab_polygons,
							output.slab_polygons,
							feature_to_collection_map);
				}

				// If we're also exporting each slab polygon to its own file.
				if (output_options.export_individual_slab_polygon_files)
				{
					// Iterate over the slab polygons and export each separately.
					BOOST_FOREACH(
							const GPlatesAppLogic::ResolvedTopologicalBoundary *resolved_geom,
							output.slab_polygons)
					{
						// We're expecting a plate id as that forms part of the filename.
						if (!resolved_geom->plate_id())
						{
							continue;
						}
						QString place_holder_replacement = "slab_";
						const QString plate_id_string = QString::number(*resolved_geom->plate_id());
						place_holder_replacement.append(plate_id_string);

						// We're exporting a sequence of one resolved geometry to its own file.
						resolved_geom_seq_type resolved_geoms(1, resolved_geom);

						export_resolved_topological_boundaries(
								target_dir,
								file_basename,
								placeholder_format_string,
								export_format,
								SLAB_POLYGON_EXPORT_TYPE,
								reconstruction_anchor_plate_id,
								reconstruction_time,
								place_holder_replacement,
								resolved_geoms,
								feature_to_collection_map);
					}
				}

				//
				// Slab polygon subsegments.
				//

				if (output_options.export_slab_edge_leading)
				{
					export_sub_segments(
							target_dir,
							file_basename,
							placeholder_format_string,
							export_format,
							SLAB_POLYGON_SUB_SEGMENTS_EXPORT_TYPE,
							reconstruction_anchor_plate_id,
							reconstruction_time,
							output_options.placeholder_slab_edge_leading,
							output.slab_edge_leading,
							feature_to_collection_map);
				}

				if (output_options.export_slab_edge_leading_left)
				{
					export_sub_segments(
							target_dir,
							file_basename,
							placeholder_format_string,
							export_format,
							SLAB_POLYGON_SUB_SEGMENTS_EXPORT_TYPE,
							reconstruction_anchor_plate_id,
							reconstruction_time,
							output_options.placeholder_slab_edge_leading_left,
							output.slab_edge_leading_left,
							feature_to_collection_map);
				}

				if (output_options.export_slab_edge_leading_right)
				{
					export_sub_segments(
							target_dir,
							file_basename,
							placeholder_format_string,
							export_format,
							SLAB_POLYGON_SUB_SEGMENTS_EXPORT_TYPE,
							reconstruction_anchor_plate_id,
							reconstruction_time,
							output_options.placeholder_slab_edge_leading_right,
							output.slab_edge_leading_right,
							feature_to_collection_map);
				}

				if (output_options.export_slab_edge_trench)
				{
					export_sub_segments(
							target_dir,
							file_basename,
							placeholder_format_string,
							export_format,
							SLAB_POLYGON_SUB_SEGMENTS_EXPORT_TYPE,
							reconstruction_anchor_plate_id,
							reconstruction_time,
							output_options.placeholder_slab_edge_trench,
							output.slab_edge_trench,
							feature_to_collection_map);
				}

				if (output_options.export_slab_edge_side)
				{
					export_sub_segments(
							target_dir,
							file_basename,
							placeholder_format_string,
							export_format,
							SLAB_POLYGON_SUB_SEGMENTS_EXPORT_TYPE,
							reconstruction_anchor_plate_id,
							reconstruction_time,
							output_options.placeholder_slab_edge_side,
							output.slab_edge_side,
							feature_to_collection_map);
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
		const std::vector<const File::Reference *> &loaded_files,
		const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
		const double &reconstruction_time)
{
	// Information to get exported by the file format exporters.
	Output output;

	collect_exports(
			resolved_geom_seq,
			reconstruction_time,
			output_options,
			output);

	output_exports(
			target_dir,
			file_basename,
			placeholder_format_string,
			export_format,
			loaded_files,
			reconstruction_anchor_plate_id,
			reconstruction_time,
			output_options,
			output);
}
