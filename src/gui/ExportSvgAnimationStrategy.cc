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
 

#include "ExportSvgAnimationStrategy.h"

#include "utils/FloatingPointComparisons.h"

#include "gui/ExportAnimationContext.h"
#include "gui/AnimationController.h"
#include "qt-widgets/ViewportWindow.h"	// ViewState, needed for .reconstruction_root()



const GPlatesGui::ExportSvgAnimationStrategy::non_null_ptr_type
GPlatesGui::ExportSvgAnimationStrategy::create(
		GPlatesGui::ExportAnimationContext &export_animation_context)
{
	return non_null_ptr_type(new ExportSvgAnimationStrategy(export_animation_context),
			GPlatesUtils::NullIntrusivePointerHandler());
}


GPlatesGui::ExportSvgAnimationStrategy::ExportSvgAnimationStrategy(
		GPlatesGui::ExportAnimationContext &export_animation_context):
	ExportAnimationStrategy(export_animation_context)
{
	// Set the ExportTemplateFilenameSequence name once here, to a sane default.
	// Later, we will let the user configure this.
	set_template_filename(QString("snapshot_%u_%0.2f.svg"));
}


bool
GPlatesGui::ExportSvgAnimationStrategy::do_export_iteration(
		std::size_t frame_index)
{
	// Get the iterator for the next filename.
	if (!d_filename_iterator_opt || !d_filename_sequence_opt) {
		d_export_animation_context_ptr->update_status_message(
				QObject::tr("Error in export iteration - not properly initialised!"));
		return false;
	}
	GPlatesUtils::ExportTemplateFilenameSequence::const_iterator &filename_it = *d_filename_iterator_opt;

	// Doublecheck that the iterator is valid.
	if (filename_it == d_filename_sequence_opt->end()) {
		d_export_animation_context_ptr->update_status_message(
				QObject::tr("Error in filename sequence - not enough filenames supplied!"));
		return false;
	}

	// Figure out a filename from the template filename sequence.
	QString basename = *filename_it++;
	// Add the target dir to that to figure out the absolute path + name.
	QString full_filename = d_export_animation_context_ptr->target_dir().absoluteFilePath(basename);

	// All that's really expected of us at this point is maybe updating
	// the dialog status message, then calculating what we want to calculate,
	// and writing whatever file we feel like writing.
	d_export_animation_context_ptr->update_status_message(
			QObject::tr("Writing geometry snapshot at frame %2 to file \"%1\"...")
			.arg(basename)
			.arg(frame_index) );

	// Here's where we do the actual work of exporting of the SVG snapshots,
	// given frame_index, filename, and target_dir.
	d_export_animation_context_ptr->view_state().create_svg_file(full_filename);
	
	// Normal exit, all good, ask the Context process the next iteration please.
	return true;
}




