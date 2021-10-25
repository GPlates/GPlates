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

#ifndef GPLATES_SCRIBE_SCRIBEINTERNALACCESS_H
#define GPLATES_SCRIBE_SCRIBEINTERNALACCESS_H

#include <boost/shared_ptr.hpp>

#include "Scribe.h"
#include "ScribeInternalUtils.h"
#include "ScribeLoadRef.h"
#include "ScribeOptions.h"
#include "Transcribe.h"
#include "TranscribeResult.h"
#include "TranscriptionScribeContext.h"

#include "utils/CallStackTracker.h"


namespace GPlatesScribe
{
	// Forward declarations.
	class Scribe;

	template <typename ObjectType>
	class LoadRef;

	template <typename ObjectType>
	class ConstructObject;

	template <typename T>
	TranscribeResult transcribe(Scribe &, boost::shared_ptr<T> &, bool);

	template <typename ObjectType>
	TranscribeResult transcribe_smart_pointer_protocol(
			const GPlatesUtils::CallStack::Trace &, Scribe &, ObjectType *&, bool);

	namespace TranscribeUtils
	{
		Scribe::Bool transcribe_file_path(
				const GPlatesUtils::CallStack::Trace &, Scribe &, QString &, const char *);

		template <typename SmartPtrType>
		Scribe::Bool load_smart_pointer_from_raw_pointer(
				const GPlatesUtils::CallStack::Trace &, Scribe &, SmartPtrType &, const char *);

		template <typename ObjectType, typename ObjectRawPtrType>
		Scribe::Bool load_raw_pointer_and_object_from_smart_pointer(
				const GPlatesUtils::CallStack::Trace &, Scribe &, ObjectType &, ObjectRawPtrType &, const char *);

		template <typename ObjectType, typename ObjectRawPtrType>
		LoadRef<ObjectType> load_raw_pointer_and_object_from_smart_pointer(
				const GPlatesUtils::CallStack::Trace &, Scribe &, ObjectRawPtrType &, const char *);
	}


	/**
	 * Limit access to the internals of class Scribe to a few functions.
	 *
	 * We don't really want friends of class Scribe to access everything in class Scribe.
	 * For example, should only be able to call functions that do const-conversions otherwise
	 * Scribe's internal object-type-checking system could be inadvertently subverted.
	 *
	 * Class ScribeInternalAccess is the only friend of class Scribe.
	 * And class ScribeInternalAccess selects its own friends.
	 * So it's the limited set of functions in class ScribeInternalAccess that limit access to class Scribe.
	 */
	class ScribeInternalAccess
	{
	private:

		template <typename ObjectType>
		static
		void
		untrack(
				Scribe &scribe,
				ObjectType &object,
				bool discard)
		{
			scribe.untrack(object, discard);
		}


		template <typename ObjectType>
		static
		bool
		transcribe_construct(
				Scribe &scribe,
				ConstructObject<ObjectType> &object,
				TranscriptionScribeContext::object_id_type object_id,
				Options options)
		{
			return scribe.transcribe_construct(object, object_id, options);
		}


		template <typename ObjectType>
		static
		bool
		transcribe_smart_pointer(
				Scribe &scribe,
				ObjectType *&object_ptr,
				bool shared_owner)
		{
			return scribe.transcribe_smart_pointer(object_ptr, shared_owner);
		}


		template <typename ObjectType>
		static
		LoadRef<ObjectType>
		create_load_ref(
				const GPlatesUtils::CallStack::Trace &transcribe_source,
				Scribe &scribe,
				ObjectType *object,
				bool release)
		{
			return LoadRef<ObjectType>(transcribe_source, scribe, object, release);
		}


		static
		Scribe::Bool
		create_bool(
				const GPlatesUtils::CallStack::Trace &transcribe_source,
				bool result,
				bool require_check)
		{
			return Scribe::Bool(transcribe_source, result, require_check);
		}


		template <typename T>
		static
		void
		reset(
				Scribe &scribe,
				boost::shared_ptr<T> &shared_ptr_object,
				T *raw_ptr)
		{
			scribe.reset(shared_ptr_object, raw_ptr);
		}

		template <typename T>
		static
		void
		reset(
				Scribe &scribe,
				boost::shared_ptr<const T> &shared_ptr_object,
				const T *raw_ptr)
		{
			scribe.reset(shared_ptr_object, raw_ptr);
		}


		// Allow LoadRef to call 'Scribe::untrack()'.
		template <typename ObjectType>
		friend class LoadRef;

		// Allow class TranscribeOwningPointerTemplate to call 'Scribe::transcribe_construct()'.
		template <typename ObjectType>
		friend class InternalUtils::TranscribeOwningPointerTemplate;

		// Allow smart pointer protocol to call 'Scribe::transcribe_smart_pointer()'.
		template <typename ObjectType>
		friend TranscribeResult transcribe_smart_pointer_protocol(
				const GPlatesUtils::CallStack::Trace &, Scribe &, ObjectType *&, bool);

		//
		// Allow transcribe utilities to construct 'Scribe::Bool' objects and construct 'LoadRef<>' objects.
		//
		friend Scribe::Bool TranscribeUtils::transcribe_file_path(
				const GPlatesUtils::CallStack::Trace &, Scribe &, QString &, const char *);
		template <typename SmartPtrType>
		friend Scribe::Bool TranscribeUtils::load_smart_pointer_from_raw_pointer(
				const GPlatesUtils::CallStack::Trace &, Scribe &, SmartPtrType &, const char *);
		template <typename ObjectType, typename ObjectRawPtrType>
		friend Scribe::Bool TranscribeUtils::load_raw_pointer_and_object_from_smart_pointer(
				const GPlatesUtils::CallStack::Trace &, Scribe &, ObjectType &, ObjectRawPtrType &, const char *);
		template <typename ObjectType, typename ObjectRawPtrType>
		friend LoadRef<ObjectType> TranscribeUtils::load_raw_pointer_and_object_from_smart_pointer(
				const GPlatesUtils::CallStack::Trace &, Scribe &, ObjectRawPtrType &, const char *);

		// Allow transcribe of boost::shared_ptr to call 'Scribe::reset()' helper function.
		template <typename T>
		friend TranscribeResult transcribe(Scribe &, boost::shared_ptr<T> &, bool);
	};
}

#endif // GPLATES_SCRIBE_SCRIBEINTERNALACCESS_H
