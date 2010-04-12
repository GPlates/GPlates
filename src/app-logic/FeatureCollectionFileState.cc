/* $Id$ */

/**
 * @file 
 * Contains the main function of GPlates.
 *
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2007 The University of Sydney, Australia
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
#include <functional>
#include <boost/bind.hpp>

#include "FeatureCollectionFileState.h"

#include "FeatureCollectionActivationStrategy.h"
#include "FeatureCollectionFileStateImpl.h"
#include "FeatureCollectionWorkflow.h"

#include "app-logic/ClassifyFeatureCollection.h"

#include "feature-visitors/FeatureClassifier.h"


namespace Decls = GPlatesAppLogic::FeatureCollectionFileStateDecls;
namespace Impl = GPlatesAppLogic::FeatureCollectionFileStateImpl;


namespace GPlatesAppLogic
{
	namespace FeatureCollectionFileStateImpl
	{
		/**
		 * A workflow for dealing with file containing reconstructable features.
		 *
		 * This workflow has the lowest priority over other workflows and
		 * will only register interest in a file if it both contains reconstructable
		 * features *and* if no other workflows has registered interest in it.
		 * This is because another workflow has a specific use in mind so we
		 * don't want to generate RFGs which will get rendered on the screen and
		 * potentially interfere with any rendering that the other workflow is doing.
		 */
		class ReconstructableWorkflow :
				public FeatureCollectionWorkflow
		{
		public:
			ReconstructableWorkflow(
					FeatureCollectionFileState &file_state)
			{
				register_workflow(file_state);
			}

			~ReconstructableWorkflow()
			{
				unregister_workflow();
			}

			virtual
			tag_type
			get_tag() const
			{
				return Decls::RECONSTRUCTABLE_WORKFLOW_TAG;
			}

			virtual
			priority_type
			get_priority() const
			{
				return PRIORITY_RECONSTRUCTABLE;
			}

			virtual
			bool
			add_file(
					file_iterator_type /*file_iter*/,
					const ClassifyFeatureCollection::classifications_type &classification,
					bool used_by_higher_priority_workflow)
			{
				// We're interested in a file if it contains features that are displayable.
				return ClassifyFeatureCollection::found_geometry_feature(classification);
			}

			virtual
			void
			remove_file(
					file_iterator_type /*file_iter*/)
			{
				// Do nothing
			}

			virtual
			bool
			changed_file(
					file_iterator_type /*file_iter*/,
					GPlatesFileIO::File &/*old_file*/,
					const ClassifyFeatureCollection::classifications_type &new_classification)
			{
				// We're interested in a file if it contains features that are displayable.
				return ClassifyFeatureCollection::found_geometry_feature(new_classification);
			}

			virtual
			void
			set_file_active(
					file_iterator_type /*file_iter*/,
					bool /*active*/)
			{
				// Do nothing - this is just a notification that
				// the file has been activated/deactivated.
			}
		};


		/**
		 * A workflow for dealing with file containing reconstruction features.
		 *
		 * This workflow will only register interest in a file if
		 * it contains reconstruction features regardless of whether another workflow
		 * registered interest in it or not. Typically a workflow will only accept
		 * reconstructable files anyway since all the active reconstruction files
		 * are typically passed to it when it is asked to do something (like solve velocities).
		 */
		class ReconstructionWorkflow :
				public FeatureCollectionWorkflow
		{
		public:
			ReconstructionWorkflow(
					FeatureCollectionFileState &file_state)
			{
				register_workflow(file_state);
			}

			~ReconstructionWorkflow()
			{
				unregister_workflow();
			}

			virtual
			tag_type
			get_tag() const
			{
				return Decls::RECONSTRUCTION_WORKFLOW_TAG;
			}

			virtual
			priority_type
			get_priority() const
			{
				return PRIORITY_RECONSTRUCTION;
			}

			virtual
			bool
			add_file(
					file_iterator_type /*file_iter*/,
					const ClassifyFeatureCollection::classifications_type &classification,
					bool /*used_by_higher_priority_workflow*/)
			{
				// We're interested in a file if it contains reconstruction features.
				return classification.test(ClassifyFeatureCollection::RECONSTRUCTION);
			}

			virtual
			void
			remove_file(
					file_iterator_type /*file_iter*/)
			{
				// Do nothing
			}

			virtual
			bool
			changed_file(
					file_iterator_type /*file_iter*/,
					GPlatesFileIO::File &/*old_file*/,
					const ClassifyFeatureCollection::classifications_type &new_classification)
			{
				// We're interested in a file if it contains reconstruction features.
				return new_classification.test(ClassifyFeatureCollection::RECONSTRUCTION);
			}

			virtual
			void
			set_file_active(
					file_iterator_type /*file_iter*/,
					bool /*active*/)
			{
				// Do nothing - this is just a notification that
				// the file has been activated/deactivated.
			}
		};
	}


}


