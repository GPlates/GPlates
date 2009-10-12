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

#include "app-logic/ClassifyFeatureCollection.h"

#include "feature-visitors/FeatureCollectionClassifier.h"


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
				public FeatureCollectionFileState::Workflow
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
				return Impl::RECONSTRUCTABLE_TAG;
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
					FeatureCollectionFileState::file_iterator /*file_iter*/,
					const ClassifyFeatureCollection::classifications_type &classification,
					bool used_by_higher_priority_workflow)
			{
				// We're interested in a file if it contains reconstructable features *and*
				// it's *not* being used by a higher priority workflow.
				return classification.test(ClassifyFeatureCollection::RECONSTRUCTABLE) &&
						!used_by_higher_priority_workflow;
			}

			virtual
			void
			remove_file(
					FeatureCollectionFileState::file_iterator /*file_iter*/)
			{
				// Do nothing
			}

			virtual
			bool
			changed_file(
					FeatureCollectionFileState::file_iterator /*file_iter*/,
					GPlatesFileIO::File &/*old_file*/,
					const ClassifyFeatureCollection::classifications_type &new_classification)
			{
				// We're interested in a file if it contains reconstructable features.
				return new_classification.test(ClassifyFeatureCollection::RECONSTRUCTABLE);
			}

			virtual
			void
			set_file_active(
					FeatureCollectionFileState::file_iterator /*file_iter*/,
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
				public FeatureCollectionFileState::Workflow
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
				return Impl::RECONSTRUCTION_TAG;
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
					FeatureCollectionFileState::file_iterator /*file_iter*/,
					const ClassifyFeatureCollection::classifications_type &classification,
					bool /*used_by_higher_priority_workflow*/)
			{
				// We're interested in a file if it contains reconstruction features.
				return classification.test(ClassifyFeatureCollection::RECONSTRUCTION);
			}

			virtual
			void
			remove_file(
					FeatureCollectionFileState::file_iterator /*file_iter*/)
			{
				// Do nothing
			}

			virtual
			bool
			changed_file(
					FeatureCollectionFileState::file_iterator /*file_iter*/,
					GPlatesFileIO::File &/*old_file*/,
					const ClassifyFeatureCollection::classifications_type &new_classification)
			{
				// We're interested in a file if it contains reconstruction features.
				return new_classification.test(ClassifyFeatureCollection::RECONSTRUCTION);
			}

			virtual
			void
			set_file_active(
					FeatureCollectionFileState::file_iterator /*file_iter*/,
					bool /*active*/)
			{
				// Do nothing - this is just a notification that
				// the file has been activated/deactivated.
			}
		};
	}
}


GPlatesAppLogic::FeatureCollectionFileState::FeatureCollectionFileState() :
	d_reconstructable_workflow(new Impl::ReconstructableWorkflow(*this)),
	d_reconstruction_workflow(new Impl::ReconstructionWorkflow(*this))
{
}


GPlatesAppLogic::FeatureCollectionFileState::~FeatureCollectionFileState()
{
	// boost::scoped_ptr destructor requires complete type
}


GPlatesAppLogic::FeatureCollectionFileState::file_iterator_range
GPlatesAppLogic::FeatureCollectionFileState::loaded_files()
{
	return file_iterator_range(
			file_iterator::create(d_active_lists_manager.loaded_files_begin()),
			file_iterator::create(d_active_lists_manager.loaded_files_end()));
}


GPlatesAppLogic::FeatureCollectionFileState::active_file_iterator_range
GPlatesAppLogic::FeatureCollectionFileState::get_active_reconstructable_files()
{
	return active_file_iterator_range(
			active_file_iterator::create(
					d_active_lists_manager.active_files_begin(Impl::RECONSTRUCTABLE_TAG)),
			active_file_iterator::create(
					d_active_lists_manager.active_files_end(Impl::RECONSTRUCTABLE_TAG)));
}


