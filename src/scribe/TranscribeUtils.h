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

#include <boost/mpl/not.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/static_assert.hpp>
#include <boost/type_traits/is_pointer.hpp>
#include <boost/type_traits/remove_pointer.hpp>
#include <QString>
#include <QStringList>

#include "Scribe.h"
#include "ScribeInternalAccess.h"
#include "ScribeLoadRef.h"
#include "ScribeSaveLoadConstructObject.h"

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
		 * For example:
		 *
		 *		GPlatesScribe::TranscribeUtils::FilePath transcribe_file_path;
		 *		
		 *		if (scribe.is_saving())
		 *		{
		 *			transcribe_file_path.set_file_path(file_path);
		 *		}
		 *		
		 *		if (!scribe.transcribe(TRANSCRIBE_SOURCE, transcribe_file_path, "file_path", GPlatesScribe::DONT_TRACK))
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
			 * Conversion from a QString file path.
			 */
			FilePath(
					const QString file_path)
			{
				set_file_path(file_path);
			}

			FilePath()
			{  }

			/**
			 * Set the QString file path.
			 */
			void
			set_file_path(
					const QString &file_path);

			/**
			 * Access QString file path.
			 */
			QString
			get_file_path() const;

			/**
			 * Conversion operator to QString file path.
			 */
			operator QString() const
			{
				return get_file_path();
			}

		private:

			QStringList d_split_paths;

			TranscribeResult
			transcribe(
					Scribe &scribe,
					bool transcribed_construct_data);

			// Only the scribe system should be able to transcribe.
			friend class GPlatesScribe::Access;
		};


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
		 *			"raw_pointer"))
		 *		{
		 *			return scribe.get_transcribe_result();
		 *		}
		 *
		 * NOTE: @a smart_ptr is tracked.
		 */
		template <typename SmartPtrType>
		Scribe::Bool
		load_smart_pointer_from_raw_pointer(
				const GPlatesUtils::CallStack::Trace &transcribe_source, // Use 'TRANSCRIBE_SOURCE' here
				Scribe &scribe,
				SmartPtrType &smart_ptr,
				const char *raw_ptr_tag);

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
		 *								"raw_pointer");
		 *		if (!smart_ptr_ref.is_valid())
		 *		{
		 *			return scribe.get_transcribe_result();
		 *		}
		 *		
		 *		GPlatesUtils::non_null_intrusive_ptr<Base> smart_ptr = smart_ptr_ref.get();
		 *		scribe.relocated(TRANSCRIBE_SOURCE, smart_ptr, smart_ptr_ref);
		 *
		 * NOTE: @a smart_ptr is tracked.
		 */
		template <typename SmartPtrType>
		LoadRef<SmartPtrType>
		load_smart_pointer_from_raw_pointer(
				const GPlatesUtils::CallStack::Trace &transcribe_source, // Use 'TRANSCRIBE_SOURCE' here
				Scribe &scribe,
				const char *raw_ptr_tag);


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
		 *			"smart_pointer"))
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
				const char *smart_ptr_tag);


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
		 *						"smart_pointer");
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
				const char *smart_ptr_tag);
	}
}

//
// Implementation
//

namespace GPlatesScribe
{
	namespace TranscribeUtils
	{
		template <typename SmartPtrType>
		Scribe::Bool
		load_smart_pointer_from_raw_pointer(
				const GPlatesUtils::CallStack::Trace &transcribe_source,
				Scribe &scribe,
				SmartPtrType &smart_ptr,
				const char *raw_ptr_tag)
		{
			// Track the file/line of the call site for exception messages.
			GPlatesUtils::CallStackTracker call_stack_tracker(transcribe_source);

			LoadRef<SmartPtrType> smart_ptr_ref =
					load_smart_pointer_from_raw_pointer<SmartPtrType>(
							TRANSCRIBE_SOURCE, scribe, raw_ptr_tag);
			if (!smart_ptr_ref.is_valid())
			{
				return ScribeInternalAccess::create_bool(transcribe_source, false/*result*/, true/*require_check*/);
			}

			smart_ptr = smart_ptr_ref.get();
			scribe.relocated(TRANSCRIBE_SOURCE, smart_ptr, smart_ptr_ref);

			return ScribeInternalAccess::create_bool(transcribe_source, true/*result*/, true/*require_check*/);
		}


