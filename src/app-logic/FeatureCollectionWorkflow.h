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
 
#ifndef GPLATES_APP_LOGIC_FEATURECOLLECTIONWORKFLOW_H
#define GPLATES_APP_LOGIC_FEATURECOLLECTIONWORKFLOW_H

#include <boost/bind.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

#include "ClassifyFeatureCollection.h"
#include "FeatureCollectionFileState.h"
#include "FeatureCollectionFileStateDecls.h"


namespace GPlatesAppLogic
{
	/**
	 * Used to be notified of file loads, file unloads, file changes and active status
	 * changes to a file.
	 *
	 * This is a more direct approach that using the signals emitted by
	 * @a FeatureCollectionFileState and allows the workflow to return values to the caller.
	 */
	class FeatureCollectionWorkflow :
			private boost::noncopyable
	{
	public:
		//! Typedef for the tag type used to identify specific workflow instances.
		typedef FeatureCollectionFileStateDecls::workflow_tag_type tag_type;

		/**
		 * Typedef for priority of a workflow.
		 *
		 * NOTE: Only use positive values (and zero) for your priorities as negative values
		 * are reserved for some internal workflows to allow them to get even
		 * lower priorities than the client workflows.
		 */
		typedef FeatureCollectionFileStateDecls::workflow_priority_type priority_type;

		/**
		 * Some convenient values for priority.
		 * You can use any between @a PRIORITY_LOWEST and @a PRIORITY_HIGHEST inclusive.
		 */
		enum PriorityValues
		{
			// Reconstruction has a lower priority than reconstructable so that
			// a file containing both types of features doesn't get consumed by
			// the reconstruction workflow (the reconstructable workflow won't add
			// a file if another workflow has already added it).
			PRIORITY_RECONSTRUCTION = -2,  // Reserved for internal use
			PRIORITY_RECONSTRUCTABLE = -1, // Reserved for internal use

			PRIORITY_LOWEST = 0,
			PRIORITY_NORMAL = boost::integer_traits<priority_type>::const_max / 2,
			PRIORITY_HIGHEST = boost::integer_traits<priority_type>::const_max
		};


		/**
		 * Registers @a workflow with @a file_state and returns a shared pointer
		 * that will unregister @a workflow when reference count becomes zero.
		 *
		 * NOTE: this does *not* manage memory - use an extra smart pointer for that
		 * (one to manage memory and this one to unregister).
		 * NOTE: don't share this pointer otherwise it might try to unregister
		 * after the registered workflow has been destroyed.
		 */
		static
		boost::shared_ptr<FeatureCollectionWorkflow>
		register_and_create_auto_unregister_handle(
				FeatureCollectionWorkflow *workflow,
				FeatureCollectionFileState &file_state)
		{
			workflow->register_workflow(file_state);

			return boost::shared_ptr<FeatureCollectionWorkflow>(
					workflow,
					boost::bind(&FeatureCollectionWorkflow::unregister_workflow, _1));
		}


		virtual
		~FeatureCollectionWorkflow()
		{  }

		/**
		 * Returns the tag you want to use to identify this workflow instance.
		 *
		 * A good tag to use might be the name of the derived class followed by "-tag"
		 * so that it's recognisable in the debugger. Also the tag might be visible
		 * in the GUI (used to activate workflows) so it shouldn't be too verbose.
		 */
		virtual
		tag_type
		get_tag() const = 0;

		/**
		 * Returns the priority of this workflow.
		 *
		 * More than one workflow can have the same priority however their order
		 * relative to each other is implementation defined.
		 */
		virtual
		priority_type
		get_priority() const = 0;

		/**
		 * Adds a new file.
		 *
		 * Return true if you are interested in the new file @a file_iter and have added
		 * it internally (in your derived class).
		 *
		 * The feature collection classification is passed in @a classification.
		 * Boolean @a used_by_higher_priority_workflow is true if a higher priority
		 * workflow is currently using the file.
		 */
		virtual
		bool
		add_file(
				FeatureCollectionFileState::file_iterator file_iter,
				const ClassifyFeatureCollection::classifications_type &classification,
				bool used_by_higher_priority_workflow) = 0;

		/**
		 * File @a file is about to be removed from the file state.
		 *
		 * This is only called if @a add_file returned true for @a file_iter.
		 */
		virtual
		void
		remove_file(
				FeatureCollectionFileState::file_iterator file_iter) = 0;

		/**
		 * File @a file_iter has just been changed.
		 *
		 * Return true if you are still interested in the new file referenced by @a file_iter.
		 * If false is returned then this workflow will no longer receive callbacks.
		 * The files active status is unchanged.
		 *
		 * This is only called if @a add_file returned true for @a file_iter.
		 *
		 * The new feature collection classification is passed in @a new_classification.
		 * The old file is passed in @a old_file.
		 * The file iterator @a file_iter is still the same (only the file it points to
		 * has changed) so you can use it as an id handle and use equality comparison
		 * to find any data you may have associated with it.
		 */
		virtual
		bool
		changed_file(
				FeatureCollectionFileState::file_iterator file_iter,
				GPlatesFileIO::File &old_file,
				const ClassifyFeatureCollection::classifications_type &new_classification) = 0;

		/**
		 * Activates or deactivates @a file_iter for this workflow only.
		 *
		 * This is only called if @a add_file returned true for @a file_iter.
		 */
		virtual
		void
		set_file_active(
				FeatureCollectionFileState::file_iterator file_iter,
				bool active = true) = 0;

	protected:
		//! Constructor does not register_workflow - derived class must call @a register_workflow explicitly.
		FeatureCollectionWorkflow();

		//! Registers 'this' workflow with @a file_state.
		void
		register_workflow(
				FeatureCollectionFileState &file_state);

		//! Unregisters 'this' workflow (if registered).
		void
		unregister_workflow();

	private:
		FeatureCollectionFileState *d_file_state;
	};
}

#endif // GPLATES_APP_LOGIC_FEATURECOLLECTIONWORKFLOW_H
