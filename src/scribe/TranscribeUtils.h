/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2015 The University of Sydney, Australia
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

#ifndef GPLATES_SCRIBE_TRANSCRIBEUTILS_H
#define GPLATES_SCRIBE_TRANSCRIBEUTILS_H

#include <utility>
#include <boost/mpl/not.hpp>
#include <boost/optional.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/static_assert.hpp>
#include <boost/type_traits/is_pointer.hpp>
#include <boost/type_traits/remove_pointer.hpp>
#include <QMap>
#include <QSet>
#include <QString>
#include <QStringList>

#include "Scribe.h"
#include "ScribeInternalAccess.h"
#include "ScribeLoadRef.h"
#include "ScribeObjectTag.h"
#include "ScribeSaveLoadConstructObject.h"
#include "TranscribeContext.h"

#include "utils/CallStackTracker.h"


namespace GPlatesScribe
{
	namespace TranscribeUtils
	{
		/**
		 * Transcribing a FilePath created from a file path QString containing '/' directory separators
		 * results in smaller archives/transcriptions since each path between these separators is
		 * transcribed as a separate string - and this promotes sharing when many file paths
		 * share common paths.
		 *
		 * For example:
		 *
		 *		GPlatesScribe::TranscribeUtils::FilePath transcribe_file_path;
		 *		
		 *		if (scribe.is_saving())
		 *		{
		 *			transcribe_file_path.set_file_path(file_path);
		 *		}
		 *		
		 *		if (!scribe.transcribe(TRANSCRIBE_SOURCE, transcribe_file_path, "file_path"))
		 *		{
		 *			return scribe.get_transcribe_result();
		 *		}
		 *		
		 *		if (scribe.is_loading())
		 *		{
		 *			file_path = transcribe_file_path.get_file_path();
		 *		}
		 */
		class FilePath
		{
		public:

			/**
			 * Conversion from a QString file path (containing '/' directory separators).
			 */
			FilePath(
					const QString file_path)
			{
				set_file_path(file_path);
			}

			FilePath()
			{  }

			/**
			 * Set the QString file path (containing '/' directory separators).
			 */
			void
			set_file_path(
					const QString &file_path);

			/**
			 * Access QString file path (contains '/' directory separators).
			 *
			 * If @a convert is true (the default) then calls @a convert_file_path on returned result
			 * to add or remove Windows drive letter or share name if needed for runtime system.
			 */
			QString
			get_file_path(
					bool convert = true) const;

			/**
			 * Conversion operator to QString file path (contains '/' directory separators).
			 *
			 * Same as calling 'get_file_path(true)'.
			 */
			operator QString() const
			{
				return get_file_path();
			}

		private:

			QStringList d_split_paths;

		private: // Transcribe for sessions/projects...

			TranscribeResult
			transcribe(
					Scribe &scribe,
					bool transcribed_construct_data);

			// Only the scribe system should be able to transcribe.
			friend class GPlatesScribe::Access;
		};


		/**
		 * Convenience function to save a file path using @a FilePath.
		 */
		void
		save_file_path(
				Scribe &scribe,
				const GPlatesUtils::CallStack::Trace &transcribe_source, // Use 'TRANSCRIBE_SOURCE' here
				const QString &file_path,
				const ObjectTag &file_path_tag);


		/**
		 * Convenience function to load a file path using @a FilePath.
		 *
		 * If @a convert is true then calls @a convert_file_path on returned result to
		 * add or remove Windows drive letter or share name if needed for runtime system.
		 */
		boost::optional<QString>
		load_file_path(
				Scribe &scribe,
				const GPlatesUtils::CallStack::Trace &transcribe_source, // Use 'TRANSCRIBE_SOURCE' here
				const ObjectTag &file_path_tag,
				bool convert = true);


		/**
		 * Convenience function to save a sequence of file paths (sequence of QString) using @a FilePath.
		 *
		 * Uses the sequence protocol (eg, can subsequently be loaded using a std::vector).
		 */
		void
		save_file_paths(
				Scribe &scribe,
				const GPlatesUtils::CallStack::Trace &transcribe_source, // Use 'TRANSCRIBE_SOURCE' here
				const QStringList &file_paths,
				const ObjectTag &file_paths_tag);