namespace
{
	inline
	bool
	feature_collection_contains_feature(
			const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection_ref,
			const GPlatesModel::FeatureHandle::weak_ref &feature_ref)
	{
		return feature_ref->parent_ptr() == feature_collection_ref.handle_ptr();
	}
}


GPlatesAppLogic::FeatureCollectionFileState::FeatureCollectionFileState() :
	d_active_lists_manager(new Impl::ActiveListsManager()),
	d_workflow_manager(new Impl::WorkflowManager()),
	d_default_activation_strategy(new FeatureCollectionActivationStrategy()),
	d_reconstructable_workflow(new Impl::ReconstructableWorkflow(*this)),
	d_reconstruction_workflow(new Impl::ReconstructionWorkflow(*this))
{
}


GPlatesAppLogic::FeatureCollectionFileState::~FeatureCollectionFileState()
{
	// boost::scoped_ptr destructor requires complete type
}


GPlatesAppLogic::FeatureCollectionFileState::file_iterator_range
GPlatesAppLogic::FeatureCollectionFileState::get_loaded_files()
{
	return file_iterator_range(
			file_iterator::create(d_loaded_files.begin()),
			file_iterator::create(d_loaded_files.end()));
}


void
GPlatesAppLogic::FeatureCollectionFileState::get_active_files(
		std::vector<file_iterator> &active_files,
		const workflow_tag_seq_type &workflow_tags_minus_reconstructable_reconstruction,
		bool include_reconstructable_workflow,
		bool include_reconstruction_workflow)
{
	// Get a list of all workflow tags including the reconstructable and reconstruction tags.
	workflow_tag_seq_type workflow_tags(workflow_tags_minus_reconstructable_reconstruction);
	if (include_reconstructable_workflow)
	{
		workflow_tags.push_back(Decls::RECONSTRUCTABLE_WORKFLOW_TAG);
	}
	if (include_reconstruction_workflow)
	{
		workflow_tags.push_back(Decls::RECONSTRUCTION_WORKFLOW_TAG);
	}

	const file_iterator_range loaded_files = get_loaded_files();

	// Iterate over all loaded files and see if they are active with
	// any workflow specified by the caller.
	for (file_iterator file_iter = loaded_files.begin;
		file_iter != loaded_files.end;
		++file_iter)
	{
		workflow_tag_seq_type::const_iterator workflow_tag_iter = workflow_tags.begin();
		workflow_tag_seq_type::const_iterator workflow_tag_end = workflow_tags.end();
		for ( ; workflow_tag_iter != workflow_tag_end; ++workflow_tag_iter)
		{
			const workflow_tag_type &workflow_tag = *workflow_tag_iter;

			if (is_file_active_workflow(file_iter, workflow_tag))
			{
				// Add to the caller's list.
				active_files.push_back(file_iter);

				// Continue on to the next file.
				break;
			}
		}
	}
}


