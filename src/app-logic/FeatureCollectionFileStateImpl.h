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
 
#ifndef GPLATES_APP_LOGIC_FEATURECOLLECTIONFILESTATEIMPL_H
#define GPLATES_APP_LOGIC_FEATURECOLLECTIONFILESTATEIMPL_H

#include <list>
#include <map>
#include <vector>

#include "FeatureCollectionFileStateDecls.h"
#include "FeatureCollectionFileStateImplDecls.h"


namespace GPlatesFileIO
{
	class File;
}

namespace GPlatesAppLogic
{
	class FeatureCollectionActivationStrategy;
	class FeatureCollectionWorkflow;


	namespace FeatureCollectionFileStateImpl
	{
		/**
		 * Keeps track of files whose activation with workflows has changed.
		 */
		class ActivationStateManager
		{
		public:
			/**
			 * Typedef for unique list of file iterators whose activation changed for
			 * the same workflow.
			 */
			typedef std::vector<file_iterator_type> changed_activation_sorted_unique_files_type;

			//! Typedef for type that associates changed activation files with a workflow.
			struct WorkflowFiles
			{
				WorkflowFiles(
						const FeatureCollectionFileStateDecls::workflow_tag_type &workflow_tag_) :
					workflow_tag(workflow_tag_)
				{  }

				FeatureCollectionFileStateDecls::workflow_tag_type workflow_tag;
				changed_activation_sorted_unique_files_type changed_activation_files;
			};

			//! Typedef for a sequence of workflows with changed activation files.
			typedef std::list<WorkflowFiles> changed_activation_workflows_type;


			ActivationStateManager();


			/**
			 * Sets @a file_iter active with the workflow identified by @a workflow_tag.
			 *
			 * Also keeps track of the change internally so it can be retrieved when
			 * calling @a get_changed_activation_workflow_files.
			 */
			void
			set_file_active_workflow(
					file_iterator_type file_iter,
					const FeatureCollectionFileStateDecls::workflow_tag_type &workflow_tag,
					bool activate = true);


			/**
			 * Returns the files whose activation state changed - for all workflows that
			 * had changing files.
			 *
			 * Only files (and workflows) whose final activation state differs from their
			 * initial activation state (upon first call to @a set_file_active_workflow)
			 * will be present in the returned sequence.
			 */
			const changed_activation_workflows_type &
			get_changed_activation_workflow_files() const;

		private:
			//! Keeps track of activation changes to a file.
			struct ActiveInfo
			{
				ActiveInfo(
						file_iterator_type file_iter_,
						bool initial_activation_,
						bool final_activation_) :
					file_iter(file_iter_),
					initial_activation(initial_activation_),
					final_activation(final_activation_)
				{  }

				file_iterator_type file_iter;
				bool initial_activation;
				bool final_activation;
			};

			//! Typedef for unique list of file iterators that were activated.
			typedef std::vector<ActiveInfo> activated_file_seq_type;

			//! Typedef for a mapping of workflow tags to activated file sequences.
			typedef std::map<workflow_tag_type, activated_file_seq_type>
					workflow_activated_files_map_type;

			workflow_activated_files_map_type d_workflow_activated_files_map;

			//! Cached results for @a get_changed_activation_workflow_files. 
			mutable changed_activation_workflows_type d_changed_activation_workflows;
			mutable bool d_is_changed_activation_workflows_dirty;


			//! Builds and caches the list of files whose activation state has actually changed.
			void
			build_changed_activation_workflow_files() const;
		};


		class ActiveListsManager;

		/**
		 * The implementation of @a FeatureCollectionActivationStrategy::ActiveState that
		 * delegates to @a ActivationStateManager and @a ActiveListsManager.
		 */
		class ActiveStateImpl :
				private boost::noncopyable
		{
		public:
			//! Typedef for unique list of file iterators whose activation changed.
			typedef std::vector<file_iterator_type> changed_activation_sorted_unique_files_type;


			ActiveStateImpl(
					ActivationStateManager &activation_state_manager,
					ActiveListsManager &active_lists_manager,
					const FeatureCollectionFileStateDecls::workflow_tag_type &workflow_tag) :
				d_activation_state_manager(activation_state_manager),
				d_active_lists_manager(active_lists_manager),
				d_workflow_tag(workflow_tag)
			{  }

			//! Returns the iteration range of active files for the workflow specified in constructor.
			active_file_iterator_range_type
			get_active_workflow_files();

			//! Activates the file for the workflow specified in constructor.
			void
			set_file_active_workflow(
					file_iterator_type file_iter,
					bool activate = true);

		private:
			ActivationStateManager &d_activation_state_manager;
			ActiveListsManager &d_active_lists_manager;
			FeatureCollectionFileStateDecls::workflow_tag_type d_workflow_tag;
		};


		/**
		 * Synchronises the main list of all loaded files with active lists that point into
		 * the main list - mainly to make sure all dependent active lists remove elements when
		 * an element from the main list is removed.
		 */
		class ActiveListsManager
		{
		public:
			/**
			 * Registers @a workflow.
			 * Multiple workflow instances having the same tag are *not* allowed.
			 */
			void
			register_workflow(
					FeatureCollectionWorkflow *workflow);

