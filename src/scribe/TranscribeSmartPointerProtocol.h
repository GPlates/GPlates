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

#ifndef GPLATES_SCRIBE_TRANSCRIBESMARTPOINTERPROTOCOL_H
#define GPLATES_SCRIBE_TRANSCRIBESMARTPOINTERPROTOCOL_H

#include <boost/optional.hpp>

#include "Scribe.h"
#include "ScribeBool.h"
#include "ScribeInternalAccess.h"
#include "TranscribeResult.h"

#include "utils/CallStackTracker.h"


namespace GPlatesScribe
{
	/**
	 * Used to ensure different smart pointer types are transcribed such that they can be switched
	 * without breaking backward/forward compatibility.
	 *
	 * This also makes smart pointer classes interchangeable with raw pointers.
	 *
	 * Some smart pointer types include boost::shared_ptr, boost::scoped_ptr, boost::intrusive_ptr
	 * and GPlatesUtils::non_null_intrusive_ptr.
	 *
	 * @a use_count_hint is the number of owners of the pointed-to object on the *save* path
	 * (or none if unknown). It is only used inside a raw stream ("raw lane") subtree - a hint
	 * of 1 (sole owner) skips shared-object deduplication for the pointed-to object.
	 * IMPORTANT: only pass a hint that counts *all* owners of the pointed-to object
	 * (eg, an intrusive reference count - see @a get_intrusive_use_count_hint) - a hint that
	 * can under-count owners (eg, boost::shared_ptr's use count, which excludes weak pointers
	 * that could lock and transcribe later) breaks aliasing in the raw lane.
	 */
	template <typename ObjectType>
	Bool
	transcribe_smart_pointer_protocol(
			const GPlatesUtils::CallStack::Trace &transcribe_source, // Use 'TRANSCRIBE_SOURCE' here
			Scribe &scribe,
			ObjectType *&object_ptr,
			bool shared_owner,
			boost::optional<unsigned int> use_count_hint)
	{
		// Track the file/line of the call site for exception messages.
		GPlatesUtils::CallStackTracker call_stack_tracker(transcribe_source);

		// Wrap in a Bool object to force caller to check return code.
		return ScribeInternalAccess::create_bool(
				transcribe_source,
				ScribeInternalAccess::transcribe_smart_pointer(scribe, object_ptr, shared_owner, use_count_hint),
				scribe.is_loading()/*require_check*/);
	}


	namespace Implementation
	{
		//! Overload for pointed-to types with an intrusive reference count (eg, GPlatesUtils::ReferenceCount).
		template <typename ObjectType>
		auto
		get_intrusive_use_count_hint(
				const ObjectType *object_ptr,
				int/*preferred overload*/)
				-> decltype(
						static_cast<void>(object_ptr->get_reference_count()),
						boost::optional<unsigned int>())
		{
			if (object_ptr == nullptr)
			{
				return boost::none;
			}

			return boost::optional<unsigned int>(
					static_cast<unsigned int>(object_ptr->get_reference_count()));
		}

		//! Overload for pointed-to types without an intrusive reference count.
		template <typename ObjectType>
		boost::optional<unsigned int>
		get_intrusive_use_count_hint(
				const ObjectType *,
				long/*fallback overload*/)
		{
			return boost::none;
		}
	}

	/**
	 * Returns the intrusive reference count of the pointed-to object, if it has one
	 * (ie, if 'ObjectType' has a 'get_reference_count()' method - eg, derives from
	 * GPlatesUtils::ReferenceCount), otherwise returns none.
	 *
	 * An intrusive reference count counts *all* owners of the pointed-to object (there are no
	 * weak intrusive pointers), which is what makes it safe to use as a use count hint for
	 * @a transcribe_smart_pointer_protocol.
	 */
	template <typename ObjectType>
	boost::optional<unsigned int>
	get_intrusive_use_count_hint(
			const ObjectType *object_ptr)
	{
		return Implementation::get_intrusive_use_count_hint(object_ptr, 0/*prefer intrusive overload*/);
	}
}

#endif // GPLATES_SCRIBE_TRANSCRIBESMARTPOINTERPROTOCOL_H