GPlatesAppLogic::FeatureCollectionFileState::active_file_iterator_range
GPlatesAppLogic::FeatureCollectionFileState::get_active_reconstruction_files()
{
	return active_file_iterator_range(
			active_file_iterator::create(
					d_active_lists_manager.active_files_begin(Impl::RECONSTRUCTION_TAG)),
			active_file_iterator::create(
					d_active_lists_manager.active_files_end(Impl::RECONSTRUCTION_TAG)));
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
		Workflow *workflow)
{
	d_workflow_manager.register_workflow(workflow);
	d_active_lists_manager.register_workflow(workflow);
}


void
GPlatesAppLogic::FeatureCollectionFileState::unregister_workflow(
		Workflow *workflow)
{
	d_workflow_manager.unregister_workflow(workflow);
	d_active_lists_manager.unregister_workflow(workflow);
}


std::vector<GPlatesAppLogic::FeatureCollectionFileState::Workflow::tag_type>
GPlatesAppLogic::FeatureCollectionFileState::get_workflow_tags(
		file_iterator file_iter) const
{
	typedef std::vector<Workflow::tag_type> workflow_tag_seq_type;

	workflow_tag_seq_type workflow_tags =
			file_iter.get_file_node()->file_node_state().active_state().get_tags();

	// Remove the reconstructable and reconstruction workflow tags since
	// the client is not aware of those (they are just used internally).
	// When the client wants to do something with those workflows it
	// uses API methods specific to those workflows.
	workflow_tag_seq_type::iterator reconstructable_tag_iter = std::find(
			workflow_tags.begin(), workflow_tags.end(), Impl::RECONSTRUCTABLE_TAG);
	if (reconstructable_tag_iter != workflow_tags.end())
	{
		workflow_tags.erase(reconstructable_tag_iter);
	}
	workflow_tag_seq_type::iterator reconstruction_tag_iter = std::find(
			workflow_tags.begin(), workflow_tags.end(), Impl::RECONSTRUCTION_TAG);
	if (reconstruction_tag_iter != workflow_tags.end())
	{
		workflow_tags.erase(reconstruction_tag_iter);
	}

	return workflow_tags;
}


GPlatesAppLogic::FeatureCollectionFileState::active_file_iterator_range
GPlatesAppLogic::FeatureCollectionFileState::get_active_workflow_files(
		const QString &workflow_tag)
{
	return active_file_iterator_range(
			active_file_iterator::create(
					d_active_lists_manager.active_files_begin(workflow_tag)),
			active_file_iterator::create(
					d_active_lists_manager.active_files_end(workflow_tag)));
}


GPlatesAppLogic::FeatureCollectionFileState::file_iterator_range
GPlatesAppLogic::FeatureCollectionFileState::add_files(
		const std::vector<GPlatesFileIO::File::shared_ref> &files_to_add)
{
	const file_iterator new_files_end = file_iterator::create(
			d_active_lists_manager.loaded_files_end());
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
	// Keep a copy of the file's original active state - this is all empty/inactive
	// since the file didn't exist until now.
	const file_node_active_state_type old_file_node_active_state;

	// Determine if file has reconstructable/reconstruction features.
	const ClassifyFeatureCollection::classifications_type classification =
			ClassifyFeatureCollection::classify_feature_collection(*file_to_add);

	// Add to our list of loaded files.
	// This is where 'file_to_add' and its file state actually lives.
	// This is why we add it as a temporary so that we are forced to access
	// the real file node and not the temporary (in this method).
	const file_seq_iterator_impl_type file_iter_impl = d_active_lists_manager.add_file(
			// the file node
			file_node_type(
					file_to_add,
					// the initial file node state
					file_node_state_type(classification)));

	// Create an iterator that the clients can access.
	const file_iterator file_iter = file_iterator::create(file_iter_impl);

	// Keep a reference to the file's active state so we can change it.
	file_node_state_type &new_file_node_state = file_iter.get_file_node()->file_node_state();

	// Ask each registered workflow if they're interested in the new file.
	// This includes the pseudo workflow of reconstructing the reconstructable features
	// in the file (if it has any) - it will only register interest if no other workflows do.
	//
	// This can change the file's active state.
	d_workflow_manager.add_file(file_iter);

	// Now that we've initialised the active state for this file get the lists manager
	// to update its active lists.
	d_active_lists_manager.update_active_state_lists(file_iter_impl);

	// Emit signals if we are activating the new file.
	emit_activation_signals(
			file_iter, old_file_node_active_state, new_file_node_state.active_state());

	return file_iter;
}