GPlatesAppLogic::FeatureCollectionFileState::active_file_iterator_range
GPlatesAppLogic::FeatureCollectionFileState::get_active_reconstructable_files()
{
	return d_active_lists_manager->get_active_files(Decls::RECONSTRUCTABLE_WORKFLOW_TAG);
}


GPlatesAppLogic::FeatureCollectionFileState::active_file_iterator_range
GPlatesAppLogic::FeatureCollectionFileState::get_active_reconstruction_files()
{
	return d_active_lists_manager->get_active_files(Decls::RECONSTRUCTION_WORKFLOW_TAG);
}


GPlatesAppLogic::FeatureCollectionFileState::active_file_iterator_range
GPlatesAppLogic::FeatureCollectionFileState::get_active_workflow_files(
		const QString &workflow_tag)
{
	return d_active_lists_manager->get_active_files(workflow_tag);
}


GPlatesAppLogic::FeatureCollectionFileState::file_iterator
GPlatesAppLogic::FeatureCollectionFileState::convert_to_file_iterator(
		active_file_iterator active_file_iter)
{
	// We extract the iterator implementation of the active file iterator.
	// This iterator implementation dereferences to an iterator implementation
	// of a loaded file iterator.
	// That then gets wrapped back into an iterator wrapper that the client can use.
	return file_iterator::create(
			*active_file_iter.get_iterator_impl());
}


void
GPlatesAppLogic::FeatureCollectionFileState::register_workflow(
		FeatureCollectionWorkflow *workflow,
		FeatureCollectionActivationStrategy *activation_strategy)
{
	// Use the default activation strategy if the caller didn't specify one.
	if (activation_strategy == NULL)
	{
		activation_strategy = d_default_activation_strategy.get();
	}

	d_workflow_manager->register_workflow(workflow, activation_strategy);
	d_active_lists_manager->register_workflow(workflow);
	// Add to list of registered workflow tags.
	d_registered_workflow_seq.push_back(workflow->get_tag());
}


void
GPlatesAppLogic::FeatureCollectionFileState::unregister_workflow(
		FeatureCollectionWorkflow *workflow)
{
	d_workflow_manager->unregister_workflow(workflow);
	d_active_lists_manager->unregister_workflow(workflow);

	const workflow_tag_type workflow_tag = workflow->get_tag();

	// Remove from the list of registered workflow tags.
	d_registered_workflow_seq.remove(workflow_tag);


	// Iterate through all loaded files and remove the workflow's tag from the active states
	// if it is currently in them.
	// Since we're managing the lifetime of the file nodes we'll be responsible
	// for clearing out the workflow tags.
	file_seq_iterator_impl_type files_iter = d_loaded_files.begin();
	const file_seq_iterator_impl_type files_end = d_loaded_files.end();
	for ( ; files_iter != files_end; ++files_iter)
	{
		file_node_type &file_node = *files_iter;

		if (file_node.file_node_state().active_state().does_tag_exist(workflow_tag))
		{
			file_node.file_node_state().active_state().remove_tag(workflow_tag);
		}
	}
}


const GPlatesAppLogic::FeatureCollectionFileState::workflow_tag_seq_type &
GPlatesAppLogic::FeatureCollectionFileState::get_registered_workflow_tags() const
{
	return d_registered_workflow_seq;
}


void
GPlatesAppLogic::FeatureCollectionFileState::set_reconstructable_activation_strategy(
		FeatureCollectionActivationStrategy *activation_strategy)
{
	d_workflow_manager->set_activation_strategy(
			activation_strategy, Decls::RECONSTRUCTABLE_WORKFLOW_TAG);
}


void
GPlatesAppLogic::FeatureCollectionFileState::set_reconstruction_activation_strategy(
		FeatureCollectionActivationStrategy *activation_strategy)
{
	d_workflow_manager->set_activation_strategy(
			activation_strategy, Decls::RECONSTRUCTION_WORKFLOW_TAG);
}


