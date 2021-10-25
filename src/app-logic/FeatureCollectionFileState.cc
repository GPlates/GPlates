/* $Id$ */

/**
 * @file 
 * Contains the main function of GPlates.
 *
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2007, 2010 The University of Sydney, Australia
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
#include <boost/foreach.hpp>
#include <QDebug>

#include "FeatureCollectionFileState.h"

#include "file-io/FeatureCollectionFileFormatRegistry.h"
#include "file-io/ReadErrorAccumulation.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/GPlatesException.h"

#include "model/Model.h"
#include "model/NotificationGuard.h"

#include "scribe/Scribe.h"


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


GPlatesAppLogic::FeatureCollectionFileState::FeatureCollectionFileState(
		GPlatesModel::ModelInterface &model_interface) :
	d_model(model_interface),
	d_num_currently_loaded_files(0)
{
}


GPlatesAppLogic::FeatureCollectionFileState::~FeatureCollectionFileState()
{
	// Remove all currently loaded files from the model.
	for (file_handle_type file_handle = 0; file_handle < d_file_slots.size(); ++file_handle)
	{
		if (d_file_slots[file_handle].d_is_active_in_model)
		{
			remove_file(file_reference(*this, file_handle));
		}
	}
}


std::vector<GPlatesAppLogic::FeatureCollectionFileState::const_file_reference>
GPlatesAppLogic::FeatureCollectionFileState::get_loaded_files() const
{
	// Resize the vector to the number of currently loaded files.
	std::vector<const_file_reference> file_references(
			d_num_currently_loaded_files,
			const_file_reference(*this, 0/*dummy file handle*/));

	// For assertion checking.
	std::size_t num_loaded_files = 0;
	std::vector<bool> file_index_used(d_num_currently_loaded_files, false/*initial value*/);

	// Iterate over all file slots (some are in use and others are not).
	std::size_t num_file_slots = d_file_slots.size();
	for (file_handle_type file_handle = 0; file_handle < num_file_slots; ++file_handle)
	{
		const FileSlot &file_slot = d_file_slots[file_handle];

		// If file is currently loaded then return it to the caller.
		if (file_slot.d_is_active_in_model)
		{
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					file_slot.d_index_into_file_index_array < d_file_indices.size(),
					GPLATES_ASSERTION_SOURCE);
			const file_index_type file_index =
					d_file_indices[file_slot.d_index_into_file_index_array];

			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					file_index < file_references.size(),
					GPLATES_ASSERTION_SOURCE);

			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					!file_index_used[file_index],
					GPLATES_ASSERTION_SOURCE);
			// For assertion checking.
			file_index_used[file_index] = true;

			// Store file reference in the correct location in the caller's array.
			file_references[file_index] = const_file_reference(*this, file_handle);

			++num_loaded_files;
		}
	}

	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			num_loaded_files == d_num_currently_loaded_files,
			GPLATES_ASSERTION_SOURCE);

	return file_references;
}


std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference>
GPlatesAppLogic::FeatureCollectionFileState::get_loaded_files()
{
	// Re-use the 'const' version of this method.
	const std::vector<const_file_reference> const_loaded_files =
			static_cast<const FeatureCollectionFileState *>(this)
					->get_loaded_files();

	// Convert from const file references to non-const file references.
	std::vector<file_reference> loaded_files;
	loaded_files.reserve(const_loaded_files.size());
	BOOST_FOREACH(const const_file_reference &const_loaded_file, const_loaded_files)
	{
		loaded_files.push_back(file_reference(*this, const_loaded_file.get_file_handle()));
	}

	return loaded_files;
}


