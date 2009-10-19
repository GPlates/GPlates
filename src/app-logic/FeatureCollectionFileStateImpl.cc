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

#include <algorithm>
#include <iterator>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>

#include "FeatureCollectionFileStateImpl.h"

#include "FeatureCollectionActivationStrategy.h"
#include "FeatureCollectionWorkflow.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"


namespace Decls = GPlatesAppLogic::FeatureCollectionFileStateDecls;


void
GPlatesAppLogic::FeatureCollectionFileStateImpl::FileNodeActiveState::clear_tags()
{
	d_active_map.clear();
}


void
GPlatesAppLogic::FeatureCollectionFileStateImpl::FileNodeActiveState::add_tag(
		const workflow_tag_type &tag,
		bool active)
{
	const std::pair<active_map_type::iterator, bool> inserted = d_active_map.insert(
			active_map_type::value_type(tag, active));

	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			inserted.second,
			GPLATES_ASSERTION_SOURCE);
}


void
GPlatesAppLogic::FeatureCollectionFileStateImpl::FileNodeActiveState::remove_tag(
		const workflow_tag_type &tag)
{
	active_map_type::iterator iter = d_active_map.find(tag);
					
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			iter != d_active_map.end(),
			GPLATES_ASSERTION_SOURCE);

	d_active_map.erase(iter);
}


bool
GPlatesAppLogic::FeatureCollectionFileStateImpl::FileNodeActiveState::does_tag_exist(
		const workflow_tag_type &tag) const
{
	return d_active_map.find(tag) != d_active_map.end();
}


bool
GPlatesAppLogic::FeatureCollectionFileStateImpl::FileNodeActiveState::get_active(
		const workflow_tag_type &tag) const
{
	active_map_type::const_iterator iter = d_active_map.find(tag);
	
	return iter != d_active_map.end() && iter->second;
}


void
GPlatesAppLogic::FeatureCollectionFileStateImpl::FileNodeActiveState::set_active(
		const workflow_tag_type &tag,
		bool active)
{
	active_map_type::iterator iter = d_active_map.find(tag);
	
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			iter != d_active_map.end(),
			GPLATES_ASSERTION_SOURCE);

	iter->second = active;
}


std::vector<GPlatesAppLogic::FeatureCollectionFileStateImpl::workflow_tag_type>
GPlatesAppLogic::FeatureCollectionFileStateImpl::FileNodeActiveState::get_tags() const
{
	std::vector<workflow_tag_type> tags;

	for (active_map_type::const_iterator iter = d_active_map.begin();
		iter != d_active_map.end();
		++iter)
	{
		const workflow_tag_type &tag = iter->first;
		tags.push_back(tag);
	}

	return tags;
}


void
GPlatesAppLogic::FeatureCollectionFileStateImpl::ActiveListsManager::register_workflow(
		FeatureCollectionWorkflow *workflow)
{
	// Insert workflow into our active list set.
	std::pair<active_state_lists_type::iterator, bool> inserted = d_active_state_lists.insert(
			active_state_lists_type::value_type(
					workflow->get_tag(),
					file_iterator_seq_impl_type()));

	// NOTE: if a workflow instance has already been registered with the same tag then
	// an assertion is raised.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			inserted.second,
			GPLATES_ASSERTION_SOURCE);
}


void
GPlatesAppLogic::FeatureCollectionFileStateImpl::ActiveListsManager::unregister_workflow(
		FeatureCollectionWorkflow *workflow)
{
	// Remove 'workflow' from our active list set.
	const active_state_lists_type::iterator active_lists_iter  =
			d_active_state_lists.find(workflow->get_tag());
	if (active_lists_iter != d_active_state_lists.end())
	{
		d_active_state_lists.erase(active_lists_iter);
	}
}


GPlatesAppLogic::FeatureCollectionFileStateImpl::active_file_iterator_range_type
GPlatesAppLogic::FeatureCollectionFileStateImpl::ActiveListsManager::get_active_files(
		const workflow_tag_type &tag)
{
	return active_file_iterator_range_type(
			active_file_iterator_type::create(d_active_state_lists[tag].begin()),
			active_file_iterator_type::create(d_active_state_lists[tag].end()));
}