		/**
		 * Same as other overload of @a save_file_paths, except can be used with any container with
		 * 'begin' and 'end' iterators (eg, std::vector).
		 */
		template <typename FilePathsIter>
		void
		save_file_paths(
				Scribe &scribe,
				const GPlatesUtils::CallStack::Trace &transcribe_source, // Use 'TRANSCRIBE_SOURCE' here
				FilePathsIter file_paths_begin,
				FilePathsIter file_paths_end,
				const ObjectTag &file_paths_tag);


		/**
		 * Convenience function to load a sequence of file paths (sequence of QString) using @a FilePath.
		 *
		 * Uses the sequence protocol (eg, compatible with having been saved using a std::vector).
		 *
		 * If @a convert is true then calls @a convert_file_path on returned result to
		 * add or remove Windows drive letter or share name if needed for runtime system.
		 */
		boost::optional<QStringList>
		load_file_paths(
				Scribe &scribe,
				const GPlatesUtils::CallStack::Trace &transcribe_source, // Use 'TRANSCRIBE_SOURCE' here
				const ObjectTag &file_paths_tag,
				bool convert = true);


		/**
		 * Convert a file path to a path appropriate for the runtime operating system.
		 *
		 * On Windows this adds the root drive letter to absolute paths.
		 * On Mac/Linux this removes driver letters and network share names.
		 *
		 * Note: Resource files (eg, ":/age.cpt") and empty file paths are left unchanged.
		 */
		QString
		convert_file_path(
				const QString &file_path);


		/**
		 * Convert a file path (transcribed into the project file) to be relative to the location
		 * of the project file being loaded (instead of the location the project file was saved to).
		 *
		 * The returned path is the absolute path formed by (1) the absolute path of the *loaded* project
		 * file location and (2) the relative path (formed between the *saved* file location and the
		 * *saved* project location).
		 *
		 * If successful then the location of the converted file path relative to the loaded project file location
		 * will match the location of the saved file path relative to the saved project file location.
		 * For example, "C:/gplates/data/data.gpml" is relative to "C:/gplates/projs/proj.gproj" as
		 * "../data/", and hence "/home/gplates/data/data.gpml" is relative to "/home/gplates/projs/proj.gproj"
		 * using the same relative path (note: it is also relative to "/home/gplates/projects/proj.gproj"
		 * since both "projs/" and "projects/" satisfy "../").
		 *
		 * If a *saved* file path cannot made be relative to the *saved* project file path then we
		 * return the *saved* file path converted using @a convert_file_path.
		 * For example this happens if the saved file and saved project file paths are on different drives.
		 *
		 * Note: Both @a file_path_when_saved and @a project_file_path_when_saved should *not* have
		 * been converted (using @a convert_file_path) because we want to compare the project filename
		 * on the system the project was saved on with the data filenames on the same save system.
		 *
		 * Note: If the project file save and load directories are the same then just returns
		 * @a file_path_when_saved (converted using @a convert_file_path).
		 *
		 * Note: Resource files (eg, ":/age.cpt") and empty file paths are returned unchanged
		 * (except for conversion using @a convert_file_path).
		 */
		QString
		convert_file_path_relative_to_project(
				const QString &file_path_when_saved,
				const QString &project_file_path_when_saved,
				const QString &project_file_path_when_loaded);


		/**
		 * Returns true if file path is a resource file or empty path.
		 *
		 * A resource file path starts with ":/" or "qrc:///".
		 */
		bool
		is_resource_file_path_or_empty_path(
				const QString &file_path);


