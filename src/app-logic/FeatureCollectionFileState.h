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
#include <map>
#include <vector>
#include <boost/cstdint.hpp>
#include <boost/integer_traits.hpp>
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
#include <QObject>
#include <QString>

#include "FeatureCollectionFileStateImpl.h"

#include "app-logic/ClassifyFeatureCollection.h"

#include "file-io/File.h"


namespace GPlatesAppLogic
{
	namespace FeatureCollectionFileStateImpl
	{
		class ReconstructableWorkflow;
		class ReconstructionWorkflow;
	}


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
		 * A [begin,end) range of bidirectional iteration over sequence of files.
		 * Iterators dereference to 'GPlatesFileIO::File &'.
		 */
		template <class IteratorType>
		struct IteratorRange
		{
			IteratorRange(
					IteratorType begin_,
					IteratorType end_) :
				begin(begin_),
				end(end_)
			{  }

			IteratorType begin; //!< The begin iterator in iterator range [begin,end)
			IteratorType end; //!< The end iterator in iterator range [begin,end)
		};

		/**
		 * Typedef for iterator over loaded files.
		 */
		typedef IteratorRange<file_iterator> file_iterator_range;

		/**
		 * Typedef for iterator over files used for other purposes
		 * such as active reconstructable files.
		 */
		typedef IteratorRange<active_file_iterator> active_file_iterator_range;


