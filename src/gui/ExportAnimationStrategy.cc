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
 

#include "ExportAnimationStrategy.h"

#include "app-logic/ApplicationState.h"

#include "gui/AnimationController.h"
#include "gui/ExportAnimationContext.h"

#include "presentation/ViewState.h"


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
	d_filename_sequence_opt = GPlatesFileIO::ExportTemplateFilenameSequence(filename,
			d_export_animation_context_ptr->view_state().get_application_state().get_current_anchored_plate_id(),
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