void
GPlatesAppLogic::FeatureCollectionFileState::set_workflow_activation_strategy(
		FeatureCollectionActivationStrategy *activation_strategy,
		const workflow_tag_type &workflow_tag)
{
	d_workflow_manager->set_activation_strategy(
			activation_strategy, workflow_tag);
}


std::vector<GPlatesAppLogic::FeatureCollectionFileState::workflow_tag_type>
GPlatesAppLogic::FeatureCollectionFileState::get_workflow_tags(
		file_iterator file_iter) const
{
	std::vector<workflow_tag_type> workflow_tags =
			file_iter.get_file_node()->file_node_state().active_state().get_tags();

	// Remove the reconstructable and reconstruction workflow tags since
	// the client is not aware of those (they are just used internally).
	// When the client wants to do something with those workflows it
	// uses API methods specific to those workflows.
	std::vector<workflow_tag_type>::iterator reconstructable_tag_iter = std::find(
			workflow_tags.begin(), workflow_tags.end(), Decls::RECONSTRUCTABLE_WORKFLOW_TAG);
	if (reconstructable_tag_iter != workflow_tags.end())
	{
		workflow_tags.erase(reconstructable_tag_iter);
	}
	std::vector<workflow_tag_type>::iterator reconstruction_tag_iter = std::find(
			workflow_tags.begin(), workflow_tags.end(), Decls::RECONSTRUCTION_WORKFLOW_TAG);
	if (reconstruction_tag_iter != workflow_tags.end())
	{
		workflow_tags.erase(reconstruction_tag_iter);
	}

	return workflow_tags;
}


GPlatesAppLogic::FeatureCollectionFileState::file_iterator_range
GPlatesAppLogic::FeatureCollectionFileState::add_files(
		const std::vector<GPlatesFileIO::File::shared_ref> &files_to_add)
{
	const file_iterator new_files_end = file_iterator::create(d_loaded_files.end());
	file_iterator new_files_begin = new_files_end;

	if (files_to_add.empty())
	{
		// Return empty range.
		return file_iterator_range(new_files_begin, new_files_end);
	}

	// Iterate over the files passed in by the caller.
	typedef std::vector<GPlatesFileIO::File::shared_ref> file_seq_type;
	file_seq_type::const_iterator files_to_add_iter = files_to_add.begin();
	file_seq_type::const_iterator files_to_add_end = files_to_add.end();
	for ( ; files_to_add_iter != files_to_add_end; ++files_to_add_iter)
	{
		const GPlatesFileIO::File::shared_ref &file_to_add = *files_to_add_iter;

		const file_iterator file_iter = add_file_internal(file_to_add);

		// If this is the first file in the sequence then set the begin iterator
		// to be returned to client.
		if (new_files_begin == new_files_end)
		{
			new_files_begin = file_iter;
		}
	}

	// Emit this signal once the other signals are emitted to
	// signal that all requested files have been added.
	emit end_add_feature_collections(*this, new_files_begin, new_files_end);
	emit file_state_changed(*this);

	// Also let direct caller know which files were added so doesn't have to
	// listen to signal and interrupt its call flow.
	return file_iterator_range(new_files_begin, new_files_end);
}


GPlatesAppLogic::FeatureCollectionFileState::file_iterator
GPlatesAppLogic::FeatureCollectionFileState::add_file(
		const GPlatesFileIO::File::shared_ref &file_to_add)
{
	const std::vector<GPlatesFileIO::File::shared_ref> files_to_add(1, file_to_add);

	// Reuse the add multiple files function so we emit all the right signals, etc.
	const file_iterator_range files_added = add_files(files_to_add);

	// We know only only file was added so return begin iterator of iterator range.
	return files_added.begin;
}