std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference>
GPlatesAppLogic::FeatureCollectionFileState::add_files(
		const std::vector<GPlatesFileIO::File::non_null_ptr_type> &files_to_add)
{
	std::vector<file_reference> file_references;
	file_references.reserve(files_to_add.size());

	// Iterate over the files passed in by the caller.
	typedef std::vector<GPlatesFileIO::File::non_null_ptr_type> file_seq_type;
	file_seq_type::const_iterator files_to_add_iter = files_to_add.begin();
	file_seq_type::const_iterator files_to_add_end = files_to_add.end();
	for ( ; files_to_add_iter != files_to_add_end; ++files_to_add_iter)
	{
		const GPlatesFileIO::File::non_null_ptr_type &file_to_add = *files_to_add_iter;

		const file_handle_type file_handle = add_file_internal(file_to_add);

		// Append a file reference to the caller's array.
		file_references.push_back(file_reference(*this, file_handle));
	}

	// Emit to signal that all requested files have been added.
	Q_EMIT file_state_files_added(*this, file_references);
	Q_EMIT file_state_changed(*this);

	// Also let direct caller know which files were added so doesn't have to
	// listen to signal and interrupt its call flow.
	return file_references;
}


GPlatesAppLogic::FeatureCollectionFileState::file_reference
GPlatesAppLogic::FeatureCollectionFileState::add_file(
		const GPlatesFileIO::File::non_null_ptr_type &file_to_add)
{
	const std::vector<GPlatesFileIO::File::non_null_ptr_type> files_to_add(1, file_to_add);

	// Reuse the add multiple files function so we emit all the right signals, etc.
	const std::vector<file_reference> files_added = add_files(files_to_add);

	// We know only one file was added.
	return files_added.front();
}


GPlatesAppLogic::FeatureCollectionFileState::file_handle_type
GPlatesAppLogic::FeatureCollectionFileState::add_file_internal(
		const GPlatesFileIO::File::non_null_ptr_type &new_file)
{
	// Add the new file to the model (if it hasn't been already) so that the model
	// can track undo/redo of the feature collection in the file.
	const GPlatesFileIO::File::Reference::non_null_ptr_type new_file_ref =
			new_file->add_feature_collection_to_model(d_model);

	const FileSlot new_file_slot(new_file_ref, d_file_indices.size());
	d_file_indices.push_back(d_num_currently_loaded_files);

	file_handle_type new_file_handle;
	if (d_free_file_handles.empty())
	{
		// Create a new file handle.
		new_file_handle = d_file_slots.size();

		// Append new file slot.
		d_file_slots.push_back(new_file_slot);
	}
	else
	{
		// Reuse previously released file handles.
		new_file_handle = d_free_file_handles.back();
		d_free_file_handles.pop_back();

		// Store new file slot in the reused file slot.
		d_file_slots[new_file_handle] = new_file_slot;
	}

	++d_num_currently_loaded_files;

	// Attach a callback to the feature collection weak ref in the new file slot that
	// contains the callback. Only we have access to this weak ref and we make sure
	// the client doesn't have access to it. If we attached the callback to the
	// weak ref inside the File object then the client could access a copy of that weak ref,
	// by calling File::get_feature_collection(), and hence get a copy of our callback which
	// could mean the callback is called multiple times (once for each weak ref copy) and this
	// would break our code which assumes the callback is only called once per event.
	d_file_slots[new_file_handle].d_file_slot_extra->d_callback_feature_collection
			.attach_callback(new FeatureCollectionUnloadCallback(*this, new_file_handle));

	return new_file_handle;
}


void
GPlatesAppLogic::FeatureCollectionFileState::remove_file(
		file_reference file)
{
	const file_handle_type file_handle = file.get_file_handle();

	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			file_handle < d_file_slots.size(),
			GPLATES_ASSERTION_SOURCE);
	FileSlot &file_slot = d_file_slots[file_handle];

	// Feature collection should be active in the model.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			file_slot.d_is_active_in_model,
			GPLATES_ASSERTION_SOURCE);

	// Unload the feature collection - remove it from the feature store root in the model.
	const GPlatesModel::FeatureCollectionHandle::weak_ref feature_collection =
			file_slot.d_file_slot_extra->d_file_ref->get_feature_collection();
	// First check that the feature collection has not already been unloaded for some reason
	// or if the model that contains it has been destroyed (effectively unloading it).
	if (feature_collection.is_valid())
	{
		// This will probably become a method of BasicHandle sometime.
		GPlatesModel::FeatureStoreRootHandle *parent_store_root = feature_collection->parent_ptr();
		GPlatesModel::FeatureStoreRootHandle::iterator iter =
				GPlatesModel::FeatureStoreRootHandle::iterator(
						*parent_store_root, feature_collection->index_in_container());
		parent_store_root->remove(iter);
	}

	// Let the feature collection callback handle the rest.
	// If the feature collection was successfully removed then the deactivate model
	// callback will get called.
	// Also we'll emit signals there since the callback might get called sometime
	// after returning from this function due to a scope block of callback notifications
	// higher up in the call stack. When that scope block ends the model will notify
	// callbacks and we'll emit our own signals.
}


