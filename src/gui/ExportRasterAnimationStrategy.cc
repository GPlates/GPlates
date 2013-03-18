/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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
#include <QImage>

#include "ExportRasterAnimationStrategy.h"
#include "AnimationController.h"
#include "ExportAnimationContext.h"

#include "global/GPlatesException.h"

#include "presentation/ViewState.h"

#include "qt-widgets/ReconstructionViewWidget.h"
#include "qt-widgets/SceneView.h"
#include "qt-widgets/ViewportWindow.h"


GPlatesGui::ExportRasterAnimationStrategy::ExportRasterAnimationStrategy(
		GPlatesGui::ExportAnimationContext &export_animation_context,
		const const_configuration_ptr &configuration):
	ExportAnimationStrategy(export_animation_context),
	d_configuration(configuration)
{
	set_template_filename(d_configuration->get_filename_template());
}

bool
GPlatesGui::ExportRasterAnimationStrategy::do_export_iteration(
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
			QObject::tr("Writing raster at frame %2 to file \"%1\"...")
			.arg(basename)
			.arg(frame_index) );

	// Here's where we do the actual work of exporting of the raster snapshots,
	// given frame_index, filename, and target_dir.
	GPlatesQtWidgets::SceneView &active_scene_view =
			d_export_animation_context_ptr->view_state().get_other_view_state()
				.reconstruction_view_widget().active_view();
	try
	{
		const QSize raster_size = active_scene_view.get_viewport_size();

		// Render to the raster image file.
		const QImage raster_image = active_scene_view.render_to_qimage(raster_size);

		// Save the raster to file.
		raster_image.save(full_filename);
	}
	catch (GPlatesGlobal::Exception &exc)
	{
		qWarning() << "Caught exception exporting to colour (RGBA) raster: " << exc;
		return false;
	}
	
	// Normal exit, all good, ask the Context process the next iteration please.
	return true;
}