void
GPlatesAppLogic::FeatureCollectionFileState::remove_file(
		file_iterator file_iter)
{
	emit begin_remove_feature_collection(*this, file_iter);

	// Keep a copy of the file's original active state.
	const file_node_active_state_type old_file_node_active_state =
			file_iter.get_file_node()->file_node_state().active_state();

	// This can change the file's active state.
	d_workflow_manager.remove_file(file_iter);

	// The new file node active state (should be all empty).
	const file_node_active_state_type &new_file_node_active_state =
			file_iter.get_file_node()->file_node_state().active_state();

	// Emit signals if we are deactivating the file - we do this before we destroy the file.
	emit_activation_signals(file_iter, old_file_node_active_state, new_file_node_active_state);

	// This will destroy the file.
	// Also cleans up active lists based on the current active state (which should be all inactive).
	d_active_lists_manager.remove_file(file_iter.get_iterator_impl());

	emit end_remove_feature_collection(*this);
	emit file_state_changed(*this);
}


void
GPlatesAppLogic::FeatureCollectionFileState::reset_file(
		file_iterator file_iter,
		const GPlatesFileIO::File::shared_ref &new_file)
{
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


	// Keep a shared reference to the original file before we replace it.
	// We want to keep it alive long enough that we can pass it to any
	// workflows that are currently interested in it.
	const GPlatesFileIO::File::shared_ref old_file = file_iter.get_file_node()->file();

	// Keep a copy of the file's original state.
	const file_node_state_type old_file_node_state =
			file_iter.get_file_node()->file_node_state();

	// Keep a reference to the file's active state so we can change it.
	file_node_state_type &new_file_node_state = file_iter.get_file_node()->file_node_state();

	// Now update the file node state by determining if feature collection
	// has reconstructable and reconstruction features.
	new_file_node_state.feature_collection_classification() =
			ClassifyFeatureCollection::classify_feature_collection(*new_file);

	// Set the new file in the file node.
	file_iter.get_file_node()->file() = new_file;

	// This can change the file's active state.
	// We're also doing this after we actually replace the file so that the workflows
	// see the new file through 'file_iter'.
	d_workflow_manager.changed_file(file_iter, *old_file);

	// Get the lists manager to update its internal active lists.
	d_active_lists_manager.update_active_state_lists(file_iter.get_iterator_impl());

	// Emit signals as necessary.
	emit_activation_signals(
			file_iter,
			old_file_node_state.active_state(),
			new_file_node_state.active_state());
}


void
GPlatesAppLogic::FeatureCollectionFileState::set_file_active_reconstructable(
		file_iterator file_iter,
		bool activate)
{
	if (!set_file_active_workflow_internal(file_iter, Impl::RECONSTRUCTABLE_TAG, activate))
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
	if (!set_file_active_workflow_internal(file_iter, Impl::RECONSTRUCTION_TAG, activate))
	{
		return;
	}

	emit end_set_file_active_reconstruction(*this, file_iter);
	emit file_state_changed(*this);
}


void
GPlatesAppLogic::FeatureCollectionFileState::set_file_active_workflow(
		file_iterator file_iter,
		const Workflow::tag_type &workflow_tag,
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
		const Workflow::tag_type &workflow_tag,
		bool activate)
{
	// Return early if the specified workflow tag is not known by the specified file.
	if (!file_iter.get_file_node()->file_node_state().active_state().exists(workflow_tag))
	{
		return false;
	}

	// Keep a copy of the file's original state.
	const file_node_state_type old_file_node_state = file_iter.get_file_node()->file_node_state();

	d_workflow_manager.set_active(file_iter, workflow_tag, activate);

	// Get the lists manager to update its internal active lists.
	d_active_lists_manager.update_active_state_lists(file_iter.get_iterator_impl());

	// Reference the file's new active state.
	const file_node_state_type &new_file_node_state = file_iter.get_file_node()->file_node_state();

	// Emit signals as necessary.
	emit_activation_signals(
			file_iter,
			old_file_node_state.active_state(),
			new_file_node_state.active_state());

	// Let user know that the workflow was notified.
	return true;
}