void
GPlatesAppLogic::FeatureCollectionFileStateImpl::ActiveListsManager::update_active_lists(
		const ActivationStateManager &activation_state_manager)
{
	//
	// Here we iterate through the list of files (associated with workflows) and
	// add or remove from active lists.
	// The list of file/workflows is only those whose activation state has actually
	// changed (eg, from inactive to active or vice versa).
	//

	// Get all files whose activation state has actually changed since
	// 'activation_state_manager' was created.
	const ActivationStateManager::changed_activation_workflows_type changed_activation_files =
			activation_state_manager.get_changed_activation_workflow_files();

	// Iterate through the workflow groups.
	ActivationStateManager::changed_activation_workflows_type::const_iterator
			changed_activation_iter = changed_activation_files.begin();
	ActivationStateManager::changed_activation_workflows_type::const_iterator
			changed_activation_end = changed_activation_files.end();
	for ( ; changed_activation_iter != changed_activation_end; ++changed_activation_iter)
	{
		const ActivationStateManager::WorkflowFiles &workflow_files =
				*changed_activation_iter;

		const workflow_tag_type &workflow_tag = workflow_files.workflow_tag;

		// Iterate through the files of the current workflow.
		ActivationStateManager::changed_activation_sorted_unique_files_type::const_iterator
				workflow_files_iter = workflow_files.changed_activation_files.begin();
		ActivationStateManager::changed_activation_sorted_unique_files_type::const_iterator
				workflow_files_end = workflow_files.changed_activation_files.end();
		for ( ; workflow_files_iter != workflow_files_end; ++workflow_files_iter)
		{
			const file_iterator_type file_iter = *workflow_files_iter;

			// Add to active list if file is active for the current workflow.
			if (file_iter.get_file_node()->file_node_state().active_state().get_active(workflow_tag))
			{
				add_to(d_active_state_lists[workflow_tag], file_iter.get_iterator_impl());
			}
			else
			{
				remove_from(d_active_state_lists[workflow_tag], file_iter.get_iterator_impl());
			}
		}
	}
}


void
GPlatesAppLogic::FeatureCollectionFileStateImpl::ActiveListsManager::add_to(
		file_iterator_seq_impl_type &active_state_list,
		const file_seq_iterator_impl_type &file_iter)
{
	// See if it's already in the list.
	file_iterator_seq_iterator_impl_type active_state_list_iter = std::find(
			active_state_list.begin(),
			active_state_list.end(),
			file_iter);

	// Only add if it wasn't already in the list.
	if (active_state_list_iter == active_state_list.end())
	{
		active_state_list.push_back(file_iter);
	}
}


void
GPlatesAppLogic::FeatureCollectionFileStateImpl::ActiveListsManager::remove_from(
		file_iterator_seq_impl_type &active_state_list,
		const file_seq_iterator_impl_type &file_iter)
{
	// See if it's in the list.
	file_iterator_seq_iterator_impl_type active_state_list_iter = std::find(
			active_state_list.begin(),
			active_state_list.end(),
			file_iter);

	// Remove from list if it's in the list.
	if (active_state_list_iter != active_state_list.end())
	{
		active_state_list.erase(active_state_list_iter);
	}
}


GPlatesAppLogic::FeatureCollectionFileStateImpl::active_file_iterator_range_type
GPlatesAppLogic::FeatureCollectionFileStateImpl::ActiveStateImpl::get_active_workflow_files()
{
	return d_active_lists_manager.get_active_files(d_workflow_tag);
}


void
GPlatesAppLogic::FeatureCollectionFileStateImpl::ActiveStateImpl::set_file_active_workflow(
		file_iterator_type file_iter,
		bool activate)
{
	// Simply delegate to the activation state manager.
	d_activation_state_manager.set_file_active_workflow(file_iter, d_workflow_tag, activate);
}


GPlatesAppLogic::FeatureCollectionFileStateImpl::ActivationStateManager::ActivationStateManager() :
	d_is_changed_activation_workflows_dirty(false)
{
}