const GPlatesFileIO::File::Reference &
GPlatesAppLogic::FeatureCollectionFileState::get_file(
		file_handle_type file_handle) const
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			file_handle < d_file_slots.size() &&
					d_file_slots[file_handle].d_is_active_in_model,
			GPLATES_ASSERTION_SOURCE);

	return *d_file_slots[file_handle].d_file_slot_extra->d_file_ref;
}


GPlatesFileIO::File::Reference &
GPlatesAppLogic::FeatureCollectionFileState::get_file(
		file_handle_type file_handle)
{
	// Re-use the 'const' version of this method.
	return const_cast<GPlatesFileIO::File::Reference &>(
			static_cast<const FeatureCollectionFileState *>(this)
					->get_file(file_handle));
}


GPlatesAppLogic::FeatureCollectionFileState::file_index_type
GPlatesAppLogic::FeatureCollectionFileState::get_file_index(
		file_handle_type file_handle) const
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			file_handle < d_file_slots.size(),
			GPLATES_ASSERTION_SOURCE);

	return d_file_indices[d_file_slots[file_handle].d_index_into_file_index_array];
}


void
GPlatesAppLogic::FeatureCollectionFileState::set_file_info(
		file_handle_type file_handle,
		const GPlatesFileIO::FileInfo &new_file_info,
		boost::optional<GPlatesFileIO::FeatureCollectionFileFormat::Configuration::shared_ptr_to_const_type> new_file_configuration)
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			file_handle < d_file_slots.size() &&
					d_file_slots[file_handle].d_is_active_in_model,
			GPLATES_ASSERTION_SOURCE);

	// Set the new file info.
	d_file_slots[file_handle].d_file_slot_extra->d_file_ref->set_file_info(new_file_info, new_file_configuration);

	file_reference file_ref(*this, file_handle);
	Q_EMIT file_state_file_info_changed(*this, file_ref);
	Q_EMIT file_state_changed(*this);
}


void
GPlatesAppLogic::FeatureCollectionFileState::deactivated_feature_collection(
		file_handle_type file_handle)
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			file_handle < d_file_slots.size(),
			GPLATES_ASSERTION_SOURCE);
	FileSlot &file_slot = d_file_slots[file_handle];

	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			file_slot.d_is_active_in_model,
			GPLATES_ASSERTION_SOURCE);

	// Let clients know a file is about to be removed.
	// We need to do this here rather than in FeatureCollectionFileState:remove because
	// an undo of a file addition also is equivalent to a file remove as far as the client knows.
	Q_EMIT file_state_file_about_to_be_removed(*this, file_reference(*this, file_handle));

	// Flag the slot as not a currently loaded file.
	file_slot.d_is_active_in_model = false;

	//
	// Now that the file is not currently loaded we need to modify the file indices of
	// files loaded after this file.
	// We do this by decrementing all file indices above the current file's index.
	//

	const std::size_t num_file_indices = d_file_indices.size();

	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			file_slot.d_index_into_file_index_array < num_file_indices,
			GPLATES_ASSERTION_SOURCE);

	// Iterate over all file indices greater than the current file's file index.
	// This should be a relatively fast operation even if hundreds of files are loaded
	// because we're iterating through a vector which has good spatial cache coherency.
	//
	// NOTE: We even decrement file indices of files that are not currently loaded but
	// have not yet been deleted in the model (ie, deactivated in the model).
	// This is because those files can be reactivated by undo and if that happens they
	// will automatically have the correct file index.
	for (file_index_type index_into_file_indices = file_slot.d_index_into_file_index_array + 1;
		index_into_file_indices < num_file_indices;
		++index_into_file_indices)
	{
		// Decrement the file index.
		--d_file_indices[index_into_file_indices];
	}

	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_num_currently_loaded_files > 0,
			GPLATES_ASSERTION_SOURCE);
	--d_num_currently_loaded_files;

	Q_EMIT file_state_changed(*this);
}


