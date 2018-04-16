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

#include <list>
#include <map>
#include "global/CompilerWarnings.h"
PUSH_MSVC_WARNINGS
// Disable Visual Studio warning "qualifier applied to function type has no meaning; ignored" in boost 1.36.0
DISABLE_MSVC_WARNING(4181)
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
POP_MSVC_WARNINGS

// Suppress compiler warning 4503 'decorated name length exceeded' in Visual Studio 2008.
// It's too hard to avoid using the 'convert typedef to struct' trick.
DISABLE_MSVC_WARNING(4503)

#include "CitcomsResolvedTopologicalBoundaryExport.h"

#include "CitcomsGMTFormatResolvedTopologicalBoundaryExport.h"
#include "FeatureCollectionFileFormat.h"
#include "FeatureCollectionFileFormatRegistry.h"
#include "FileFormatNotSupportedException.h"
#include "ReconstructionGeometryExportImpl.h"
#include "CitcomsResolvedTopologicalBoundaryExportImpl.h"
#include "OgrFormatResolvedTopologicalGeometryExport.h"

#include "app-logic/GeometryUtils.h"
#include "app-logic/TopologyUtils.h"

#include "feature-visitors/PropertyValueFinder.h"

#include "model/PropertyName.h"

#include "property-values/GmlPoint.h"

#include <boost/foreach.hpp>

using namespace GPlatesFileIO::ReconstructionGeometryExportImpl;
using namespace GPlatesFileIO::CitcomsResolvedTopologicalBoundaryExportImpl;


namespace GPlatesFileIO
{
	namespace CitcomsResolvedTopologicalBoundaryExport
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
				// one file with all sub segments from all topologies 
				sub_segment_group_seq_type lines;

				// plate polygons
				resolved_topologies_seq_type platepolygons;

				// plate polygon sub_segment types
				sub_segment_group_seq_type ridge_transforms;
				sub_segment_group_seq_type subductions;
				sub_segment_group_seq_type left_subductions;
				sub_segment_group_seq_type right_subductions;

				// network polygons
				resolved_topologies_seq_type network_polygons;

				// network polygon sub_segment types
				sub_segment_group_seq_type network_ridge_transforms;
				sub_segment_group_seq_type network_subductions;
				sub_segment_group_seq_type network_left_subductions;
				sub_segment_group_seq_type network_right_subductions;

				// slab polygons
				resolved_topologies_seq_type slab_polygons;

				// slab polygon sub_segment types
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
					sub_segment_ptr_seq_type::const_iterator sub_segment_iter;
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