void
GPlatesAppLogic::FeatureCollectionFileStateImpl::ActivationStateManager::set_file_active_workflow(
		file_iterator_type file_iter,
		const FeatureCollectionFileStateDecls::workflow_tag_type &workflow_tag,
		bool activate)
{
	// Mark our cached changed activation files as needing a rebuild.
	d_is_changed_activation_workflows_dirty = true;

	FileNodeActiveState &active_state =
			file_iter.get_file_node()->file_node_state().active_state();

	activated_file_seq_type &activated_files = d_workflow_activated_files_map[workflow_tag];

	using boost::lambda::_1;

	// Keep track of which files were changed - keep list unique.
	activated_file_seq_type::iterator activated_files_iter = std::find_if(
			activated_files.begin(),
			activated_files.end(),
			boost::lambda::bind(&ActiveInfo::file_iter, _1) == file_iter);

	// If 'file_iter' was not added before then add now.
	if (activated_files_iter == activated_files.end())
	{
		const ActiveInfo active_info(
				file_iter,
				active_state.get_active(workflow_tag)/*initial_activation*/,
				activate/*final_activation*/);
		activated_files_iter = activated_files.insert(activated_files.end(), active_info);
	}

	// Record activation so we can return results to the client.
	activated_files_iter->final_activation = activate;


	// And make the activation change directly through the file iterator.
	// Note that we don't add a tag if it doesn't exist.
	// It shouldn't happen but I'm not sure I want to assert it.
	if (active_state.does_tag_exist(workflow_tag))
	{
		active_state.set_active(workflow_tag, activate);
	}
}


const GPlatesAppLogic::FeatureCollectionFileStateImpl::ActivationStateManager::changed_activation_workflows_type &
GPlatesAppLogic::FeatureCollectionFileStateImpl::ActivationStateManager::get_changed_activation_workflow_files() const
{
	if (d_is_changed_activation_workflows_dirty)
	{
		build_changed_activation_workflow_files();

		d_is_changed_activation_workflows_dirty = false;
	}

	// The result to return to the caller.
	return d_changed_activation_workflows;
}


void
GPlatesAppLogic::FeatureCollectionFileStateImpl::ActivationStateManager::
		build_changed_activation_workflow_files() const
{
	// Clears any cached results.
	d_changed_activation_workflows.clear();

	// Iterate over all workflows.
	workflow_activated_files_map_type::const_iterator workflow_activated_files_iter =
			d_workflow_activated_files_map.begin();
	const workflow_activated_files_map_type::const_iterator workflow_activated_files_end =
			d_workflow_activated_files_map.end();
	for ( ; workflow_activated_files_iter != workflow_activated_files_end; ++workflow_activated_files_iter)
	{
		const workflow_tag_type &workflow_tag = workflow_activated_files_iter->first;
		const activated_file_seq_type &activated_files = workflow_activated_files_iter->second;

		changed_activation_sorted_unique_files_type changed_activation_unique_files;

		// Iterate through our activated files and return to the caller only those
		// whose final activation differs from their initial activation.
		activated_file_seq_type::const_iterator activated_files_iter = activated_files.begin();
		const activated_file_seq_type::const_iterator activated_files_end = activated_files.end();
		for ( ; activated_files_iter != activated_files_end; ++activated_files_iter)
		{
			const ActiveInfo &active_info = *activated_files_iter;

			if (active_info.final_activation != active_info.initial_activation)
			{
				changed_activation_unique_files.push_back(active_info.file_iter);
			}
		}

		// Add a workflow entry if there were any files in it that changed activation state.
		if (!changed_activation_unique_files.empty())
		{
			d_changed_activation_workflows.push_back(WorkflowFiles(workflow_tag));
			d_changed_activation_workflows.back().changed_activation_files.swap(
					changed_activation_unique_files);
		}
	}
}


