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
 
#include <QString>

#include "ExportAnimationStrategy.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/Layer.h"
#include "app-logic/ReconstructGraph.h"

#include "gui/AnimationController.h"
#include "gui/ExportAnimationContext.h"

#include "presentation/ViewState.h"
#include "presentation/VisualLayer.h"
#include "presentation/VisualLayers.h"


GPlatesGui::ExportAnimationStrategy::ExportAnimationStrategy(
		GPlatesGui::ExportAnimationContext &export_animation_context):
	d_export_animation_context_ptr(&export_animation_context),
	d_filename_sequence_opt(boost::none),
	d_filename_iterator_opt(boost::none)
{  }


void
GPlatesGui::ExportAnimationStrategy::set_template_filename(
		const QString &filename)
{
	GPlatesPresentation::ViewState &view_state = d_export_animation_context_ptr->view_state();
	GPlatesAppLogic::ApplicationState &application_state = view_state.get_application_state();

	//
	// Get the name of the visual layer associated with the default reconstruction tree layer.
	// 
	// If there is no reconstruction tree layer loaded then the layer name will be empty.
	//

	QString default_recon_tree_layer_name;

	GPlatesAppLogic::Layer default_recon_tree_layer =
			application_state.get_reconstruct_graph().get_default_reconstruction_tree_layer();

	boost::weak_ptr<const GPlatesPresentation::VisualLayer> default_recon_tree_visual_layer =
			view_state.get_visual_layers().get_visual_layer(default_recon_tree_layer);

	boost::shared_ptr<const GPlatesPresentation::VisualLayer> locked_default_recon_tree_visual_layer =
			default_recon_tree_visual_layer.lock();
	if (locked_default_recon_tree_visual_layer)
	{
		default_recon_tree_layer_name = locked_default_recon_tree_visual_layer->get_name();
	}

	//
	// Create the export template filename sequence.
	//

	d_filename_sequence_opt = GPlatesFileIO::ExportTemplateFilenameSequence(
			filename,
			application_state.get_current_anchored_plate_id(),
			default_recon_tree_layer_name,
			d_export_animation_context_ptr->get_sequence().actual_start_time,
			d_export_animation_context_ptr->get_sequence().actual_end_time,
			d_export_animation_context_ptr->get_sequence().raw_time_increment,
			d_export_animation_context_ptr->get_sequence().should_finish_exactly_on_end_time);
	d_filename_iterator_opt = d_filename_sequence_opt->begin();
}

bool
GPlatesGui::ExportAnimationStrategy::check_filename_sequence()
{
	// Get the iterator for the next filename.
	if (!d_filename_iterator_opt || !d_filename_sequence_opt) 
	{
		d_export_animation_context_ptr->update_status_message(
				QObject::tr("Error in export iteration - not properly initialised!"));
		return false;
	}
	GPlatesFileIO::ExportTemplateFilenameSequence::const_iterator &filename_it= 
		*d_filename_iterator_opt;

	// Double check that the iterator is valid.
	if (filename_it == d_filename_sequence_opt->end()) 
	{
		d_export_animation_context_ptr->update_status_message(
				QObject::tr("Error in filename sequence - not enough filenames supplied!"));
		return false;
	}
	return true;
}