		/**
		 * Load a saved object raw pointer into a smart pointer.
		 *
		 * Raw pointers and smart pointers are transcription compatible, so they can be used interchangeably.
		 *
		 * This is useful for backward compatibility when a raw pointer is saved to an archive but
		 * it needs to be loaded into a smart pointer. This can happen if an old version used a
		 * raw pointer but the current version replaced it with a smart pointer and the current
		 * version is loading from an old archive.
		 *
		 * For example:
		 *
		 *		boost::shared_ptr<Base> smart_ptr;
		 *		if (!GPlatesScribe::TranscribeUtils::load_smart_pointer_from_raw_pointer(
		 *			TRANSCRIBE_SOURCE,
		 *			scribe,
		 *			smart_ptr,
		 *			"raw_pointer",
		 *			true)) // track
		 *		{
		 *			return scribe.get_transcribe_result();
		 *		}
		 */
		template <typename SmartPtrType>
		Scribe::Bool
		load_smart_pointer_from_raw_pointer(
				const GPlatesUtils::CallStack::Trace &transcribe_source, // Use 'TRANSCRIBE_SOURCE' here
				Scribe &scribe,
				SmartPtrType &smart_ptr,
				const ObjectTag &raw_ptr_tag,
				bool track);

		/**
		 * An overload of @a load_smart_pointer_from_raw_pointer that returns a LoadRef<>.
		 *
		 * This is useful for GPlatesUtils::non_null_intrusive_ptr<> since it has no default constructor.
		 *
		 * For example:
		 *
		 *		GPlatesScribe::LoadRef< GPlatesUtils::non_null_intrusive_ptr<Base> > smart_ptr_ref =
		 *				GPlatesScribe::TranscribeUtils::load_smart_pointer_from_raw_pointer<
		 *						GPlatesUtils::non_null_intrusive_ptr<Base> >(
		 *								TRANSCRIBE_SOURCE,
		 *								scribe,
		 *								"raw_pointer",
		 *								true); // track
		 *		if (!smart_ptr_ref.is_valid())
		 *		{
		 *			return scribe.get_transcribe_result();
		 *		}
		 *		
		 *		GPlatesUtils::non_null_intrusive_ptr<Base> smart_ptr = smart_ptr_ref.get();
		 *		scribe.relocated(TRANSCRIBE_SOURCE, smart_ptr, smart_ptr_ref);
		 */
		template <typename SmartPtrType>
		LoadRef<SmartPtrType>
		load_smart_pointer_from_raw_pointer(
				const GPlatesUtils::CallStack::Trace &transcribe_source, // Use 'TRANSCRIBE_SOURCE' here
				Scribe &scribe,
				const ObjectTag &raw_ptr_tag,
				bool track);


		/**
		 * Load a saved smart pointer into a raw pointer and its pointed-to object.
		 *
		 * Raw pointers and smart pointers are transcription compatible, so they can be used interchangeably.
		 *
		 * This is useful for backward compatibility when a smart pointer is saved to an archive but
		 * it needs to be loaded into a raw pointer and its pointed-to object. This can happen if an
		 * old version used a smart pointer but the current version replaced it with a raw pointer
		 * and its pointed-to object and the current version is loading from an old archive.
		 *
		 * For example:
		 *
		 *		Derived object;
		 *		Base *object_ptr;
		 *		if (!GPlatesScribe::TranscribeUtils::load_raw_pointer_and_object_from_smart_pointer(
		 *			TRANSCRIBE_SOURCE,
		 *			scribe,
		 *			object,
		 *			object_ptr,
		 *			"smart_pointer",
		 *			true)) // track
		 *		{
		 *			return scribe.get_transcribe_result();
		 *		}
		 *		
		 *		assert(object_ptr == &object);
		 *
		 * If the object type 'ObjectType' does not match the actual loaded object type then false is returned.
		 * So this only really works if you know the object type saved in the archive.
		 * So, in the above example, if an object of type other that 'Derived' is loaded then false is returned.
		 *
		 * NOTE: There are no tracking options (both the raw pointer and the pointed-to object are tracked).
		 * Both @a object and @a object_raw_ptr must be tracked because:
		 *   (1) A tracked pointer and an untracked object - the object is not tracked so we don't
		 *       know if/where it will move to and hence cannot update the tracked pointer.
		 *   (2) An untracked pointer and a tracked object - if the tracked object is relocated then
		 *       we cannot update the untracked pointer to point to it (since it's untracked).
		 *   (3) An untracked pointer and an untracked object - for the above reasons.
		 */
		template <typename ObjectType, typename ObjectRawPtrType>
		Scribe::Bool
		load_raw_pointer_and_object_from_smart_pointer(
				const GPlatesUtils::CallStack::Trace &transcribe_source, // Use 'TRANSCRIBE_SOURCE' here
				Scribe &scribe,
				ObjectType &object,
				ObjectRawPtrType &object_raw_ptr,
				const ObjectTag &smart_ptr_tag,
				bool track);