bool
GPlatesAppLogic::FeatureCollectionFileState::is_reconstructable_workflow_using_file(
		file_iterator file) const
{
	return is_file_using_workflow(file, Impl::RECONSTRUCTABLE_TAG);
}


bool
GPlatesAppLogic::FeatureCollectionFileState::is_file_active_reconstructable(
		file_iterator file) const
{
	return is_file_active_workflow(file, Impl::RECONSTRUCTABLE_TAG);
}


bool
GPlatesAppLogic::FeatureCollectionFileState::is_reconstruction_workflow_using_file(
		file_iterator file) const
{
	return is_file_using_workflow(file, Impl::RECONSTRUCTION_TAG);
}


bool
GPlatesAppLogic::FeatureCollectionFileState::is_file_active_reconstruction(
		file_iterator file) const
{
	return is_file_active_workflow(file, Impl::RECONSTRUCTION_TAG);
}


bool
GPlatesAppLogic::FeatureCollectionFileState::is_file_using_workflow(
		file_iterator file,
		const Workflow::tag_type &workflow_tag) const
{
	return file.get_file_node()->file_node_state().active_state().exists(
			workflow_tag);
}


bool
GPlatesAppLogic::FeatureCollectionFileState::is_file_active_workflow(
		file_iterator file,
		const Workflow::tag_type &workflow_tag) const
{
	return file.get_file_node()->file_node_state().active_state().get_active(
			workflow_tag);
}


void
GPlatesAppLogic::FeatureCollectionFileState::emit_activation_signals(
		file_iterator file_iter,
		const file_node_active_state_type &old_file_node_active_state,
		const file_node_active_state_type &new_file_node_active_state)
{
	visit_union(
			old_file_node_active_state,
			new_file_node_active_state,
			boost::bind(&FeatureCollectionFileState::emit_activation_signal,
					this, file_iter, _1, _2, _3));
}


void
GPlatesAppLogic::FeatureCollectionFileState::emit_activation_signal(
		file_iterator file_iter,
		const active_state_tag_type &active_list_tag,
		bool old_active_state,
		bool new_active_state)
{
	// Only interested in active state changes.
	if (old_active_state == new_active_state)
	{
		return;
	}

	if (active_list_tag == Impl::RECONSTRUCTABLE_TAG)
	{
		emit reconstructable_file_activation(*this, file_iter, new_active_state);
	}
	else if (active_list_tag == Impl::RECONSTRUCTION_TAG)
	{
		emit reconstruction_file_activation(*this, file_iter, new_active_state);
	}
	else
	{
		emit workflow_file_activation(*this, file_iter, active_list_tag, new_active_state);
	}
}


GPlatesAppLogic::FeatureCollectionFileState::Workflow::Workflow() :
	d_file_state(NULL)
{
}


void
GPlatesAppLogic::FeatureCollectionFileState::Workflow::register_workflow(
		FeatureCollectionFileState &file_state)
{
	// Return if already registered.
	if (d_file_state)
	{
		return;
	}

	d_file_state = &file_state;
	d_file_state->register_workflow(this);
}


void
GPlatesAppLogic::FeatureCollectionFileState::Workflow::unregister_workflow()
{
	// Since this is called from a destructor we cannot let any exceptions escape.
	// If one is thrown we just have to lump it and continue on.
	try
	{
		if (d_file_state)
		{
			d_file_state->unregister_workflow(this);
		}
	}
	catch (...)
	{
	}
}


