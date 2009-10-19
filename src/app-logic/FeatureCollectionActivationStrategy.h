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
 
#ifndef GPLATES_APP_LOGIC_FEATURECOLLECTIONACTIVATIONSTRATEGY_H
#define GPLATES_APP_LOGIC_FEATURECOLLECTIONACTIVATIONSTRATEGY_H

#include <boost/noncopyable.hpp>

#include "FeatureCollectionFileState.h"
#include "FeatureCollectionFileStateDecls.h"


namespace GPlatesAppLogic
{
	namespace FeatureCollectionFileStateImpl
	{
		class ActiveStateImpl;
	}


	/**
	 * Used, by @a FeatureCollectionFileState, to determine how to activate/deactivate
	 * files in different workflows.
	 */
	class FeatureCollectionActivationStrategy :
			private boost::noncopyable
	{
	public:
		/**
		 * Allows activation strategies to change the internal active state of
		 * @a FeatureCollectionFileState without going through its external API.
		 *
		 * Since each API call to @a FeatureCollectionFileState emits a *single*
		 * 'file_state_changed' signal, by using this @a ActiveState interface we avoid
		 * generating multiple of these signals (which would cause clients of
		 * @a FeatureCollectionFileState unnecessary updates such as generating
		 * a new reconstruction).
		 *
		 * NOTE: You currently only have access to the active files of the workflow
		 * that the activation strategy was registered to. This is all that is required
		 * now (and makes the implementation easier since we only have to keep track of
		 * activation changes to files active in one workflow).
		 * If this changes in future make sure that clients of @a FeatureCollectionFileState
		 * get notified of any activation changes (via signals) and that any workflows
		 * are notified of activation changes.
		 */
		class ActiveState :
				private boost::noncopyable
		{
		public:
			ActiveState(
					FeatureCollectionFileStateImpl::ActiveStateImpl &impl) :
				d_impl(impl)
			{  }


			/**
			 * Get an iteration range over the active files for the workflow that this
			 * activation strategy was registered with.
			 */
			FeatureCollectionFileState::active_file_iterator_range
			get_active_workflow_files();


			/**
			 * Activates file @a file_iter for the workflow that this
			 * activation strategy was registered with.
			 */
			void
			set_file_active_workflow(
					FeatureCollectionFileState::file_iterator file_iter,
					bool activate = true);

		private:
			FeatureCollectionFileStateImpl::ActiveStateImpl &d_impl;
		};


		virtual
		~FeatureCollectionActivationStrategy()
		{  }


		/**
		 * Notification that file @a new_file_iter was added to the workflow that this
		 * activation strategy is associated with.
		 *
		 * Before this method is called the file is inactive.
		 *
		 * The default behaviour is to activate the added file without
		 * changing the active state of any other files.
		 */
		virtual
		void
		added_file_to_workflow(
				FeatureCollectionFileState::file_iterator new_file_iter,
				ActiveState &active_state)
		{
			// Activate the workflow with the new file.
			active_state.set_file_active_workflow(new_file_iter, true);
		}


		/**
		 * Notification that file @a file_iter is about to be removed from the workflow
		 * that this activation strategy is associated with.
		 *
		 * The default behaviour is to deactivate the removed file without
		 * changing the active state of any other files.
		 */
		virtual
		void
		removing_file_from_workflow(
				FeatureCollectionFileState::file_iterator file_iter,
				ActiveState &active_state)
		{
			// Deactivate the file with the workflow.
			active_state.set_file_active_workflow(file_iter, false);
		}


		/**
		 * Notification that file @a file_iter was changed and the workflow, that this
		 * activation strategy is associated with, decided it was *not* interested
		 * in the file anymore.
		 *
		 * The default behaviour is to deactivate the changed file without changing
		 * the active state of any other files.
		 */
		virtual
		void
		workflow_rejected_changed_file(
				FeatureCollectionFileState::file_iterator file_iter,
				ActiveState &active_state)
		{
			// Deactivate the file with the workflow.
			active_state.set_file_active_workflow(file_iter, false);
		}


		/**
		 * Notification that file @a file_iter was activated with the workflow that this
		 * activation strategy is associated with.
		 *
		 * The default behaviour is to activate the file without changing
		 * the active state of any other files.
		 */
		virtual
		void
		set_active(
				FeatureCollectionFileState::file_iterator file_iter,
				bool activate,
				ActiveState &active_state)
		{
			// Activate the file with the workflow.
			active_state.set_file_active_workflow(file_iter, activate);
		}
	};
}

#endif // GPLATES_APP_LOGIC_FEATURECOLLECTIONACTIVATIONSTRATEGY_H
