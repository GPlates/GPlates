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

#include "ReconstructionActivationStrategy.h"


void
GPlatesAppLogic::ReconstructionActivationStrategy::added_file_to_workflow(
		FeatureCollectionFileState::file_iterator new_file_iter,
		ActiveState &active_state)
{
	// Seactivate all other files in the workflow first.
	deactivate_all_files_in_workflow(active_state);

	// Activate the new reconstruction file.
	active_state.set_file_active_workflow(new_file_iter, true);
}


void
GPlatesAppLogic::ReconstructionActivationStrategy::set_active(
		FeatureCollectionFileState::file_iterator file_iter,
		bool activate,
		ActiveState &active_state)
{
	// If we're activating a file then deactivate all other files
	// in the workflow first.
	if (activate)
	{
		deactivate_all_files_in_workflow(active_state);
	}

	active_state.set_file_active_workflow(file_iter, activate);
}


void
GPlatesAppLogic::ReconstructionActivationStrategy::deactivate_all_files_in_workflow(
		ActiveState &active_state)
{
	// Get the list of all active reconstruction files.
	FeatureCollectionFileState::active_file_iterator_range active_file_iter_range =
			active_state.get_active_workflow_files();

	// Deactivate them all.
	GPlatesAppLogic::FeatureCollectionFileState::active_file_iterator active_file_iter =
			active_file_iter_range.begin;
	for ( ; active_file_iter != active_file_iter_range.end; ++active_file_iter)
	{
		// Convert to an iterator we can use in the ActiveState interface.
		GPlatesAppLogic::FeatureCollectionFileState::file_iterator file_iter =
				GPlatesAppLogic::FeatureCollectionFileState::convert_to_file_iterator(active_file_iter);

		// Deactivate reconstruction file.
		active_state.set_file_active_workflow(file_iter, false);
	}
}
