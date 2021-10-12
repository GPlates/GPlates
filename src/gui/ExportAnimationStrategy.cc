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

#include "app-logic/Reconstruct.h"

#include "gui/AnimationController.h"
#include "gui/ExportAnimationContext.h"

#include "utils/FloatingPointComparisons.h"

#include "presentation/ViewState.h"


GPlatesGui::ExportAnimationStrategy::ExportAnimationStrategy(
		GPlatesGui::ExportAnimationContext &export_animation_context):
	d_export_animation_context_ptr(&export_animation_context),
	d_filename_sequence_opt(boost::none),
	d_filename_iterator_opt(boost::none)
{
	set_template_filename(QString("dummy_%u_%d_%A.gpml"));
}


void
GPlatesGui::ExportAnimationStrategy::set_template_filename(
		const QString &filename)
{
	d_filename_sequence_opt = GPlatesUtils::ExportTemplateFilenameSequence(filename,
			d_export_animation_context_ptr->view_state().get_reconstruct().get_current_anchored_plate_id(),
			d_export_animation_context_ptr->animation_controller().start_time(),
			d_export_animation_context_ptr->animation_controller().end_time(),
			d_export_animation_context_ptr->animation_controller().raw_time_increment(),
			d_export_animation_context_ptr->animation_controller().should_finish_exactly_on_end_time());
	d_filename_iterator_opt = d_filename_sequence_opt->begin();
}