void
GPlatesAppLogic::FeatureCollectionFileState::ActiveListsManager::register_workflow(
		Workflow *workflow)
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
GPlatesAppLogic::FeatureCollectionFileState::ActiveListsManager::unregister_workflow(
		Workflow *workflow)
{
	const Workflow::tag_type &workflow_tag = workflow->get_tag();

	// Iterate through all loaded files and remove the workflow's tag from the active states
	// if it is currently in them.
	// Since we're managing the lifetime of the file nodes (and the active states contained
	// within) we'll be responsible for clearing out the workflow tags.
	file_seq_iterator_impl_type files_iter = d_loaded_files.begin();
	const file_seq_iterator_impl_type files_end = d_loaded_files.end();
	for ( ; files_iter != files_end; ++files_iter)
	{
		Impl::FileNodeInfo &file_node_info = *files_iter;

		if (file_node_info.prev_active_state.exists(workflow_tag))
		{
			file_node_info.prev_active_state.remove(workflow_tag);
		}
		if (file_node_info.file_node.file_node_state().active_state().exists(workflow_tag))
		{
			file_node_info.file_node.file_node_state().active_state().remove(workflow_tag);
		}
	}

	// Remove 'workflow' from our active list set.
	active_state_lists_type::iterator active_lists_iter  =
			d_active_state_lists.find(workflow->get_tag());
	if (active_lists_iter != d_active_state_lists.end())
	{
		d_active_state_lists.erase(active_lists_iter);
	}
}


GPlatesAppLogic::FeatureCollectionFileState::file_iterator_seq_iterator_impl_type
GPlatesAppLogic::FeatureCollectionFileState::ActiveListsManager::active_files_begin(
		const active_state_tag_type &tag)
{
	return d_active_state_lists[tag].begin();
}


GPlatesAppLogic::FeatureCollectionFileState::file_iterator_seq_iterator_impl_type
GPlatesAppLogic::FeatureCollectionFileState::ActiveListsManager::active_files_end(
		const active_state_tag_type &tag)
{
	return d_active_state_lists[tag].end();
}


GPlatesAppLogic::FeatureCollectionFileState::file_seq_iterator_impl_type
GPlatesAppLogic::FeatureCollectionFileState::ActiveListsManager::add_file(
		const file_node_type &file_node)
{
	// Add to internal list of files.
	const file_seq_iterator_impl_type file_iter =
			d_loaded_files.insert(
					d_loaded_files.end(),
					// previous active state defaults to empty/inactive
					Impl::FileNodeInfo(file_node));

	// Add to dependent lists depending on changes to the file node active state.
	// The prev active state is set to empty/inactive above so this amounts to
	// creating active lists depending on the active state stored in 'file_node'.
	update_active_state_lists(file_iter);

	return file_iter;
}


void
GPlatesAppLogic::FeatureCollectionFileState::ActiveListsManager::remove_file(
		const file_seq_iterator_impl_type &file_iter)
{
	// Remove from dependent lists depending on changes to the file node active state.
	// NOTE: Just as we set the prev active state in 'add_file' to empty/inactive - we
	// clear the current active state here so that 'update_active_state_lists' will
	// remove all active lists.
	file_iter->file_node.file_node_state().active_state().clear();
	update_active_state_lists(file_iter);

	// Remove from list of loaded files.
	d_loaded_files.erase(file_iter);
}


void
GPlatesAppLogic::FeatureCollectionFileState::ActiveListsManager::update_active_state_lists(
		const file_seq_iterator_impl_type &file_iter)
{
	const file_node_active_state_type &old_file_node_active_state = file_iter->prev_active_state;
	const file_node_active_state_type &new_file_node_active_state =
			file_iter->file_node.file_node_state().active_state();

	visit_union(
			old_file_node_active_state,
			new_file_node_active_state,
			boost::bind(&ActiveListsManager::update_active_state_list_internal,
					this, _1, file_iter, _2, _3));

	// Now that we've updated our active lists we should set the file's previous active state to
	// its current active state.
	file_iter->prev_active_state = new_file_node_active_state;
}


