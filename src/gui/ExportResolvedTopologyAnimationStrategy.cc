/* $Id$ */
 
/**
 * \file 
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

#include <boost/foreach.hpp>
#include <boost/optional.hpp>
#include <QFileInfo>
#include <QString>

#include "ExportResolvedTopologyAnimationStrategy.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/AppLogicUtils.h"
#include "app-logic/ReconstructionGeometryUtils.h"
#include "app-logic/Reconstruction.h"
#include "app-logic/ResolvedTopologicalBoundary.h"

#include "file-io/ResolvedTopologicalBoundaryExport.h"

#include "gui/ExportAnimationContext.h"
#include "gui/AnimationController.h"

#include "presentation/ViewState.h"

#include "utils/ExportTemplateFilenameSequence.h"

#include "view-operations/RenderedGeometryUtils.h"


namespace
{
#if 0
	void
	export_individual_platepolygon_file(
			const GPlatesAppLogic::ResolvedTopologicalBoundary &resolved_geom,
			const GMTOldFeatureIdStyleHeader &platepolygon_header,
			const GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type &platepolygon_geom,
			const QDir &target_dir,
			const QString &filebasename,
			const QString &placeholder_string)
	{
		// We're expecting a plate id as that forms part of the filename.
		if (!resolved_geom.plate_id())
		{
			return;
		}
		const QString plate_id_string = QString::number(*resolved_geom.plate_id());

		GMTFeatureExporter individual_platepolygon_exporter(
			get_full_output_filename(target_dir, filebasename, placeholder_string, plate_id_string));

		individual_platepolygon_exporter.print_gmt_header_and_geometry(
				platepolygon_header, platepolygon_geom);
	}

	void
	export_platepolygon(
			const GPlatesAppLogic::ResolvedTopologicalBoundary &resolved_geom,
			const GPlatesModel::FeatureHandle::const_weak_ref &platepolygon_feature_ref,
			GMTFeatureExporter &all_platepolygons_exporter,
			const QDir &target_dir,
			const QString &filebasename,
			const QString &placeholder_string)
	{
		// The geometry stored in the resolved topological geometry should be a PolygonOnSphere.
		const GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type platepolygon_geom =
				resolved_geom.resolved_topology_geometry();
		if (!platepolygon_geom)
		{
			return;
		}

		GMTOldFeatureIdStyleHeader platepolygon_header(platepolygon_feature_ref);

		// Export each platepolygon to a file containing all platepolygons.
		all_platepolygons_exporter.print_gmt_header_and_geometry(
				platepolygon_header, platepolygon_geom);

#if 1
// FIXME: export needs finer grain' controls in the gui, for now 
// commented out on 2010-11-12 by user request to clean up export clutter

		// Also export each platepolygon to a separate file.
		export_individual_platepolygon_file(
				resolved_geom,
				platepolygon_header,
				platepolygon_geom,
				target_dir,
				filebasename,
				placeholder_string);
#endif 
	}

// FIXME: export needs fine grain controls in the gui, for now 
// commented out on 2010-11-12 by user request to clean up export clutter
	void
	export_individual_slab_polygon_file(
			const GPlatesAppLogic::ResolvedTopologicalBoundary &resolved_geom,
			const SlabPolygonStyleHeader &slab_polygon_header,
			const GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type &slab_polygon_geom,
			const QDir &target_dir,
			const QString &filebasename,
			const QString &placeholder_string)
	{
		// We're expecting a plate id as that forms part of the filename.
		if (!resolved_geom.plate_id())
		{
			return;
		}

		QString string = "slab_";
		const QString plate_id_string = QString::number(*resolved_geom.plate_id());
		string.append(plate_id_string);

		GMTFeatureExporter individual_slab_polygon_exporter(
			get_full_output_filename(target_dir, filebasename, placeholder_string, string));

		individual_slab_polygon_exporter.print_gmt_header_and_geometry(
				slab_polygon_header, slab_polygon_geom);
	}

	void
	export_slab_polygon(
			const GPlatesAppLogic::ResolvedTopologicalBoundary &resolved_geom,
			const GPlatesModel::FeatureHandle::const_weak_ref &slab_polygon_feature_ref,
			GMTFeatureExporter &all_slab_polygons_exporter,
			const QDir &target_dir,
			const QString &filebasename,
			const QString &placeholder_string)
	{
		// The geometry stored in the resolved topological geometry should be a PolygonOnSphere.
		const GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type slab_polygon_geom =
				resolved_geom.resolved_topology_geometry();
		if (!slab_polygon_geom)
		{
			return;
		}

		// create the header line for this feature 
		SlabPolygonStyleHeader slab_polygon_header(slab_polygon_feature_ref);

		// Export each polygon to a file containing all polygons.
		all_slab_polygons_exporter.print_gmt_header_and_geometry(
				slab_polygon_header, slab_polygon_geom);

#if 1
// FIXME: export needs fine grain controls in the gui, for now 
// commented out on 2010-11-12 by user request to clean up export clutter

		// Also export each Slab Polygon to a separate file.
		export_individual_slab_polygon_file(
				resolved_geom,
				slab_polygon_header,
				slab_polygon_geom,
				target_dir,
				filebasename,
				placeholder_string);
#endif
	}


	void
	export_sub_segment(
			const GPlatesAppLogic::ResolvedTopologicalBoundary::SubSegment &sub_segment,
			const GPlatesModel::FeatureHandle::const_weak_ref &platepolygon_feature_ref,
			const double &recon_time,
			GMTFeatureExporter &ridge_transform_exporter,
			GMTFeatureExporter &subduction_exporter,
			GMTFeatureExporter &subduction_left_exporter,
			GMTFeatureExporter &subduction_right_exporter)
	{
		// Determine the feature type of subsegment.
		const SubSegmentType sub_segment_type = get_sub_segment_type(sub_segment, recon_time);

		// The files with specific types of subsegments use a different header format
		// than the file with all subsegments (regardless of type).
		PlatePolygonSubSegmentHeader sub_segment_header(
				sub_segment.get_feature_ref(), 
				platepolygon_feature_ref,
				sub_segment, 
				sub_segment_type);

		// Export subsegment depending on its feature type.
		switch (sub_segment_type)
		{
		case SUB_SEGMENT_TYPE_SUBDUCTION_ZONE_LEFT:
			subduction_left_exporter.print_gmt_header_and_geometry(
					sub_segment_header, sub_segment.get_geometry());
			subduction_exporter.print_gmt_header_and_geometry(
					sub_segment_header, sub_segment.get_geometry());
			break;

		case SUB_SEGMENT_TYPE_SUBDUCTION_ZONE_RIGHT:
			subduction_right_exporter.print_gmt_header_and_geometry(
					sub_segment_header, sub_segment.get_geometry());
			subduction_exporter.print_gmt_header_and_geometry(
					sub_segment_header, sub_segment.get_geometry());
			break;

		case SUB_SEGMENT_TYPE_SUBDUCTION_ZONE_UNKNOWN:
			// We know it's a subduction zone but don't know if left or right
			// so export to the subduction zone file only.
			subduction_exporter.print_gmt_header_and_geometry(
					sub_segment_header, sub_segment.get_geometry());
			break;

		case SUB_SEGMENT_TYPE_OTHER:
		default:
			ridge_transform_exporter.print_gmt_header_and_geometry(
					sub_segment_header, sub_segment.get_geometry());
			break;
		}
	}

	void
	export_slab_sub_segment(
			const GPlatesAppLogic::ResolvedTopologicalBoundary::SubSegment &sub_segment,
			const GPlatesModel::FeatureHandle::const_weak_ref &slab_polygon_feature_ref,
			const double &recon_time,
			GMTFeatureExporter &slab_edge_leading_exporter,
			GMTFeatureExporter &slab_edge_leading_left_exporter,
			GMTFeatureExporter &slab_edge_leading_right_exporter,
			GMTFeatureExporter &slab_edge_trench_exporter,
			GMTFeatureExporter &slab_edge_side_exporter)
	{
		// Determine the feature type of subsegment.
		const SubSegmentType sub_segment_type = get_slab_sub_segment_type(sub_segment, recon_time);

		SlabPolygonSubSegmentHeader sub_segment_header(
				sub_segment.get_feature_ref(), 
				slab_polygon_feature_ref,
				sub_segment, 
				sub_segment_type);

		// Export subsegment depending on its feature type.
		switch (sub_segment_type)
		{
		case SUB_SEGMENT_TYPE_SLAB_EDGE_LEADING_LEFT:
			slab_edge_leading_left_exporter.print_gmt_header_and_geometry(
					sub_segment_header, sub_segment.get_geometry());
			slab_edge_leading_exporter.print_gmt_header_and_geometry(
					sub_segment_header, sub_segment.get_geometry());
			break;

		case SUB_SEGMENT_TYPE_SLAB_EDGE_LEADING_RIGHT:
			slab_edge_leading_right_exporter.print_gmt_header_and_geometry(
					sub_segment_header, sub_segment.get_geometry());
			slab_edge_leading_exporter.print_gmt_header_and_geometry(
					sub_segment_header, sub_segment.get_geometry());
			break;

		case SUB_SEGMENT_TYPE_SLAB_EDGE_TRENCH:
			slab_edge_trench_exporter.print_gmt_header_and_geometry(
					sub_segment_header, sub_segment.get_geometry());
			break;

		case SUB_SEGMENT_TYPE_SLAB_EDGE_SIDE:
		default:
			slab_edge_side_exporter.print_gmt_header_and_geometry(
					sub_segment_header, sub_segment.get_geometry());
			break;
		}
	}




	void
	export_sub_segments(
			const GPlatesAppLogic::ResolvedTopologicalBoundary &resolved_geom,
			const GPlatesModel::FeatureHandle::const_weak_ref &platepolygon_feature_ref,
			const double &recon_time,
			GMTFeatureExporter &all_sub_segments_exporter,
			GMTFeatureExporter &ridge_transform_exporter,
			GMTFeatureExporter &subduction_exporter,
			GMTFeatureExporter &subduction_left_exporter,
			GMTFeatureExporter &subduction_right_exporter)
	{
		// Iterate over the subsegments contained in the current resolved topological geometry
		// and write each to GMT format file.
		GPlatesAppLogic::ResolvedTopologicalBoundary::sub_segment_const_iterator sub_segment_iter =
				resolved_geom.sub_segment_begin();
		GPlatesAppLogic::ResolvedTopologicalBoundary::sub_segment_const_iterator sub_segment_end =
				resolved_geom.sub_segment_end();
		for ( ; sub_segment_iter != sub_segment_end; ++sub_segment_iter)
		{
			const GPlatesAppLogic::ResolvedTopologicalBoundary::SubSegment &sub_segment =
					*sub_segment_iter;

			// The file with all subsegments (regardless of type) uses a different
			// header format than the files with specific types of subsegments.
			GMTOldFeatureIdStyleHeader all_sub_segment_header(sub_segment.get_feature_ref());

			// All subsegments get exported to this file.
			all_sub_segments_exporter.print_gmt_header_and_geometry(
					all_sub_segment_header, sub_segment.get_geometry());

			// Also export the sub segment to another file based on its feature type.
			export_sub_segment(
					sub_segment, platepolygon_feature_ref, recon_time,
					ridge_transform_exporter, subduction_exporter,
					subduction_left_exporter, subduction_right_exporter);
		}
	}


	void
	export_slab_sub_segments(
			const GPlatesAppLogic::ResolvedTopologicalBoundary &resolved_geom,
			const GPlatesModel::FeatureHandle::const_weak_ref &platepolygon_feature_ref,
			const double &recon_time,
			GMTFeatureExporter &all_sub_segments_exporter,
			GMTFeatureExporter &slab_edge_leading_exporter,
			GMTFeatureExporter &slab_edge_leading_left_exporter,
			GMTFeatureExporter &slab_edge_leading_right_exporter,
			GMTFeatureExporter &slab_edge_trench_exporter,
			GMTFeatureExporter &slab_edge_side_exporter)
	{
		// Iterate over the subsegments contained in the current resolved topological geometry
		// and write each to GMT format file.
		GPlatesAppLogic::ResolvedTopologicalBoundary::sub_segment_const_iterator sub_segment_iter =
				resolved_geom.sub_segment_begin();
		GPlatesAppLogic::ResolvedTopologicalBoundary::sub_segment_const_iterator sub_segment_end =
				resolved_geom.sub_segment_end();
		for ( ; sub_segment_iter != sub_segment_end; ++sub_segment_iter)
		{
			const GPlatesAppLogic::ResolvedTopologicalBoundary::SubSegment &sub_segment =
					*sub_segment_iter;

			// The file with all subsegments (regardless of type) uses a different
			// header format than the files with specific types of subsegments.
			GMTOldFeatureIdStyleHeader all_sub_segment_header(sub_segment.get_feature_ref());

#if 0
//NOTE: we do not need this all lines export for slabs

			// All subsegments get exported to this file.
			all_sub_segments_exporter.print_gmt_header_and_geometry(
					all_sub_segment_header, sub_segment.get_geometry());
#endif

			// Also export the sub segment to another file based on its feature type.
			export_slab_sub_segment(
					sub_segment, platepolygon_feature_ref, recon_time,
					slab_edge_leading_exporter, 
					slab_edge_leading_left_exporter, 
					slab_edge_leading_right_exporter, 
					slab_edge_trench_exporter,
					slab_edge_side_exporter);
		}
	}
#endif
}

const QString 
GPlatesGui::ExportResolvedTopologyAnimationStrategy::
	DEFAULT_RESOLVED_TOPOLOGIES_GMT_FILENAME_TEMPLATE
		="Polygons.%P.%d.xy";

const QString 
GPlatesGui::ExportResolvedTopologyAnimationStrategy::
	DEFAULT_RESOLVED_TOPOLOGIES_SHP_FILENAME_TEMPLATE
		="Polygons.%P.%d.shp";

const QString 
GPlatesGui::ExportResolvedTopologyAnimationStrategy::
	RESOLVED_TOPOLOGIES_FILENAME_TEMPLATE_DESC
		=FORMAT_CODE_DESC;

const QString GPlatesGui::ExportResolvedTopologyAnimationStrategy::RESOLVED_TOPOLOGIES_DESC  =
		"Export resolved topologies:\n"
		"- exports resolved topological closed plate polygons,\n"
		"- optionally exports the subsegment geometries of polygon boundaries.\n";

//
// Plate Polygon related data 
//
const QString
GPlatesGui::ExportResolvedTopologyAnimationStrategy::s_placeholder_platepolygons("platepolygons");

const QString
GPlatesGui::ExportResolvedTopologyAnimationStrategy::s_placeholder_lines("lines");

const QString
GPlatesGui::ExportResolvedTopologyAnimationStrategy::s_placeholder_ridge_transforms("ridge_transform_boundaries");

const QString
GPlatesGui::ExportResolvedTopologyAnimationStrategy::s_placeholder_subductions("subduction_boundaries");

const QString
GPlatesGui::ExportResolvedTopologyAnimationStrategy::s_placeholder_left_subductions("subduction_boundaries_sL");

const QString
GPlatesGui::ExportResolvedTopologyAnimationStrategy::s_placeholder_right_subductions("subduction_boundaries_sR");

//
// Slab Polygon related data
//
const QString
GPlatesGui::ExportResolvedTopologyAnimationStrategy::s_placeholder_slab_polygons("slab_polygons");

const QString
GPlatesGui::ExportResolvedTopologyAnimationStrategy::s_placeholder_slab_edge_leading("slab_edges_leading");

const QString
GPlatesGui::ExportResolvedTopologyAnimationStrategy::s_placeholder_slab_edge_leading_left("slab_edges_leading_sL");

const QString
GPlatesGui::ExportResolvedTopologyAnimationStrategy::s_placeholder_slab_edge_leading_right("slab_edges_leading_sR");

const QString
GPlatesGui::ExportResolvedTopologyAnimationStrategy::s_placeholder_slab_edge_trench("slab_edges_trench");

const QString
GPlatesGui::ExportResolvedTopologyAnimationStrategy::s_placeholder_slab_edge_side("slab_edges_side");




const GPlatesGui::ExportResolvedTopologyAnimationStrategy::non_null_ptr_type
GPlatesGui::ExportResolvedTopologyAnimationStrategy::create(
		GPlatesGui::ExportAnimationContext &export_animation_context,
		FileFormat format,
		const ExportAnimationStrategy::Configuration& cfg)
{
	ExportResolvedTopologyAnimationStrategy * ptr = 
			new ExportResolvedTopologyAnimationStrategy(
					export_animation_context,
					cfg.filename_template());

	ptr->d_file_format = format;
		
	return non_null_ptr_type(
			ptr,
			GPlatesUtils::NullIntrusivePointerHandler());
}


GPlatesGui::ExportResolvedTopologyAnimationStrategy::ExportResolvedTopologyAnimationStrategy(
		GPlatesGui::ExportAnimationContext &export_animation_context,
		const QString &filename_template):
	ExportAnimationStrategy(export_animation_context)
{
	set_template_filename(filename_template);
	
	GPlatesAppLogic::FeatureCollectionFileState &file_state =
			d_export_animation_context_ptr->view_state().get_application_state()
					.get_feature_collection_file_state();

	// From the file state, obtain the list of all currently loaded files.
	const std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference> loaded_files =
			file_state.get_loaded_files();

	// Add them to our list of loaded files.
	BOOST_FOREACH(GPlatesAppLogic::FeatureCollectionFileState::file_reference file_ref, loaded_files)
	{
		d_loaded_files.push_back(&file_ref.get_file());
	}
}


void
GPlatesGui::ExportResolvedTopologyAnimationStrategy::set_template_filename(
		const QString &filename)
{
	// We want "Polygons" to look like "Polygons.%P.%d" as that
	// is what is expected by the workflow (external to GPlates) that uses
	// this export.
	// The '%P' placeholder string will get replaced for each type of export
	// in 'do_export_iteration()'.
	// The "%d" tells ExportTemplateFilenameSequence to insert the reconstruction time.
#if 0
	//The placeholders, %P and %d", have been taken care by "export" dialog.
	//So there is no need to add them here again.
	const QString suffix =
			"." +
			GPlatesUtils::ExportTemplateFilename::PLACEHOLDER_FORMAT_STRING +
			".%d";

	const QString modified_template_filename =
			append_suffix_to_template_filebasename(filename, suffix);

#endif
	// Call base class method to actually set the template filename.
	//ExportAnimationStrategy::set_template_filename(modified_template_filename);
	ExportAnimationStrategy::set_template_filename(filename);
}


bool
GPlatesGui::ExportResolvedTopologyAnimationStrategy::do_export_iteration(
		std::size_t frame_index)
{
	if(!check_filename_sequence())
	{
		return false;
	}
	GPlatesUtils::ExportTemplateFilenameSequence::const_iterator &filename_it = 
		*d_filename_iterator_opt;

	// Assemble parts of this iteration's filename from the template filename sequence.
	QString output_filebasename = *filename_it++;

	//
	// Here's where we would do the actual exporting of the resolved topologies.
	// The View is already set to the appropriate reconstruction time for
	// this frame; all we have to do is the maths and the file-writing (to @a full_filename)
	//

	GPlatesAppLogic::ApplicationState &application_state =
			d_export_animation_context_ptr->view_state().get_application_state();

	const double &reconstruction_time = application_state.get_current_reconstruction_time();

	// Get any ReconstructionGeometry objects that are visible in any active layers
	// of the RenderedGeometryCollection.
	GPlatesViewOperations::RenderedGeometryUtils::reconstruction_geom_seq_type reconstruction_geom_seq;
	GPlatesViewOperations::RenderedGeometryUtils::get_unique_reconstruction_geometries(
			reconstruction_geom_seq,
			d_export_animation_context_ptr->view_state().get_rendered_geometry_collection());

	// Get any ReconstructionGeometry objects that are of type ResolvedTopologicalBoundary.
	resolved_geom_seq_type resolved_geom_seq;
	GPlatesAppLogic::ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type_sequence(
			reconstruction_geom_seq.begin(),
			reconstruction_geom_seq.end(),
			resolved_geom_seq);

	// Export the various files.
	export_files(resolved_geom_seq, reconstruction_time, output_filebasename);
	
	// Normal exit, all good, ask the Context to process the next iteration please.
	return true;
}


void
GPlatesGui::ExportResolvedTopologyAnimationStrategy::wrap_up(
		bool export_successful)
{
	// If we need to do anything after writing a whole batch of velocity files,
	// here's the place to do it.
	// Of course, there's also the destructor, which should free up any resources
	// we acquired in the constructor; this method is intended for any "last step"
	// iteration operations that might need to occur. Perhaps all iterations end
	// up in the same file and we should close that file (if all steps completed
	// successfully).
}


void
GPlatesGui::ExportResolvedTopologyAnimationStrategy::export_files(
		const resolved_geom_seq_type &resolved_geom_seq,
		const double &recon_time,
		const QString &filebasename)
{
	const QDir &target_dir = d_export_animation_context_ptr->target_dir();

	// TODO: Expose these options in the export GUI and only set placeholder strings for
	// those export types selected by the user.
	GPlatesFileIO::ResolvedTopologicalBoundaryExport::OutputOptions output_options;
	output_options.placeholder_platepolygons = s_placeholder_platepolygons;
	output_options.placeholder_lines = s_placeholder_lines;
	output_options.placeholder_ridge_transforms = s_placeholder_ridge_transforms;
	output_options.placeholder_subductions = s_placeholder_subductions;
	output_options.placeholder_left_subductions = s_placeholder_left_subductions;
	output_options.placeholder_right_subductions = s_placeholder_right_subductions;
	output_options.placeholder_slab_polygons = s_placeholder_slab_polygons;
	output_options.placeholder_slab_edge_leading = s_placeholder_slab_edge_leading;
	output_options.placeholder_slab_edge_leading_left = s_placeholder_slab_edge_leading_left;
	output_options.placeholder_slab_edge_leading_right = s_placeholder_slab_edge_leading_right;
	output_options.placeholder_slab_edge_trench = s_placeholder_slab_edge_trench;
	output_options.placeholder_slab_edge_side = s_placeholder_slab_edge_side;

	// For time being only also export subsegments of plate polygons to 'lines' file.
	// Not also exporting subsegments of slab polygons to 'lines' file.
	//
	// TODO: Remove when this can be set by user via the GUI.
	output_options.export_plate_polygon_subsegments_to_lines = true;
	output_options.export_slab_polygon_subsegments_to_lines = false;

	GPlatesFileIO::ResolvedTopologicalBoundaryExport::export_resolved_topological_boundaries(
			target_dir,
			filebasename,
			GPlatesUtils::ExportTemplateFilename::PLACEHOLDER_FORMAT_STRING,
			output_options,
			GPlatesFileIO::ResolvedTopologicalBoundaryExport::get_export_file_format(filebasename),
			resolved_geom_seq,
			d_loaded_files,
			d_export_animation_context_ptr->view_state().get_application_state().get_current_anchored_plate_id(),
			d_export_animation_context_ptr->view_time());
}


const QString&
GPlatesGui::ExportResolvedTopologyAnimationStrategy::get_default_filename_template()
{
	switch(d_file_format)
	{
	case SHAPEFILE:
		return DEFAULT_RESOLVED_TOPOLOGIES_SHP_FILENAME_TEMPLATE;
		break;
	case GMT:
		return DEFAULT_RESOLVED_TOPOLOGIES_GMT_FILENAME_TEMPLATE;
		break;
	default:
		return DEFAULT_RESOLVED_TOPOLOGIES_GMT_FILENAME_TEMPLATE;
		break;
	}
}

