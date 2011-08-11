/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2009, 2010 The University of Sydney, Australia
 * Copyright (C) 2010, 2011 Geological Survey of Norway
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

#include "ExportReconstructedGeometryAnimationStrategy.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/FeatureCollectionFileState.h"

#include "gui/ExportAnimationContext.h"
#include "gui/AnimationController.h"

#include "presentation/ViewState.h"


GPlatesGui::ExportReconstructedGeometryAnimationStrategy::ExportReconstructedGeometryAnimationStrategy(
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
}


bool
GPlatesGui::ExportReconstructedGeometryAnimationStrategy::do_export_iteration(
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
			QObject::tr("Writing reconstructed geometries at frame %2 to file \"%1\"...")
			.arg(basename)
			.arg(frame_index) );

	// Here's where we do the actual work of exporting of the RFGs,
	// given frame_index, filename, reconstructable files and geoms, and target_dir. Etc.
	try
	{

		// TODO: Get 'export_single_output_file' and 'export_per_input_file' from user (via GUI).
		GPlatesViewOperations::VisibleReconstructionGeometryExport::export_visible_reconstructed_feature_geometries(
			full_filename,
			d_export_animation_context_ptr->view_state().get_rendered_geometry_collection(),
			d_export_animation_context_ptr->view_state().get_application_state().get_feature_collection_file_format_registry(),
			d_loaded_files,
			d_export_animation_context_ptr->view_state().get_application_state().get_current_anchored_plate_id(),
			d_export_animation_context_ptr->view_time(),
			d_configuration->file_options.export_to_a_single_file,
			d_configuration->file_options.export_to_multiple_files);

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
