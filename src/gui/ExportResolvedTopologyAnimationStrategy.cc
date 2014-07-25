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

#include <boost/foreach.hpp>

#include "ExportResolvedTopologyAnimationStrategy.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/FeatureCollectionFileState.h"

#include "gui/ExportAnimationContext.h"
#include "gui/AnimationController.h"

#include "presentation/ViewState.h"

GPlatesGui::ExportResolvedTopologyAnimationStrategy::ExportResolvedTopologyAnimationStrategy(
		GPlatesGui::ExportAnimationContext &export_animation_context,
		const const_configuration_ptr &cfg) :
	ExportAnimationStrategy(export_animation_context),
	d_configuration(cfg)
{
	set_template_filename(d_configuration->get_filename_template());
	
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

	const GPlatesAppLogic::ReconstructGraph &reconstruct_graph =
			d_export_animation_context_ptr->view_state().get_application_state().get_reconstruct_graph();

	// Check all the active reconstruction layers, and get their input files.
	GPlatesAppLogic::ReconstructGraph::const_iterator it = reconstruct_graph.begin(),
													end = reconstruct_graph.end();
	for (; it != end ; ++it)
	{
		if ((it->get_type() == GPlatesAppLogic::LayerTaskType::RECONSTRUCTION) && it->is_active())
		{

			// The 'reconstruct geometries' layer has input feature collections on its main input channel.
			const QString main_input_channel = it->get_main_input_feature_collection_channel();
			const std::vector<GPlatesAppLogic::Layer::InputConnection> main_inputs =
					it->get_channel_inputs(main_input_channel);

			// Loop over all input connections to get the files (feature collections) for the current target layer.
			BOOST_FOREACH(const GPlatesAppLogic::Layer::InputConnection& main_input_connection, main_inputs)
			{
				boost::optional<GPlatesAppLogic::Layer::InputFile> input_file =
						main_input_connection.get_input_file();
				// If it's not a file (ie, it's a layer) then continue to the next file.
				if(!input_file)
				{
					continue;
				}
				d_loaded_reconstruction_files.push_back(&(input_file->get_file().get_file()));
			}


		}
	}
}


bool
GPlatesGui::ExportResolvedTopologyAnimationStrategy::do_export_iteration(
		std::size_t frame_index)
{
	GPlatesFileIO::ExportTemplateFilenameSequence::const_iterator &filename_it = 
		*d_filename_iterator_opt;

	// Figure out a filename from the template filename sequence.
	QString basename = *filename_it++;
	// Add the target dir to that to figure out the absolute path + name.
	QString full_filename = d_export_animation_context_ptr->target_dir().absoluteFilePath(basename);

	// All that's really expected of us at this point is maybe updating
	// the dialog status message, then calculating what we want to calculate,
	// and writing whatever file we feel like writing.
	d_export_animation_context_ptr->update_status_message(
			QObject::tr("Writing resolved topologies at frame %2 to file \"%1\"...")
			.arg(basename)
			.arg(frame_index) );

	// Here's where we do the actual work of exporting of the resolved topologies,
	// given frame_index, filename, reconstructable files and geoms, and target_dir. Etc.
	try
	{

		GPlatesViewOperations::VisibleReconstructionGeometryExport::export_visible_resolved_topologies(
			full_filename,
			d_export_animation_context_ptr->view_state().get_rendered_geometry_collection(),
			d_export_animation_context_ptr->view_state().get_application_state().get_feature_collection_file_format_registry(),
			d_loaded_files,
			d_loaded_reconstruction_files,
			d_export_animation_context_ptr->view_state().get_application_state().get_current_anchored_plate_id(),
			d_export_animation_context_ptr->view_time(),
			d_configuration->file_options.export_to_a_single_file,
			d_configuration->file_options.export_to_multiple_files,
			d_configuration->file_options.separate_output_directory_per_file,
			d_configuration->export_topological_lines,
			d_configuration->export_topological_polygons,
			d_configuration->force_polygon_orientation,
			d_configuration->wrap_to_dateline);

	}
	catch (std::exception &exc)
	{
		d_export_animation_context_ptr->update_status_message(
			QObject::tr("Error writing reconstructed geometry file \"%1\": %2")
					.arg(full_filename)
					.arg(exc.what()));
		return false;
	}
	catch (...)
	{
		// FIXME: Catch all proper exceptions we might get here.
		d_export_animation_context_ptr->update_status_message(
			QObject::tr("Error writing reconstructed geometry file \"%1\": unknown error!").arg(full_filename));
		return false;
	}
	
	// Normal exit, all good, ask the Context process the next iteration please.
	return true;
}
