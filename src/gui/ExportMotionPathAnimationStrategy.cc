/* $Id: ExportFlowlinesAnimationStrategy.cc 8309 2010-05-06 14:40:07Z rwatson $ */

/**
 * \file 
 * $Revision: 8309 $
 * $Date: 2010-05-06 16:40:07 +0200 (to, 06 mai 2010) $ 
 * 
 * Copyright (C) 2010 Geological Survey of Norway
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
#include <QFileInfo>
#include <QString>
#include "ExportMotionPathAnimationStrategy.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/AppLogicUtils.h"
#include "app-logic/FeatureCollectionFileState.h"

#include "gui/ExportAnimationContext.h"
#include "gui/AnimationController.h"

#include "presentation/ViewState.h"
#include "view-operations/VisibleReconstructionGeometryExport.h"


namespace
{

	QString
	substitute_placeholder(
			const QString &output_filebasename,
			const QString &placeholder,
			const QString &placeholder_replacement)
	{
		return QString(output_filebasename).replace(placeholder, placeholder_replacement);
	}

	QString
	calculate_output_basename(
			const QString &output_filename,
			const QString &flowlines_filename)
	{
		const QString output_basename = substitute_placeholder(
				output_filename,
				GPlatesFileIO::ExportTemplateFilename::PLACEHOLDER_FORMAT_STRING,
				flowlines_filename);

		return output_basename;
	}
}

GPlatesGui::ExportMotionPathAnimationStrategy::ExportMotionPathAnimationStrategy(
		GPlatesGui::ExportAnimationContext &export_animation_context,
		const const_configuration_ptr &configuration):
	ExportAnimationStrategy(export_animation_context),
	d_configuration(configuration)
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
			const GPlatesAppLogic::LayerInputChannelName::Type main_input_channel =
					it->get_main_input_feature_collection_channel();
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

void
GPlatesGui::ExportMotionPathAnimationStrategy::set_template_filename(
		const QString &filename)
{
	ExportAnimationStrategy::set_template_filename(filename);
}

bool
GPlatesGui::ExportMotionPathAnimationStrategy::do_export_iteration(
		std::size_t frame_index)
{
	GPlatesFileIO::ExportTemplateFilenameSequence::const_iterator &filename_it = 
		*d_filename_iterator_opt;

	// Figure out a filename from the template filename sequence.
	QString basename = *filename_it++;
	// Add the target dir to that to figure out the absolute path + name.
	QString full_filename = d_export_animation_context_ptr->target_dir().absoluteFilePath(basename);

	// Next, the file writing. Update the dialog status message.
	d_export_animation_context_ptr->update_status_message(
		QObject::tr("Writing motion tracks at frame %2 to file \"%1\"...")
		.arg(basename)
		.arg(frame_index) );


	try
	{
		GPlatesViewOperations::VisibleReconstructionGeometryExport::export_visible_reconstructed_motion_paths(
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
			d_configuration->wrap_to_dateline);

	}
	catch (std::exception &exc)
	{
		d_export_animation_context_ptr->update_status_message(
			QObject::tr("Error writing reconstructed motion track file \"%1\": %2")
					.arg(full_filename)
					.arg(exc.what()));
		return false;
	}
	catch (...)
	{
		// FIXME: Catch all proper exceptions we might get here.
		d_export_animation_context_ptr->update_status_message(
			QObject::tr("Error writing reconstructed motion track file \"%1\": unknown error!").arg(full_filename));
		return false;
	}


	// Normal exit, all good, ask the Context to process the next iteration please.
	return true;
}


void
GPlatesGui::ExportMotionPathAnimationStrategy::wrap_up(
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