GPlatesAppLogic::FeatureCollectionFileState::file_iterator
GPlatesAppLogic::FeatureCollectionFileState::add_file_internal(
		const GPlatesFileIO::File::shared_ref &file_to_add)
{
	// Keep track of any file activation changes.
	activation_state_manager_type activation_state_manager;

	// Determine if file has reconstructable/reconstruction features.
	const ClassifyFeatureCollection::classifications_type classification =
			ClassifyFeatureCollection::classify_feature_collection(*file_to_add);

	// Add to our list of loaded files.
	// This is where 'file_to_add' and its file state actually lives.
	// This is why we add it as a temporary so that we are forced to access
	// the real file node (the inserted one) and not the temporary.
	const file_seq_iterator_impl_type file_iter_impl =
			d_loaded_files.insert(
					d_loaded_files.end(),
					// the file node
					file_node_type(
							file_to_add,
							// the initial file node state
							file_node_state_type(classification)));

	// Create an iterator that the clients can access.
	const file_iterator file_iter = file_iterator::create(file_iter_impl);

	// Ask each registered workflow if they're interested in the new file.
	// This includes the pseudo workflow of reconstructing the reconstructable features
	// in the file (if it has any) - it will only register interest if no other workflows do.
	//
	// This can change the file's active state.
	d_workflow_manager->add_file(
			file_iter, *d_active_lists_manager, activation_state_manager);

	// Emit activation signals for any files that changed activation state.
	emit_activation_signals(activation_state_manager);

	return file_iter;
}


void
GPlatesAppLogic::FeatureCollectionFileState::remove_file(
		file_iterator file_iter)
{
	emit begin_remove_feature_collection(*this, file_iter);

	// Keep track of any file activation changes.
	activation_state_manager_type activation_state_manager;

	// This can change the file's active state.
	d_workflow_manager->remove_file(file_iter, *d_active_lists_manager, activation_state_manager);

	// Emit activation signals for any files that changed activation state.
	// We do this before destroying the file since the signals emitted pass
	// file iterators so they must remain valid.
	emit_activation_signals(activation_state_manager);

	// And finally remove the actual file.
	// This will destroy the file.
	d_loaded_files.erase(file_iter.get_iterator_impl());

	emit end_remove_feature_collection(*this);
	emit file_state_changed(*this);
}


void
GPlatesAppLogic::FeatureCollectionFileState::reset_file(
		file_iterator file_iter,
		const GPlatesFileIO::File::shared_ref &new_file)
{
	emit begin_reset_feature_collection(*this, file_iter);

	change_file_internal(file_iter, new_file);

	emit end_reset_feature_collection(*this, file_iter);
	emit file_state_changed(*this);
}


void
GPlatesAppLogic::FeatureCollectionFileState::reclassify_feature_collection(
		file_iterator file_iter)
{
	//
	// Here we are classifying presumably because the feature collection has been modified
	// outside our control.
	//

	emit begin_reclassify_feature_collection(*this, file_iter);

	// We haven't actually changed the file itself but we can reuse functionality
	// if we pretend we have a new file and set it to the existing file.
	const GPlatesFileIO::File::shared_ref old_file = file_iter.get_file_node()->file();
	change_file_internal(file_iter, old_file/*new_file*/);

	emit end_reclassify_feature_collection(*this, file_iter);
	emit file_state_changed(*this);
}


GPlatesAppLogic::ClassifyFeatureCollection::classifications_type
GPlatesAppLogic::FeatureCollectionFileState::get_feature_collection_classification(
		file_iterator file_iter) const
{
	return file_iter.get_file_node()->file_node_state().feature_collection_classification();
}


