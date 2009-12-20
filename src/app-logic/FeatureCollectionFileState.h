/* $Id$ */

/**
 * @file 
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2007, 2009 The University of Sydney, Australia
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

#ifndef GPLATES_APP_LOGIC_FEATURECOLLECTIONFILESTATE_H
#define GPLATES_APP_LOGIC_FEATURECOLLECTIONFILESTATE_H

#include <list>
#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
#include <QObject>

#include "FeatureCollectionFileStateDecls.h"
#include "FeatureCollectionFileStateImplDecls.h"

#include "app-logic/ClassifyFeatureCollection.h"

#include "file-io/File.h"


namespace GPlatesAppLogic
{
	namespace FeatureCollectionFileStateImpl
	{
		class ActiveListsManager;
		class ActivationStateManager;
		class FileNode;
		class FileNodeActiveState;
		class FileNodeState;
		class ReconstructableWorkflow;
		class ReconstructionWorkflow;
		class WorkflowManager;
	}

	class FeatureCollectionActivationStrategy;
	class FeatureCollectionWorkflow;


	/**
	 * Holds information associated with the currently loaded and active feature collection files.
	 */
	class FeatureCollectionFileState :
			public QObject,
			private boost::noncopyable
	{
		Q_OBJECT

	public:
		/**
		 * Typedef for the bidirectional iterator over loaded files.
		 *
		 * The iterator deferences to 'GPlatesFileIO::File &'.
		 *
		 * NOTE: this iterator can be used to modify a @a GPlatesFileIO::File object
		 * but it cannot modify the GPlatesFileIO::shared_ref's stored internally and hence
		 * cannot affect when the feature collections are unloaded.
		 */
		typedef FeatureCollectionFileStateImpl::file_iterator_type file_iterator;

		/**
		 * Typedef for the bidirectional iterator over a sequence of active files for a specific
		 * workflow (eg, active reconstructable files).
		 *
		 * This is a different type than @a file_iterator. A @a file_iterator is valid as
		 * long as the file it points to is currently loaded. However a @a active_file_iterator
		 * is only valid when it the file is active for that particular workflow
		 * (eg, active reconstructable files). As a result it is not passed as arguments to
		 * methods of this class - @a file_iterator is used for that instead.
		 * @a active_file_iterator should be used purely for localised iteration over an
		 * active group of files and then discarded.
		 * 
		 * The iterator deferences to 'GPlatesFileIO::File &'.
		 *
		 * NOTE: the iterator can be used to modify a @a GPlatesFileIO::File object
		 * but it cannot modify the GPlatesFileIO::shared_ref's stored internally and hence
		 * cannot affect when the feature collections are unloaded.
		 */
		typedef FeatureCollectionFileStateImpl::active_file_iterator_type active_file_iterator;


		/**
		 * Typedef for iterator over loaded files.
		 *
		 * Use the data members file_iterator_range::begin and
		 * file_iterator_range::end to traverse the iteration range.
		 */
		typedef FeatureCollectionFileStateImpl::file_iterator_range_type
				file_iterator_range;

		/**
		 * Typedef for iterator over files used for other purposes
		 * such as active reconstructable files.
		 *
		 * Use the data members active_file_iterator_range::begin and
		 * active_file_iterator_range::end to traverse the iteration range.
		 */
		typedef FeatureCollectionFileStateImpl::active_file_iterator_range_type
				active_file_iterator_range;


		/**
		 * Typedef for the tag used to identify workflows.
		 */
		typedef FeatureCollectionFileStateDecls::workflow_tag_type workflow_tag_type;

		//! Typedef for a sequence of registered workflows.
		typedef std::list<workflow_tag_type> workflow_tag_seq_type;


		//! Constructor.
		FeatureCollectionFileState();

		//! Destructor.
		~FeatureCollectionFileState();


		/**
		 * Returns an iteration range over all currently loaded files.
		 *
		 * The iterators dereference to 'GPlatesFileIO::File &'.
		 *
		 * Returned iterator type can be used in methods to add, remove files, etc.
		 *
		 * Use the data members file_iterator_range::begin and
		 * file_iterator_range::end to traverse the iteration range.
		 */
		file_iterator_range
		get_loaded_files();


		/**
		 * Finds the list of files that are active with any workflows in @a workflow_tags and
		 * appends the results to @a active_files.
		 *
		 * @a include_reconstructable_workflow and @a include_reconstruction_workflow
		 * are currently used because the user currently has no way of getting at their
		 * workflow tags. This will be fixed soon when all workflows are treated equally.
		 */
		void
		get_active_files(
				std::vector<file_iterator> &active_files,
				const workflow_tag_seq_type &workflow_tags,
				bool include_reconstructable_workflow = false,
				bool include_reconstruction_workflow = false);


		/**
		 * Returns an iteration range over all currently active reconstructable files.
		 *
		 * The iterators dereference to 'GPlatesFileIO::File &'.
		 *
		 * Use the data members active_file_iterator_range::begin and
		 * active_file_iterator_range::end to traverse the iteration range.
		 */
		active_file_iterator_range
		get_active_reconstructable_files();


		/**
		 * Returns an iteration range over all currently active reconstruction files.
		 *
		 * The iterators dereference to 'GPlatesFileIO::File &'.
		 *
		 * Use the data members active_file_iterator_range::begin and
		 * active_file_iterator_range::end to traverse the iteration range.
		 */
		active_file_iterator_range
		get_active_reconstruction_files();


		/**
		 * Returns an iteration range over all currently active workflow files
		 * identified by the tag @a workflow_tag.
		 *
		 * The iterators dereference to 'GPlatesFileIO::File &'.
		 *
		 * Use the data members active_file_iterator_range::begin and
		 * active_file_iterator_range::end to traverse the iteration range.
		 */
		active_file_iterator_range
		get_active_workflow_files(
				const workflow_tag_type &workflow_tag);


		/**
		 * Converts an iterator of type @a active_file_iterator to an iterator
		 * of type @a file_iterator so it can be passed to other methods of this class.
		 *
		 * A @a file_iterator is more stable in that it remains valid as long as the file
		 * it points to is loaded (ie, hasn't been removed with @a remove_file) whereas
		 * a @a active_file_iterator is only valid as long as the file it's pointing to
		 * is active for the particular workflow it was obtained from
		 * (eg, @a get_active_reconstructable_files).
		 *
		 * Converting to a @a file_iterator makes the iterator more stable and means
		 * it can be passed to other methods of this class.
		 */
		static
		file_iterator
		convert_to_file_iterator(
				active_file_iterator);


		/**
		 * Registers @a workflow to be notified whenever a new feature collection is loaded
		 * to see if it is interested in using it.
		 * 
		 * Also registers the initial activation strategy to attach to the workflow.
		 * If @a activation_strategy is NULL then the default strategy is used which
		 * activates all files added to the workflow.
		 *
		 * You do not need to call this explicitly if your class derived from
		 * @a FeatureCollectionWorkflow calls the @a FeatureCollectionWorkflow
		 * 'register_workflow' method.
		 *
		 * NOTE: It is an error to register multiple @a FeatureCollectionWorkflow instances
		 * that have the same tag.
		 * An assertion failure exception is thrown if this happens.
		 */
		void
		register_workflow(
				FeatureCollectionWorkflow *workflow,
				FeatureCollectionActivationStrategy *activation_strategy = NULL);


		/**
		 * Unregisters @a workflow from 'this' file state.
		 *
		 * You do not need to call this explicitly if your class derived from @a FeatureCollectionWorkflow
		 * calls the @a FeatureCollectionWorkflow 'register_workflow' method.
		 */
		void
		unregister_workflow(
				FeatureCollectionWorkflow *workflow);

		/**
		 * Returns a list of currently registered workflow tags.
		 */
		const workflow_tag_seq_type &
		get_registered_workflow_tags() const;


		/**
		 * Sets the activation strategy attached to the reconstructable workflow.
		 */
		void
		set_reconstructable_activation_strategy(
				FeatureCollectionActivationStrategy *activation_strategy);


		/**
		 * Sets the activation strategy attached to the reconstruction workflow.
		 */
		void
		set_reconstruction_activation_strategy(
				FeatureCollectionActivationStrategy *activation_strategy);


		/**
		 * Sets the activation strategy attached to the workflow identified by @a workflow_tag.
		 */
		void
		set_workflow_activation_strategy(
				FeatureCollectionActivationStrategy *activation_strategy,
				const workflow_tag_type &workflow_tag);


		/**
		 * Returns a list of workflow tags that have been attached to file @a file_iter.
		 * This does not include the implementation defined internal tags for the
		 * reconstructable and reconstruction workflows.
		 */
		std::vector<workflow_tag_type>
		get_workflow_tags(
				file_iterator file_iter) const;


		/**
		 * Adds multiple feature collection files and activates them as reconstructable
		 * and/or reconstruction files depending on the types of features in them.
		 *
		 * Emits signal @a end_add_feature_collections and @a file_state_changed
		 * just before returning.
		 * Also emits @a reconstructable_file_activation or @a reconstruction_file_activation
		 * signals as each new file is added.
		 *
		 * Returns an iteration range over the newly added files.
		 * The iterators dereference to 'GPlatesFileIO::File &'.
		 *
		 * Use the data members file_iterator_range::begin and
		 * file_iterator_range::end to traverse the iteration range.
		 */
		file_iterator_range
		add_files(
				const std::vector<GPlatesFileIO::File::shared_ref> &files);


		/**
		 * Add a file so the application can see what's loaded.
		 *
		 * Emits signal @a end_add_feature_collections and @a file_state_changed
		 * just before returning.
		 * Also emits @a reconstructable_file_activation or @a reconstruction_file_activation
		 * signal when new file is added.
		 */
		file_iterator
		add_file(
				const GPlatesFileIO::File::shared_ref &file);


		/**
		 * Remove @a file from the collection of loaded files known to the application.
		 *
		 * Emits signal @a begin_remove_feature_collection just before removing.
		 * Emits signal @a end_remove_feature_collection and @a file_state_changed
		 * just after removed.
		 * Also emits @a reconstructable_file_activation or @a reconstruction_file_activation
		 * signal if file was active.
		 *
		 * NOTE: If there are no other @a File references to this loaded file then
		 * it will automatically have its feature collection unloaded.
		 * Even if there are other @a File references keeping it alive then
		 * the signals are still emitted. This is ok since some other code may still
		 * be using the file for its own secret purposes but as far as the application
		 * in general is concerned the file and its feature collection are no longer known.
		 */
		void
		remove_file(
				file_iterator file);


		/**
		 * Changes the file referenced by @a file_iter to be @a file.
		 *
		 * This effectively removes the previous file referenced by @a file_iter
		 * replacing it with @a file.
		 *
		 * Emits signal @a begin_reset_feature_collection just before reseting file.
		 * Emits signal @a end_reset_feature_collection and @a file_state_changed
		 * just before returning.
		 * Also emits @a reconstructable_file_activation or @a reconstruction_file_activation
		 * signal if file was previously active but becomes inactive due to new
		 * feature collection not having any reconstructable or reconstruction features.
		 */
		void
		reset_file(
				file_iterator file_iter,
				const GPlatesFileIO::File::shared_ref &new_file);


		/**
		 * Reclassify the feature collection referenced inside @a file_iter.
		 *
		 * This is not normally necessary but can be required when the feature collection
		 * is modified.
		 *
		 * Emits signal @a begin_reclassify_feature_collection just before reclassifying.
		 * Emits signal @a end_reclassify_feature_collection and @a file_state_changed
		 * just before returning.
		 * Also emits @a reconstructable_file_activation or @a reconstruction_file_activation
		 * signal if file was previously active but becomes inactive due to feature collection
		 * not having any reconstructable or reconstruction features anymore.
		 */
		void
		reclassify_feature_collection(
				file_iterator file_iter);


		/**
		 * Returns the current feature collection classification of @a file_iter.
		 */
		ClassifyFeatureCollection::classifications_type
		get_feature_collection_classification(
				file_iterator file_iter) const;


		/**
		 * Activates or deactivates @a file_iter as a reconstructable file.
		 *
		 * This does not remove the file.
		 *
		 * The file will only be activated if it contains reconstructable features.
		 *
		 * Emits signal @a end_set_file_active_reconstructable and @a file_state_changed
		 * just before returning.
		 * Also emits @a reconstructable_file_activation signal if active state changed.
		 */
		void
		set_file_active_reconstructable(
				file_iterator file_iter,
				bool activate = true);


		/**
		 * Activates or deactivates @a file_iter as a reconstruction file.
		 *
		 * This does not remove the file.
		 *
		 * The file will only be activated if it contains reconstruction features.
		 *
		 * Emits signal @a end_set_file_active_reconstruction and @a file_state_changed
		 * just before returning.
		 * Also emits @a reconstruction_file_activation signal if active state changed.
		 */
		void
		set_file_active_reconstruction(
				file_iterator file_iter,
				bool activate = true);


		/**
		 * Activates or deactivates @a file_iter as a file for the workflow identified
		 * by @a workflow_tag.
		 *
		 * This does not remove the file.
		 *
		 * The file will only be activated if @a workflow_tag is in the list of workflow
		 * tags returned by @a get_workflow_tags (with @a file_iter as its argument).
		 *
		 * Emits signal @a end_set_file_active_workflow and @a file_state_changed
		 * just before returning.
		 * Also emits @a workflow_file_activation signal if active state changed.
		 */
		void
		set_file_active_workflow(
				file_iterator file_iter,
				const workflow_tag_type &workflow_tag,
				bool activate = true);


		/**
		 * Tests if the @a file_iter is being used for reconstructable feature data.
		 *
		 * This can return false even if the file contains reconstructable features.
		 * This can happen if the file was claimed by another workflow in which case
		 * no RFGs are generated from the reconstructable features.
		 */
		bool
		is_reconstructable_workflow_using_file(
				file_iterator file_iter) const;


		/**
		 * Tests if the @a file_iter is active and being used for reconstructable
		 * feature data.
		 */
		bool
		is_file_active_reconstructable(
				file_iterator file_iter) const;


		/**
		 * Tests if the @a file_iter is being used for reconstruction feature data.
		 */
		bool
		is_reconstruction_workflow_using_file(
				file_iterator file_iter) const;


		/**
		 * Tests if the @a file_iter is active and being used for reconstruction
		 * tree data.
		 */
		bool
		is_file_active_reconstruction(
				file_iterator file_iter) const;

		/**
		 * Tests if the @a file_iter is being used for the workflow identified
		 * by @a workflow_tag.
		 */
		bool
		is_file_using_workflow(
				file_iterator file_iter,
				const workflow_tag_type &workflow_tag) const;


		/**
		 * Tests if the @a file_iter is active and being used for the workflow identified
		 * by @a workflow_tag.
		 */
		bool
		is_file_active_workflow(
				file_iterator file_iter,
				const workflow_tag_type &workflow_tag) const;

	signals:
		// NOTE: all signals/slots should use namespace scope for all arguments
		//       otherwise differences between signals and slots will cause Qt
		//       to not be able to connect them at runtime.

		//
		// The following signals only occur at the end (and in some cases also the beginning)
		// of a public method of this class. These signals can be used to perform
		// a reconstruction with the knowledge that they are the last signal emitted
		// from a public method (and hence all state changes and other signals have been made).
		//

		void
		end_add_feature_collections(
				GPlatesAppLogic::FeatureCollectionFileState &file_state,
				GPlatesAppLogic::FeatureCollectionFileState::file_iterator new_files_begin,
				GPlatesAppLogic::FeatureCollectionFileState::file_iterator new_files_end);

		void
		begin_remove_feature_collection(
				GPlatesAppLogic::FeatureCollectionFileState &file_state,
				GPlatesAppLogic::FeatureCollectionFileState::file_iterator file);

		void
		end_remove_feature_collection(
				GPlatesAppLogic::FeatureCollectionFileState &file_state);

		void
		begin_reset_feature_collection(
				GPlatesAppLogic::FeatureCollectionFileState &file_state,
				GPlatesAppLogic::FeatureCollectionFileState::file_iterator file);

		void
		end_reset_feature_collection(
				GPlatesAppLogic::FeatureCollectionFileState &file_state,
				GPlatesAppLogic::FeatureCollectionFileState::file_iterator file);

		void
		begin_reclassify_feature_collection(
				GPlatesAppLogic::FeatureCollectionFileState &file_state,
				GPlatesAppLogic::FeatureCollectionFileState::file_iterator file);

		void
		end_reclassify_feature_collection(
				GPlatesAppLogic::FeatureCollectionFileState &file_state,
				GPlatesAppLogic::FeatureCollectionFileState::file_iterator file);

		void
		end_set_file_active_reconstructable(
				GPlatesAppLogic::FeatureCollectionFileState &file_state,
				GPlatesAppLogic::FeatureCollectionFileState::file_iterator file);

		void
		end_set_file_active_reconstruction(
				GPlatesAppLogic::FeatureCollectionFileState &file_state,
				GPlatesAppLogic::FeatureCollectionFileState::file_iterator file);

		void
		end_set_file_active_workflow(
				GPlatesAppLogic::FeatureCollectionFileState &file_state,
				GPlatesAppLogic::FeatureCollectionFileState::file_iterator file,
				const GPlatesAppLogic::FeatureCollectionFileState::workflow_tag_type &workflow_tag);


		//
		// An alternative to the above signals is to listen to only one signal
		// when all you are interested in is knowing that the state has been modified.
		// This is useful for performing reconstructions.
		// This signal is emitted at the end of every public method of this class.
		//

		void
		file_state_changed(
				GPlatesAppLogic::FeatureCollectionFileState &file_state);


		//
		// The following signals can be generated at any time inside the methods
		// of this class and occur whenever a file is activated or deactivated.
		//

		void
		reconstructable_file_activation(
				GPlatesAppLogic::FeatureCollectionFileState &file_state,
				GPlatesAppLogic::FeatureCollectionFileState::file_iterator file,
				bool activation);

		void
		reconstruction_file_activation(
				GPlatesAppLogic::FeatureCollectionFileState &file_state,
				GPlatesAppLogic::FeatureCollectionFileState::file_iterator file,
				bool activation);

		void
		workflow_file_activation(
				GPlatesAppLogic::FeatureCollectionFileState &file_state,
				GPlatesAppLogic::FeatureCollectionFileState::file_iterator file,
				const GPlatesAppLogic::FeatureCollectionFileState::workflow_tag_type &workflow_tag,
				bool activation);


	private:
		//! Typedef for the feature collection file node.
		typedef FeatureCollectionFileStateImpl::FileNode file_node_type;

		//! Typedef for the feature collection file state node.
		typedef FeatureCollectionFileStateImpl::FileNodeState file_node_state_type;

		//! Typedef for the feature collection file state active node.
		typedef FeatureCollectionFileStateImpl::FileNodeActiveState file_node_active_state_type;

		//! Typedef for the type that keeps track of any file activation changes.
		typedef FeatureCollectionFileStateImpl::ActivationStateManager activation_state_manager_type;

		//! Typedef for a sequence of files.
		typedef FeatureCollectionFileStateImpl::file_seq_impl_type file_seq_impl_type;
		typedef file_seq_impl_type::iterator file_seq_iterator_impl_type;

		//! Typedef for a sequence of file iterators.
		typedef FeatureCollectionFileStateImpl::file_iterator_seq_impl_type file_iterator_seq_impl_type;
		typedef file_iterator_seq_impl_type::iterator file_iterator_seq_iterator_impl_type;


		//! The sequence of all currently loaded files.
		file_seq_impl_type d_loaded_files;

		//! The currently registered workflows.
		workflow_tag_seq_type d_registered_workflow_seq;

		//! Manages the active files lists for each workflow.
		boost::scoped_ptr<FeatureCollectionFileStateImpl::ActiveListsManager> d_active_lists_manager;

		/**
		 * Manages the workflows and their activation strategies and handles notification
		 * of workflows (eg, file adds, removes, changes, etc).
		 */
		boost::scoped_ptr<FeatureCollectionFileStateImpl::WorkflowManager> d_workflow_manager;

		//! The default behaviour for activating files for all workflows.
		boost::scoped_ptr<FeatureCollectionActivationStrategy> d_default_activation_strategy;

		// NOTE: these should be declared last since they register with 'this'
		// which requires its state to be initialised.

		/**
		 * The workflow for handling files with reconstructable features.
		 */
		boost::scoped_ptr<FeatureCollectionFileStateImpl::ReconstructableWorkflow>
				d_reconstructable_workflow;

		/**
		 * The workflow for handling files with reconstruction features.
		 */
		boost::scoped_ptr<FeatureCollectionFileStateImpl::ReconstructionWorkflow>
				d_reconstruction_workflow;


		file_iterator
		add_file_internal(
				const GPlatesFileIO::File::shared_ref &file_to_add);


		void
		change_file_internal(
				file_iterator file_iter,
				const GPlatesFileIO::File::shared_ref &new_file);


		bool
		set_file_active_workflow_internal(
				file_iterator file_iter,
				const workflow_tag_type &workflow_tag,
				bool activate);


		/**
		 * Emits activation signals for all workflow files whose activation state has changed.
		 */
		void
		emit_activation_signals(
				const activation_state_manager_type &activation_state_manager);


		void
		emit_activation_signal(
				file_iterator file_iter,
				const workflow_tag_type &workflow_tag);
	};
}

#endif // GPLATES_APP_LOGIC_FEATURECOLLECTIONFILESTATE_H
