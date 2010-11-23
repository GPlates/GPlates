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
#include "ExportMotionTrackAnimationStrategy.h"

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
				GPlatesUtils::ExportTemplateFilename::PLACEHOLDER_FORMAT_STRING,
				flowlines_filename);

		return output_basename;
	}
}

const QString 
GPlatesGui::ExportMotionTrackAnimationStrategy::DEFAULT_MOTION_TRACKS_GMT_FILENAME_TEMPLATE
		="motion_track_output_%u_%0.2f.xy";
const QString 
GPlatesGui::ExportMotionTrackAnimationStrategy::DEFAULT_MOTION_TRACKS_SHP_FILENAME_TEMPLATE
		="motion_track_output_%u_%0.2f.shp";
      
const QString 
GPlatesGui::ExportMotionTrackAnimationStrategy::MOTION_TRACKS_FILENAME_TEMPLATE_DESC
		= FORMAT_CODE_DESC; 
const QString 
GPlatesGui::ExportMotionTrackAnimationStrategy::MOTION_TRACKS_DESC
		="Export motion tracks.";

const GPlatesGui::ExportMotionTrackAnimationStrategy::non_null_ptr_type
GPlatesGui::ExportMotionTrackAnimationStrategy::create(
		GPlatesGui::ExportAnimationContext &export_animation_context,
		FileFormat format,
		const ExportAnimationStrategy::Configuration& cfg)
{
	ExportMotionTrackAnimationStrategy * ptr=
			new ExportMotionTrackAnimationStrategy(export_animation_context,
					cfg.filename_template()); 
	
	ptr->d_file_format = format;
	return non_null_ptr_type(
			ptr,
			GPlatesUtils::NullIntrusivePointerHandler());
}

GPlatesGui::ExportMotionTrackAnimationStrategy::ExportMotionTrackAnimationStrategy(
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
GPlatesGui::ExportMotionTrackAnimationStrategy::set_template_filename(
		const QString &filename)
{

	ExportAnimationStrategy::set_template_filename(filename);
}

bool
GPlatesGui::ExportMotionTrackAnimationStrategy::do_export_iteration(
		std::size_t frame_index)
{

	if(!check_filename_sequence())
	{
		return false;
	}
	GPlatesUtils::ExportTemplateFilenameSequence::const_iterator &filename_it = 
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


	try {


		GPlatesViewOperations::VisibleReconstructionGeometryExport::export_visible_reconstruced_motion_tracks(
			full_filename,
			d_export_animation_context_ptr->view_state().get_rendered_geometry_collection(),
			d_loaded_files,
			d_export_animation_context_ptr->view_state().get_application_state().get_current_anchored_plate_id(),
			d_export_animation_context_ptr->view_time());

	} catch (...) {
		// FIXME: Catch all proper exceptions we might get here.
		d_export_animation_context_ptr->update_status_message(
			QObject::tr("Error writing reconstructed motion track file \"%1\"!").arg(full_filename));
		return false;
	}


	// Normal exit, all good, ask the Context to process the next iteration please.
	return true;
}


void
GPlatesGui::ExportMotionTrackAnimationStrategy::wrap_up(
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

const QString&
GPlatesGui::ExportMotionTrackAnimationStrategy::get_default_filename_template()
{
	switch(d_file_format)
	{
	case GMT:
		return DEFAULT_MOTION_TRACKS_GMT_FILENAME_TEMPLATE;
		break;
	case SHAPEFILE:
		return DEFAULT_MOTION_TRACKS_SHP_FILENAME_TEMPLATE;
		break;
	default:
		return DEFAULT_MOTION_TRACKS_GMT_FILENAME_TEMPLATE;
		break;
	}
}

const QString&
GPlatesGui::ExportMotionTrackAnimationStrategy::get_filename_template_desc()
{
	return MOTION_TRACKS_FILENAME_TEMPLATE_DESC;
}