		template <typename SmartPtrType>
		LoadRef<SmartPtrType>
		load_smart_pointer_from_raw_pointer(
				const GPlatesUtils::CallStack::Trace &transcribe_source,
				Scribe &scribe,
				const char *raw_ptr_tag)
		{
			// Compile-time assertion to ensure that 'SmartPtrType' is *not* a raw pointer type.
			BOOST_STATIC_ASSERT(boost::mpl::not_< boost::is_pointer<SmartPtrType> >::value);

			return scribe.load<SmartPtrType>(transcribe_source, raw_ptr_tag);
		}


		template <typename ObjectType, typename ObjectRawPtrType>
		Scribe::Bool
		load_raw_pointer_and_object_from_smart_pointer(
				const GPlatesUtils::CallStack::Trace &transcribe_source,
				Scribe &scribe,
				ObjectType &object,
				ObjectRawPtrType &object_raw_ptr,
				const char *smart_ptr_tag)
		{
			// Track the file/line of the call site for exception messages.
			GPlatesUtils::CallStackTracker call_stack_tracker(transcribe_source);

			LoadRef<ObjectType> object_ref =
					load_raw_pointer_and_object_from_smart_pointer<ObjectType>(
							TRANSCRIBE_SOURCE,
							scribe,
							object_raw_ptr,
							smart_ptr_tag);
			if (!object_ref.is_valid())
			{
				return ScribeInternalAccess::create_bool(transcribe_source, false/*result*/, true/*require_check*/);
			}

			// Copy of relocate the object.
			object = object_ref.get();
			scribe.relocated(TRANSCRIBE_SOURCE, object, object_ref);

			return ScribeInternalAccess::create_bool(transcribe_source, true/*result*/, true/*require_check*/);
		}


		template <typename ObjectType, typename ObjectRawPtrType>
		LoadRef<ObjectType>
		load_raw_pointer_and_object_from_smart_pointer(
				const GPlatesUtils::CallStack::Trace &transcribe_source,
				Scribe &scribe,
				ObjectRawPtrType &object_raw_ptr,
				const char *smart_ptr_tag)
		{
			// Track the file/line of the call site for exception messages.
			GPlatesUtils::CallStackTracker call_stack_tracker(transcribe_source);

			// Compile-time assertion to ensure that 'ObjectRawPtrType' is a raw pointer type.
			BOOST_STATIC_ASSERT(boost::is_pointer<ObjectRawPtrType>::value);

			// The static dereferenced object raw pointer type.
			typedef typename boost::remove_pointer<ObjectRawPtrType>::type dereferenced_object_raw_ptr_type;

			// Load into an owning *raw* pointer (instead of smart pointer) so we can both access
			// the pointed-to object and get a *tracked* raw pointer.
			LoadRef<ObjectRawPtrType> object_raw_ptr_ref =
					scribe.load<ObjectRawPtrType>(TRANSCRIBE_SOURCE, smart_ptr_tag, EXCLUSIVE_OWNER);
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
			scribe.relocated(TRANSCRIBE_SOURCE, object_raw_ptr, object_raw_ptr_ref);

			// Copy the pointed-to object into a new heap object.
			LoadConstructObjectOnHeap<ObjectType> load_construct_object;
			load_construct_object.construct_object(
					static_cast<ObjectType &>(*object_raw_ptr_ref.get()));

			// Relocate object.
			scribe.relocated(
					TRANSCRIBE_SOURCE,
					load_construct_object.get_object(),
					static_cast<ObjectType &>(*object_raw_ptr_ref.get()));

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