		/**
		 * An overload of @a load_raw_pointer_and_object_from_smart_pointer that returns a LoadRef<>
		 * for the object pointed to by the raw pointer.
		 *
		 * This is useful when 'ObjectType' has no default constructor.
		 *
		 * For example:
		 *
		 *		Base *object_ptr;
		 *		GPlatesScribe::LoadRef<Derived> object_ref =
		 *				GPlatesScribe::TranscribeUtils::load_raw_pointer_and_object_from_smart_pointer<Derived>(
		 *						TRANSCRIBE_SOURCE,
		 *						scribe,
		 *						object_ptr,
		 *						"smart_pointer",
		 *						true); // track
		 *		{
		 *			return scribe.get_transcribe_result();
		 *		}
		 *		
		 *		Derived object = object_ref.get();
		 *		scribe.relocated(TRANSCRIBE_SOURCE, object, object_ref);
		 *		
		 *		assert(object_ptr == &object);
		 *
		 * NOTE: There are no tracking options (both the raw pointer and the pointed-to object are tracked).
		 */
		template <typename ObjectType, typename ObjectRawPtrType>
		LoadRef<ObjectType>
		load_raw_pointer_and_object_from_smart_pointer(
				const GPlatesUtils::CallStack::Trace &transcribe_source, // Use 'TRANSCRIBE_SOURCE' here
				Scribe &scribe,
				ObjectRawPtrType &object_raw_ptr,
				const ObjectTag &smart_ptr_tag,
				bool track);
	}


	/**
	 * Used to record transcribed @a FilePath objects.
	 *
	 * This is useful if file paths need to be adjusted, in the loading path, to be
	 * relative to the project file location (instead of using the recorded absolute paths).
	 */
	template <>
	class TranscribeContext<TranscribeUtils::FilePath>
	{
	public:

		/**
		 * In the loading path, convert transcribed file paths to be relative to the loaded
		 * project file rather than relative to the location of the project file when it was saved.
		 *
		 * If a file path cannot be converted to be relative then we keep its original (saved) file path
		 * (converted using @a convert_file_path).
		 *
		 * Note: The saved project file path should *not* have been converted using @a convert_file_path.
		 *
		 * Note: Resource files (eg, ":/age.cpt") and empty file paths are not converted.
		 */
		void
		set_load_relative_file_paths(
				const std::pair<
						QString/*project_file_path_when_saved*/,
						QString/*project_file_path_when_loaded*/> &load_relative_file_paths)
		{
			d_load_relative_file_paths = load_relative_file_paths;
		}


		/**
		 * In the loading path, rename any files in the specified mapping.
		 *
		 * This is used to rename missing files to existing files when a project/session references
		 * files that no longer exist.
		 *
		 * In the case of relative file paths (for project files - see @a set_load_relative_file_paths)
		 * the remapping is from file paths relative to the loaded project file location.
		 */
		void
		set_load_file_path_remapping(
				boost::optional< QMap<QString/*missing*/, QString/*existing*/> > load_file_path_remapping)
		{
			d_load_file_path_remapping = load_file_path_remapping;
		}