void
GPlatesAppLogic::FeatureCollectionFileState::ActiveListsManager::update_active_state_list_internal(
		const active_state_tag_type &active_list_tag,
		const file_seq_iterator_impl_type &file_iter,
		bool old_active_state,
		bool new_active_state)
{
	// Only interested in active state changes.
	if (old_active_state == new_active_state)
	{
		return;
	}

	// List should currently exist if we're being passed in an active list tag.
	file_iterator_seq_impl_type &active_state_list = d_active_state_lists[active_list_tag];

	// If state was just activated then add to list.
	if (new_active_state && !old_active_state)
	{
		add_to(active_state_list, file_iter);
	}

	// If state was just deactivated then remove from list.
	if (old_active_state && !new_active_state)
	{
		remove_from(active_state_list, file_iter);
	}
}


void
GPlatesAppLogic::FeatureCollectionFileState::ActiveListsManager::add_to(
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
GPlatesAppLogic::FeatureCollectionFileState::ActiveListsManager::remove_from(
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


void
GPlatesAppLogic::FeatureCollectionFileState::WorkflowManager::register_workflow(
		Workflow *workflow)
{
	// Insert workflow into our registered list.
	std::pair<workflow_map_type::iterator, bool> inserted = d_workflow_map.insert(
			workflow_map_type::value_type(workflow->get_tag(), workflow));

	// NOTE: if a workflow instance has already been registered with the same tag then
	// an assertion is raised.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			inserted.second,
			GPLATES_ASSERTION_SOURCE);

	// Sort the workflows according to their priority so when we add a file to a workflow
	// we can tell it if any higher priority workflows are using it.
	const sorted_workflow_seq_type::iterator insert_iter =
			d_sorted_workflow_seq.insert(d_sorted_workflow_seq.end(), workflow);

	// Sort the newly added workflow into the existing ones according to priority.
	std::inplace_merge(
			d_sorted_workflow_seq.begin(),
			insert_iter,
			d_sorted_workflow_seq.end(),
			boost::bind(std::less<Workflow::priority_type>(),
					boost::bind(&Workflow::get_priority, _1),
					boost::bind(&Workflow::get_priority, _2)));
}


void
GPlatesAppLogic::FeatureCollectionFileState::WorkflowManager::unregister_workflow(
		Workflow *workflow)
{
	// Remove 'workflow' from our map.
	workflow_map_type::iterator map_iter  = d_workflow_map.find(workflow->get_tag());
	if (map_iter != d_workflow_map.end())
	{
		d_workflow_map.erase(map_iter);
	}

	// Remove 'workflow' from our priority sorted sequence.
	sorted_workflow_seq_type::iterator sorted_iter = std::find(
			d_sorted_workflow_seq.begin(), d_sorted_workflow_seq.end(), workflow);
	if (sorted_iter != d_sorted_workflow_seq.end())
	{
		d_sorted_workflow_seq.erase(sorted_iter);
	}
}


void
GPlatesAppLogic::FeatureCollectionFileState::WorkflowManager::add_file(
		file_iterator file_iter)
{
	const ClassifyFeatureCollection::classifications_type file_classification =
			file_iter.get_file_node()->file_node_state().feature_collection_classification();

	file_node_active_state_type &file_active_state =
			file_iter.get_file_node()->file_node_state().active_state();

	bool used_by_higher_priority_workflow = false;

	// Iterate over all registered workflows from highest priority to lowest.
	sorted_workflow_seq_type::const_reverse_iterator workflow_iter = d_sorted_workflow_seq.rbegin();
	const sorted_workflow_seq_type::const_reverse_iterator workflow_end = d_sorted_workflow_seq.rend();
	for ( ; workflow_iter != workflow_end; ++workflow_iter)
	{
		Workflow *workflow = *workflow_iter;

		if (workflow->add_file(file_iter, file_classification, used_by_higher_priority_workflow))
		{
			// Get the workflow tag.
			const Workflow::tag_type workflow_tag = workflow->get_tag();

			// Activate the current workflow with the new file by adding its workflow tag
			// to the file's active state and setting it to true.
			file_active_state.add(workflow_tag, true);

			used_by_higher_priority_workflow = true;
		}
	}
}


void
GPlatesAppLogic::FeatureCollectionFileState::WorkflowManager::remove_file(
		file_iterator file_iter)
{
	file_node_active_state_type &file_active_state =
			file_iter.get_file_node()->file_node_state().active_state();

	// Get the workflow tags attached to the file.
	const std::vector<Workflow::tag_type> workflow_tags = file_active_state.get_tags();

	// Iterate through the workflow tags.
	std::vector<Workflow::tag_type>::const_iterator tags_iter = workflow_tags.begin();
	const std::vector<Workflow::tag_type>::const_iterator tags_end = workflow_tags.end();
	for ( ; tags_iter != tags_end; ++tags_iter)
	{
		const Workflow::tag_type &workflow_tag = *tags_iter;

		Workflow *workflow = get_workflow(workflow_tag);

		// Notify each workflow that we're about to remove the file.
		workflow->remove_file(file_iter);

		// Detach the current workflow from the file.
		file_active_state.remove(workflow_tag);
	}
}


void
GPlatesAppLogic::FeatureCollectionFileState::WorkflowManager::changed_file(
		file_iterator file_iter,
		GPlatesFileIO::File &old_file)
{
	const ClassifyFeatureCollection::classifications_type new_file_classification =
			file_iter.get_file_node()->file_node_state().feature_collection_classification();

	file_node_active_state_type &file_active_state =
			file_iter.get_file_node()->file_node_state().active_state();

	bool used_by_higher_priority_workflow = false;

	// Iterate over all registered workflows from highest priority to lowest.
	sorted_workflow_seq_type::const_reverse_iterator workflow_iter = d_sorted_workflow_seq.rbegin();
	const sorted_workflow_seq_type::const_reverse_iterator workflow_end = d_sorted_workflow_seq.rend();
	for ( ; workflow_iter != workflow_end; ++workflow_iter)
	{
		Workflow *workflow = *workflow_iter;
		const Workflow::tag_type &workflow_tag = workflow->get_tag();

		// See if this workflow tag matches a workflow that was previously interested
		// in 'file_iter'.
		if (file_active_state.exists(workflow_tag))
		{
			// The current workflow was previously interested in this file.
			// Notify workflow that file is about to be changed and see if it's still interested.
			if (workflow->changed_file(file_iter, old_file, new_file_classification))
			{
				used_by_higher_priority_workflow = true;
			}
			else
			{
				// Detach this workflow from the file.
				file_active_state.remove(workflow_tag);
			}
		}
		else
		{
			// The current workflow was *not* previously interested in this file.
			// But since the file has changed maybe they will be now - let's ask them.
			if (workflow->add_file(
					file_iter, new_file_classification, used_by_higher_priority_workflow))
			{
				// Activate the current workflow with the new file by adding its workflow tag
				// to the file's active state and setting it to true.
				file_active_state.add(workflow_tag, true);

				used_by_higher_priority_workflow = true;
			}
		}
	}
}


void
GPlatesAppLogic::FeatureCollectionFileState::WorkflowManager::set_active(
		file_iterator file_iter,
		const Workflow::tag_type &workflow_tag,
		bool activate)
{
	Workflow *workflow = get_workflow(workflow_tag);

	// Activate the current workflow with the file.
	file_iter.get_file_node()->file_node_state().active_state().set_active(workflow_tag, activate);

	// Notify the workflow.
	workflow->set_file_active(file_iter, activate);
}


GPlatesAppLogic::FeatureCollectionFileState::Workflow *
GPlatesAppLogic::FeatureCollectionFileState::WorkflowManager::get_workflow(
		const Workflow::tag_type &workflow_tag)
{
	const workflow_map_type::iterator workflow_iter = d_workflow_map.find(workflow_tag);

	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			workflow_iter != d_workflow_map.end(),
			GPLATES_ASSERTION_SOURCE);

	Workflow *workflow = workflow_iter->second;

	return workflow;
}
