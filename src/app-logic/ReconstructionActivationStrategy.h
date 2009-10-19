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
 
#ifndef GPLATES_APP_LOGIC_RECONSTRUCTIONACTIVATIONSTRATEGY_H
#define GPLATES_APP_LOGIC_RECONSTRUCTIONACTIVATIONSTRATEGY_H

#include "FeatureCollectionActivationStrategy.h"


namespace GPlatesAppLogic
{
	/**
	 * This strategy for activating reconstruction feature collections first
	 * deactivates all other reconstruction files before activating the newly
	 * added reconstruction file.
	 *
	 * This ensures only one reconstruction file is active at a time.
	 * This could be changed later if groups of reconstruction files are supported
	 * where multiple reconstruction files can form a group which gets activated/deactivated
	 * as a unit (eg, all other groups get deactivated leaving only one group active at any
	 * one time).
	 */
	class ReconstructionActivationStrategy :
			public FeatureCollectionActivationStrategy
	{
	public:


		/**
		 * Notification that file @a new_file_iter was added to the workflow that this
		 * activation strategy is associated with.
		 */
		virtual
		void
		added_file_to_workflow(
				FeatureCollectionFileState::file_iterator new_file_iter,
				ActiveState &active_state);


		/**
		 * Notification that file @a file_iter was activated with the workflow that this
		 * activation strategy is associated with.
		 */
		virtual
		void
		set_active(
				FeatureCollectionFileState::file_iterator file_iter,
				bool activate,
				ActiveState &active_state);

	private:
		void
		deactivate_all_files_in_workflow(
				ActiveState &active_state);
	};
}

#endif // GPLATES_APP_LOGIC_RECONSTRUCTIONACTIVATIONSTRATEGY_H