		/**
		 * Returns a unique sorted list of all file paths (@a FilePath) transcribed.
		 *
		 * If @a convert is true (the default) then calls @a convert_file_path on returned results
		 * to add or remove Windows drive letter or share name if needed for runtime system.
		 *
		 * If @a exclude_resource_and_empty_file_paths is true (the default) then file paths that
		 * are resource files (eg, ":/age.cpt") or empty paths are excluded.
		 */
		QStringList
		get_file_paths(
				bool convert = true,
				bool exclude_resource_and_empty_file_paths = true) const;

	private:

		QStringList d_file_paths;

		boost::optional< std::pair<
				QString/*project_file_path_when_saved*/,
				QString/*project_file_path_when_loaded*/> > d_load_relative_file_paths;

		boost::optional< QMap<QString/*missing*/, QString/*existing*/> > d_load_file_path_remapping;

		friend class TranscribeUtils::FilePath;
	};
}

//
// Implementation
//

namespace GPlatesScribe
{
	namespace TranscribeUtils
	{
		template <typename FilePathsIter>
		void
		save_file_paths(
				Scribe &scribe,
				const GPlatesUtils::CallStack::Trace &transcribe_source,
				FilePathsIter file_paths_begin,
				FilePathsIter file_paths_end,
				const ObjectTag &file_paths_tag)
		{
			unsigned int file_paths_index = 0;
			for (FilePathsIter file_paths_iter = file_paths_begin;
				file_paths_iter != file_paths_end;
				++file_paths_iter, ++file_paths_index)
			{
				const QString file_path = *file_paths_iter;

				save_file_path(scribe, transcribe_source, file_path, file_paths_tag[file_paths_index]);
			}

			// Save number of file paths.
			scribe.save(transcribe_source, file_paths_index, file_paths_tag.sequence_size());
		}


		template <typename SmartPtrType>
		Scribe::Bool
		load_smart_pointer_from_raw_pointer(
				const GPlatesUtils::CallStack::Trace &transcribe_source,
				Scribe &scribe,
				SmartPtrType &smart_ptr,
				const ObjectTag &raw_ptr_tag,
				bool track)
		{
			// Track the file/line of the call site for exception messages.
			GPlatesUtils::CallStackTracker call_stack_tracker(transcribe_source);

			LoadRef<SmartPtrType> smart_ptr_ref =
					load_smart_pointer_from_raw_pointer<SmartPtrType>(TRANSCRIBE_SOURCE, scribe, raw_ptr_tag, track);
			if (!smart_ptr_ref.is_valid())
			{
				return ScribeInternalAccess::create_bool(transcribe_source, false/*result*/, true/*require_check*/);
			}

			smart_ptr = smart_ptr_ref.get();

			if (track)
			{
				scribe.relocated(TRANSCRIBE_SOURCE, smart_ptr, smart_ptr_ref);
			}

			return ScribeInternalAccess::create_bool(transcribe_source, true/*result*/, true/*require_check*/);
		}


		template <typename SmartPtrType>
		LoadRef<SmartPtrType>
		load_smart_pointer_from_raw_pointer(
				const GPlatesUtils::CallStack::Trace &transcribe_source,
				Scribe &scribe,
				const ObjectTag &raw_ptr_tag,
				bool track)
		{
			// Compile-time assertion to ensure that 'SmartPtrType' is *not* a raw pointer type.
			BOOST_STATIC_ASSERT(boost::mpl::not_< boost::is_pointer<SmartPtrType> >::value);

			return scribe.load<SmartPtrType>(transcribe_source, raw_ptr_tag, track ? TRACK : 0);
		}


