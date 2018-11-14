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
				// all polygons
				resolved_topologies_seq_type all_polygons;

				// all polygon sub_segment types
				sub_segment_group_seq_type all_boundaries;
				sub_segment_group_seq_type all_boundaries_ridge_transform;
				sub_segment_group_seq_type all_boundaries_subduction;
				sub_segment_group_seq_type all_boundaries_subduction_left;
				sub_segment_group_seq_type all_boundaries_subduction_right;

				// plate polygons
				resolved_topologies_seq_type plate_polygons;

				// plate polygon sub_segment types
				sub_segment_group_seq_type plate_boundaries;
				sub_segment_group_seq_type plate_boundaries_ridge_transform;
				sub_segment_group_seq_type plate_boundaries_subduction;
				sub_segment_group_seq_type plate_boundaries_subduction_left;
				sub_segment_group_seq_type plate_boundaries_subduction_right;

				// network polygons
				resolved_topologies_seq_type network_polygons;

				// network polygon sub_segment types
				sub_segment_group_seq_type network_boundaries;
				sub_segment_group_seq_type network_boundaries_ridge_transform;
				sub_segment_group_seq_type network_boundaries_subduction;
				sub_segment_group_seq_type network_boundaries_subduction_left;
				sub_segment_group_seq_type network_boundaries_subduction_right;

				// slab polygons
				resolved_topologies_seq_type slab_polygons;

				// slab polygon sub_segment types
				sub_segment_group_seq_type slab_edges;
				sub_segment_group_seq_type slab_edges_leading;
				sub_segment_group_seq_type slab_edges_leading_left;
				sub_segment_group_seq_type slab_edges_leading_right;
				sub_segment_group_seq_type slab_edges_trench;
				sub_segment_group_seq_type slab_edges_side;
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
					const GPlatesAppLogic::ResolvedTopologicalGeometrySubSegment::non_null_ptr_type &sub_segment = *sub_segment_iter;

					// The export file with all subsegments (regardless of type).
					if (output_options.export_plate_boundaries)
					{
						output.plate_boundaries.back().sub_segments.push_back(sub_segment.get());
					}
					if (output_options.export_plate_boundaries_to_all_boundaries_file)
					{
						output.all_boundaries.back().sub_segments.push_back(sub_segment.get());
					}

					// Also export the sub segment to another file based on its feature type.

					// Determine the feature type of subsegment.
					const SubSegmentType sub_segment_type =
							get_sub_segment_type(sub_segment->get_feature_ref(), reconstruction_time);

					// Export subsegment depending on its feature type.
					switch (sub_segment_type)
					{
					case SUB_SEGMENT_TYPE_SUBDUCTION_ZONE_LEFT:
						if (output_options.export_plate_boundaries)
						{
							output.plate_boundaries_subduction.back().sub_segments.push_back(sub_segment.get());
							output.plate_boundaries_subduction_left.back().sub_segments.push_back(sub_segment.get());
						}
						if (output_options.export_plate_boundaries_to_all_boundaries_file)
						{
							output.all_boundaries_subduction.back().sub_segments.push_back(sub_segment.get());
							output.all_boundaries_subduction_left.back().sub_segments.push_back(sub_segment.get());
						}
						break;

					case SUB_SEGMENT_TYPE_SUBDUCTION_ZONE_RIGHT:
						if (output_options.export_plate_boundaries)
						{
							output.plate_boundaries_subduction.back().sub_segments.push_back(sub_segment.get());
							output.plate_boundaries_subduction_right.back().sub_segments.push_back(sub_segment.get());
						}
						if (output_options.export_plate_boundaries_to_all_boundaries_file)
						{
							output.all_boundaries_subduction.back().sub_segments.push_back(sub_segment.get());
							output.all_boundaries_subduction_right.back().sub_segments.push_back(sub_segment.get());
						}
						break;

					case SUB_SEGMENT_TYPE_SUBDUCTION_ZONE_UNKNOWN:
						// We know it's a subduction zone but don't know if left or right
						// so export to the subduction zone file only.
						if (output_options.export_plate_boundaries)
						{
							output.plate_boundaries_subduction.back().sub_segments.push_back(sub_segment.get());
						}
						if (output_options.export_plate_boundaries_to_all_boundaries_file)
						{
							output.all_boundaries_subduction.back().sub_segments.push_back(sub_segment.get());
						}
						break;

					case SUB_SEGMENT_TYPE_OTHER:
					default:
						if (output_options.export_plate_boundaries)
						{
							output.plate_boundaries_ridge_transform.back().sub_segments.push_back(sub_segment.get());
						}
						if (output_options.export_plate_boundaries_to_all_boundaries_file)
						{
							output.all_boundaries_ridge_transform.back().sub_segments.push_back(sub_segment.get());
						}
						break;
					}
				}
			}


			void
			add_topological_closed_plate_boundary(
					const GPlatesAppLogic::ReconstructionGeometry *resolved_geom,
					const double &reconstruction_time,
					const OutputOptions &output_options,
					Output &output)
			{
				// Add the plate polygon if it's being exported to 'plate' polygon files.
				if (output_options.export_plate_polygons_to_a_single_file ||
					output_options.export_individual_plate_polygon_files)
				{
					output.plate_polygons.push_back(resolved_geom);
				}

				// Add the plate polygon if it's being exported to the 'all' polygons file.
				if (output_options.export_plate_polygons_to_all_polygons_file)
				{
					output.all_polygons.push_back(resolved_geom);
				}

				// The export files for subsegments.
				if (output_options.export_plate_boundaries ||
					output_options.export_plate_boundaries_to_all_boundaries_file)
				{
					if (output_options.export_plate_boundaries)
					{
						output.plate_boundaries.push_back(SubSegmentGroup(resolved_geom));
						output.plate_boundaries_ridge_transform.push_back(SubSegmentGroup(resolved_geom));
						output.plate_boundaries_subduction.push_back(SubSegmentGroup(resolved_geom));
						output.plate_boundaries_subduction_left.push_back(SubSegmentGroup(resolved_geom));
						output.plate_boundaries_subduction_right.push_back(SubSegmentGroup(resolved_geom));
					}

					if (output_options.export_plate_boundaries_to_all_boundaries_file)
					{
						output.all_boundaries.push_back(SubSegmentGroup(resolved_geom));
						output.all_boundaries_ridge_transform.push_back(SubSegmentGroup(resolved_geom));
						output.all_boundaries_subduction.push_back(SubSegmentGroup(resolved_geom));
						output.all_boundaries_subduction_left.push_back(SubSegmentGroup(resolved_geom));
						output.all_boundaries_subduction_right.push_back(SubSegmentGroup(resolved_geom));
					}

					add_topological_closed_plate_boundary_sub_segments(
							resolved_geom,
							reconstruction_time,
							output_options,
							output);
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
					const GPlatesAppLogic::ResolvedTopologicalGeometrySubSegment::non_null_ptr_type &sub_segment = *sub_segment_iter;

					// The export file with all subsegments (regardless of type).
					if (output_options.export_network_boundaries)
					{
						output.network_boundaries.back().sub_segments.push_back(sub_segment.get());
					}
					if (output_options.export_network_boundaries_to_all_boundaries_file)
					{
						output.all_boundaries.back().sub_segments.push_back(sub_segment.get());
					}

					// Also export the sub segment to another file based on its feature type.

					// Determine the feature type of subsegment.
					const SubSegmentType sub_segment_type =
							get_sub_segment_type(sub_segment->get_feature_ref(), reconstruction_time);

					// Export subsegment depending on its feature type.
					switch (sub_segment_type)
					{
					case SUB_SEGMENT_TYPE_SUBDUCTION_ZONE_LEFT:
						if (output_options.export_network_boundaries)
						{
							output.network_boundaries_subduction.back().sub_segments.push_back(sub_segment.get());
							output.network_boundaries_subduction_left.back().sub_segments.push_back(sub_segment.get());
						}
						if (output_options.export_network_boundaries_to_all_boundaries_file)
						{
							output.all_boundaries_subduction.back().sub_segments.push_back(sub_segment.get());
							output.all_boundaries_subduction_left.back().sub_segments.push_back(sub_segment.get());
						}
						break;

					case SUB_SEGMENT_TYPE_SUBDUCTION_ZONE_RIGHT:
						if (output_options.export_network_boundaries)
						{
							output.network_boundaries_subduction.back().sub_segments.push_back(sub_segment.get());
							output.network_boundaries_subduction_right.back().sub_segments.push_back(sub_segment.get());
						}
						if (output_options.export_network_boundaries_to_all_boundaries_file)
						{
							output.all_boundaries_subduction.back().sub_segments.push_back(sub_segment.get());
							output.all_boundaries_subduction_right.back().sub_segments.push_back(sub_segment.get());
						}
						break;

					case SUB_SEGMENT_TYPE_SUBDUCTION_ZONE_UNKNOWN:
						// We know it's a subduction zone but don't know if left or right
						// so export to the subduction zone file only.
						if (output_options.export_network_boundaries)
						{
							output.network_boundaries_subduction.back().sub_segments.push_back(sub_segment.get());
						}
						if (output_options.export_network_boundaries_to_all_boundaries_file)
						{
							output.all_boundaries_subduction.back().sub_segments.push_back(sub_segment.get());
						}
						break;

					case SUB_SEGMENT_TYPE_OTHER:
					default:
						if (output_options.export_network_boundaries)
						{
							output.network_boundaries_ridge_transform.back().sub_segments.push_back(sub_segment.get());
						}
						if (output_options.export_network_boundaries_to_all_boundaries_file)
						{
							output.all_boundaries_ridge_transform.back().sub_segments.push_back(sub_segment.get());
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
				// Add the network polygon if it's being exported to 'network' polygon files.
				if (output_options.export_network_polygons_to_a_single_file ||
					output_options.export_individual_network_polygon_files)
				{
					output.network_polygons.push_back(resolved_geom);
				}

				// Add the network polygon if it's being exported to the 'all' polygons file.
				if (output_options.export_network_polygons_to_all_polygons_file)
				{
					output.all_polygons.push_back(resolved_geom);
				}

				// The export files for subsegments.
				if (output_options.export_network_boundaries ||
					output_options.export_network_boundaries_to_all_boundaries_file)
				{
					if (output_options.export_network_boundaries)
					{
						output.network_boundaries.push_back(SubSegmentGroup(resolved_geom));
						output.network_boundaries_ridge_transform.push_back(SubSegmentGroup(resolved_geom));
						output.network_boundaries_subduction.push_back(SubSegmentGroup(resolved_geom));
						output.network_boundaries_subduction_left.push_back(SubSegmentGroup(resolved_geom));
						output.network_boundaries_subduction_right.push_back(SubSegmentGroup(resolved_geom));
					}

					if (output_options.export_network_boundaries_to_all_boundaries_file)
					{
						output.all_boundaries.push_back(SubSegmentGroup(resolved_geom));
						output.all_boundaries_ridge_transform.push_back(SubSegmentGroup(resolved_geom));
						output.all_boundaries_subduction.push_back(SubSegmentGroup(resolved_geom));
						output.all_boundaries_subduction_left.push_back(SubSegmentGroup(resolved_geom));
						output.all_boundaries_subduction_right.push_back(SubSegmentGroup(resolved_geom));
					}

					add_topological_network_boundary_sub_segments(
							resolved_geom,
							reconstruction_time,
							output_options,
							output);
				}
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
					const GPlatesAppLogic::ResolvedTopologicalGeometrySubSegment::non_null_ptr_type &sub_segment = *sub_segment_iter;

					// The export file with all subsegments (regardless of type).
					if (output_options.export_slab_boundaries)
					{
						output.slab_edges.back().sub_segments.push_back(sub_segment.get());
					}
					if (output_options.export_slab_boundaries_to_all_boundaries_file)
					{
						output.all_boundaries.back().sub_segments.push_back(sub_segment.get());
					}

					// Also export the sub segment to another file based on its feature type.

					if (output_options.export_slab_boundaries)
					{
						// Determine the feature type of subsegment.
						const SubSegmentType slab_sub_segment_type =
								get_slab_sub_segment_type(sub_segment->get_feature_ref(), reconstruction_time);

						// Export subsegment depending on its feature type.
						switch (slab_sub_segment_type)
						{
						case SUB_SEGMENT_TYPE_SLAB_EDGE_LEADING_LEFT:
							output.slab_edges_leading.back().sub_segments.push_back(sub_segment.get());
							output.slab_edges_leading_left.back().sub_segments.push_back(sub_segment.get());
							break;

						case SUB_SEGMENT_TYPE_SLAB_EDGE_LEADING_RIGHT:
							output.slab_edges_leading.back().sub_segments.push_back(sub_segment.get());
							output.slab_edges_leading_right.back().sub_segments.push_back(sub_segment.get());
							break;

						case SUB_SEGMENT_TYPE_SLAB_EDGE_TRENCH:
							output.slab_edges_trench.back().sub_segments.push_back(sub_segment.get());
							break;

						case SUB_SEGMENT_TYPE_SLAB_EDGE_SIDE:
						default:
							output.slab_edges_side.back().sub_segments.push_back(sub_segment.get());
							break;
						}
					}

					if (output_options.export_slab_boundaries_to_all_boundaries_file)
					{
						// Determine the feature type of subsegment.
						const SubSegmentType sub_segment_type =
								get_sub_segment_type(sub_segment->get_feature_ref(), reconstruction_time);

						// Export subsegment depending on its feature type.
						switch (sub_segment_type)
						{
						case SUB_SEGMENT_TYPE_SUBDUCTION_ZONE_LEFT:
							output.all_boundaries_subduction.back().sub_segments.push_back(sub_segment.get());
							output.all_boundaries_subduction_left.back().sub_segments.push_back(sub_segment.get());
							break;

						case SUB_SEGMENT_TYPE_SUBDUCTION_ZONE_RIGHT:
							output.all_boundaries_subduction.back().sub_segments.push_back(sub_segment.get());
							output.all_boundaries_subduction_right.back().sub_segments.push_back(sub_segment.get());
							break;

						case SUB_SEGMENT_TYPE_SUBDUCTION_ZONE_UNKNOWN:
							// We know it's a subduction zone but don't know if left or right
							// so export to the subduction zone file only.
							output.all_boundaries_subduction.back().sub_segments.push_back(sub_segment.get());
							break;

						case SUB_SEGMENT_TYPE_OTHER:
						default:
							output.all_boundaries_ridge_transform.back().sub_segments.push_back(sub_segment.get());
							break;
						}
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
				// Add the slab polygon if it's being exported to 'slab' polygon files.
				if (output_options.export_slab_polygons_to_a_single_file ||
					output_options.export_individual_slab_polygon_files)
				{
					output.slab_polygons.push_back(resolved_geom);
				}

				// Add the slab polygon if it's being exported to the 'all' polygons file.
				if (output_options.export_slab_polygons_to_all_polygons_file)
				{
					output.all_polygons.push_back(resolved_geom);
				}

				// The export files for subsegments.
				if (output_options.export_slab_boundaries ||
					output_options.export_slab_boundaries_to_all_boundaries_file)
				{
					if (output_options.export_slab_boundaries)
					{
						output.slab_edges.push_back(SubSegmentGroup(resolved_geom));
						output.slab_edges_leading.push_back(SubSegmentGroup(resolved_geom));
						output.slab_edges_leading_left.push_back(SubSegmentGroup(resolved_geom));
						output.slab_edges_leading_right.push_back(SubSegmentGroup(resolved_geom));
						output.slab_edges_trench.push_back(SubSegmentGroup(resolved_geom));
						output.slab_edges_side.push_back(SubSegmentGroup(resolved_geom));
					}

					if (output_options.export_slab_boundaries_to_all_boundaries_file)
					{
						output.all_boundaries.push_back(SubSegmentGroup(resolved_geom));
						output.all_boundaries_ridge_transform.push_back(SubSegmentGroup(resolved_geom));
						output.all_boundaries_subduction.push_back(SubSegmentGroup(resolved_geom));
						output.all_boundaries_subduction_left.push_back(SubSegmentGroup(resolved_geom));
						output.all_boundaries_subduction_right.push_back(SubSegmentGroup(resolved_geom));
					}

					add_topological_slab_boundary_sub_segments(
							resolved_geom,
							reconstruction_time,
							output_options,
							output);
				}
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

					static const GPlatesModel::FeatureType slab_type = 
							GPlatesModel::FeatureType::create_gpml("TopologicalSlabBoundary");
				
					// See if a slab (ie, specifically the "TopologicalSlabBoundary" feature type).
					if (feature_ref.get()->feature_type() == slab_type)
					{	
						add_topological_slab_boundary(
								resolved_geom,
								reconstruction_time,
								output_options,
								output);
					}
					// Otherwise see if a topological polygon.
					//
					// Note: Previously we just tested for the "TopologicalClosedPlateBoundary" feature type, but
					// now we test for any feature containing a topological polygon geometry because it's now
					// possible for almost any feature type to have a topological geometry (where previously it
					// was limited to a few specific feature types).
					else if (GPlatesAppLogic::TopologyUtils::is_topological_boundary_feature(feature_ref.get()))
					{
						add_topological_closed_plate_boundary(
								resolved_geom,
								reconstruction_time,
								output_options,
								output);
					}
					// otherwise see if a topological network...
					else if (GPlatesAppLogic::TopologyUtils::is_topological_network_feature(feature_ref.get()))
					{
						//
						// NOTE: We're just exporting the resolved topological boundary of the network.
						//
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
				// All polygons.
				//

				if (output_options.export_plate_polygons_to_all_polygons_file ||
					output_options.export_network_polygons_to_all_polygons_file ||
					output_options.export_slab_polygons_to_all_polygons_file)
				{
					export_resolved_topological_boundaries(
							target_dir,
							file_basename,
							placeholder_format_string,
							export_format,
							ALL_POLYGON_EXPORT_TYPE,
							reconstruction_anchor_plate_id,
							reconstruction_time,
							output_options.placeholder_all_polygons,
							output.all_polygons,
							feature_to_collection_map,
							active_reconstruction_files,
							output_options.wrap_geometries_to_the_dateline);
				}

				//
				// All polygon subsegments.
				//

				if (output_options.export_plate_boundaries_to_all_boundaries_file ||
					output_options.export_network_boundaries_to_all_boundaries_file ||
					output_options.export_slab_boundaries_to_all_boundaries_file)
				{
					// all boundaries
					export_sub_segments(
							target_dir,
							file_basename,
							placeholder_format_string,
							export_format,
							ALL_SUB_SEGMENTS_EXPORT_TYPE,
							reconstruction_anchor_plate_id,
							reconstruction_time,
							output_options.placeholder_all_boundaries,
							output.all_boundaries,
							feature_to_collection_map,
							active_reconstruction_files,
							output_options.wrap_geometries_to_the_dateline);

					// all boundaries ridge transform
					export_sub_segments(
							target_dir,
							file_basename,
							placeholder_format_string,
							export_format,
							ALL_SUB_SEGMENTS_EXPORT_TYPE,
							reconstruction_anchor_plate_id,
							reconstruction_time,
							output_options.placeholder_all_boundaries_ridge_transform,
							output.all_boundaries_ridge_transform,
							feature_to_collection_map,
							active_reconstruction_files,
							output_options.wrap_geometries_to_the_dateline);

					// all boundaries subduction
					export_sub_segments(
							target_dir,
							file_basename,
							placeholder_format_string,
							export_format,
							ALL_SUB_SEGMENTS_EXPORT_TYPE,
							reconstruction_anchor_plate_id,
							reconstruction_time,
							output_options.placeholder_all_boundaries_subduction,
							output.all_boundaries_subduction,
							feature_to_collection_map,
							active_reconstruction_files,
							output_options.wrap_geometries_to_the_dateline);

					// all boundaries subduction left
					export_sub_segments(
							target_dir,
							file_basename,
							placeholder_format_string,
							export_format,
							ALL_SUB_SEGMENTS_EXPORT_TYPE,
							reconstruction_anchor_plate_id,
							reconstruction_time,
							output_options.placeholder_all_boundaries_subduction_left,
							output.all_boundaries_subduction_left,
							feature_to_collection_map,
							active_reconstruction_files,
							output_options.wrap_geometries_to_the_dateline);

					// all boundaries subduction right
					export_sub_segments(
							target_dir,
							file_basename,
							placeholder_format_string,
							export_format,
							ALL_SUB_SEGMENTS_EXPORT_TYPE,
							reconstruction_anchor_plate_id,
							reconstruction_time,
							output_options.placeholder_all_boundaries_subduction_right,
							output.all_boundaries_subduction_right,
							feature_to_collection_map,
							active_reconstruction_files,
							output_options.wrap_geometries_to_the_dateline);
				}

				//
				// Plate polygons.
				//

				if (output_options.export_plate_polygons_to_a_single_file)
				{
					export_resolved_topological_boundaries(
							target_dir,
							file_basename,
							placeholder_format_string,
							export_format,
							PLATE_POLYGON_EXPORT_TYPE,
							reconstruction_anchor_plate_id,
							reconstruction_time,
							output_options.placeholder_plate_polygons,
							output.plate_polygons,
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
							output.plate_polygons)
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

				if (output_options.export_plate_boundaries)
				{
					// plate boundaries
					export_sub_segments(
							target_dir,
							file_basename,
							placeholder_format_string,
							export_format,
							PLATE_POLYGON_SUB_SEGMENTS_EXPORT_TYPE,
							reconstruction_anchor_plate_id,
							reconstruction_time,
							output_options.placeholder_plate_boundaries,
							output.plate_boundaries,
							feature_to_collection_map,
							active_reconstruction_files,
							output_options.wrap_geometries_to_the_dateline);

					// plate boundaries ridge transform
					export_sub_segments(
							target_dir,
							file_basename,
							placeholder_format_string,
							export_format,
							PLATE_POLYGON_SUB_SEGMENTS_EXPORT_TYPE,
							reconstruction_anchor_plate_id,
							reconstruction_time,
							output_options.placeholder_plate_boundaries_ridge_transform,
							output.plate_boundaries_ridge_transform,
							feature_to_collection_map,
							active_reconstruction_files,
							output_options.wrap_geometries_to_the_dateline);

					// plate boundaries subduction
					export_sub_segments(
							target_dir,
							file_basename,
							placeholder_format_string,
							export_format,
							PLATE_POLYGON_SUB_SEGMENTS_EXPORT_TYPE,
							reconstruction_anchor_plate_id,
							reconstruction_time,
							output_options.placeholder_plate_boundaries_subduction,
							output.plate_boundaries_subduction,
							feature_to_collection_map,
							active_reconstruction_files,
							output_options.wrap_geometries_to_the_dateline);

					// plate boundaries subduction left
					export_sub_segments(
							target_dir,
							file_basename,
							placeholder_format_string,
							export_format,
							PLATE_POLYGON_SUB_SEGMENTS_EXPORT_TYPE,
							reconstruction_anchor_plate_id,
							reconstruction_time,
							output_options.placeholder_plate_boundaries_subduction_left,
							output.plate_boundaries_subduction_left,
							feature_to_collection_map,
							active_reconstruction_files,
							output_options.wrap_geometries_to_the_dateline);

					// plate boundaries subduction right
					export_sub_segments(
							target_dir,
							file_basename,
							placeholder_format_string,
							export_format,
							PLATE_POLYGON_SUB_SEGMENTS_EXPORT_TYPE,
							reconstruction_anchor_plate_id,
							reconstruction_time,
							output_options.placeholder_plate_boundaries_subduction_right,
							output.plate_boundaries_subduction_right,
							feature_to_collection_map,
							active_reconstruction_files,
							output_options.wrap_geometries_to_the_dateline);
				}

				//
				// Slab polygons.
				//

				if (output_options.export_slab_polygons_to_a_single_file)
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

				if (output_options.export_slab_boundaries)
				{
					// slab edges
					export_sub_segments(
							target_dir,
							file_basename,
							placeholder_format_string,
							export_format,
							SLAB_POLYGON_SUB_SEGMENTS_EXPORT_TYPE,
							reconstruction_anchor_plate_id,
							reconstruction_time,
							output_options.placeholder_slab_edges,
							output.slab_edges,
							feature_to_collection_map,
							active_reconstruction_files,
							output_options.wrap_geometries_to_the_dateline);

					// slab edges leading
					export_sub_segments(
							target_dir,
							file_basename,
							placeholder_format_string,
							export_format,
							SLAB_POLYGON_SUB_SEGMENTS_EXPORT_TYPE,
							reconstruction_anchor_plate_id,
							reconstruction_time,
							output_options.placeholder_slab_edges_leading,
							output.slab_edges_leading,
							feature_to_collection_map,
							active_reconstruction_files,
							output_options.wrap_geometries_to_the_dateline);

					// slab edges leading left
					export_sub_segments(
							target_dir,
							file_basename,
							placeholder_format_string,
							export_format,
							SLAB_POLYGON_SUB_SEGMENTS_EXPORT_TYPE,
							reconstruction_anchor_plate_id,
							reconstruction_time,
							output_options.placeholder_slab_edges_leading_left,
							output.slab_edges_leading_left,
							feature_to_collection_map,
							active_reconstruction_files,
							output_options.wrap_geometries_to_the_dateline);

					// slab edges leading right
					export_sub_segments(
							target_dir,
							file_basename,
							placeholder_format_string,
							export_format,
							SLAB_POLYGON_SUB_SEGMENTS_EXPORT_TYPE,
							reconstruction_anchor_plate_id,
							reconstruction_time,
							output_options.placeholder_slab_edges_leading_right,
							output.slab_edges_leading_right,
							feature_to_collection_map,
							active_reconstruction_files,
							output_options.wrap_geometries_to_the_dateline);

					// slab edges trench
					export_sub_segments(
							target_dir,
							file_basename,
							placeholder_format_string,
							export_format,
							SLAB_POLYGON_SUB_SEGMENTS_EXPORT_TYPE,
							reconstruction_anchor_plate_id,
							reconstruction_time,
							output_options.placeholder_slab_edges_trench,
							output.slab_edges_trench,
							feature_to_collection_map,
							active_reconstruction_files,
							output_options.wrap_geometries_to_the_dateline);

					// slab edges side
					export_sub_segments(
							target_dir,
							file_basename,
							placeholder_format_string,
							export_format,
							SLAB_POLYGON_SUB_SEGMENTS_EXPORT_TYPE,
							reconstruction_anchor_plate_id,
							reconstruction_time,
							output_options.placeholder_slab_edges_side,
							output.slab_edges_side,
							feature_to_collection_map,
							active_reconstruction_files,
							output_options.wrap_geometries_to_the_dateline);
				}

				//
				// Network polygons
				//
				// NOTE: We're just exporting the resolved topological boundary of the network.
				//

				if (output_options.export_network_polygons_to_a_single_file)
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

				if (output_options.export_network_boundaries)
				{
					// network boundaries
					export_sub_segments(
							target_dir,
							file_basename,
							placeholder_format_string,
							export_format,
							NETWORK_POLYGON_SUB_SEGMENTS_EXPORT_TYPE,
							reconstruction_anchor_plate_id,
							reconstruction_time,
							output_options.placeholder_network_boundaries,
							output.network_boundaries,
							feature_to_collection_map,
							active_reconstruction_files,
							output_options.wrap_geometries_to_the_dateline);

					// network boundaries ridge transform
					export_sub_segments(
							target_dir,
							file_basename,
							placeholder_format_string,
							export_format,
							NETWORK_POLYGON_SUB_SEGMENTS_EXPORT_TYPE,
							reconstruction_anchor_plate_id,
							reconstruction_time,
							output_options.placeholder_network_boundaries_ridge_transform,
							output.network_boundaries_ridge_transform,
							feature_to_collection_map,
							active_reconstruction_files,
							output_options.wrap_geometries_to_the_dateline);

					// network boundaries subduction
					export_sub_segments(
							target_dir,
							file_basename,
							placeholder_format_string,
							export_format,
							NETWORK_POLYGON_SUB_SEGMENTS_EXPORT_TYPE,
							reconstruction_anchor_plate_id,
							reconstruction_time,
							output_options.placeholder_network_boundaries_subduction,
							output.network_boundaries_subduction,
							feature_to_collection_map,
							active_reconstruction_files,
							output_options.wrap_geometries_to_the_dateline);

					// network boundaries subduction left
					export_sub_segments(
							target_dir,
							file_basename,
							placeholder_format_string,
							export_format,
							NETWORK_POLYGON_SUB_SEGMENTS_EXPORT_TYPE,
							reconstruction_anchor_plate_id,
							reconstruction_time,
							output_options.placeholder_network_boundaries_subduction_left,
							output.network_boundaries_subduction_left,
							feature_to_collection_map,
							active_reconstruction_files,
							output_options.wrap_geometries_to_the_dateline);

					// network boundaries subduction right
					export_sub_segments(
							target_dir,
							file_basename,
							placeholder_format_string,
							export_format,
							NETWORK_POLYGON_SUB_SEGMENTS_EXPORT_TYPE,
							reconstruction_anchor_plate_id,
							reconstruction_time,
							output_options.placeholder_network_boundaries_subduction_right,
							output.network_boundaries_subduction_right,
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
