/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

#include <map>
#include <boost/foreach.hpp>

#include "ResolvedTopologicalGeometryExport.h"

#include "app-logic/ReconstructedFeatureGeometry.h"
#include "app-logic/ResolvedTopologicalBoundary.h"
#include "app-logic/ResolvedTopologicalLine.h"
#include "app-logic/ResolvedTopologicalNetwork.h"
#include "app-logic/ResolvedTopologicalSection.h"

#include "GMTFormatResolvedTopologicalGeometryExport.h"
#include "FeatureCollectionFileFormat.h"
#include "FeatureCollectionFileFormatRegistry.h"
#include "FileFormatNotSupportedException.h"
#include "OgrFormatResolvedTopologicalGeometryExport.h"
#include "ReconstructionGeometryExportImpl.h"

using namespace GPlatesFileIO::ReconstructionGeometryExportImpl;


namespace GPlatesFileIO
{
	namespace ResolvedTopologicalGeometryExport
	{
		namespace
		{
			//! Typedef for a sequence of @a FeatureGeometryGroup objects.
			typedef std::list< FeatureGeometryGroup<GPlatesAppLogic::ReconstructionGeometry> >
					feature_geometry_group_seq_type;

			//! Typedef for a sequence of @a FeatureCollectionFeatureGroup objects.
			typedef std::list< FeatureCollectionFeatureGroup<GPlatesAppLogic::ReconstructionGeometry> >
					grouped_features_seq_type;


			void
			export_resolved_topological_geometries_impl(
					bool export_per_collection,
					const QString &filename,
					Format export_format,
					const feature_geometry_group_seq_type &grouped_recon_geoms_seq,
					const std::vector<const File::Reference *> &referenced_files,
					const std::vector<const File::Reference *> &active_reconstruction_files,
					const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
					const double &reconstruction_time,
					boost::optional<GPlatesMaths::PolygonOrientation::Orientation> force_polygon_orientation,
					bool wrap_to_dateline)
			{
				switch (export_format)
				{
				case SHAPEFILE:
					OgrFormatResolvedTopologicalGeometryExport::export_resolved_topological_geometries(
						export_per_collection,
						grouped_recon_geoms_seq,
						filename,
						referenced_files,
						active_reconstruction_files,
						reconstruction_anchor_plate_id,
						reconstruction_time,
						force_polygon_orientation,
						wrap_to_dateline);
					break;

				case OGRGMT:
					OgrFormatResolvedTopologicalGeometryExport::export_resolved_topological_geometries(
						export_per_collection,
						grouped_recon_geoms_seq,
						filename,
						referenced_files,
						active_reconstruction_files,
						reconstruction_anchor_plate_id,
						reconstruction_time,
						force_polygon_orientation,
						wrap_to_dateline);
					break;

				case GMT:
					GMTFormatResolvedTopologicalGeometryExport::export_resolved_topological_geometries(
						grouped_recon_geoms_seq,
						filename,
						referenced_files,
						active_reconstruction_files,
						reconstruction_anchor_plate_id,
						reconstruction_time,
						force_polygon_orientation);
					break;

				default:
					throw FileFormatNotSupportedException(GPLATES_EXCEPTION_SOURCE,
						"Chosen export format is not currently supported.");
				}
			}


