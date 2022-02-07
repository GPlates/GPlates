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

#include <exception>
#include <QDebug>
#include <QImage>
#include <QImageWriter>

#include "ExportImageAnimationStrategy.h"

#include "AnimationController.h"
#include "ExportAnimationContext.h"

#include "presentation/ViewState.h"

#include "qt-widgets/ReconstructionViewWidget.h"
#include "qt-widgets/SceneView.h"
#include "qt-widgets/ViewportWindow.h"

#include "view-operations/RenderedGeometryCollection.h"


GPlatesGui::ExportImageAnimationStrategy::ExportImageAnimationStrategy(
		GPlatesGui::ExportAnimationContext &export_animation_context,
		const const_configuration_ptr &configuration):
	ExportAnimationStrategy(export_animation_context),
	d_configuration(configuration)
{
	set_template_filename(d_configuration->get_filename_template());
}

bool
GPlatesGui::ExportImageAnimationStrategy::do_export_iteration(
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
			QObject::tr("Writing image at frame %2 to file \"%1\"...")
			.arg(basename)
			.arg(frame_index) );

	// Here's where we do the actual work of exporting of the image snapshots,
	// given frame_index, filename, and target_dir.
	GPlatesQtWidgets::SceneView &active_scene_view =
			d_export_animation_context_ptr->view_state().get_other_view_state()
				.reconstruction_view_widget().active_view();
	GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection =
			d_export_animation_context_ptr->view_state().get_rendered_geometry_collection();
	try
	{
		// Get current rendered layer active state so we can restore later.
		const GPlatesViewOperations::RenderedGeometryCollection::MainLayerActiveState
				prev_rendered_layer_active_state =
						rendered_geometry_collection.capture_main_layer_active_state();

		// Turn off rendering of all layers except the reconstruction layer.
		for (unsigned int layer_index = 0;
			layer_index < GPlatesViewOperations::RenderedGeometryCollection::NUM_LAYERS;
			++layer_index)
		{
			const GPlatesViewOperations::RenderedGeometryCollection::MainLayerType layer =
					static_cast<GPlatesViewOperations::RenderedGeometryCollection::MainLayerType>(layer_index);

			if (layer != GPlatesViewOperations::RenderedGeometryCollection::RECONSTRUCTION_LAYER)
			{
				rendered_geometry_collection.set_main_layer_active(layer, false);
			}
		}

		// Render to the image file.
		const QImage image = active_scene_view.render_to_qimage(
				d_configuration->image_resolution_options.image_size);
		if (image.isNull())
		{
			// Most likely a memory allocation failure.
			d_export_animation_context_ptr->update_status_message(
				QObject::tr("Error exporting to colour (RGBA) image file \"%1\" due to insufficient memory")
						.arg(full_filename));
			return false;
		}

		QImageWriter image_writer(full_filename);

		// If format is TIFF then compress.
		//
		// FIXME: Should probably give the user an option to compress (for file formats supporting it).
		if (d_configuration->image_type == Configuration::TIFF)
		{
			image_writer.setCompression(1); // LZW-compression
		}

		// Save the image to file.
		image_writer.write(image);

		// Restore previous rendered layer active state.
		rendered_geometry_collection.restore_main_layer_active_state(prev_rendered_layer_active_state);
	}
	catch (std::exception &exc)
	{
		d_export_animation_context_ptr->update_status_message(
			QObject::tr("Error exporting to colour (RGBA) image file \"%1\": %2")
					.arg(full_filename)
					.arg(exc.what()));
		return false;
	}
	
	// Normal exit, all good, ask the Context process the next iteration please.
	return true;
}