		/**
		 * Used to be notified of file loads, file unloads, file changes and active status
		 * changes to a file.
		 *
		 * This is a more direct approach that using the signals emitted by
		 * @a FeatureCollectionFileState and allows the workflow to return values to the caller.
		 */
		class Workflow :
				private boost::noncopyable
		{
		public:
			//! Typedef for the tag type used to identify specific workflow instances.
			typedef QString tag_type;

			/**
			 * Typedef for priority of a workflow.
			 *
			 * NOTE: Only use positive values (and zero) for your priorities as negative values
			 * are reserved for some internal workflows to allow them to get even
			 * lower priorities than the client workflows.
			 */
			typedef boost::int32_t priority_type;

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

			virtual
			~Workflow()
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
					file_iterator file_iter,
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
					file_iterator file_iter) = 0;

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
					file_iterator file_iter,
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
					file_iterator file_iter,
					bool active = true) = 0;

		protected:
			//! Constructor does not register_workflow - derived class must call @a register_workflow explicitly.
			Workflow();

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


		//! Constructor.
		FeatureCollectionFileState();

		//! Destructor.
		~FeatureCollectionFileState();


		/**
		 * Returns an iteration range over all currently loaded files.
		 * The iterators dereference to 'GPlatesFileIO::File &'.
		 * Returned iterator type can be used in methods to add, remove files, etc.
		 */
		file_iterator_range
		loaded_files();


		/**
		 * Returns an iteration range over all currently active reconstructable files.
		 * The iterators dereference to 'GPlatesFileIO::File &'.
		 */
		active_file_iterator_range
		get_active_reconstructable_files();


		/**
		 * Returns an iteration range over all currently active reconstruction files.
		 * The iterators dereference to 'GPlatesFileIO::File &'.
		 */
		active_file_iterator_range
		get_active_reconstruction_files();


		/**
		 * Returns an iteration range over all currently active workflow files
		 * identified by the tag @a workflow_tag.
		 *
		 * The iterators dereference to 'GPlatesFileIO::File &'.
		 */
		active_file_iterator_range
		get_active_workflow_files(
				const Workflow::tag_type &workflow_tag);


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
		 * You do not need to call this explicitly if your class derived from @a Workflow
		 * calls the @a Workflow 'register_workflow' method.
		 *
		 * NOTE: It is an error to register multiple @a Workflow instances that have the same tag.
		 * An assertion failure exception is thrown if this happens.
		 */
		void
		register_workflow(
				Workflow *workflow);


		/**
		 * Unregisters @a workflow from 'this' file state.
		 *
		 * You do not need to call this explicitly if your class derived from @a Workflow
		 * calls the @a Workflow 'register_workflow' method.
		 */
		void
		unregister_workflow(
				Workflow *workflow);


		/**
		 * Returns a list of workflow tags that have been attached to file @a file_iter.
		 * This does not include the implementation defined internal tags for the
		 * reconstructable and reconstruction workflows.
		 */
		std::vector<Workflow::tag_type>
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
				const Workflow::tag_type &workflow_tag,
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
				const Workflow::tag_type &workflow_tag) const;


		/**
		 * Tests if the @a file_iter is active and being used for the workflow identified
		 * by @a workflow_tag.
		 */
		bool
		is_file_active_workflow(
				file_iterator file_iter,
				const Workflow::tag_type &workflow_tag) const;

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
		end_reset_feature_collection(
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
				const GPlatesAppLogic::FeatureCollectionFileState::Workflow::tag_type &workflow_tag);


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
				const GPlatesAppLogic::FeatureCollectionFileState::Workflow::tag_type &workflow_tag,
				bool activation);


	private:
		//! Typedef for the feature collection file node.
		typedef FeatureCollectionFileStateImpl::FileNode file_node_type;

		//! Typedef for the feature collection file state node.
		typedef FeatureCollectionFileStateImpl::FileNodeState file_node_state_type;

		//! Typedef for the feature collection file state active node.
		typedef FeatureCollectionFileStateImpl::FileNodeActiveState file_node_active_state_type;

		//! Typedef for the type used to identify a particular active state.
		typedef FeatureCollectionFileStateImpl::active_state_tag_type active_state_tag_type;

		//! Typedef for a sequence of files.
		typedef FeatureCollectionFileStateImpl::file_seq_impl_type file_seq_impl_type;
		typedef file_seq_impl_type::const_iterator file_seq_const_iterator_impl_type;
		typedef file_seq_impl_type::iterator file_seq_iterator_impl_type;

		//! Typedef for a sequence of file iterators.
		typedef FeatureCollectionFileStateImpl::file_iterator_seq_impl_type file_iterator_seq_impl_type;
		typedef file_iterator_seq_impl_type::const_iterator file_iterator_seq_const_iterator_impl_type;
		typedef file_iterator_seq_impl_type::iterator file_iterator_seq_iterator_impl_type;


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
					Workflow *workflow);

			/**
			 * Unregisters @a workflow.
			 */
			void
			unregister_workflow(
					Workflow *workflow);

			file_seq_iterator_impl_type
			add_file(
					const file_node_type &file_node);

			void
			remove_file(
					const file_seq_iterator_impl_type &file_iter);

			void
			update_active_state_lists(
					const file_seq_iterator_impl_type &file_iter);


			file_seq_iterator_impl_type
			loaded_files_begin()
			{
				return d_loaded_files.begin();
			}

			file_seq_iterator_impl_type
			loaded_files_end()
			{
				return d_loaded_files.end();
			}

			file_iterator_seq_iterator_impl_type
			active_files_begin(
					const active_state_tag_type &tag);

			file_iterator_seq_iterator_impl_type
			active_files_end(
					const active_state_tag_type &tag);

		private:
			//! The sequence of all loaded files.
			file_seq_impl_type d_loaded_files;

			//! Typedef to map tags to active state lists.
			typedef std::map<active_state_tag_type, file_iterator_seq_impl_type>
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

			void
			update_active_state_list_internal(
					const active_state_tag_type &active_state_tag,
					const file_seq_iterator_impl_type &file_iter,
					bool old_active_state,
					bool new_active_state);
		};


		/**
		 * Manages workflows and notifying them.
		 */
		class WorkflowManager
		{
		public:
			/**
			 * Registers @a workflow.
			 * Multiple workflow instances having the same tag are *not* allowed.
			 */
			void
			register_workflow(
					Workflow *workflow);

			/**
			 * Unregisters @a workflow.
			 */
			void
			unregister_workflow(
					Workflow *workflow);

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
					file_iterator file_iter);

			/**
			 * Notifies workflows attached to @a file_iter that the file is being removed.
			 * Also detaches all workflows from the file.
			 */
			void
			remove_file(
					file_iterator file_iter);

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
					file_iterator file_iter,
					GPlatesFileIO::File &old_file);

			/**
			 * Notifies the workflow identified by @a workflow_tag that the file @a file_iter
			 * has been activated/deactivated.
			 * 
			 */
			void
			set_active(
					file_iterator file_iter,
					const Workflow::tag_type &workflow_tag,
					bool activate = true);

		private:
			//! Typedef for a mapping of workflow tags to @a Workflow pointers.
			typedef std::map<Workflow::tag_type, Workflow *> workflow_map_type;

			//! Typedef for a sequence workflows sorted by priority.
			typedef std::vector<Workflow *> sorted_workflow_seq_type;

			//! Typedef for a sequence of workflow tags.
			typedef std::vector<Workflow::tag_type> workflow_tag_seq_type;

			//! Used to keep track of our registered @a Workflow objects.
			workflow_map_type d_workflow_map;

			//! Used to keep track of workflows sorted by their priorities.
			sorted_workflow_seq_type d_sorted_workflow_seq;


			//! Returns the workflow matching the tag @a workflow_tag.
			Workflow *
			get_workflow(
					const Workflow::tag_type &workflow_tag);
		};


		ActiveListsManager d_active_lists_manager;
		WorkflowManager d_workflow_manager;

		// NOTE: these should be declared last since they register with 'this'
		// which requires its state to be initialised.
		boost::scoped_ptr<FeatureCollectionFileStateImpl::ReconstructableWorkflow> d_reconstructable_workflow;
		boost::scoped_ptr<FeatureCollectionFileStateImpl::ReconstructionWorkflow> d_reconstruction_workflow;


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
				const Workflow::tag_type &workflow_tag,
				bool activate);


		/**
		 * Emits @a reconstructable_file_activation and @a reconstruction_file_activation
		 * signals if the active state of @a file_iter has changed.
		 */
		void
		emit_activation_signals(
				file_iterator file_iter,
				const file_node_active_state_type &old_file_node_active_state,
				const file_node_active_state_type &new_file_node_active_state);


		void
		emit_activation_signal(
				file_iterator file_iter,
				const active_state_tag_type &active_list_tag,
				bool old_active_state,
				bool new_active_state);
	};
}

#endif // GPLATES_APP_LOGIC_FEATURECOLLECTIONFILESTATE_H