void
GPlatesAppLogic::FeatureCollectionFileState::change_file_internal(
		file_iterator file_iter,
		const GPlatesFileIO::File::shared_ref &new_file)
{
	//
	// A scope block is created so that the old file gets destroyed
	// before we emit a final signal.
	// This ensures that we don't have the old feature collection
	// lying around causing duplicate features if the final signal
	// does something like generate a new reconstruction.
	//
	// NOTE: Since this method is private and is called from the public methods (which
	// themselves emit the final signal) this method forms a natural scope block
	// so we don't need to worry about creating a new scope block.
	//


	// Keep track of any file activation changes.
	activation_state_manager_type activation_state_manager;

	// Keep a shared reference to the original file before we replace it.
	// We want to keep it alive long enough that we can pass it to any
	// workflows that are currently interested in it.
	const GPlatesFileIO::File::shared_ref old_file = file_iter.get_file_node()->file();

	// Now update the file node state by determining if feature collection
	// has reconstructable and reconstruction features.
	file_iter.get_file_node()->file_node_state().feature_collection_classification() =
			ClassifyFeatureCollection::classify_feature_collection(*new_file);

	// Set the new file in the file node.
	file_iter.get_file_node()->file() = new_file;

	// This can change the file's active state.
	// We're also doing this after we actually replace the file so that the workflows
	// see the new file through 'file_iter'.
	d_workflow_manager->changed_file(
			file_iter, *old_file, *d_active_lists_manager, activation_state_manager);

	// Emit activation signals for any files that changed activation state.
	emit_activation_signals(activation_state_manager);
}


void
GPlatesAppLogic::FeatureCollectionFileState::set_file_active_reconstructable(
		file_iterator file_iter,
		bool activate)
{
	if (!set_file_active_workflow_internal(
			file_iter, Decls::RECONSTRUCTABLE_WORKFLOW_TAG, activate))
	{
		return;
	}

	emit end_set_file_active_reconstructable(*this, file_iter);
	emit file_state_changed(*this);
}


void
GPlatesAppLogic::FeatureCollectionFileState::set_file_active_reconstruction(
		file_iterator file_iter,
		bool activate)
{
	if (!set_file_active_workflow_internal(
			file_iter, Decls::RECONSTRUCTION_WORKFLOW_TAG, activate))
	{
		return;
	}

	emit end_set_file_active_reconstruction(*this, file_iter);
	emit file_state_changed(*this);
}


void
GPlatesAppLogic::FeatureCollectionFileState::set_file_active_workflow(
		file_iterator file_iter,
		const workflow_tag_type &workflow_tag,
		bool activate)
{
	if (!set_file_active_workflow_internal(file_iter, workflow_tag, activate))
	{
		return;
	}

	emit end_set_file_active_workflow(*this, file_iter, workflow_tag);
	emit file_state_changed(*this);
}


bool
GPlatesAppLogic::FeatureCollectionFileState::set_file_active_workflow_internal(
		file_iterator file_iter,
		const workflow_tag_type &workflow_tag,
		bool activate)
{
	// Return early if the specified workflow tag is not known by the specified file.
	if (!file_iter.get_file_node()->file_node_state().active_state().does_tag_exist(workflow_tag))
	{
		return false;
	}

	// Keep track of any file activation changes.
	activation_state_manager_type activation_state_manager;

	d_workflow_manager->set_active(
			file_iter, workflow_tag, activate, *d_active_lists_manager, activation_state_manager);

	// Emit activation signals for any files that changed activation state.
	emit_activation_signals(activation_state_manager);

	// Let user know that the workflow was notified.
	return true;
}


bool
GPlatesAppLogic::FeatureCollectionFileState::is_reconstructable_workflow_using_file(
		file_iterator file) const
{
	return is_file_using_workflow(file, Decls::RECONSTRUCTABLE_WORKFLOW_TAG);
}


bool
GPlatesAppLogic::FeatureCollectionFileState::is_file_active_reconstructable(
		file_iterator file) const
{
	return is_file_active_workflow(file, Decls::RECONSTRUCTABLE_WORKFLOW_TAG);
}


bool
GPlatesAppLogic::FeatureCollectionFileState::is_reconstruction_workflow_using_file(
		file_iterator file) const
{
	return is_file_using_workflow(file, Decls::RECONSTRUCTION_WORKFLOW_TAG);
}


bool
GPlatesAppLogic::FeatureCollectionFileState::is_file_active_reconstruction(
		file_iterator file) const
{
	return is_file_active_workflow(file, Decls::RECONSTRUCTION_WORKFLOW_TAG);
}