void
GPlatesAppLogic::FeatureCollectionFileState::reactivated_feature_collection(
		file_handle_type file_handle)
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			file_handle < d_file_slots.size(),
			GPLATES_ASSERTION_SOURCE);
	FileSlot &file_slot = d_file_slots[file_handle];

	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			!file_slot.d_is_active_in_model,
			GPLATES_ASSERTION_SOURCE);
	// Flag the slot as a currently loaded file again.
	file_slot.d_is_active_in_model = true;

	//
	// Now that the file is currently loaded we need to modify the file indices of
	// files loaded after this file.
	// We do this by incrementing all file indices above the current file's index.
	//

	const std::size_t num_file_indices = d_file_indices.size();

	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			file_slot.d_index_into_file_index_array < num_file_indices,
			GPLATES_ASSERTION_SOURCE);

	// Iterate over all file indices greater than the current file's file index.
	// This should be a relatively fast operation even if hundreds of files are loaded
	// because we're iterating through a vector which has good spatial cache coherency.
	//
	// NOTE: We even increment file indices of files that are not currently loaded but
	// have not yet been deleted in the model (ie, deactivated in the model).
	// This is because those files can be reactivated by undo and if that happens they
	// will automatically have the correct file index.
	for (file_index_type index_into_file_indices = file_slot.d_index_into_file_index_array + 1;
		index_into_file_indices < num_file_indices;
		++index_into_file_indices)
	{
		// Increment the file index.
		++d_file_indices[index_into_file_indices];
	}

	// Increment the number of currently loaded files.
	++d_num_currently_loaded_files;

	// To our clients this will look like a file has been added.
	const std::vector<file_reference> file_references(1, file_reference(*this, file_handle));
	Q_EMIT file_state_files_added(*this, file_references);
	Q_EMIT file_state_changed(*this);
}


void
GPlatesAppLogic::FeatureCollectionFileState::destroying_feature_collection(
		file_handle_type file_handle)
{
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			file_handle < d_file_slots.size(),
			GPLATES_ASSERTION_SOURCE);
	FileSlot &deleted_file_slot = d_file_slots[file_handle];

	// It is possible that this function is called when d_is_active_in_model is true.
	// This is the case if we don't get a deactivation signal before the impending
	// destruction signal. This can occur if there is a notification guard blocking
	// the deactivation signal, but notification guards don't block impending
	// destruction signals.
	if (deleted_file_slot.d_is_active_in_model)
	{
		deactivated_feature_collection(file_handle);
	}

#if 0
	// Should have already been deactivated in the model.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			!deleted_file_slot.d_is_active_in_model,
			GPLATES_ASSERTION_SOURCE);
