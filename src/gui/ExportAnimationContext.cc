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

#include "gui/AnimationController.h"
#include "gui/ExportSvgAnimationStrategy.h"
#include "gui/ExportVelocityAnimationStrategy.h"
#include "gui/ExportReconstructedGeometryAnimationStrategy.h"
#include "gui/ExportResolvedTopologyAnimationStrategy.h"

#include "utils/ExportTemplateFilenameSequence.h"
#include "utils/ExportAnimationStrategyFactory.h"

#include "qt-widgets/ExportAnimationDialog.h"


GPlatesGui::ExportAnimationContext::ExportAnimationContext(
		GPlatesQtWidgets::ExportAnimationDialog &export_animation_dialog_,
		GPlatesGui::AnimationController &animation_controller_,
		GPlatesPresentation::ViewState &view_state_,
		GPlatesQtWidgets::ViewportWindow &viewport_window_):
	d_export_animation_dialog_ptr(&export_animation_dialog_),
	d_animation_controller_ptr(&animation_controller_),
	d_view_state(&view_state_),
	d_viewport_window(&viewport_window_),
	d_abort_now(false),
	d_export_running(false)	

{ }

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

GPlatesPresentation::ViewState &
GPlatesGui::ExportAnimationContext::view_state()
{
	return *d_view_state;
}

GPlatesQtWidgets::ViewportWindow &
GPlatesGui::ExportAnimationContext::viewport_window()
{
	return *d_viewport_window;
}

bool
GPlatesGui::ExportAnimationContext::do_export()
{
	d_export_running = true;
	// Setting this flag to 'true' while we are exporting will cause us to abort.
	d_abort_now = false;
	
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
		ExportersMapType::iterator export_it = d_exporters_map.begin();
		ExportersMapType::iterator export_end = d_exporters_map.end();

		d_export_animation_dialog_ptr->update_single_frame_progress_bar(
					0,d_exporters_map.size());
		int count=0;
		for (; export_it != export_end; ++export_it, count++) 
		{
			ok = ok && (*export_it).second->do_export_iteration(frame_index);
			d_export_animation_dialog_ptr->update_single_frame_progress_bar(
					count, d_exporters_map.size());
		}
		
		if ( ! ok) {
			// Failed. Just quit the whole thing.
			ExportersMapType::iterator failed_it = d_exporters_map.begin();
			ExportersMapType::iterator failed_end = d_exporters_map.end();
			for (; failed_it != failed_end; ++failed_it) {
				(*failed_it).second->wrap_up(false);
			}
			d_export_running = false;
			d_abort_now = false;
			return false;
		}

		// Move the dialog's progress bar to indicate we have finished this frame number.
		d_export_animation_dialog_ptr->update_progress_bar(length, frame_number);
		d_export_animation_dialog_ptr->update_single_frame_progress_bar(
			count, d_exporters_map.size());
	}

	// All finished! Allow exporters to do some clean-up, if they need to.
	ExportersMapType::iterator done_it = d_exporters_map.begin();
	ExportersMapType::iterator done_end = d_exporters_map.end();
	for (; done_it != done_end; ++done_it) {
		(*done_it).second->wrap_up(true);
	}

	// Update dialog - successful finish.
	d_export_running = false;
	cleanup_exporters_map();
	update_status_message(QObject::tr("Export Finished."));
	return true;
}

void
GPlatesGui::ExportAnimationContext::update_status_message(
		const QString &message)
{
	d_export_animation_dialog_ptr->update_status_message(message);
}

void
GPlatesGui::ExportAnimationContext::add_exporter(
		GPlatesUtils::Exporter_ID id,
		const ExportAnimationStrategy::Configuration& cfg)
{
	ExportersMapType::iterator it =
		d_exporters_map.find(id);

	if(it!=d_exporters_map.end())
	{
		d_exporters_map.erase(it);
	}

	d_exporters_map.insert(
			std::pair<
				GPlatesUtils::Exporter_ID, 
				ExportAnimationStrategy::non_null_ptr_type>(
						id,
						GPlatesUtils::ExportAnimationStrategyFactory::create_exporter(
								id,
								*this,
								cfg)));

	return;
}