						const GPlatesFileIO::File::Reference *file = map_iter->second.first;
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
			add_topological_closed_plate_boundary_sub_segments(
					const GPlatesAppLogic::ReconstructionGeometry *resolved_geom,
					const double &reconstruction_time,
					const OutputOptions &output_options,
					Output &output)
			{
				// Get the resolved boundary subsegments.
				boost::optional<const GPlatesAppLogic::sub_segment_seq_type &> boundary_sub_segments =
						GPlatesAppLogic::ReconstructionGeometryUtils
								::get_resolved_topological_boundary_sub_segment_sequence(resolved_geom);
				// If not a ResolvedTopologicalBoundary or ResolvedTopologicalNetwork then skip.
				if (!boundary_sub_segments)
				{
					return;
				}

				// Iterate over the subsegments contained in the current resolved topological geometry.
				GPlatesAppLogic::sub_segment_seq_type::const_iterator sub_segment_iter = boundary_sub_segments->begin();
				GPlatesAppLogic::sub_segment_seq_type::const_iterator sub_segment_end = boundary_sub_segments->end();
				for ( ; sub_segment_iter != sub_segment_end; ++sub_segment_iter)
				{
					const GPlatesAppLogic::ResolvedTopologicalGeometrySubSegment &sub_segment = *sub_segment_iter;

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
			}


			void
			add_topological_network_boundary_sub_segments(
					const GPlatesAppLogic::ReconstructionGeometry *resolved_geom,
					const double &reconstruction_time,
					const OutputOptions &output_options,
					Output &output)
			{
				// Get the resolved boundary subsegments.
				boost::optional<const GPlatesAppLogic::sub_segment_seq_type &> boundary_sub_segments =
						GPlatesAppLogic::ReconstructionGeometryUtils
								::get_resolved_topological_boundary_sub_segment_sequence(resolved_geom);
				// If not a ResolvedTopologicalBoundary or ResolvedTopologicalNetwork then skip.
				if (!boundary_sub_segments)
				{
					return;
				}

				// Iterate over the subsegments contained in the current resolved topological geometry.
				GPlatesAppLogic::sub_segment_seq_type::const_iterator sub_segment_iter = boundary_sub_segments->begin();
				GPlatesAppLogic::sub_segment_seq_type::const_iterator sub_segment_end = boundary_sub_segments->end();
				for ( ; sub_segment_iter != sub_segment_end; ++sub_segment_iter)
				{
					const GPlatesAppLogic::ResolvedTopologicalGeometrySubSegment &sub_segment = *sub_segment_iter;

					// The export file with all subsegments (regardless of type).
					if (output_options.export_network_polygon_subsegments_to_lines)
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
						if (output_options.export_network_subductions)
						{
							output.network_subductions.back().sub_segments.push_back(&sub_segment);
						}
						if (output_options.export_network_left_subductions)
						{
							output.network_left_subductions.back().sub_segments.push_back(&sub_segment);
						}
						break;

					case SUB_SEGMENT_TYPE_SUBDUCTION_ZONE_RIGHT:
						if (output_options.export_network_subductions)
						{
							output.network_subductions.back().sub_segments.push_back(&sub_segment);
						}
						if (output_options.export_network_right_subductions)
						{
							output.network_right_subductions.back().sub_segments.push_back(&sub_segment);
						}
						break;

					case SUB_SEGMENT_TYPE_SUBDUCTION_ZONE_UNKNOWN:
						// We know it's a subduction zone but don't know if left or right
						// so export to the subduction zone file only.
						if (output_options.export_network_subductions)
						{
							output.network_subductions.back().sub_segments.push_back(&sub_segment);
						}
						break;

					case SUB_SEGMENT_TYPE_OTHER:
					default:
						if (output_options.export_network_ridge_transforms)
						{
							output.network_ridge_transforms.back().sub_segments.push_back(&sub_segment);
						}
						break;
					}
				}
			}


			void
			add_topological_network_boundary(
					const GPlatesAppLogic::ReconstructionGeometry *resolved_geom,
					const double &reconstruction_time,
					const OutputOptions &output_options,
					Output &output)
			{
				// Add the plate polygon if they are being exported.
				if (output_options.export_all_network_polygons_to_a_single_file ||
					output_options.export_individual_network_polygon_files)
				{
					output.network_polygons.push_back(resolved_geom);
				}

				// The export file with all subsegments (regardless of type).
				if (output_options.export_network_polygon_subsegments_to_lines)
				{
					output.lines.push_back(SubSegmentGroup(resolved_geom));
				}
				// The export file for all subductions.
				if (output_options.export_network_subductions)
				{
					output.network_subductions.push_back(SubSegmentGroup(resolved_geom));
				}
				// The export file for all left subductions.
				if (output_options.export_network_left_subductions)
				{
					output.network_left_subductions.push_back(SubSegmentGroup(resolved_geom));
				}
				// The export file for all right subductions.
				if (output_options.export_network_right_subductions)
				{
					output.network_right_subductions.push_back(SubSegmentGroup(resolved_geom));
				}
				// The export file for all ridge transforms.
				if (output_options.export_network_ridge_transforms)
				{
					output.network_ridge_transforms.push_back(SubSegmentGroup(resolved_geom));
				}

				add_topological_network_boundary_sub_segments(
						resolved_geom,
						reconstruction_time,
						output_options,
						output);
			}


			void
			add_topological_closed_plate_boundary(
					const GPlatesAppLogic::ReconstructionGeometry *resolved_geom,
					const double &reconstruction_time,
					const OutputOptions &output_options,
					Output &output)
			{
				// Add the plate polygon if they are being exported.
				if (output_options.export_all_plate_polygons_to_a_single_file ||
					output_options.export_individual_plate_polygon_files)
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

				add_topological_closed_plate_boundary_sub_segments(
						resolved_geom,
						reconstruction_time,
						output_options,
						output);
			}