#endif

	//
	// Reuse the file slot and compact the slots array and compact the file indices array.
	//
	// This is done so we don't slowly consume memory if a user never shuts down
	// GPlates and continually loads/unloads feature collections.
	//

	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			deleted_file_slot.d_index_into_file_index_array < d_file_indices.size(),
			GPLATES_ASSERTION_SOURCE);
	// Compact the file indices array by erasing the entry containing the
	// deleted file's file index.
	// This is an O(N) operation in the number of loaded files but should be relatively fast
	// since the vector has good spatial cache coherency.
	d_file_indices.erase(
			d_file_indices.begin() + deleted_file_slot.d_index_into_file_index_array);

	// The above operation means we need to change the indices into the file index array
	// for all file slots whose index into the file index array is greater than that of the
	// deleted file.
	// Because the file slots are not necessarily ordered by these indices we'll need to
	// search all file slots.
	// This should be a relatively fast operation even if hundreds of files are loaded
	// because we're iterating through a vector which has good spatial cache coherency.
	//
	// NOTE: We even decrement indices into the file indices array of files that are not
	// currently loaded but have not yet been deleted in the model (ie, deactivated in the model).
	// This is because those files can be reactivated by undo and if that happens they
	// will automatically have the correct index into the file index array.
	const FeatureCollectionFileState::file_slot_seq_type::iterator file_slot_end = d_file_slots.end();
	for (FeatureCollectionFileState::file_slot_seq_type::iterator file_slot_iter = d_file_slots.begin();
		file_slot_iter != file_slot_end;
		++file_slot_iter)
	{
		if (file_slot_iter->d_index_into_file_index_array >
			deleted_file_slot.d_index_into_file_index_array)
		{
			--file_slot_iter->d_index_into_file_index_array;
		}
	}

	// Release the file slot for reuse.
	d_free_file_handles.push_back(file_handle);
}