namespace GPlatesAppLogic
{
	namespace FeatureCollectionFileStateImpl
	{
		namespace
		{
			/**
			 * Get the activation strategy to change the active state of file(s).
			 *
			 * NOTE: this is placed in the anonymous namespace to avoid declaring it in
			 * the header file - because it uses a nested class
			 * FeatureCollectionActivationStrategy::ActiveState which requires including
			 * the header "FeatureCollectionActivationStrategy.h" which includes
			 * the header "FeatureCollectionFileState.h" which we're trying to avoid
			 * since we are the implementation of the latter and this gives us a cyclic
			 * dependency. We could just not make it a nested class and then provide
			 * a forward declaration in the header but the nested class fits nicely
			 * with the idea that only @a FeatureCollectionActivationStrategy uses
			 * @a FeatureCollectionActivationStrategy::ActiveState.
			 *
			 * @param activation_strategy_function a function taking one argument (an instance
			 * of @a FeatureCollectionActivationStrategy::ActiveState) that calls a particular
			 * method on an activation strategy.
			 */
			void
			process_activation_strategy(
					boost::function<void (FeatureCollectionActivationStrategy::ActiveState &)>
							activation_strategy_function,
					const workflow_tag_type &workflow_tag,
					ActiveListsManager &active_lists_manager,
					ActivationStateManager &activation_state_manager)
			{
				// Interface for activation strategies to manipulate the active state.
				// Activation strategy will only have access to the active files
				// of the current workflow.
				ActiveStateImpl active_state_impl(
						activation_state_manager, active_lists_manager, workflow_tag);
				FeatureCollectionActivationStrategy::ActiveState active_state(active_state_impl);

				// Get activation strategy attached to current workflow to determine
				// what gets activated.
				activation_strategy_function(active_state);
			}
		}
	}
}

void
GPlatesAppLogic::FeatureCollectionFileStateImpl::WorkflowManager::register_workflow(
		FeatureCollectionWorkflow *workflow,
		FeatureCollectionActivationStrategy *activation_strategy)
{
	// Insert workflow into our registered list.
	std::pair<workflow_map_type::iterator, bool> inserted_map = d_workflow_map.insert(
			workflow_map_type::value_type(
					workflow->get_tag(),
					WorkflowInfo(workflow, activation_strategy)));

	// Point to the inserted element.
	WorkflowInfo *workflow_info = &inserted_map.first->second;

	// NOTE: if a workflow instance has already been registered with the same tag then
	// an assertion is raised.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			inserted_map.second,
			GPLATES_ASSERTION_SOURCE);

	// Sort the workflows according to their priority so when we add a file to a workflow
	// we can tell it if any higher priority workflows are using it.
	const sorted_workflow_seq_type::iterator insert_iter =
			d_sorted_workflow_seq.insert(d_sorted_workflow_seq.end(), workflow_info);

	// Sort the newly added workflow into the existing ones according to priority.
	std::inplace_merge(
			d_sorted_workflow_seq.begin(),
			insert_iter,
			d_sorted_workflow_seq.end(),
			boost::bind(std::less<Decls::workflow_priority_type>(),
					boost::bind(&FeatureCollectionWorkflow::get_priority,
							boost::bind(&WorkflowInfo::workflow, _1)),
					boost::bind(&FeatureCollectionWorkflow::get_priority,
							boost::bind(&WorkflowInfo::workflow, _2))));
}


void
GPlatesAppLogic::FeatureCollectionFileStateImpl::WorkflowManager::unregister_workflow(
		FeatureCollectionWorkflow *workflow)
{
	// Remove 'workflow' from our map.
	workflow_map_type::iterator map_iter  = d_workflow_map.find(workflow->get_tag());
	if (map_iter != d_workflow_map.end())
	{
		d_workflow_map.erase(map_iter);
	}

	using boost::lambda::_1;

	// Remove 'workflow' from our priority sorted sequence.
	sorted_workflow_seq_type::iterator sorted_iter = std::find_if(
			d_sorted_workflow_seq.begin(),
			d_sorted_workflow_seq.end(),
			boost::lambda::bind(&WorkflowInfo::workflow, _1) == workflow);
	if (sorted_iter != d_sorted_workflow_seq.end())
	{
		d_sorted_workflow_seq.erase(sorted_iter);
	}
}