			void
			add_topological_slab_boundary_sub_segments(
					const GPlatesAppLogic::ReconstructionGeometry *resolved_geom,
					const double &reconstruction_time,
					const OutputOptions &output_options,
					Output &output)
			{
				// Get the resolved boundary subsegments.
				boost::optional<const GPlatesAppLogic::sub_segment_seq_type &> boundary_sub_segments =
						GPlatesAppLogic::ReconstructionGeometryUtils
								::get_resolved_topological_boundary_sub_segment_sequence(resolved_geom);
				// If not a ResolvedTopologicalBoundary or ResolvedTopologicalNetwork then skip.
				if (!boundary_sub_segments)
				{
					return;
				}

				// Iterate over the subsegments contained in the current resolved topological geometry.
				GPlatesAppLogic::sub_segment_seq_type::const_iterator sub_segment_iter = boundary_sub_segments->begin();
				GPlatesAppLogic::sub_segment_seq_type::const_iterator sub_segment_end = boundary_sub_segments->end();
				for ( ; sub_segment_iter != sub_segment_end; ++sub_segment_iter)
				{
					const GPlatesAppLogic::ResolvedTopologicalGeometrySubSegment &sub_segment = *sub_segment_iter;

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
			}


			void
			add_topological_slab_boundary(
					const GPlatesAppLogic::ReconstructionGeometry *resolved_geom,
					const double &reconstruction_time,
					const OutputOptions &output_options,
					Output &output)
			{
				// Add the slab polygon if they are being exported.
				if (output_options.export_all_slab_polygons_to_a_single_file ||
					output_options.export_individual_slab_polygon_files)
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

				add_topological_slab_boundary_sub_segments(
						resolved_geom,
						reconstruction_time,
						output_options,
						output);
			}


			void
			collect_exports(
					const resolved_topologies_seq_type &resolved_topologies,
					const double &reconstruction_time,
					const OutputOptions &output_options,
					Output &output)
			{
				// Iterate over the resolved topological geometries and
				// collect information for the file format exporter.
				resolved_topologies_seq_type::const_iterator resolved_seq_iter = resolved_topologies.begin();
				resolved_topologies_seq_type::const_iterator resolved_seq_end = resolved_topologies.end();
				for ( ; resolved_seq_iter != resolved_seq_end; ++resolved_seq_iter)
				{
					const GPlatesAppLogic::ReconstructionGeometry *resolved_geom = *resolved_seq_iter;

					// Feature handle reference to topology feature.
					boost::optional<GPlatesModel::FeatureHandle::weak_ref> feature_ref =
							GPlatesAppLogic::ReconstructionGeometryUtils::get_feature_ref(resolved_geom);
					if (!feature_ref)
					{
						continue;
					}

					//
					// FIXME
					//
					// Are interested specifically in feature types
					// "TopologicalClosedPlateBoundary" and "TopologicalSlabBoundary" ?
					// Otherwise it's better to just select any feature that has a
					// 'gpml:TopologicalPolygon' property (regardless of feature type).
					// Because it's now possible for almost any feature type to have a topological
					// geometry (where previously it was limited to a few specific feature types).
					// In which case we should use
					// "GPlatesAppLogic::TopologyUtils::is_topological_boundary_feature()".
					//

					static const GPlatesModel::FeatureType plate_type = 
						GPlatesModel::FeatureType::create_gpml("TopologicalClosedPlateBoundary");

					if (feature_ref.get()->feature_type() == plate_type)
					{
						add_topological_closed_plate_boundary(
								resolved_geom,
								reconstruction_time,
								output_options,
								output);
					}


					static const GPlatesModel::FeatureType slab_type = 
						GPlatesModel::FeatureType::create_gpml("TopologicalSlabBoundary");
				
					if (feature_ref.get()->feature_type() == slab_type)
					{	
						add_topological_slab_boundary(
								resolved_geom,
								reconstruction_time,
								output_options,
								output);
					}


					//
					// NOTE: We're just exporting the resolved topological boundary of the network.
					//
					if (GPlatesAppLogic::TopologyUtils::is_topological_network_feature(feature_ref.get()))
					{
						add_topological_network_boundary(
								resolved_geom,
								reconstruction_time,
								output_options,
								output);
					}

				}
			}


			/**
			 * Exports a sequence of resolved topological boundaries to the specified export file format.
			 */
			void
			export_resolved_topological_boundaries_file(
					const QString &filename,
					Format export_format,
					ResolvedTopologicalBoundaryExportType export_type,
					const resolved_topologies_seq_type &resolved_topologies,
					const std::vector<const File::Reference *> &referenced_files,
					const std::vector<const File::Reference *> &active_reconstruction_files,
					const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
					const double &reconstruction_time,
					bool wrap_to_dateline)
			{
				if (resolved_topologies.empty())
				{
					return;
				}

				switch (export_format)
				{
				case GMT:
					CitcomsGMTFormatResolvedTopologicalBoundaryExport::export_resolved_topological_boundaries(
						resolved_topologies,
						export_type,
						filename,
						referenced_files,
						active_reconstruction_files,
						reconstruction_anchor_plate_id,
						reconstruction_time);
					break;

                // Both SHAPEFILE and OGRGMT formats use the same
                // OgrFormat.... exporter.
				case SHAPEFILE:
                case OGRGMT:
					OgrFormatResolvedTopologicalGeometryExport::export_citcoms_resolved_topological_boundaries(
						resolved_topologies,
						filename,
						referenced_files,
						active_reconstruction_files,
						reconstruction_anchor_plate_id,
						reconstruction_time,
						wrap_to_dateline);		
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
					const std::vector<const File::Reference *> &active_reconstruction_files,
					const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
					const double &reconstruction_time,
					bool wrap_to_dateline)
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
					CitcomsGMTFormatResolvedTopologicalBoundaryExport::export_sub_segments(
						sub_segment_groups,
						export_type,
						filename,
						referenced_files,
						active_reconstruction_files,
						reconstruction_anchor_plate_id,
						reconstruction_time);
					break;

                // Both SHAPEFILE and OGRGMT formats use the same
                // OgrFormat.... exporter.
                case SHAPEFILE:
                case OGRGMT:
					OgrFormatResolvedTopologicalGeometryExport::export_citcoms_sub_segments(
						sub_segment_groups,
						filename,
						referenced_files,
						active_reconstruction_files,
						reconstruction_anchor_plate_id,
						reconstruction_time,
						wrap_to_dateline);		
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
					const resolved_topologies_seq_type &resolved_topologies,
					const feature_handle_to_collection_map_type &feature_to_collection_map,
					const std::vector<const File::Reference *> &active_reconstruction_files,
					bool wrap_to_dateline)
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
						resolved_topologies,
						feature_to_collection_map);