GPlatesScribe::TranscribeResult
GPlatesAppLogic::FeatureCollectionFileState::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	// Set up transcribe context for constructing FeatureCollectionUnloadCallback.
	GPlatesScribe::TranscribeContext<FeatureCollectionUnloadCallback>
			feature_collection_unload_callback_transcribe_context(*this);
	GPlatesScribe::Scribe::ScopedTranscribeContextGuard<FeatureCollectionUnloadCallback>
			feature_collection_unload_callback_transcribe_context_guard(
					scribe,
					feature_collection_unload_callback_transcribe_context);

	if (scribe.is_saving())
	{
		file_slot_seq_type file_slots;
	}

	// Note that there might be some inactive file slots (if some feature collections were
	// deactivated in the model, so can be reactivated by undo, but not yet destroyed).
	// But we can't remove those and compress down the file slots because nested class FileReference
	// contains an integer file handle that indexes into the file slots and we don't want to find
	// all these file references and modify their handles (eg, they are found in the layers system).
	// So we'll transcribe the active and inactive file slots. And maybe a future session/project
	// will make use of the active flag (if it also tracks deactivated feature collections in the model).
	//
	// Note that we don't transcribe the free file handles (slots than can be reused due to destroyed
	// feature collections) because they can be obtained from inactive file slots.
	// And we don't transcribe the file indices (the load order of active files) because hese can be
	// obtained on loading without transcribing (though it won't reflect the original load order
	// on the saving system).
	if (
		// Transcribe the number of loaded files (active file slots).
		!scribe.transcribe(TRANSCRIBE_SOURCE, d_num_currently_loaded_files, "d_num_loaded_files") ||
		// Transcribe the file slots (active and inactive).
		!scribe.transcribe(TRANSCRIBE_SOURCE, d_file_slots, "d_file_slots"))
	{
		return scribe.get_transcribe_result();
	}

	if (scribe.is_loading())
	{
		// We want to merge model events across this scope so that only one model event is generated.
		GPlatesModel::NotificationGuard model_notification_guard(d_model.access_model());

		d_free_file_handles.clear();
		d_file_indices.clear();

		// Gets incremented for each active file.
		unsigned int file_index = 0;

		// Make adjustments to the transcribed data.
		file_handle_type file_handle;
		for (file_handle = 0; file_handle < d_file_slots.size(); ++file_handle)
		{
			const FileSlot &file_slot = d_file_slots[file_handle];

			// If file is not currently active then it means it was deactivated in the model, or
			// possibly even destroyed (ending up in the free file handles list) on the save system.
			// In either case, on loading archive, we consider inactive files to be non-reactivatable.
			if (!file_slot.d_is_active_in_model)
			{
				// Add to the free file handles list.
				d_free_file_handles.push_back(file_handle);

				// Skip to the next file.
				continue;
			}

			// Keep the order of files loaded.
			d_file_indices.push_back(file_index);
			++file_index;
		}

		//
		// Attempt to load the feature collections from the files.
		//

		// File references to pass when emitting state-changed signal.
		std::vector<file_reference> file_references;

		for (file_handle = 0; file_handle < d_file_slots.size(); ++file_handle)
		{
			const FileSlot &file_slot = d_file_slots[file_handle];

			// Skip in the free list (ie, not currently used file slots).
			if (!file_slot.d_is_active_in_model)
			{
				continue;
			}

			if (!file_slot.d_file_slot_extra)
			{
				d_file_slots.clear(); // Avoid destructor attempting to remove files.

				// Unexpected NULL pointer transcribed - must be incompatible transcription.
				return GPlatesScribe::TRANSCRIBE_INCOMPATIBLE;
			}

			GPlatesFileIO::File::Reference::non_null_ptr_type file_ref =
					file_slot.d_file_slot_extra->d_file_ref;

			if (!file_ref->get_feature_collection().is_valid())
			{
				d_file_slots.clear(); // Avoid destructor attempting to remove files.

				// An (empty) feature collection should exist - must be incompatible transcription.
				return GPlatesScribe::TRANSCRIBE_INCOMPATIBLE;
			}

			// Get information that is not transcribed into the archive.
			GPlatesScribe::TranscribeContext<FeatureCollectionFileState> &transcribe_context =
					scribe.get_transcribe_context<FeatureCollectionFileState>();

			try
			{
				// Read features from the file into the feature collection.
				// Both the filename and target feature collection are in the file reference.
				// Note that the target feature collection starts out empty.
				bool contains_unsaved_changes;
				transcribe_context.feature_collection_file_format_registry.read_feature_collection(
						*file_ref,
						// Pass any read errors up to a higher context so they can be displayed in the GUI...
						transcribe_context.read_errors,
						contains_unsaved_changes);

				// The file has been freshly loaded from disk.
				// If no model changes were needed during loading
				// (eg, to make it compatible with GPGIM) then it's clean.
				if (!contains_unsaved_changes)
				{
					GPlatesModel::FeatureCollectionHandle::weak_ref feature_collection_ref =
							file_ref->get_feature_collection();
					if (feature_collection_ref.is_valid())
					{
						feature_collection_ref->clear_unsaved_changes();
					}
				}
			}
			catch (GPlatesGlobal::Exception &exc)
			{
				const QString filename = file_ref->get_file_info().get_display_name(false/*use_absolute_path_name*/);

				// Log the detailed error message.
				qWarning() << "Error: Unexpected error loading file '"
					<< filename << "' - ignoring file: " << exc;
			}

			// Used when emitting state-changed signal.
			file_references.push_back(file_reference(*this, file_handle));
		}

		// Emit to signal that all requested files have been added/loaded.
		Q_EMIT file_state_files_added(*this, file_references);
		Q_EMIT file_state_changed(*this);
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


// Define in '.cc' file to avoid having to include "Scribe.h" in header file.
template <class FileStateQualifiedType>
GPlatesScribe::TranscribeResult
GPlatesAppLogic::FeatureCollectionFileState::FileReference<FileStateQualifiedType>::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	// If already transcribed using (non-default) constructor then nothing left to do.
	if (!transcribed_construct_data)
	{
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_file_handle, "d_file_handle"))
		{
			return scribe.get_transcribe_result();
		}
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}
// Explicitly instantiate since not implicitly instantiated because:
//  1) although defined in this '.cc' file, not used (implicitly instantiated) in this '.cc' file, and
//  2) not defined in '.h' file (where client use would implicitly instantiate it).
template
GPlatesScribe::TranscribeResult
GPlatesAppLogic::FeatureCollectionFileState::FileReference<
		GPlatesAppLogic::FeatureCollectionFileState>::transcribe(
				GPlatesScribe::Scribe &,
				bool);