			/**
			 * Unregisters @a workflow.
			 */
			void
			unregister_workflow(
					FeatureCollectionWorkflow *workflow);

			active_file_iterator_range_type
			get_active_files(
					const workflow_tag_type &tag);

			void
			update_active_lists(
					const ActivationStateManager &activation_state_manager);

		private:
			//! Typedef to map tags to active state lists.
			typedef std::map<workflow_tag_type, file_iterator_seq_impl_type>
					active_state_lists_type;

			//! A map of tags to all active state lists.
			active_state_lists_type d_active_state_lists;


			static
			void
			add_to(
					file_iterator_seq_impl_type &active_state_list,
					const file_seq_iterator_impl_type &file_iter);

			static
			void
			remove_from(
					file_iterator_seq_impl_type &active_state_list,
					const file_seq_iterator_impl_type &file_iter);
		};


		/**
		 * Manages workflows and notifying them.
		 */
		class WorkflowManager
		{
		public:
			/**
			 * Registers @a workflow and the activation strategy attached to it.
			 * Multiple workflow instances having the same tag are *not* allowed.
			 */
			void
			register_workflow(
					FeatureCollectionWorkflow *workflow,
					FeatureCollectionActivationStrategy *activation_strategy);

			/**
			 * Unregisters @a workflow.
			 */
			void
			unregister_workflow(
					FeatureCollectionWorkflow *workflow);

			/**
			 * Sets the activation strategy to be used when working with the workflow
			 * identified by @a workflow_tag.
			 */
			void
			set_activation_strategy(
					FeatureCollectionActivationStrategy *activation_strategy,
					const FeatureCollectionFileStateDecls::workflow_tag_type &workflow_tag);

			/**
			 * Adds @a file_iter to all registered workflows.
			 *
			 * This includes the pseudo workflow of reconstructing the reconstructable features
			 * in the file (if it has any) - it will only register interest if no other workflows do.
			 * 
			 * All interested workflows are attached to the file and set as active for the file.
			 * Note that multiple workflows can show interest in the same file.
			 */
			void
			add_file(
					file_iterator_type file_iter,
					ActiveListsManager &active_lists_manager,
					ActivationStateManager &activation_state_manager);

			/**
			 * Notifies workflows attached to @a file_iter that the file is being removed.
			 * Also detaches all workflows from the file.
			 */
			void
			remove_file(
					file_iterator_type file_iter,
					ActiveListsManager &active_lists_manager,
					ActivationStateManager &activation_state_manager);

			/**
			 * Notifies all workflows currently interested in @a file_iter that the file
			 * has been changed.
			 *
			 * Also since the file is effectively a new file it also asks the other workflows
			 * (that are not currently interested in @a file_iter) if they are now interested.
			 *
			 * All interested workflows are attached to the file and set as active for the file.
			 * Note that multiple workflows can show interest in the same file.
			 */
			void
			changed_file(
					file_iterator_type file_iter,
					GPlatesFileIO::File &old_file,
					ActiveListsManager &active_lists_manager,
					ActivationStateManager &activation_state_manager);

			/**
			 * Notifies the workflow identified by @a workflow_tag that the file @a file_iter
			 * has been activated/deactivated.
			 * 
			 */
			void
			set_active(
					file_iterator_type file_iter,
					const FeatureCollectionFileStateDecls::workflow_tag_type &workflow_tag,
					bool activate,
					ActiveListsManager &active_lists_manager,
					ActivationStateManager &activation_state_manager);

		private:
			//! Simply groups a workflow with the activation strategy assigned to it.
			struct WorkflowInfo
			{
				WorkflowInfo(
						FeatureCollectionWorkflow *workflow_,
						FeatureCollectionActivationStrategy *activation_strategy_) :
					workflow(workflow_),
					activation_strategy(activation_strategy_)
				{  }

				FeatureCollectionWorkflow *workflow;
				FeatureCollectionActivationStrategy *activation_strategy;
			};

			//! Typedef for a mapping of workflow tags to @a Workflow pointers.
			typedef std::map<FeatureCollectionFileStateDecls::workflow_tag_type,
					WorkflowInfo> workflow_map_type;

			//! Typedef for a sequence workflows sorted by priority.
			typedef std::vector<WorkflowInfo *> sorted_workflow_seq_type;

			//! Typedef for a sequence of workflow tags.
			typedef std::vector<FeatureCollectionFileStateDecls::workflow_tag_type>	
					workflow_tag_seq_type;

			//! Used to keep track of our registered workflows.
			workflow_map_type d_workflow_map;

			//! Used to keep track of workflows sorted by their priorities.
			sorted_workflow_seq_type d_sorted_workflow_seq;


			//! Notifies workflows of any activation changes recorded by @a activation_state_manager.
			void
			notify_workflows_of_activation_changes(
					ActivationStateManager &activation_state_manager);


			//! Returns the workflow matching the tag @a workflow_tag.
			WorkflowInfo &
			get_workflow_info(
					const FeatureCollectionFileStateDecls::workflow_tag_type &workflow_tag);
		};
	}
}

#endif // GPLATES_APP_LOGIC_FEATURECOLLECTIONFILESTATEIMPL_H