void
GPlatesAppLogic::FeatureCollectionFileStateImpl::WorkflowManager::set_activation_strategy(
		FeatureCollectionActivationStrategy *activation_strategy,
		const workflow_tag_type &workflow_tag)
{
	workflow_map_type::iterator map_iter  = d_workflow_map.find(workflow_tag);
	if (map_iter != d_workflow_map.end())
	{
		WorkflowInfo &workflow_info = map_iter->second;

		workflow_info.activation_strategy = activation_strategy;
	}
}


void
GPlatesAppLogic::FeatureCollectionFileStateImpl::WorkflowManager::add_file(
		file_iterator_type file_iter,
		ActiveListsManager &active_lists_manager,
		ActivationStateManager &activation_state_manager)
{
	FileNodeActiveState &file_active_state =
			file_iter.get_file_node()->file_node_state().active_state();

	const ClassifyFeatureCollection::classifications_type file_classification =
			file_iter.get_file_node()->file_node_state().feature_collection_classification();

	bool used_by_higher_priority_workflow = false;

	// Iterate over all registered workflows from highest priority to lowest.
	sorted_workflow_seq_type::const_reverse_iterator workflow_iter = d_sorted_workflow_seq.rbegin();
	const sorted_workflow_seq_type::const_reverse_iterator workflow_end = d_sorted_workflow_seq.rend();
	for ( ; workflow_iter != workflow_end; ++workflow_iter)
	{
		const WorkflowInfo *workflow_info = *workflow_iter;
		FeatureCollectionWorkflow *workflow = workflow_info->workflow;

		if (workflow->add_file(file_iter, file_classification, used_by_higher_priority_workflow))
		{
			used_by_higher_priority_workflow = true;

			// Get the workflow tag.
			const workflow_tag_type workflow_tag = workflow->get_tag();

			// Add the current workflow tag to the file - set it as initially
			// inactive because the activation strategy will determine if it wants
			// to activate it or not.
			file_active_state.add_tag(workflow_tag, false);

			// Get activation strategy to activate file(s) in response.
			process_activation_strategy(
					boost::bind(&FeatureCollectionActivationStrategy::added_file_to_workflow,
							workflow_info->activation_strategy, file_iter, _1),
					workflow_tag,
					active_lists_manager,
					activation_state_manager);
		}
	}

	// Get the active lists manager to update its internal active lists in response
	// to the activation changes.
	active_lists_manager.update_active_lists(activation_state_manager);

	// Notify workflows of any activation changes.
	notify_workflows_of_activation_changes(activation_state_manager);
}


void
GPlatesAppLogic::FeatureCollectionFileStateImpl::WorkflowManager::remove_file(
		file_iterator_type file_iter,
		ActiveListsManager &active_lists_manager,
		ActivationStateManager &activation_state_manager)
{
	FileNodeActiveState &file_active_state =
			file_iter.get_file_node()->file_node_state().active_state();

	// Get the workflow tags attached to the file.
	const std::vector<workflow_tag_type> workflow_tags = file_active_state.get_tags();

	// Iterate through the workflow tags and tell the workflow's activation strategies
	// that the file is about to be removed.
	std::vector<workflow_tag_type>::const_iterator workflow_tags_iter = workflow_tags.begin();
	const std::vector<workflow_tag_type>::const_iterator workflow_tags_end = workflow_tags.end();
	for ( ; workflow_tags_iter != workflow_tags_end; ++workflow_tags_iter)
	{
		const workflow_tag_type &workflow_tag = *workflow_tags_iter;

		WorkflowInfo &workflow_info = get_workflow_info(workflow_tag);

		// Get activation strategy to activate file(s) in response to file removal.
		process_activation_strategy(
				boost::bind(&FeatureCollectionActivationStrategy::removing_file_from_workflow,
						workflow_info.activation_strategy, file_iter, _1),
				workflow_tag,
				active_lists_manager,
				activation_state_manager);

		// Deactivate the file in the workflow just in case the activation strategy forgets to.
		activation_state_manager.set_file_active_workflow(file_iter, workflow_tag, false);
		// Also remove the workflow tag from the file.
		file_active_state.remove_tag(workflow_tag);
	}

	// Get the active lists manager to update its internal active lists in response
	// to the activation changes.
	active_lists_manager.update_active_lists(activation_state_manager);

	// Notify workflows of any activation changes.
	// This should at least notify workflows that the current file is being
	// deactivated - this is because we removed the workflows from the file in the
	// previous loop. Note that we do this before actually notifying the workflows
	// that the file is being removed so that they have a chance to deactivate a file before
	// removing it (doesn't make sense the other way around).
	notify_workflows_of_activation_changes(activation_state_manager);

	// Iterate through the workflow tags and actually notify the workflows that the file
	// is being removed.
	workflow_tags_iter = workflow_tags.begin();
	for ( ; workflow_tags_iter != workflow_tags_end; ++workflow_tags_iter)
	{
		const workflow_tag_type &workflow_tag = *workflow_tags_iter;

		WorkflowInfo &workflow_info = get_workflow_info(workflow_tag);

		// Notify each workflow that we're about to remove the file.
		workflow_info.workflow->remove_file(file_iter);
	}
}


