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
 
#include <QDir>
#include <vector>
#include "ExportAnimationContext.h"

#include "utils/FloatingPointComparisons.h"
#include "utils/ExportTemplateFilenameSequence.h"

#include "gui/AnimationController.h"
#include "qt-widgets/ExportAnimationDialog.h"
#include "qt-widgets/ViewportWindow.h"	// ViewState, needed for .reconstruction_root()

#include "gui/ExportSvgAnimationStrategy.h"
#include "gui/ExportVelocityAnimationStrategy.h"
#include "gui/ExportReconstructedGeometryAnimationStrategy.h"



GPlatesGui::ExportAnimationContext::ExportAnimationContext(
		GPlatesQtWidgets::ExportAnimationDialog &export_animation_dialog_,
		GPlatesGui::AnimationController &animation_controller_,
		GPlatesQtWidgets::ViewportWindow &view_state_):
	d_export_animation_dialog_ptr(&export_animation_dialog_),
	d_animation_controller_ptr(&animation_controller_),
	d_view_state_ptr(&view_state_),
	d_abort_now(false),
	d_export_running(false),
	d_svg_exporter_enabled(true),
	d_velocity_exporter_enabled(true),
	d_gmt_exporter_enabled(true),
	d_shp_exporter_enabled(true)
{  }


const double &
GPlatesGui::ExportAnimationContext::view_time() const
{
	return d_animation_controller_ptr->view_time();
}


const GPlatesGui::AnimationController &
GPlatesGui::ExportAnimationContext::animation_controller() const
{
	return *d_animation_controller_ptr;
}


GPlatesQtWidgets::ViewportWindow &
GPlatesGui::ExportAnimationContext::view_state()
{
	return *d_view_state_ptr;
}



bool
GPlatesGui::ExportAnimationContext::do_export()
{
	d_export_running = true;
	// Setting this flag to 'true' while we are exporting will cause us to abort.
	d_abort_now = false;
	
	// Prepare the exporters.
	std::vector<ExportAnimationStrategy::non_null_ptr_type> exporters;
	typedef std::vector<ExportAnimationStrategy::non_null_ptr_type>::iterator exporters_iterator_type;
	if (d_svg_exporter_enabled) {
		exporters.push_back(ExportSvgAnimationStrategy::create(*this));
	}
	if (d_velocity_exporter_enabled) {
		exporters.push_back(ExportVelocityAnimationStrategy::create(*this));
	}
	if (d_gmt_exporter_enabled) {
		exporters.push_back(ExportReconstructedGeometryAnimationStrategy::create(*this));
	}
	if (d_shp_exporter_enabled) {
		exporters.push_back(ExportReconstructedGeometryAnimationStrategy::create(*this));
		// The quickest way to get Shapefile export in there as an added bonus is to just
		// override the ExportTemplateFilenameSequence right here.
		exporters.back()->set_template_filename(shp_exporter_filename_template());
		// This should be fine for now since we need to overhaul the filename parameter setup anyway.
	}
	
	// Determine how many frames we need to iterate through.
	std::size_t length = d_animation_controller_ptr->duration_in_frames();

	// Set the progress bar to 0 - we haven't finished writing frame 1 yet.
	d_export_animation_dialog_ptr->update_progress_bar(length, 0);
	
	for (std::size_t frame_index = 0, frame_number = 1;
			frame_index < length;
			++frame_index, ++frame_number) {
		if (d_abort_now) {
			update_status_message(QObject::tr("Export Aborted"));
			d_export_running = false;
			d_abort_now = false;
			return false;
		}
		
		// Manipulate the View to set the correct time, ready for the export strategies to do their thing.
		update_status_message(QObject::tr("Reconstructing to %1 Ma...")
				.arg(d_animation_controller_ptr->calculate_time_for_frame(frame_index), 0, 'f', 2 ));
		d_animation_controller_ptr->set_view_frame(frame_index);

		// Run through each of the exporters for one iteration.
		bool ok = true;
		exporters_iterator_type export_it = exporters.begin();
		exporters_iterator_type export_end = exporters.end();
		for (; export_it != export_end; ++export_it) {
			ok = ok && (*export_it)->do_export_iteration(frame_index);
		}

		if ( ! ok) {
			// Failed. Just quit the whole thing.
			exporters_iterator_type failed_it = exporters.begin();
			exporters_iterator_type failed_end = exporters.end();
			for (; failed_it != failed_end; ++failed_it) {
				(*failed_it)->wrap_up(false);
			}
			d_export_running = false;
			d_abort_now = false;
			return false;
		}

		// Move the dialog's progress bar to indicate we have finished this frame number.
		d_export_animation_dialog_ptr->update_progress_bar(length, frame_number);
	}

	// All finished! Allow exporters to do some clean-up, if they need to.
	exporters_iterator_type done_it = exporters.begin();
	exporters_iterator_type done_end = exporters.end();
	for (; done_it != done_end; ++done_it) {
		(*done_it)->wrap_up(true);
	}

	// Update dialog - successful finish.
	d_export_running = false;
	update_status_message(QObject::tr("Export Finished."));
	
	return true;
}


void
GPlatesGui::ExportAnimationContext::update_status_message(
		QString message)
{
	d_export_animation_dialog_ptr->update_status_message(message);
}