				export_resolved_topological_boundaries_file(
						filename,
						export_format,
						export_type,
						resolved_topologies,
						referenced_files,
						active_reconstruction_files,
						reconstruction_anchor_plate_id,
						reconstruction_time,
						wrap_to_dateline);
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
					const feature_handle_to_collection_map_type &feature_to_collection_map,
					const std::vector<const File::Reference *> &active_reconstruction_files,
					bool wrap_to_dateline)
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
						active_reconstruction_files,
						reconstruction_anchor_plate_id,
						reconstruction_time,
						wrap_to_dateline);
			}


			void
			output_exports(
					const QDir &target_dir,
					const QString &file_basename,
					const QString &placeholder_format_string,
					Format export_format,
					const std::vector<const File::Reference *> &loaded_files,
					const std::vector<const File::Reference *> &active_reconstruction_files,
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
					output_options.export_slab_polygon_subsegments_to_lines ||
					output_options.export_network_polygon_subsegments_to_lines)
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
							feature_to_collection_map,
							active_reconstruction_files,
							output_options.wrap_geometries_to_the_dateline);
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
							feature_to_collection_map,
							active_reconstruction_files,
							output_options.wrap_geometries_to_the_dateline);
				}

				// If we're also exporting each plate polygon to its own file.
				if (output_options.export_individual_plate_polygon_files)
				{
					// Iterate over the plate geometries and group them by plate id.
					// Export each plate id separately.
					// We're really supposed to export each plate geometry separately but
					// it's possible to have multiple plate geometries with the same plate id
					// and previously this was causing the same export file to be overwritten
					// as each subsequent plate geometry with the same plate id was exported.

					// Track all reconstruction geometries associated with each plate id.
					typedef std::map<
							GPlatesModel::integer_plate_id_type,
							std::vector<const GPlatesAppLogic::ReconstructionGeometry *>
					> plate_id_resolved_geoms_map_type;
					plate_id_resolved_geoms_map_type plate_id_resolved_geoms;

					// Group resolved geometries by plate id.
					BOOST_FOREACH(
							const GPlatesAppLogic::ReconstructionGeometry *resolved_geom,
							output.platepolygons)
					{
						// We're expecting a plate id as that will form part of the filename.
						boost::optional<GPlatesModel::integer_plate_id_type> resolved_geom_plate_id =
								GPlatesAppLogic::ReconstructionGeometryUtils::get_plate_id( resolved_geom);
						if (!resolved_geom_plate_id)
						{
							resolved_geom_plate_id = 0;
						}

						plate_id_resolved_geoms[resolved_geom_plate_id.get()].push_back(resolved_geom);
					}

					// Export each plate id file.
					BOOST_FOREACH(
							const plate_id_resolved_geoms_map_type::value_type &plate_id_resolved_geoms_map_entry,
							plate_id_resolved_geoms)
					{
						const GPlatesModel::integer_plate_id_type resolved_geom_plate_id =
								plate_id_resolved_geoms_map_entry.first;
						const std::vector<const GPlatesAppLogic::ReconstructionGeometry *> &resolved_geoms =
								plate_id_resolved_geoms_map_entry.second;

						QString place_holder_replacement = "plate_";
						const QString plate_id_string = QString::number(resolved_geom_plate_id);
						place_holder_replacement.append(plate_id_string);

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
								feature_to_collection_map,
								active_reconstruction_files,
								output_options.wrap_geometries_to_the_dateline);
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
							feature_to_collection_map,
							active_reconstruction_files,
							output_options.wrap_geometries_to_the_dateline);
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
							feature_to_collection_map,
							active_reconstruction_files,
							output_options.wrap_geometries_to_the_dateline);
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
							feature_to_collection_map,
							active_reconstruction_files,
							output_options.wrap_geometries_to_the_dateline);
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
							feature_to_collection_map,
							active_reconstruction_files,
							output_options.wrap_geometries_to_the_dateline);
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
							feature_to_collection_map,
							active_reconstruction_files,
							output_options.wrap_geometries_to_the_dateline);
				}

				// If we're also exporting each slab polygon to its own file.
				if (output_options.export_individual_slab_polygon_files)
				{
					// Iterate over the slab geometries and group them by plate id.
					// Export each plate id separately.
					// We're really supposed to export each slab geometry separately but
					// it's possible to have multiple slab geometries with the same plate id
					// and previously this was causing the same export file to be overwritten
					// as each subsequent slab geometry with the same plate id was exported.

					// Track all reconstruction geometries associated with each plate id.
					typedef std::map<
							GPlatesModel::integer_plate_id_type,
							std::vector<const GPlatesAppLogic::ReconstructionGeometry *>
					> plate_id_resolved_geoms_map_type;
					plate_id_resolved_geoms_map_type plate_id_resolved_geoms;

					// Group resolved geometries by plate id.
					BOOST_FOREACH(
							const GPlatesAppLogic::ReconstructionGeometry *resolved_geom,
							output.slab_polygons)
					{
						// We're expecting a plate id as that will form part of the filename.
						boost::optional<GPlatesModel::integer_plate_id_type> resolved_geom_plate_id =
								GPlatesAppLogic::ReconstructionGeometryUtils::get_plate_id( resolved_geom);
						if (!resolved_geom_plate_id)
						{
							resolved_geom_plate_id = 0;
						}

						plate_id_resolved_geoms[resolved_geom_plate_id.get()].push_back(resolved_geom);
					}

					// Export each plate id file.
					BOOST_FOREACH(
							const plate_id_resolved_geoms_map_type::value_type &plate_id_resolved_geoms_map_entry,
							plate_id_resolved_geoms)
					{
						const GPlatesModel::integer_plate_id_type resolved_geom_plate_id =
								plate_id_resolved_geoms_map_entry.first;
						const std::vector<const GPlatesAppLogic::ReconstructionGeometry *> &resolved_geoms =
								plate_id_resolved_geoms_map_entry.second;

						QString place_holder_replacement = "slab_";
						const QString plate_id_string = QString::number(resolved_geom_plate_id);
						place_holder_replacement.append(plate_id_string);

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
								feature_to_collection_map,
								active_reconstruction_files,
								output_options.wrap_geometries_to_the_dateline);
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
							feature_to_collection_map,
							active_reconstruction_files,
							output_options.wrap_geometries_to_the_dateline);
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
							feature_to_collection_map,
							active_reconstruction_files,
							output_options.wrap_geometries_to_the_dateline);
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
							feature_to_collection_map,
							active_reconstruction_files,
							output_options.wrap_geometries_to_the_dateline);
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
							feature_to_collection_map,
							active_reconstruction_files,
							output_options.wrap_geometries_to_the_dateline);
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
							feature_to_collection_map,
							active_reconstruction_files,
							output_options.wrap_geometries_to_the_dateline);
				}

				//
				// Network polygons
				//
				// NOTE: We're just exporting the resolved topological boundary of the network.
				//

				if (output_options.export_all_network_polygons_to_a_single_file)
				{
					export_resolved_topological_boundaries(
							target_dir,
							file_basename,
							placeholder_format_string,
							export_format,
							NETWORK_POLYGON_EXPORT_TYPE,
							reconstruction_anchor_plate_id,
							reconstruction_time,
							output_options.placeholder_networks,
							output.network_polygons,
							feature_to_collection_map,
							active_reconstruction_files,
							output_options.wrap_geometries_to_the_dateline);
				}

				// If we're also exporting each plate polygon to its own file.
				if (output_options.export_individual_network_polygon_files)
				{
					// Iterate over the topological networks and group by plate id.
					// Export each plate id separately.
					// We're really supposed to export each network geometry separately but
					// it's possible to have multiple network geometries with the same plate id
					// and previously this was causing the same export file to be overwritten
					// as each subsequent network geometry with the same plate id was exported.

					// Track all reconstruction geometries associated with each plate id.
					typedef std::map<
							GPlatesModel::integer_plate_id_type,
							std::vector<const GPlatesAppLogic::ReconstructionGeometry *>
					> plate_id_resolved_geoms_map_type;
					plate_id_resolved_geoms_map_type plate_id_resolved_geoms;

					// Group resolved geometries by plate id.
					BOOST_FOREACH(
							const GPlatesAppLogic::ReconstructionGeometry *resolved_geom,
							output.network_polygons)
					{
						// We're expecting a plate id as that will form part of the filename.
						boost::optional<GPlatesModel::integer_plate_id_type> resolved_geom_plate_id =
								GPlatesAppLogic::ReconstructionGeometryUtils::get_plate_id( resolved_geom);
						if (!resolved_geom_plate_id)
						{
							resolved_geom_plate_id = 0;
						}

						plate_id_resolved_geoms[resolved_geom_plate_id.get()].push_back(resolved_geom);
					}

					// Export each plate id file.
					BOOST_FOREACH(
							const plate_id_resolved_geoms_map_type::value_type &plate_id_resolved_geoms_map_entry,
							plate_id_resolved_geoms)
					{
						const GPlatesModel::integer_plate_id_type resolved_geom_plate_id =
								plate_id_resolved_geoms_map_entry.first;
						const std::vector<const GPlatesAppLogic::ReconstructionGeometry *> &resolved_geoms =
								plate_id_resolved_geoms_map_entry.second;

						QString place_holder_replacement = "network_";
						const QString plate_id_string = QString::number(resolved_geom_plate_id);
						place_holder_replacement.append(plate_id_string);

						export_resolved_topological_boundaries(
								target_dir,
								file_basename,
								placeholder_format_string,
								export_format,
								NETWORK_POLYGON_EXPORT_TYPE,
								reconstruction_anchor_plate_id,
								reconstruction_time,
								place_holder_replacement,
								resolved_geoms,
								feature_to_collection_map,
								active_reconstruction_files,
								output_options.wrap_geometries_to_the_dateline);
					}
				}

				//
				// Network polygon subsegments.
				//

				if (output_options.export_network_ridge_transforms)
				{
					export_sub_segments(
							target_dir,
							file_basename,
							placeholder_format_string,
							export_format,
							NETWORK_POLYGON_SUB_SEGMENTS_EXPORT_TYPE,
							reconstruction_anchor_plate_id,
							reconstruction_time,
							output_options.placeholder_network_ridge_transforms,
							output.network_ridge_transforms,
							feature_to_collection_map,
							active_reconstruction_files,
							output_options.wrap_geometries_to_the_dateline);
				}

				if (output_options.export_network_subductions)
				{
					export_sub_segments(
							target_dir,
							file_basename,
							placeholder_format_string,
							export_format,
							NETWORK_POLYGON_SUB_SEGMENTS_EXPORT_TYPE,
							reconstruction_anchor_plate_id,
							reconstruction_time,
							output_options.placeholder_network_subductions,
							output.network_subductions,
							feature_to_collection_map,
							active_reconstruction_files,
							output_options.wrap_geometries_to_the_dateline);
				}

				if (output_options.export_network_left_subductions)
				{
					export_sub_segments(
							target_dir,
							file_basename,
							placeholder_format_string,
							export_format,
							NETWORK_POLYGON_SUB_SEGMENTS_EXPORT_TYPE,
							reconstruction_anchor_plate_id,
							reconstruction_time,
							output_options.placeholder_network_left_subductions,
							output.network_left_subductions,
							feature_to_collection_map,
							active_reconstruction_files,
							output_options.wrap_geometries_to_the_dateline);
				}

				if (output_options.export_network_right_subductions)
				{
					export_sub_segments(
							target_dir,
							file_basename,
							placeholder_format_string,
							export_format,
							NETWORK_POLYGON_SUB_SEGMENTS_EXPORT_TYPE,
							reconstruction_anchor_plate_id,
							reconstruction_time,
							output_options.placeholder_network_right_subductions,
							output.network_right_subductions,
							feature_to_collection_map,
							active_reconstruction_files,
							output_options.wrap_geometries_to_the_dateline);
				}
			}
		}
	}
}


