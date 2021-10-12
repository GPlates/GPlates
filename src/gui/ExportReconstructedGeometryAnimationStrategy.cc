/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2009, 2010 The University of Sydney, Australia
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
 

#include "ExportReconstructedGeometryAnimationStrategy.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/FeatureCollectionFileState.h"

#include "gui/ExportAnimationContext.h"
#include "gui/AnimationController.h"

#include "presentation/ViewState.h"

#include "utils/FloatingPointComparisons.h"

const QString 
GPlatesGui::ExportReconstructedGeometryAnimationStrategy::DEFAULT_RECONSTRUCTED_GEOMETRIES_GMT_FILENAME_TEMPLATE
		="reconstructed_%u_%0.2f.xy";
const QString 
GPlatesGui::ExportReconstructedGeometryAnimationStrategy::DEFAULT_RECONSTRUCTED_GEOMETRIES_SHP_FILENAME_TEMPLATE
		="reconstructed_%u_%0.2f.shp";
const QString GPlatesGui::ExportReconstructedGeometryAnimationStrategy::RECONSTRUCTED_GEOMETRIES_FILENAME_TEMPLATE_DESC
		=FORMAT_CODE_DESC;
const QString GPlatesGui::ExportReconstructedGeometryAnimationStrategy::RECONSTRUCTED_GEOMETRIES_DESC
		="Export reconstructed geometries.";


const GPlatesGui::ExportReconstructedGeometryAnimationStrategy::non_null_ptr_type
GPlatesGui::ExportReconstructedGeometryAnimationStrategy::create(
		GPlatesGui::ExportAnimationContext &export_animation_context,
		FileFormat format,
		const ExportAnimationStrategy::Configuration &cfg)
{
	ExportReconstructedGeometryAnimationStrategy * ptr=
			new ExportReconstructedGeometryAnimationStrategy(
					export_animation_context,
					cfg.filename_template());

	ptr->d_file_format=format;
	
	return non_null_ptr_type(
			ptr,
			GPlatesUtils::NullIntrusivePointerHandler());
}

GPlatesGui::ExportReconstructedGeometryAnimationStrategy::ExportReconstructedGeometryAnimationStrategy(
		GPlatesGui::ExportAnimationContext &export_animation_context,
		const QString &filename_template):
	ExportAnimationStrategy(export_animation_context)
{
	set_template_filename(filename_template);
	
	GPlatesAppLogic::FeatureCollectionFileState &file_state =
			d_export_animation_context_ptr->view_state().get_application_state()
					.get_feature_collection_file_state();

	// From the ViewState, obtain the list of files with usable reconstructed geometry in them.
	// Copy GPlatesFileIO:File into GPlatesFileIO:File::weak_ref.
	GPlatesAppLogic::FeatureCollectionFileState::active_file_iterator_range active_files =
			file_state.get_active_reconstructable_files();
	GPlatesAppLogic::FeatureCollectionFileState::active_file_iterator iter = active_files.begin;
	GPlatesAppLogic::FeatureCollectionFileState::active_file_iterator end = active_files.end;
	for ( ; iter != end; ++iter)
	{
		GPlatesFileIO::File &file = *iter;
		d_active_reconstructable_files.push_back(&file);
	}
}


bool
GPlatesGui::ExportReconstructedGeometryAnimationStrategy::do_export_iteration(
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

	// All that's really expected of us at this point is maybe updating
	// the dialog status message, then calculating what we want to calculate,
	// and writing whatever file we feel like writing.
	d_export_animation_context_ptr->update_status_message(
			QObject::tr("Writing reconstructed geometries at frame %2 to file \"%1\"...")
			.arg(basename)
			.arg(frame_index) );

	// Here's where we do the actual work of exporting of the RFGs,
	// given frame_index, filename, reconstructable files and geoms, and target_dir. Etc.
	try {

		GPlatesViewOperations::VisibleReconstructedFeatureGeometryExport::export_visible_geometries(
				full_filename,
				d_export_animation_context_ptr->view_state().get_rendered_geometry_collection(),
				d_active_reconstructable_files,
				d_export_animation_context_ptr->view_state().get_application_state().get_current_anchored_plate_id(),
				d_export_animation_context_ptr->view_time());

	} catch (...) {
		// FIXME: Catch all proper exceptions we might get here.
		d_export_animation_context_ptr->update_status_message(
				QObject::tr("Error writing reconstructed geometry file \"%1\"!").arg(full_filename));
		return false;
	}
	
	// Normal exit, all good, ask the Context process the next iteration please.
	return true;
}

const QString&
GPlatesGui::ExportReconstructedGeometryAnimationStrategy::get_default_filename_template()
{
	switch(d_file_format)
	{
	case SHAPEFILE:
		return DEFAULT_RECONSTRUCTED_GEOMETRIES_SHP_FILENAME_TEMPLATE;
		break;
	case GMT:
		return DEFAULT_RECONSTRUCTED_GEOMETRIES_GMT_FILENAME_TEMPLATE;
		break;
	default:
		return DEFAULT_RECONSTRUCTED_GEOMETRIES_GMT_FILENAME_TEMPLATE;
		break;
	}
}