			void
			export_resolved_topological_sections_impl(
					bool export_per_collection,
					const QString &filename,
					Format export_format,
					const std::vector<const GPlatesAppLogic::ResolvedTopologicalSection *> &resolved_topological_sections,
					const std::vector<const File::Reference *> &referenced_files,
					const std::vector<const File::Reference *> &active_reconstruction_files,
					const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
					const double &reconstruction_time,
					bool wrap_to_dateline)
			{
				switch (export_format)
				{
				case SHAPEFILE:
					OgrFormatResolvedTopologicalGeometryExport::export_resolved_topological_sections(
						export_per_collection,
						resolved_topological_sections,
						filename,
						referenced_files,
						active_reconstruction_files,
						reconstruction_anchor_plate_id,
						reconstruction_time,
						wrap_to_dateline);
					break;

				case OGRGMT:
					OgrFormatResolvedTopologicalGeometryExport::export_resolved_topological_sections(
						export_per_collection,
						resolved_topological_sections,
						filename,
						referenced_files,
						active_reconstruction_files,
						reconstruction_anchor_plate_id,
						reconstruction_time,
						wrap_to_dateline);
					break;

				case GMT:
					GMTFormatResolvedTopologicalGeometryExport::export_resolved_topological_sections(
						resolved_topological_sections,
						filename,
						referenced_files,
						active_reconstruction_files,
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


GPlatesFileIO::ResolvedTopologicalGeometryExport::Format
GPlatesFileIO::ResolvedTopologicalGeometryExport::get_export_file_format(
		const QFileInfo& file_info,
		const FeatureCollectionFileFormat::Registry &file_format_registry)
{
	// Since we're using a feature collection file format to export
	// our RTGs we'll use the feature collection file format code.
	const boost::optional<FeatureCollectionFileFormat::Format> feature_collection_file_format =
			file_format_registry.get_file_format(file_info);
	if (!feature_collection_file_format ||
		!file_format_registry.does_file_format_support_writing(feature_collection_file_format.get()))
	{
		return UNKNOWN;
	}

	// Only some feature collection file formats are used for exporting resolved topological geometries.
	switch (feature_collection_file_format.get())
	{
	case FeatureCollectionFileFormat::WRITE_ONLY_XY_GMT:
		return GMT;
	case FeatureCollectionFileFormat::SHAPEFILE:
		return SHAPEFILE;
	case FeatureCollectionFileFormat::OGRGMT:
		return OGRGMT;
	default:
		break;
	}

	return UNKNOWN;
}


void
GPlatesFileIO::ResolvedTopologicalGeometryExport::export_resolved_topological_geometries(
		const QString &filename,
		Format export_format,
		const std::vector<const GPlatesAppLogic::ReconstructionGeometry *> &resolved_topologies,
		const std::vector<const File::Reference *> &active_files,
		const std::vector<const File::Reference *> &active_reconstruction_files,
		const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
		const double &reconstruction_time,
		bool export_single_output_file,
		bool export_per_input_file,
		bool export_separate_output_directory_per_input_file,
		boost::optional<GPlatesMaths::PolygonOrientation::Orientation> force_polygon_orientation,
		bool wrap_to_dateline)
{
	// Get the list of active reconstructable feature collection files that contain
	// the features referenced by the ReconstructionGeometry objects.
	feature_handle_to_collection_map_type feature_to_collection_map;
	std::vector<const File::Reference *> referenced_files;
	get_files_referenced_by_geometries(
			referenced_files, resolved_topologies, active_files,
			feature_to_collection_map);

	// Group the ReconstructionGeometry objects by their feature.
	feature_geometry_group_seq_type grouped_recon_geom_seq;
	group_reconstruction_geometries_with_their_feature(
			grouped_recon_geom_seq, resolved_topologies, feature_to_collection_map);

	// Group the feature-groups with their collections. 
	grouped_features_seq_type grouped_features_seq;
	group_feature_geom_groups_with_their_collection(
			feature_to_collection_map,
			grouped_features_seq,
			grouped_recon_geom_seq);

	// For shapefiles exporting-per-collection retains the shapefile attributes from the original features.
	//
	// For shapefiles exporting-as-a-single-file ignores the shapefile attributes from the original features.
	// This is necessary since the features came from multiple input files which might
	// have different attribute field names making it difficult to merge into a single output.
	//
	// FIXME: An alternative is for Shapefile/OGR exporter to explicitly check field names for overlap.

	if (export_single_output_file)
	{
		// If all features came from a single file then export per collection...
		const bool export_per_collection = (grouped_features_seq.size() == 1);
		export_resolved_topological_geometries_impl(
				export_per_collection,
				filename,
				export_format,
				grouped_recon_geom_seq,
				referenced_files,
				active_reconstruction_files,
				reconstruction_anchor_plate_id,
				reconstruction_time,
				force_polygon_orientation,
				wrap_to_dateline);
	}

	if (export_per_input_file)
	{
		std::vector<QString> output_filenames;
		get_output_filenames(
				output_filenames,
				filename,
				grouped_features_seq,
				export_separate_output_directory_per_input_file);

		grouped_features_seq_type::const_iterator grouped_features_iter = grouped_features_seq.begin();
		grouped_features_seq_type::const_iterator grouped_features_end = grouped_features_seq.end();
		for (std::vector<QString>::const_iterator output_filename_iter = output_filenames.begin();
			grouped_features_iter != grouped_features_end;
			++grouped_features_iter, ++output_filename_iter)
		{
			export_resolved_topological_geometries_impl(
					true/*export_per_collection*/,
					*output_filename_iter,
					export_format,
					grouped_features_iter->feature_geometry_groups,
					referenced_files,
					active_reconstruction_files,
					reconstruction_anchor_plate_id,
					reconstruction_time,
					force_polygon_orientation,
					wrap_to_dateline);
		}
	}
}


void
GPlatesFileIO::ResolvedTopologicalGeometryExport::export_resolved_topological_sections(
		const QString &filename,
		Format export_format,
		const std::vector<const GPlatesAppLogic::ResolvedTopologicalSection *> &resolved_topological_sections,
		const std::vector<const File::Reference *> &active_files,
		const std::vector<const File::Reference *> &active_reconstruction_files,
		const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
		const double &reconstruction_time,
		bool export_single_output_file,
		bool export_per_input_file,
		bool export_separate_output_directory_per_input_file,
		bool wrap_to_dateline)
{
	// Get a list of the resolved topological section ReconstructionGeometry's.
	// We'll use these to determine which features/collections each section came from.
	std::vector<const GPlatesAppLogic::ReconstructionGeometry *> resolved_topological_section_recon_geoms;
	BOOST_FOREACH(
			const GPlatesAppLogic::ResolvedTopologicalSection *resolved_topological_section,
			resolved_topological_sections)
	{
		resolved_topological_section_recon_geoms.push_back(
				resolved_topological_section->get_reconstruction_geometry().get());
	}

	// Get the list of active reconstructable feature collection files that contain
	// the features referenced by the ReconstructionGeometry objects.
	feature_handle_to_collection_map_type feature_to_collection_map;
	std::vector<const File::Reference *> referenced_files;
	get_files_referenced_by_geometries(
			referenced_files, resolved_topological_section_recon_geoms, active_files,
			feature_to_collection_map);

	// Group the ReconstructionGeometry objects by their feature.
	feature_geometry_group_seq_type grouped_recon_geom_seq;
	group_reconstruction_geometries_with_their_feature(
			grouped_recon_geom_seq, resolved_topological_section_recon_geoms, feature_to_collection_map);

	// Group the feature-groups with their collections. 
	grouped_features_seq_type grouped_features_seq;
	group_feature_geom_groups_with_their_collection(
			feature_to_collection_map,
			grouped_features_seq,
			grouped_recon_geom_seq);

	// For shapefiles exporting-per-collection retains the shapefile attributes from the original features.
	//
	// For shapefiles exporting-as-a-single-file ignores the shapefile attributes from the original features.
	// This is necessary since the features came from multiple input files which might
	// have different attribute field names making it difficult to merge into a single output.
	//
	// FIXME: An alternative is for Shapefile/OGR exporter to explicitly check field names for overlap.

	if (export_single_output_file)
	{
		// If all features came from a single file then export per collection...
		const bool export_per_collection = (grouped_features_seq.size() == 1);
		export_resolved_topological_sections_impl(
				export_per_collection,
				filename,
				export_format,
				resolved_topological_sections,
				referenced_files,
				active_reconstruction_files,
				reconstruction_anchor_plate_id,
				reconstruction_time,
				wrap_to_dateline);
	}

	if (export_per_input_file)
	{
		std::vector<QString> output_filenames;
		get_output_filenames(
				output_filenames,
				filename,
				grouped_features_seq,
				export_separate_output_directory_per_input_file);

		// We need to determine which resolved topological sections belong to which feature group
		// so we know which sections to write out which output file.
		typedef std::map<
				const GPlatesAppLogic::ReconstructionGeometry *,
				const GPlatesAppLogic::ResolvedTopologicalSection *>
						recon_geom_to_resolved_section_map_type;
		recon_geom_to_resolved_section_map_type recon_geom_to_resolved_section_map;

		// Initialise the mapping of reconstruction geometries to resolved topological sections.
		BOOST_FOREACH(
				const GPlatesAppLogic::ResolvedTopologicalSection *resolved_topological_section,
				resolved_topological_sections)
		{
			recon_geom_to_resolved_section_map.insert(
					recon_geom_to_resolved_section_map_type::value_type(
							resolved_topological_section->get_reconstruction_geometry().get(),
							resolved_topological_section));
		}

		// Iterate over the files to export.
		grouped_features_seq_type::const_iterator grouped_features_iter = grouped_features_seq.begin();
		grouped_features_seq_type::const_iterator grouped_features_end = grouped_features_seq.end();
		for (std::vector<QString>::const_iterator output_filename_iter = output_filenames.begin();
			grouped_features_iter != grouped_features_end;
			++grouped_features_iter, ++output_filename_iter)
		{
			// The group of resolved topological sections to write to the current output file.
			std::vector<const GPlatesAppLogic::ResolvedTopologicalSection *> resolved_topological_section_group;

			// Find the resolved topological sections associated with the features associated with the current file.
			BOOST_FOREACH(
					const FeatureGeometryGroup<GPlatesAppLogic::ReconstructionGeometry> &feature_geometry_group,
					grouped_features_iter->feature_geometry_groups)
			{
				std::vector<const GPlatesAppLogic::ReconstructionGeometry *>::const_iterator recon_geoms_iter =
						feature_geometry_group.recon_geoms.begin();
				std::vector<const GPlatesAppLogic::ReconstructionGeometry *>::const_iterator recon_geoms_end =
						feature_geometry_group.recon_geoms.end();
				for ( ; recon_geoms_iter != recon_geoms_end; ++recon_geoms_iter)
				{
					const GPlatesAppLogic::ReconstructionGeometry *recon_geom = *recon_geoms_iter;

					recon_geom_to_resolved_section_map_type::const_iterator section_iter =
							recon_geom_to_resolved_section_map.find(recon_geom);
					if (section_iter != recon_geom_to_resolved_section_map.end())
					{
						const GPlatesAppLogic::ResolvedTopologicalSection *section = section_iter->second;

						resolved_topological_section_group.push_back(section);
					}
				}
			}

			export_resolved_topological_sections_impl(
					true/*export_per_collection*/,
					*output_filename_iter,
					export_format,
					resolved_topological_section_group,
					referenced_files,
					active_reconstruction_files,
					reconstruction_anchor_plate_id,
					reconstruction_time,
					wrap_to_dateline);
		}
	}
}