		template <typename ObjectType, typename ObjectRawPtrType>
		Scribe::Bool
		load_raw_pointer_and_object_from_smart_pointer(
				const GPlatesUtils::CallStack::Trace &transcribe_source,
				Scribe &scribe,
				ObjectType &object,
				ObjectRawPtrType &object_raw_ptr,
				const ObjectTag &smart_ptr_tag,
				bool track)
		{
			// Track the file/line of the call site for exception messages.
			GPlatesUtils::CallStackTracker call_stack_tracker(transcribe_source);

			LoadRef<ObjectType> object_ref =
					load_raw_pointer_and_object_from_smart_pointer<ObjectType>(
							TRANSCRIBE_SOURCE,
							scribe,
							object_raw_ptr,
							smart_ptr_tag,
							track);
			if (!object_ref.is_valid())
			{
				return ScribeInternalAccess::create_bool(transcribe_source, false/*result*/, true/*require_check*/);
			}

			// Copy of relocate the object.
			object = object_ref.get();
			if (track)
			{
				// Note that this will also update the raw pointer (due to tracking).
				scribe.relocated(TRANSCRIBE_SOURCE, object, object_ref);
			}
			else  // untracked...
			{
				// Need to update raw pointer since it's untracked (and hence not updated by tracking system).
				object_raw_ptr = &object;
			}

			return ScribeInternalAccess::create_bool(transcribe_source, true/*result*/, true/*require_check*/);
		}


		template <typename ObjectType, typename ObjectRawPtrType>
		LoadRef<ObjectType>
		load_raw_pointer_and_object_from_smart_pointer(
				const GPlatesUtils::CallStack::Trace &transcribe_source,
				Scribe &scribe,
				ObjectRawPtrType &object_raw_ptr,
				const ObjectTag &smart_ptr_tag,
				bool track)
		{
			// Track the file/line of the call site for exception messages.
			GPlatesUtils::CallStackTracker call_stack_tracker(transcribe_source);

			// Compile-time assertion to ensure that 'ObjectRawPtrType' is a raw pointer type.
			BOOST_STATIC_ASSERT(boost::is_pointer<ObjectRawPtrType>::value);

			// The static dereferenced object raw pointer type.
			typedef typename boost::remove_pointer<ObjectRawPtrType>::type dereferenced_object_raw_ptr_type;

			// Load into an owning *raw* pointer (instead of smart pointer) so we can both access
			// the pointed-to object and get a raw pointer.
			unsigned int load_options = EXCLUSIVE_OWNER;
			if (track)
			{
				load_options |= TRACK;
			}
			LoadRef<ObjectRawPtrType> object_raw_ptr_ref =
					scribe.load<ObjectRawPtrType>(TRANSCRIBE_SOURCE, smart_ptr_tag, load_options);
			if (!object_raw_ptr_ref.is_valid())
			{
				return LoadRef<ObjectType>();
			}

			// Make sure the owned object gets destroyed (after we copy from it).
			boost::scoped_ptr<dereferenced_object_raw_ptr_type> object_owner(object_raw_ptr_ref.get());

			// If the actual loaded object type does not match 'ObjectType' then
			// we can't copy it into 'object'.
			if (typeid(*object_raw_ptr_ref.get()) != typeid(ObjectType))
			{
				return LoadRef<ObjectType>();
			}

			// Copy and relocate raw pointer.
			object_raw_ptr = object_raw_ptr_ref.get();
			if (track)
			{
				scribe.relocated(TRANSCRIBE_SOURCE, object_raw_ptr, object_raw_ptr_ref);
			}

			// Copy the pointed-to object into a new heap object.
			LoadConstructObjectOnHeap<ObjectType> load_construct_object;
			load_construct_object.construct_object(
					static_cast<ObjectType &>(*object_raw_ptr_ref.get()));

			if (track)
			{
				// Relocate object.
				// Note that this will also update the raw pointer (due to tracking).
				scribe.relocated(
						TRANSCRIBE_SOURCE,
						load_construct_object.get_object(),
						static_cast<ObjectType &>(*object_raw_ptr_ref.get()));
			}
			else  // untracked...
			{
				// Need to update raw pointer since it's untracked (and hence not updated by tracking system).
				object_raw_ptr = &load_construct_object.get_object();
			}

			// Return reference to load construct object.
			return ScribeInternalAccess::create_load_ref(
							transcribe_source,
							scribe,
							load_construct_object.release(),
							// Transferring object heap ownership from load construct object to LoadRef<>...
							true/*release*/);
		}
	}
}

#endif // #ifndef GPLATES_SCRIBE_TRANSCRIBEUTILS_H