template
GPlatesScribe::TranscribeResult
GPlatesAppLogic::FeatureCollectionFileState::FileReference<
		const GPlatesAppLogic::FeatureCollectionFileState>::transcribe(
				GPlatesScribe::Scribe &,
				bool);


// Define in '.cc' file to avoid having to include "Scribe.h" in header file.
template <class FileStateQualifiedType>
GPlatesScribe::TranscribeResult
GPlatesAppLogic::FeatureCollectionFileState::FileReference<FileStateQualifiedType>::transcribe_construct_data(
		GPlatesScribe::Scribe &scribe,
		GPlatesScribe::ConstructObject< FileReference<FileStateQualifiedType> > &file_ref)
{
	if (scribe.is_saving())
	{
		scribe.save(TRANSCRIBE_SOURCE, file_ref->d_file_handle, "d_file_handle");
	}
	else // loading...
	{
		GPlatesScribe::LoadRef<file_handle_type> file_handle =
				scribe.load<file_handle_type>(TRANSCRIBE_SOURCE, "d_file_handle");
		if (!file_handle.is_valid())
		{
			return scribe.get_transcribe_result();
		}

		// Get information that is not transcribed into the archive.
		GPlatesScribe::TranscribeContext< FileReference<FileStateQualifiedType> > &transcribe_context =
				scribe.get_transcribe_context< FileReference<FileStateQualifiedType> >();
		FeatureCollectionFileState &file_state = transcribe_context.feature_collection_file_state;

		// Construct the object from the constructor data.
		file_ref.construct_object(boost::ref(file_state), file_handle);

		scribe.relocated(TRANSCRIBE_SOURCE, file_ref->d_file_handle, file_handle);
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}
// Explicitly instantiate since not implicitly instantiated because:
//  1) although defined in this '.cc' file, not used (implicitly instantiated) in this '.cc' file, and
//  2) not defined in '.h' file (where client use would implicitly instantiate it).
template
GPlatesScribe::TranscribeResult
GPlatesAppLogic::FeatureCollectionFileState::FileReference<
		GPlatesAppLogic::FeatureCollectionFileState>::transcribe_construct_data(
				GPlatesScribe::Scribe &,
				GPlatesScribe::ConstructObject< FileReference<GPlatesAppLogic::FeatureCollectionFileState> > &);
template
GPlatesScribe::TranscribeResult
GPlatesAppLogic::FeatureCollectionFileState::FileReference<
		const GPlatesAppLogic::FeatureCollectionFileState>::transcribe_construct_data(
				GPlatesScribe::Scribe &,
				GPlatesScribe::ConstructObject< FileReference<const GPlatesAppLogic::FeatureCollectionFileState> > &);


GPlatesScribe::TranscribeResult
GPlatesAppLogic::FeatureCollectionFileState::FileSlotExtra::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	// If already transcribed using (non-default) constructor then nothing left to do.
	if (!transcribed_construct_data)
	{
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_file_ref, "d_file_ref"))
		{
			return scribe.get_transcribe_result();
		}
	}

	if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_callback_feature_collection, "d_callback_feature_collection"))
	{
		return scribe.get_transcribe_result();
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesScribe::TranscribeResult
GPlatesAppLogic::FeatureCollectionFileState::FileSlotExtra::transcribe_construct_data(
		GPlatesScribe::Scribe &scribe,
		GPlatesScribe::ConstructObject<FileSlotExtra> &file_slot_extra)
{
	if (scribe.is_saving())
	{
		scribe.save(TRANSCRIBE_SOURCE, file_slot_extra->d_file_ref, "d_file_ref");
	}
	else // loading...
	{
		GPlatesScribe::LoadRef<GPlatesFileIO::File::Reference::non_null_ptr_type> file_ref =
				scribe.load<GPlatesFileIO::File::Reference::non_null_ptr_type>(TRANSCRIBE_SOURCE, "d_file_ref");
		if (!file_ref.is_valid())
		{
			return scribe.get_transcribe_result();
		}

		// Construct the object from the constructor data.
		file_slot_extra.construct_object(file_ref);

		scribe.relocated(TRANSCRIBE_SOURCE, file_slot_extra->d_file_ref, file_ref);
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesScribe::TranscribeResult
GPlatesAppLogic::FeatureCollectionFileState::FileSlot::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	// Transcribe whether file slot is active.
	if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_is_active_in_model, "d_is_active_in_model"))
	{
		return scribe.get_transcribe_result();
	}

	// If file is currently loaded then transcribe it.
	if (d_is_active_in_model)
	{
		if (
			// Transcribe the index into file index array.
			!scribe.transcribe(TRANSCRIBE_SOURCE, d_index_into_file_index_array, "d_index_into_file_index_array") ||
			// Transcribe the FileSlotExtra.
			!scribe.transcribe(TRANSCRIBE_SOURCE, d_file_slot_extra, "d_file_slot_extra"))
		{
			return scribe.get_transcribe_result();
		}
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesScribe::TranscribeResult
GPlatesAppLogic::FeatureCollectionFileState::FeatureCollectionUnloadCallback::transcribe(
		GPlatesScribe::Scribe &scribe,
		bool transcribed_construct_data)
{
	// If already transcribed using (non-default) constructor then nothing left to do.
	if (!transcribed_construct_data)
	{
		if (!scribe.transcribe(TRANSCRIBE_SOURCE, d_file_handle, "d_file_handle"))
		{
			return scribe.get_transcribe_result();
		}
	}

	if (!scribe.transcribe_base<
			GPlatesModel::WeakReferenceCallback<const GPlatesModel::FeatureCollectionHandle>,
			FeatureCollectionUnloadCallback>(TRANSCRIBE_SOURCE))
	{
		return scribe.get_transcribe_result();
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


GPlatesScribe::TranscribeResult
GPlatesAppLogic::FeatureCollectionFileState::FeatureCollectionUnloadCallback::transcribe_construct_data(
		GPlatesScribe::Scribe &scribe,
		GPlatesScribe::ConstructObject<FeatureCollectionUnloadCallback> &callback)
{
	if (scribe.is_saving())
	{
		scribe.save(TRANSCRIBE_SOURCE, callback->d_file_handle, "d_file_handle");
	}
	else // loading...
	{
		GPlatesScribe::LoadRef<file_handle_type> file_handle =
				scribe.load<file_handle_type>(TRANSCRIBE_SOURCE, "d_file_handle");
		if (!file_handle.is_valid())
		{
			return scribe.get_transcribe_result();
		}

		// Get information that is not transcribed into the archive.
		GPlatesScribe::TranscribeContext<FeatureCollectionUnloadCallback> &
				transcribe_context = scribe.get_transcribe_context<FeatureCollectionUnloadCallback>();

		FeatureCollectionFileState &file_state = transcribe_context.file_state;

		// Construct the object from the constructor data.
		callback.construct_object(boost::ref(file_state), file_handle);

		scribe.relocated(TRANSCRIBE_SOURCE, callback->d_file_handle, file_handle);
	}

	return GPlatesScribe::TRANSCRIBE_SUCCESS;
}


boost::optional<GPlatesAppLogic::FeatureCollectionFileState::file_reference>
GPlatesAppLogic::get_file_reference_containing_feature(
		FeatureCollectionFileState &file_state_ref,
		GPlatesModel::FeatureHandle::weak_ref feature_ref)
{
	const std::vector<FeatureCollectionFileState::file_reference> file_references =
			file_state_ref.get_loaded_files();

	BOOST_FOREACH(FeatureCollectionFileState::file_reference file_ref, file_references)
	{
		const GPlatesModel::FeatureCollectionHandle::weak_ref feature_collection_ref =
				file_ref.get_file().get_feature_collection();

		if (feature_collection_contains_feature(feature_collection_ref, feature_ref)) 
		{
			return file_ref;
		}
	}

	return boost::none;
}
