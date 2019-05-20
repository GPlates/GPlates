/* $Id$ */

/**
 * \file Exports visible reconstructed feature geometries to a file.
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#include <algorithm>
#include <iterator>
#include <vector>
#include <boost/foreach.hpp>
#include <QDir>
#include <QFileInfo>
#include <QString>

#include "VisibleReconstructionGeometryExport.h"

#include "RenderedGeometryUtils.h"
#include "RenderedGeometryCollection.h"

#include "app-logic/ReconstructionGeometryUtils.h"
#include "app-logic/ResolvedTopologicalBoundary.h"
#include "app-logic/ResolvedTopologicalLine.h"
#include "app-logic/ResolvedTopologicalNetwork.h"
#include "app-logic/TopologyUtils.h"

#include "feature-visitors/PropertyValueFinder.h"

#include "file-io/ReconstructedFeatureGeometryExport.h"
#include "file-io/ReconstructedFlowlineExport.h"
#include "file-io/ReconstructedMotionPathExport.h"
#include "file-io/ResolvedTopologicalGeometryExport.h"

#include "model/FeatureHandle.h"
#include "model/FeatureType.h"
#include "model/PropertyName.h"

#include "property-values/EnumerationContent.h"
#include "property-values/EnumerationType.h"


namespace GPlatesViewOperations
{
	namespace VisibleReconstructionGeometryExport
	{
		namespace
		{
			//! Convenience typedef for sequence of RFGs.
			typedef std::vector<const GPlatesAppLogic::ReconstructedFeatureGeometry *> reconstructed_feature_geom_seq_type;

			//! Convenience typedef for sequence of reconstructed flowline geometries.
			typedef std::vector<const GPlatesAppLogic::ReconstructedFlowline *> reconstructed_flowline_seq_type;

			//! Convenience typedef for sequence of reconstructed motion track geometries.
			typedef std::vector<const GPlatesAppLogic::ReconstructedMotionPath *> reconstructed_motion_path_seq_type;

			//! Convenience typedef for sequence of resolved topologies.
			typedef std::vector<const GPlatesAppLogic::ReconstructionGeometry *> resolved_topologies_seq_type;


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


			//! Export type of resolved topological sections.
			enum ExportTopologicalSectionType
			{
				EXPORT_TOPOLOGICAL_SECTIONS_ALL,
				EXPORT_TOPOLOGICAL_SECTIONS_SUBDUCTION,
				EXPORT_TOPOLOGICAL_SECTIONS_SUBDUCTION_LEFT,
				EXPORT_TOPOLOGICAL_SECTIONS_SUBDUCTION_RIGHT,
				EXPORT_TOPOLOGICAL_SECTIONS_RIDGE_TRANFORM
			};

			void
			export_resolved_topological_sections(
					const std::vector<GPlatesAppLogic::ResolvedTopologicalSection::non_null_ptr_type> &resolved_topological_sections,
					const QDir &target_dir,
					const QString &file_basename,
					const QString &placeholder_format_string,
					const QString &placeholder_topological_sections,
					ExportTopologicalSectionType export_topological_section_type,
					const GPlatesFileIO::FeatureCollectionFileFormat::Registry &file_format_registry,
					const files_collection_type &active_files,
					const files_collection_type &active_reconstruction_files,
					const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
					const double &reconstruction_time,
					bool export_single_output_file,
					bool export_per_input_file,
					bool export_separate_output_directory_per_input_file,
					bool wrap_to_dateline)
			{
				// Filter out the resolved topological sections we want to export and convert them to raw pointers.
				std::vector<const GPlatesAppLogic::ResolvedTopologicalSection *> filtered_resolved_topological_section_ptrs;
				BOOST_FOREACH(
						GPlatesAppLogic::ResolvedTopologicalSection::non_null_ptr_type resolved_topological_section,
						resolved_topological_sections)
				{
					if (export_topological_section_type == EXPORT_TOPOLOGICAL_SECTIONS_ALL)
					{
						filtered_resolved_topological_section_ptrs.push_back(resolved_topological_section.get());
					}
					else // export a subset of sections (ie, not all sections)...
					{
						static const GPlatesModel::FeatureType subduction_zone_type =
								GPlatesModel::FeatureType::create_gpml("SubductionZone");

						// If something is not a subduction zone then it is considering a ridge/transform.
						if (resolved_topological_section->get_feature_ref()->feature_type() == subduction_zone_type)
						{
							if (export_topological_section_type == EXPORT_TOPOLOGICAL_SECTIONS_SUBDUCTION)
							{
								filtered_resolved_topological_section_ptrs.push_back(resolved_topological_section.get());
							}
							else // exporting a left or right subduction zone...
							{
								static const GPlatesModel::PropertyName subduction_polarity_property_name =
										GPlatesModel::PropertyName::create_gpml("subductionPolarity");

								// Check for subduction polarity enumeration.
								boost::optional<GPlatesPropertyValues::Enumeration::non_null_ptr_to_const_type> subduction_polarity_enum =
										GPlatesFeatureVisitors::get_property_value<GPlatesPropertyValues::Enumeration>(
												resolved_topological_section->get_feature_ref(),
												subduction_polarity_property_name,
												reconstruction_time);
								if (subduction_polarity_enum)
								{
									static const GPlatesPropertyValues::EnumerationType subduction_polarity_enumeration_type =
											GPlatesPropertyValues::EnumerationType::create_gpml("SubductionPolarityEnumeration");

									if (subduction_polarity_enumeration_type.is_equal_to(subduction_polarity_enum.get()->get_type()))
									{
										static const GPlatesPropertyValues::EnumerationContent left("Left");
										static const GPlatesPropertyValues::EnumerationContent right("Right");

										if (left.is_equal_to(subduction_polarity_enum.get()->get_value()))
										{
											if (export_topological_section_type == EXPORT_TOPOLOGICAL_SECTIONS_SUBDUCTION_LEFT)
											{
												filtered_resolved_topological_section_ptrs.push_back(resolved_topological_section.get());
											}
										}
										else if (right.is_equal_to(subduction_polarity_enum.get()->get_value()))
										{
											if (export_topological_section_type == EXPORT_TOPOLOGICAL_SECTIONS_SUBDUCTION_RIGHT)
											{
												filtered_resolved_topological_section_ptrs.push_back(resolved_topological_section.get());
											}
										}
									}
								}
							}
						}
						else // not a subduction zone...
						{
							if (export_topological_section_type == EXPORT_TOPOLOGICAL_SECTIONS_RIDGE_TRANFORM)
							{
								filtered_resolved_topological_section_ptrs.push_back(resolved_topological_section.get());
							}
						}
					}
				}

				const QString topological_sections_filename = get_full_output_filename(
						target_dir,
						file_basename,
						placeholder_format_string,
						placeholder_topological_sections);

				GPlatesFileIO::ResolvedTopologicalGeometryExport::export_resolved_topological_sections(
						topological_sections_filename,
						GPlatesFileIO::ResolvedTopologicalGeometryExport::get_export_file_format(
								topological_sections_filename,
								file_format_registry),
						filtered_resolved_topological_section_ptrs,
						active_files,
						active_reconstruction_files,
						reconstruction_anchor_plate_id,
						reconstruction_time,
						export_single_output_file,
						export_per_input_file,
						export_separate_output_directory_per_input_file,
						wrap_to_dateline);
			}
		}
	}
}


void
GPlatesViewOperations::VisibleReconstructionGeometryExport::export_visible_reconstructed_feature_geometries(
		const QString &filename,
		const GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
		const GPlatesFileIO::FeatureCollectionFileFormat::Registry &file_format_registry,
		const files_collection_type &active_files,
		const files_collection_type &active_reconstruction_files,
		const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
		const double &reconstruction_time,
		bool export_single_output_file,
		bool export_per_input_file,
		bool export_separate_output_directory_per_input_file,
		bool wrap_to_dateline)
{
	// Get any ReconstructionGeometry objects that are visible in any active layers
	// of the RenderedGeometryCollection.
	RenderedGeometryUtils::reconstruction_geom_seq_type reconstruction_geom_seq;
	RenderedGeometryUtils::get_unique_reconstruction_geometries(
			reconstruction_geom_seq,
			rendered_geom_collection,
			// Don't want to export a duplicate reconstructed geometry if one is currently in focus...
			GPlatesViewOperations::RenderedGeometryCollection::RECONSTRUCTION_LAYER);

	// Get any ReconstructionGeometry objects that are of type ReconstructedFeatureGeometry.
	reconstructed_feature_geom_seq_type reconstruct_feature_geom_seq;
	GPlatesAppLogic::ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type_sequence(
			reconstruction_geom_seq.begin(),
			reconstruction_geom_seq.end(),
			reconstruct_feature_geom_seq);

	// Export the RFGs to a file format based on the filename extension.
	GPlatesFileIO::ReconstructedFeatureGeometryExport::export_reconstructed_feature_geometries(
			filename,
			GPlatesFileIO::ReconstructedFeatureGeometryExport::get_export_file_format(filename, file_format_registry),
			reconstruct_feature_geom_seq,
			active_files,
			active_reconstruction_files,
			reconstruction_anchor_plate_id,
			reconstruction_time,
			export_single_output_file,
			export_per_input_file,
			export_separate_output_directory_per_input_file,
			wrap_to_dateline);
}


void
GPlatesViewOperations::VisibleReconstructionGeometryExport::export_visible_reconstructed_flowlines(
	const QString &filename,
	const GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
	const GPlatesFileIO::FeatureCollectionFileFormat::Registry &file_format_registry,
	const files_collection_type &active_files,
	const files_collection_type &active_reconstruction_files,
	const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
	const double &reconstruction_time,
	bool export_single_output_file,
	bool export_per_input_file,
	bool export_separate_output_directory_per_input_file,
	bool wrap_to_dateline)
{
	// Get any ReconstructionGeometry objects that are visible in any active layers
	// of the RenderedGeometryCollection.
	RenderedGeometryUtils::reconstruction_geom_seq_type reconstruction_geom_seq;
	RenderedGeometryUtils::get_unique_reconstruction_geometries(
		reconstruction_geom_seq,
		rendered_geom_collection,
		// Don't want to export a duplicate reconstructed flowline if one is currently in focus...
		GPlatesViewOperations::RenderedGeometryCollection::RECONSTRUCTION_LAYER);

	// Get any ReconstructionGeometry objects that are of type ReconstructedFlowline.
	reconstructed_flowline_seq_type reconstructed_flowline_seq;
	GPlatesAppLogic::ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type_sequence(
		reconstruction_geom_seq.begin(),
		reconstruction_geom_seq.end(),
		reconstructed_flowline_seq);

	// Export the flowlines to a file format based on the filename extension.
	GPlatesFileIO::ReconstructedFlowlineExport::export_reconstructed_flowlines(
		filename,
		GPlatesFileIO::ReconstructedFlowlineExport::get_export_file_format(filename, file_format_registry),
		reconstructed_flowline_seq,
		active_files,
		active_reconstruction_files,
		reconstruction_anchor_plate_id,
		reconstruction_time,
		export_single_output_file,
		export_per_input_file,
		export_separate_output_directory_per_input_file,
		wrap_to_dateline);
}

void
GPlatesViewOperations::VisibleReconstructionGeometryExport::export_visible_reconstructed_motion_paths(
	const QString &filename,
	const GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
	const GPlatesFileIO::FeatureCollectionFileFormat::Registry &file_format_registry,
	const files_collection_type &active_files,
	const files_collection_type &active_reconstruction_files,
	const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
	const double &reconstruction_time,
	bool export_single_output_file,
	bool export_per_input_file,
	bool export_separate_output_directory_per_input_file,
	bool wrap_to_dateline)
{
	// Get any ReconstructionGeometry objects that are visible in any active layers
	// of the RenderedGeometryCollection.
	RenderedGeometryUtils::reconstruction_geom_seq_type reconstruction_geom_seq;
	RenderedGeometryUtils::get_unique_reconstruction_geometries(
		reconstruction_geom_seq,
		rendered_geom_collection,
		// Don't want to export a duplicate reconstructed motion path if one is currently in focus...
		GPlatesViewOperations::RenderedGeometryCollection::RECONSTRUCTION_LAYER);

	// Get any ReconstructionGeometry objects that are of type ReconstructedMotionPath.
	reconstructed_motion_path_seq_type reconstructed_motion_path_seq;
	GPlatesAppLogic::ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type_sequence(
		reconstruction_geom_seq.begin(),
		reconstruction_geom_seq.end(),
		reconstructed_motion_path_seq);

	// Export the flowlines to a file format based on the filename extension.
	GPlatesFileIO::ReconstructedMotionPathExport::export_reconstructed_motion_paths(
		filename,
		GPlatesFileIO::ReconstructedMotionPathExport::get_export_file_format(filename, file_format_registry),
		reconstructed_motion_path_seq,
		active_files,
		active_reconstruction_files,
		reconstruction_anchor_plate_id,
		reconstruction_time,
		export_single_output_file,
		export_per_input_file,
		export_separate_output_directory_per_input_file,
		wrap_to_dateline);
}

void
GPlatesViewOperations::VisibleReconstructionGeometryExport::export_visible_resolved_topologies(
		const QDir &target_dir,
		const QString &file_basename,
		const QString &placeholder_format_string,
		const QString &placeholder_topological_geometries,
		const QString &placeholder_topological_sections,
		const QString &placeholder_topological_sections_subduction,
		const QString &placeholder_topological_sections_subduction_left,
		const QString &placeholder_topological_sections_subduction_right,
		const QString &placeholder_topological_sections_ridge_transform,
		const GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
		const GPlatesFileIO::FeatureCollectionFileFormat::Registry &file_format_registry,
		const files_collection_type &active_files,
		const files_collection_type &active_reconstruction_files,
		const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
		const double &reconstruction_time,
		bool export_single_output_file,
		bool export_per_input_file,
		bool export_separate_output_directory_per_input_file,
		bool export_topological_lines,
		bool export_topological_polygons,
		bool export_topological_networks,
		bool export_topological_sections,
		boost::optional<GPlatesMaths::PolygonOrientation::Orientation> force_polygon_orientation,
		bool wrap_to_dateline)
{
	// Get any ReconstructionGeometry objects that are visible in any active layers
	// of the RenderedGeometryCollection.
	RenderedGeometryUtils::reconstruction_geom_seq_type reconstruction_geom_seq;
	RenderedGeometryUtils::get_unique_reconstruction_geometries(
			reconstruction_geom_seq,
			rendered_geom_collection,
			// Don't want to export a duplicate reconstructed geometry if one is currently in focus...
			GPlatesViewOperations::RenderedGeometryCollection::RECONSTRUCTION_LAYER);

	resolved_topologies_seq_type resolved_topologies_seq;

	// Get the ResolvedTopologicalLine objects (if requested)...
	if (export_topological_lines)
	{
		std::vector<const GPlatesAppLogic::ResolvedTopologicalLine *> resolved_topological_line_ptrs;
		GPlatesAppLogic::ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type_sequence(
				reconstruction_geom_seq.begin(),
				reconstruction_geom_seq.end(),
				resolved_topological_line_ptrs);
		std::copy(
				resolved_topological_line_ptrs.begin(),
				resolved_topological_line_ptrs.end(),
				std::back_inserter(resolved_topologies_seq));
	}

	// Get the ResolvedTopologicalBoundary objects (if requested)...
	std::vector<GPlatesAppLogic::ResolvedTopologicalBoundary::non_null_ptr_to_const_type> resolved_topological_boundaries;
	if (export_topological_polygons)
	{
		std::vector<const GPlatesAppLogic::ResolvedTopologicalBoundary *> resolved_topological_boundary_ptrs;
		GPlatesAppLogic::ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type_sequence(
				reconstruction_geom_seq.begin(),
				reconstruction_geom_seq.end(),
				resolved_topological_boundary_ptrs);
		std::copy(
				resolved_topological_boundary_ptrs.begin(),
				resolved_topological_boundary_ptrs.end(),
				std::back_inserter(resolved_topologies_seq));

		resolved_topological_boundaries.assign(
				resolved_topological_boundary_ptrs.begin(),
				resolved_topological_boundary_ptrs.end());
	}

	// Get the ResolvedTopologicalNetwork objects (if requested)...
	std::vector<GPlatesAppLogic::ResolvedTopologicalNetwork::non_null_ptr_to_const_type> resolved_topological_networks;
	if (export_topological_networks)
	{
		std::vector<const GPlatesAppLogic::ResolvedTopologicalNetwork *> resolved_topological_network_ptrs;
		GPlatesAppLogic::ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type_sequence(
				reconstruction_geom_seq.begin(),
				reconstruction_geom_seq.end(),
				resolved_topological_network_ptrs);
		std::copy(
				resolved_topological_network_ptrs.begin(),
				resolved_topological_network_ptrs.end(),
				std::back_inserter(resolved_topologies_seq));

		resolved_topological_networks.assign(
				resolved_topological_network_ptrs.begin(),
				resolved_topological_network_ptrs.end());
	}

	const QString topological_geometries_filename = get_full_output_filename(
			target_dir,
			file_basename,
			placeholder_format_string,
			placeholder_topological_geometries);

	// Export the RTGs to a file format based on the filename extension.
	GPlatesFileIO::ResolvedTopologicalGeometryExport::export_resolved_topological_geometries(
			topological_geometries_filename,
			GPlatesFileIO::ResolvedTopologicalGeometryExport::get_export_file_format(
					topological_geometries_filename,
					file_format_registry),
			resolved_topologies_seq,
			active_files,
			active_reconstruction_files,
			reconstruction_anchor_plate_id,
			reconstruction_time,
			export_single_output_file,
			export_per_input_file,
			export_separate_output_directory_per_input_file,
			force_polygon_orientation,
			wrap_to_dateline);

	if (export_topological_sections)
	{
		// Find the resolved topological sections (and their associated shared sub-segments) from the resolved boundaries/networks.
		std::vector<GPlatesAppLogic::ResolvedTopologicalSection::non_null_ptr_type> resolved_topological_sections;
		GPlatesAppLogic::TopologyUtils::find_resolved_topological_sections(
				resolved_topological_sections,
				resolved_topological_boundaries,
				resolved_topological_networks);

		// All sections.
		export_resolved_topological_sections(
				resolved_topological_sections,
				target_dir,
				file_basename,
				placeholder_format_string,
				placeholder_topological_sections,
				EXPORT_TOPOLOGICAL_SECTIONS_ALL,
				file_format_registry,
				active_files,
				active_reconstruction_files,
				reconstruction_anchor_plate_id,
				reconstruction_time,
				export_single_output_file,
				export_per_input_file,
				export_separate_output_directory_per_input_file,
				wrap_to_dateline);

		// Subduction sections.
		export_resolved_topological_sections(
				resolved_topological_sections,
				target_dir,
				file_basename,
				placeholder_format_string,
				placeholder_topological_sections_subduction,
				EXPORT_TOPOLOGICAL_SECTIONS_SUBDUCTION,
				file_format_registry,
				active_files,
				active_reconstruction_files,
				reconstruction_anchor_plate_id,
				reconstruction_time,
				export_single_output_file,
				export_per_input_file,
				export_separate_output_directory_per_input_file,
				wrap_to_dateline);

		// Left subduction sections.
		export_resolved_topological_sections(
				resolved_topological_sections,
				target_dir,
				file_basename,
				placeholder_format_string,
				placeholder_topological_sections_subduction_left,
				EXPORT_TOPOLOGICAL_SECTIONS_SUBDUCTION_LEFT,
				file_format_registry,
				active_files,
				active_reconstruction_files,
				reconstruction_anchor_plate_id,
				reconstruction_time,
				export_single_output_file,
				export_per_input_file,
				export_separate_output_directory_per_input_file,
				wrap_to_dateline);

		// Right subduction sections.
		export_resolved_topological_sections(
				resolved_topological_sections,
				target_dir,
				file_basename,
				placeholder_format_string,
				placeholder_topological_sections_subduction_right,
				EXPORT_TOPOLOGICAL_SECTIONS_SUBDUCTION_RIGHT,
				file_format_registry,
				active_files,
				active_reconstruction_files,
				reconstruction_anchor_plate_id,
				reconstruction_time,
				export_single_output_file,
				export_per_input_file,
				export_separate_output_directory_per_input_file,
				wrap_to_dateline);

		// Ridge/transform sections.
		export_resolved_topological_sections(
				resolved_topological_sections,
				target_dir,
				file_basename,
				placeholder_format_string,
				placeholder_topological_sections_ridge_transform,
				EXPORT_TOPOLOGICAL_SECTIONS_RIDGE_TRANFORM,
				file_format_registry,
				active_files,
				active_reconstruction_files,
				reconstruction_anchor_plate_id,
				reconstruction_time,
				export_single_output_file,
				export_per_input_file,
				export_separate_output_directory_per_input_file,
				wrap_to_dateline);
	}
}