bool
GPlatesAppLogic::FeatureCollectionFileState::is_file_using_workflow(
		file_iterator file,
		const workflow_tag_type &workflow_tag) const
{
	return file.get_file_node()->file_node_state().active_state().does_tag_exist(
			workflow_tag);
}


bool
GPlatesAppLogic::FeatureCollectionFileState::is_file_active_workflow(
		file_iterator file,
		const workflow_tag_type &workflow_tag) const
{
	return file.get_file_node()->file_node_state().active_state().get_active(
			workflow_tag);
}


void
GPlatesAppLogic::FeatureCollectionFileState::emit_activation_signals(
		const activation_state_manager_type &activation_state_manager)
{
	//
	// Here we iterate through the list of files (associated with workflows) and
	// emit activation signals.
	// The list of file/workflows is only those whose activation state has actually
	// changed (eg, from inactive to active or vice versa).
	//

	// Get all files whose activation state has actually changed since
	// 'activation_state_manager' was created.
	const activation_state_manager_type::changed_activation_workflows_type changed_activation_files =
			activation_state_manager.get_changed_activation_workflow_files();

	// Iterate through the workflow groups.
	activation_state_manager_type::changed_activation_workflows_type::const_iterator
			changed_activation_iter = changed_activation_files.begin();
	activation_state_manager_type::changed_activation_workflows_type::const_iterator
			changed_activation_end = changed_activation_files.end();
	for ( ; changed_activation_iter != changed_activation_end; ++changed_activation_iter)
	{
		const activation_state_manager_type::WorkflowFiles &workflow_files =
				*changed_activation_iter;

		const workflow_tag_type &workflow_tag = workflow_files.workflow_tag;

		// Iterate through the files of the current workflow.
		activation_state_manager_type::changed_activation_sorted_unique_files_type::const_iterator
				workflow_files_iter = workflow_files.changed_activation_files.begin();
		activation_state_manager_type::changed_activation_sorted_unique_files_type::const_iterator
				workflow_files_end = workflow_files.changed_activation_files.end();
		for ( ; workflow_files_iter != workflow_files_end; ++workflow_files_iter)
		{
			const file_iterator file_iter = *workflow_files_iter;

			emit_activation_signal(file_iter, workflow_tag);
		}
	}
}


void
GPlatesAppLogic::FeatureCollectionFileState::emit_activation_signal(
		file_iterator file_iter,
		const workflow_tag_type &workflow_tag)
{
	const bool new_active_state =
			file_iter.get_file_node()->file_node_state().active_state().get_active(workflow_tag);

	if (workflow_tag == Decls::RECONSTRUCTABLE_WORKFLOW_TAG)
	{
		emit reconstructable_file_activation(*this, file_iter, new_active_state);
	}
	else if (workflow_tag == Decls::RECONSTRUCTION_WORKFLOW_TAG)
	{
		emit reconstruction_file_activation(*this, file_iter, new_active_state);
	}
	else
	{
		emit workflow_file_activation(*this, file_iter, workflow_tag, new_active_state);
	}
}


boost::optional<GPlatesModel::FeatureCollectionHandle::weak_ref>
GPlatesAppLogic::get_feature_collection_containing_feature(
		GPlatesAppLogic::FeatureCollectionFileState &file_state_ref,
		GPlatesModel::FeatureHandle::weak_ref feature_ref)
{
	GPlatesAppLogic::FeatureCollectionFileState::file_iterator_range it_range =
		file_state_ref.get_loaded_files();

	GPlatesAppLogic::FeatureCollectionFileState::file_iterator it = it_range.begin;
	GPlatesAppLogic::FeatureCollectionFileState::file_iterator end = it_range.end;

	for (; it != end; ++it) 
	{
		GPlatesModel::FeatureCollectionHandle::weak_ref feature_collection_ref =
			it->get_feature_collection();

		if (feature_collection_contains_feature(feature_collection_ref, feature_ref)) 
		{
			return feature_collection_ref;
		}
	}
	return boost::none;
}
