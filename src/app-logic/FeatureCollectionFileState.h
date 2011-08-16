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

#include <cstddef> // For std::size_t
#include <utility>
#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/operators.hpp>
#include <boost/shared_ptr.hpp>
#include <QObject>

#include "file-io/FeatureCollectionFileFormat.h"
#include "file-io/File.h"

#include "model/FeatureCollectionHandle.h"
#include "model/ModelInterface.h"
#include "model/WeakReferenceCallback.h"


namespace GPlatesAppLogic
{
	/**
	 * Holds information associated with the currently loaded and active feature collection files.
	 */
	class FeatureCollectionFileState :
			public QObject,
			private boost::noncopyable
	{
		Q_OBJECT

	private:
		//! Typedef for a file handle.
		typedef std::size_t file_handle_type;

	public:
		/**
		 * Typedef for an index defining the order of currently loaded files.
		 *
		 * The least recently loaded file will have index zero and most recently loaded file
		 * will have the highest value index.
		 *
		 * The file indices of all currently loaded files always start at zero and are always
		 * contiguous (no gaps) even as files are randomly unloaded.
		 */
		typedef std::size_t file_index_type;


		/**
		 * A reference to a file loaded into @a FeatureCollectionFileState.
		 */
		class FileReference :
				public boost::equality_comparable<FileReference>
		{
		public:
			//! Constructor used by implementation only.
			FileReference(
					FeatureCollectionFileState &file_state,
					file_handle_type file_handle) :
				d_file_state(&file_state),
				d_file_handle(file_handle)
			{  }

			/**
			 * Returns the file object referenced.
			 */
			const GPlatesFileIO::File::Reference &
			get_file() const
			{
				return d_file_state->get_file(d_file_handle);
			}

			/**
			 * Returns the index of this file in the sequence of currently loaded files.
			 *
			 * The order of file indices is the order in which files were loaded.
			 *
			 * NOTE: Provided 'this' references a currently loaded file, the file index returned
			 * will always be up-to-date even after files that were loaded before this file
			 * are unloaded. For example, if a file is loaded before this file, and then this file
			 * is loaded (giving this file index 1), then if the first file is unloaded then this
			 * file will now return an index of 0 indicating it's the first of all currently
			 * loaded files.
			 */
			file_index_type
			get_file_index() const
			{
				return d_file_state->get_file_index(d_file_handle);
			}

			/**
			 * Changes the file information to @a new_file_info and the file configuration to
			 * @a new_file_configuration (such as changed read/write options).
			 *
			 * @a FeatureCollectionFileState emits signals @a file_state_file_info_changed and
			 * @a file_state_changed after setting the file info.
			 */
			void
			set_file_info(
					const GPlatesFileIO::FileInfo &new_file_info,
					boost::optional<GPlatesFileIO::FeatureCollectionFileFormat::Configuration::shared_ptr_to_const_type>
							new_file_configuration = boost::none)
			{
				d_file_state->set_file_info(d_file_handle, new_file_info, new_file_configuration);
			}

			/**
			 * Returns the @a FeatureCollectionFileState that we belong to.
			 */
			FeatureCollectionFileState &
			get_file_state() const
			{
				return *d_file_state;
			}


			/**
			 * Equality comparison.
			 * Inequality operator provided by base class boost::equality_comparable.
			 */
			friend
			bool
			operator==(
					const FileReference &lhs,
					const FileReference &rhs)
			{
				// Compare file handles.
				return lhs.d_file_handle == rhs.d_file_handle;
			}

			/**
			 * Less than comparison - useful for std::map.
			 */
			friend
			bool
			operator<(
					const FileReference &lhs,
					const FileReference &rhs)
			{
				// Compare file handles.
				return lhs.d_file_handle < rhs.d_file_handle;
			}

			//! Used by implementation only.
			file_handle_type
			get_file_handle() const
			{
				return d_file_handle;
			}

		private:
			FeatureCollectionFileState *d_file_state;
			file_handle_type d_file_handle;
		};


		/**
		 * Typedef for a reference to a loaded file.
		 *
		 * The reference deferences to 'GPlatesFileIO::File &'.
		 */
		typedef FileReference file_reference;


		//! Constructor.
		FeatureCollectionFileState(
				GPlatesModel::ModelInterface &model);

		//! Destructor.
		~FeatureCollectionFileState();


		/**
		 * Returns a sequence of file reference to all currently loaded files.
		 *
		 * The returned sequence is ordered by file index and the file indices are always
		 * contiguous (no gaps) starting at index zero.
		 * So if you have three files currently loaded they will have indices 0,1,2 and
		 * index 0 will be at the front of the returned vector and index 2 will be at the back.
		 *
		 * The returned sequence is ordered by file index (that is the order in which
		 * the files were first loaded).
		 */
		std::vector<file_reference>
		get_loaded_files();


		/**
		 * Adds multiple feature collection files and activates them.
		 *
		 * The returned sequence is ordered by file index.
		 * So if you have three files previously loaded they will have indices 0,1,2 and
		 * now you're adding three more files then they will have indices 3,4,5 and
		 * index 3 will be at the front of the returned vector (and the vector passed by the
		 * @a file_state_files_added signal) and index 5 will be at the back.
		 *
		 * Because of this you can simply iterate through the returned sequence and append
		 * to your own sequence without checking the file indices.
		 *
		 * The returned file references are also in the order as the @a files added.
		 *
		 * Emits signals @a file_state_files_added and @a file_state_changed
		 * just before returning.
		 *
		 * Returns a sequence of file references to the newly added files.
		 */
		std::vector<file_reference>
		add_files(
				const std::vector<GPlatesFileIO::File::non_null_ptr_type> &files);


		/**
		 * Adds a file and activates it.
		 *
		 * The file index of returned reference will be the next in the sequence.
		 * So if you have three files previously loaded they will have indices 0,1,2 and
		 * now you're adding a new file then it will have index 3.
		 *
		 * Emits signals @a file_state_files_added and @a file_state_changed
		 * just before returning.
		 */
		file_reference
		add_file(
				const GPlatesFileIO::File::non_null_ptr_type &file);


		/**
		 * Remove @a file from the collection of currently loaded files.
		 *
		 * The file index used by this file will be reused by the remaining currently
		 * loaded files such that there will never be any gaps in the file indices
		 * of currently loaded files.
		 * So if you have four files loaded a,b,c,d with file indices 0,1,2,3 and
		 * you remove the file b (that has index 1) then the remaining files a,c,d will
		 * now have file indices 0,1,2.
		 *
		 * Because of this you can simply erase an entry in your own sequence using the
		 * file index of the removed file without checking the file indices.
		 *
		 * Emits signal @a file_state_file_about_to_be_removed before removal and
		 * emits signal @a file_state_changed after removal.
		 * The @a file_reference passed by the @a file_state_file_about_to_be_removed signal
		 * should not dereference the internal feature collection as that might be invalid.
		 */
		void
		remove_file(
				file_reference file_ref);

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
		file_state_files_added(
				GPlatesAppLogic::FeatureCollectionFileState &file_state,
				const std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference> &new_files);

		// NOTE: Do not dereference the internal feature collection of @a file as
		// it might be invalid (if this signal was generated when "undo"ing a file add).
		//
		// If you want a signal just after the file has been removed then you'll need to
		// listen to signal @a file_state_changed (since a reference to the removed file
		// cannot be safely provided).
		void
		file_state_file_about_to_be_removed(
				GPlatesAppLogic::FeatureCollectionFileState &file_state,
				GPlatesAppLogic::FeatureCollectionFileState::file_reference file);

		void
		file_state_file_info_changed(
				GPlatesAppLogic::FeatureCollectionFileState &file_state,
				GPlatesAppLogic::FeatureCollectionFileState::file_reference file);


		//
		// An alternative to the above signals is to listen to only one signal
		// when all you are interested in is knowing that the state has been modified.
		// This is useful for performing reconstructions.
		//

		// This signal is emitted *after* any file state has changed.
		void
		file_state_changed(
				GPlatesAppLogic::FeatureCollectionFileState &file_state);


	private:
		/**
		 * Contains a loaded file's shared reference and less frequently accessed information or
		 * information that is more expensive to copy.
		 * More frequently accessed information is placed in @a FileSlot.
		 *
		 * All this information could go in @a FileSlot but is separated to minimise the
		 * cost of copying @a FileSlot objects when the file slot sequence is compacted
		 * (when a file slot is deleted).
		 */
		class FileSlotExtra
		{
		public:
			FileSlotExtra(
					const GPlatesFileIO::File::Reference::non_null_ptr_type &file_ref) :
				d_file_ref(file_ref),
				d_callback_feature_collection(file_ref->get_feature_collection())
			{  }


			//! Reference to the file.
			GPlatesFileIO::File::Reference::non_null_ptr_type d_file_ref;

			/**
			 * Keep another reference to the feature collection in 'd_file' to contain
			 * our model callback.
			 */
			GPlatesModel::FeatureCollectionHandle::const_weak_ref d_callback_feature_collection;
		};

		/**
		 * A slot to store information about a file in a sequence of loaded files.
		 */
		class FileSlot
		{
		public:
			FileSlot(
					const GPlatesFileIO::File::Reference::non_null_ptr_type &file_,
					file_index_type index_into_file_index_array) :
				d_file_slot_extra(new FileSlotExtra(file_)),
				d_index_into_file_index_array(index_into_file_index_array),
				d_is_active_in_model(true)
			{  }

			boost::shared_ptr<FileSlotExtra> d_file_slot_extra;
			file_index_type d_index_into_file_index_array;
			bool d_is_active_in_model;
		};

		//! Typedef for a sequence of @a FileSlot objects.
		typedef std::vector<FileSlot> file_slot_seq_type;

		//! Typedef for a sequence of file handles.
		typedef std::vector<file_handle_type> file_handles_seq_type;

		//! Typedef for a sequence of indices indicating the order in which files were added.
		typedef std::vector<file_index_type> file_indices_seq_type;


		/**
		 * Used to add the feature collections of new files to the model.
		 */
		GPlatesModel::ModelInterface d_model;

		/**
		 * The number of loaded files (includes files that were deactivated in the *model* and
		 * subsequently reactivated).
		 */
		std::size_t d_num_currently_loaded_files;

		/**
		 * The sequence of all currently loaded files (includes those that have been
		 * conceptually deleted in the model - ie, deactivated in the model).
		 */
		file_slot_seq_type d_file_slots;

		/**
		 * A sequence of file handles that have been released and can be reused.
		 */
		file_handles_seq_type d_free_file_handles;

		/**
		 * The sequence of file indices.
		 * Modifications are such that entries are always naturally sorted (although
		 * there can be duplicate entries when a file is deactivated in the model).
		 * This sequence is indexed by FileSlotExtra::d_index_into_file_index_array.
		 */
		file_indices_seq_type d_file_indices;


		file_handle_type
		add_file_internal(
				const GPlatesFileIO::File::non_null_ptr_type &file);

		const GPlatesFileIO::File::Reference &
		get_file(
				file_handle_type file_handle) const;

		file_index_type
		get_file_index(
				file_handle_type file_handle) const;

		void
		set_file_info(
				file_handle_type file_handle,
				const GPlatesFileIO::FileInfo &new_file_info,
				boost::optional<GPlatesFileIO::FeatureCollectionFileFormat::Configuration::shared_ptr_to_const_type>
						new_file_configuration);

		void
		deactivated_feature_collection(
				file_handle_type file_handle);

		void
		reactivated_feature_collection(
				file_handle_type file_handle);

		void
		destroying_feature_collection(
				file_handle_type file_handle);


		/**
		 * Keeps track of feature collections as they are deactivated and reactivated in the *model*.
		 */
		class FeatureCollectionUnloadCallback :
				public GPlatesModel::WeakReferenceCallback<const GPlatesModel::FeatureCollectionHandle>
		{
		public:
			explicit
			FeatureCollectionUnloadCallback(
					GPlatesAppLogic::FeatureCollectionFileState &file_state,
					file_handle_type file_handle) :
				d_file_state(file_state),
				d_file_handle(file_handle)
			{  }

			virtual
			void
			publisher_deactivated(
					const deactivated_event_type &)
			{
				d_file_state.deactivated_feature_collection(d_file_handle);
			}

			virtual
			void
			publisher_reactivated(
					const reactivated_event_type &)
			{
				d_file_state.reactivated_feature_collection(d_file_handle);
			}

			virtual
			void
			publisher_about_to_be_destroyed(
					const about_to_be_destroyed_event_type &)
			{
				d_file_state.destroying_feature_collection(d_file_handle);
			}

		private:
			GPlatesAppLogic::FeatureCollectionFileState &d_file_state;
			file_handle_type d_file_handle;
		};
	};

	boost::optional<GPlatesAppLogic::FeatureCollectionFileState::file_reference>
	get_file_reference_containing_feature(
			GPlatesAppLogic::FeatureCollectionFileState &file_state_ref,
			GPlatesModel::FeatureHandle::weak_ref feature_ref);

}

#endif // GPLATES_APP_LOGIC_FEATURECOLLECTIONFILESTATE_H