GPlatesFileIO::CitcomsResolvedTopologicalBoundaryExport::Format
GPlatesFileIO::CitcomsResolvedTopologicalBoundaryExport::get_export_file_format(
		const QFileInfo& file_info,
		const FeatureCollectionFileFormat::Registry &file_format_registry)
{
	// Since we're using a feature collection file format to export
	// our RFGs we'll use the feature collection file format code.
	const boost::optional<FeatureCollectionFileFormat::Format> feature_collection_file_format =
			file_format_registry.get_file_format(file_info);
	if (!feature_collection_file_format ||
		!file_format_registry.does_file_format_support_writing(feature_collection_file_format.get()))
	{
		return UNKNOWN;
	}

	// Only some feature collection file formats are used for exporting
	// reconstructed feature geometries because most file formats only
	// make sense for unreconstructed geometry (since they provide the
	// information required to do the reconstructions).
	switch (feature_collection_file_format.get())
	{
	case FeatureCollectionFileFormat::WRITE_ONLY_XY_GMT:
		return GMT;
	case FeatureCollectionFileFormat::OGRGMT:
		return OGRGMT;
	case FeatureCollectionFileFormat::SHAPEFILE:
		return SHAPEFILE;
	default:
		break;
	}

	return UNKNOWN;
}


void
GPlatesFileIO::CitcomsResolvedTopologicalBoundaryExport::export_resolved_topological_boundaries(
		const QDir &target_dir,
		const QString &file_basename,
		const QString &placeholder_format_string,
		const OutputOptions &output_options,
		Format export_format,
		const resolved_topologies_seq_type &resolved_topologies,
		const std::vector<const File::Reference *> &loaded_files,
		const std::vector<const File::Reference *> &active_reconstruction_files,
		const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
		const double &reconstruction_time)
{
	// Information to get exported by the file format exporters.
	Output output;

	collect_exports(
			resolved_topologies,
			reconstruction_time,
			output_options,
			output);

	output_exports(
			target_dir,
			file_basename,
			placeholder_format_string,
			export_format,
			loaded_files,
			active_reconstruction_files,
			reconstruction_anchor_plate_id,
			reconstruction_time,
			output_options,
			output);
}