void
GPlatesAppLogic::FeatureCollectionFileStateImpl::WorkflowManager::changed_file(
		file_iterator_type file_iter,
		GPlatesFileIO::File &old_file,
		ActiveListsManager &active_lists_manager,
		ActivationStateManager &activation_state_manager)
{
	const ClassifyFeatureCollection::classifications_type new_file_classification =
			file_iter.get_file_node()->file_node_state().feature_collection_classification();

	FileNodeActiveState &file_active_state =
			file_iter.get_file_node()->file_node_state().active_state();

	bool used_by_higher_priority_workflow = false;

	// Iterate over all registered workflows from highest priority to lowest.
	sorted_workflow_seq_type::const_reverse_iterator workflow_iter = d_sorted_workflow_seq.rbegin();
	const sorted_workflow_seq_type::const_reverse_iterator workflow_end = d_sorted_workflow_seq.rend();
	for ( ; workflow_iter != workflow_end; ++workflow_iter)
	{
		const WorkflowInfo *workflow_info = *workflow_iter;

		FeatureCollectionWorkflow *workflow = workflow_info->workflow;
		const workflow_tag_type &workflow_tag = workflow->get_tag();

		// See if this workflow tag matches a workflow that was previously interested
		// in 'file_iter'.
		if (file_active_state.does_tag_exist(workflow_tag))
		{
			// The current workflow was previously interested in this file.
			// Notify workflow that file is about to be changed and see if it's still interested.
			if (workflow->changed_file(file_iter, old_file, new_file_classification))
			{
				used_by_higher_priority_workflow = true;
			}
			else
			{
				// Get activation strategy to activate file(s) in response.
				process_activation_strategy(
						boost::bind(&FeatureCollectionActivationStrategy::workflow_rejected_changed_file,
								workflow_info->activation_strategy, file_iter, _1),
						workflow_tag,
						active_lists_manager,
						activation_state_manager);

				// Deactivate the file in the workflow just in case the activation strategy forgets to.
				activation_state_manager.set_file_active_workflow(file_iter, workflow_tag, false);
				// Detach this workflow from the file.
				file_active_state.remove_tag(workflow_tag);
			}
		}
		else
		{
			// The current workflow was *not* previously interested in this file.
			// But since the file has changed maybe they will be now - let's ask them.
			if (workflow->add_file(
					file_iter, new_file_classification, used_by_higher_priority_workflow))
			{
				used_by_higher_priority_workflow = true;

				// Add the current workflow tag to the file - set it as initially
				// inactive because the activation strategy will determine if it wants
				// to activate it or not.
				file_active_state.add_tag(workflow_tag, false);

				// Get activation strategy to activate file(s) in response.
				process_activation_strategy(
						boost::bind(&FeatureCollectionActivationStrategy::added_file_to_workflow,
								workflow_info->activation_strategy, file_iter, _1),
						workflow_tag,
						active_lists_manager,
						activation_state_manager);
			}
		}
	}

	// Get the active lists manager to update its internal active lists in response
	// to the activation changes.
	active_lists_manager.update_active_lists(activation_state_manager);

	// Notify workflows of any activation changes.
	notify_workflows_of_activation_changes(activation_state_manager);
}


void
GPlatesAppLogic::FeatureCollectionFileStateImpl::WorkflowManager::set_active(
		file_iterator_type file_iter,
		const workflow_tag_type &workflow_tag,
		bool activate,
		ActiveListsManager &active_lists_manager,
		ActivationStateManager &activation_state_manager)
{
	WorkflowInfo &workflow_info = get_workflow_info(workflow_tag);

	// Get activation strategy to activate file(s).
	process_activation_strategy(
			boost::bind(&FeatureCollectionActivationStrategy::set_active,
					workflow_info.activation_strategy, file_iter, activate, _1),
			workflow_tag,
			active_lists_manager,
			activation_state_manager);

	// Get the active lists manager to update its internal active lists in response
	// to the activation changes.
	active_lists_manager.update_active_lists(activation_state_manager);

	// Notify workflows of any activation changes.
	notify_workflows_of_activation_changes(activation_state_manager);
}


void
GPlatesAppLogic::FeatureCollectionFileStateImpl::WorkflowManager::notify_workflows_of_activation_changes(
		ActivationStateManager &activation_state_manager)
{
	//
	// Here we iterate through the list of files (associated with workflows) and
	// notify the workflows of the activation changes.
	// The list of file/workflows is only those whose activation state has actually
	// changed (eg, from inactive to active or vice versa).
	//

	// Get all files whose activation state has actually changed since
	// 'activation_state_manager' was created.
	const ActivationStateManager::changed_activation_workflows_type &changed_activation_files =
			activation_state_manager.get_changed_activation_workflow_files();

	// Iterate through the workflow groups.
	ActivationStateManager::changed_activation_workflows_type::const_iterator
			changed_activation_iter = changed_activation_files.begin();
	ActivationStateManager::changed_activation_workflows_type::const_iterator
			changed_activation_end = changed_activation_files.end();
	for ( ; changed_activation_iter != changed_activation_end; ++changed_activation_iter)
	{
		const ActivationStateManager::WorkflowFiles &workflow_files =
				*changed_activation_iter;

		const workflow_tag_type &workflow_tag = workflow_files.workflow_tag;

		FeatureCollectionWorkflow *workflow = get_workflow_info(workflow_tag).workflow;

		// Iterate through the files of the current workflow.
		ActivationStateManager::changed_activation_sorted_unique_files_type::const_iterator
				workflow_files_iter = workflow_files.changed_activation_files.begin();
		ActivationStateManager::changed_activation_sorted_unique_files_type::const_iterator
				workflow_files_end = workflow_files.changed_activation_files.end();
		for ( ; workflow_files_iter != workflow_files_end; ++workflow_files_iter)
		{
			const file_iterator_type changed_activation_file = *workflow_files_iter;

			// The activation state manager changed the file's active state directly so
			// we can detect the change here.
			const bool is_changed_activation_file_active =
					changed_activation_file.get_file_node()->file_node_state()
							.active_state().get_active(workflow_tag);

			// Notify the workflow of the activation change.
			workflow->set_file_active(
					changed_activation_file, is_changed_activation_file_active);
		}
	}
}


GPlatesAppLogic::FeatureCollectionFileStateImpl::WorkflowManager::WorkflowInfo &
GPlatesAppLogic::FeatureCollectionFileStateImpl::WorkflowManager::get_workflow_info(
		const workflow_tag_type &workflow_tag)
{
	const workflow_map_type::iterator workflow_iter = d_workflow_map.find(workflow_tag);

	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			workflow_iter != d_workflow_map.end(),
			GPLATES_ASSERTION_SOURCE);

	WorkflowInfo &workflow_info = workflow_iter->second;

	return workflow_info;
}
