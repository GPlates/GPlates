/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2014 The University of Sydney, Australia
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

#ifndef GPLATES_SCRIBE_SCRIBELOADREFIMPL_H
#define GPLATES_SCRIBE_SCRIBELOADREFIMPL_H

#include <boost/checked_delete.hpp>
#include <QDebug>

#include "Scribe.h"
#include "ScribeExceptions.h"
#include "ScribeInternalAccess.h"
#include "ScribeInternalUtils.h"
#include "ScribeLoadRef.h"

#include "global/GPlatesAssert.h"


namespace GPlatesScribe
{
	//
	// Define nested 'struct TrackingDeleter' here (ie, after class Scribe) to avoid cyclic
	// dependency (since it makes a call to class Scribe and class Scribe calls 'LoadRef')...
	//
	template <typename ObjectType>
	struct LoadRef<ObjectType>::TrackingDeleter
	{
		explicit
		TrackingDeleter(
				const GPlatesUtils::CallStack::Trace &transcribe_source_,
				Scribe *scribe_,
				bool release_) :
			transcribe_source(transcribe_source_),
			scribe(scribe_),
			is_valid_called(false),
			release(release_),
			exception_thrown(false)
		{  }

		void
		operator()(
				ObjectType *object_ptr)
		{
			// Track the file/line of the call site for exception messages.
			// This is the file/line at which a load call was made which, in turn, returned a 'LoadRef<>'.
			GPlatesUtils::CallStackTracker call_stack_tracker(transcribe_source);

			// NOTE: The 'is_valid()-was-not-called' check is *not* done here - it's done in
			// '~LoadRef()' instead. This deleter is called from
			// 'boost::detail::shared_count::~shared_count()' which, being a destructor declared
			// without an exception-specification, is implicitly 'noexcept(true)'
			// (see [class.dtor]/3) - so any exception thrown here would call 'std::terminate'
			// without unwinding the stack (no handler could catch it and nothing would be logged).

			// Release the object if we are not referencing an existing object but instead own
			// the object we are referencing.
			if (release)
			{
				// We shouldn't be throwing any exceptions in deleter.
				// If one is thrown we just have to lump it and continue on.
				try
				{
					// Untrack the object if it is still tracked.
					// We do this regardless of whether the object load requested tracking or not
					// (ie, whether it's 'relocatable').
					//
					// If the object is already *untracked* then this 'untrack()' call does nothing.
					// There are two cases where the object is already *untracked*:
					//   (1) Tracking was requested (ie, 'TRACK' was specified) and
					//       the scribe client has relocated the LoadRef<> to its final object location, or
					//   (2) The object is *not* relocatable (ie, scribe client did not specify TRACK) and
					//       hence the object has already been untracked (and its children were also untracked).
					//
					// If the object is currently being *tracked* then it means the object is relocatable,
					// because the scribe client requested tracking (ie, specified TRACK),
					// but the object was not relocated and hence not used.
					// In this case we discard it which means we untrack it *and* all its children
					// (eg, an owning pointer *and* its pointed-to object).
					LoadRef<ObjectType>::untrack(scribe, object_ptr, true/*discard*/);
				}
				catch (...)
				{
				}

				boost::checked_delete(object_ptr);
			}
			// ...else referencing existing object so leave it alone.
		}

		GPlatesUtils::CallStack::Trace transcribe_source;
		Scribe *scribe;
		bool is_valid_called;
		bool release; //!< Whether to delete the object or not.
		bool exception_thrown; //!< Avoid throwing exception in LoadRef<>::get() and subsequently in ~LoadRef<>().
	};


	template <typename ObjectType>
	LoadRef<ObjectType>::~LoadRef() noexcept(false)
	{
		if (!d_object)
		{
			return;
		}

		TrackingDeleter *const tracking_deleter = boost::get_deleter<TrackingDeleter>(d_object);

		if (tracking_deleter == NULL ||
			tracking_deleter->is_valid_called)
		{
			return;
		}

		// If 'LoadRef<>::get()' has already thrown this same exception then don't throw another one.
		// Note that this covers the case where that exception was *caught* (and hence we are no
		// longer unwinding), which the 'is_unwinding()' test below would not detect.
		if (tracking_deleter->exception_thrown)
		{
			return;
		}

		// Only the *last* 'LoadRef' referencing the object gets to complain. The object is allowed
		// to be checked via any copy, so the verdict is only final once no other copy could still
		// call 'is_valid()'.
		if (d_object.use_count() > 1)
		{
			return;
		}

		// Track the file/line of the call site for exception messages.
		// This is the file/line at which a load call was made which, in turn, returned a 'LoadRef<>'.
		GPlatesUtils::CallStackTracker call_stack_tracker(tracking_deleter->transcribe_source);

		// If an exception is already propagating then throwing here would call 'std::terminate' and
		// destroy the information carried by that (more interesting) exception. So just log instead.
		//
		// This should be rare, since the programmer should be calling 'LoadRef<>::is_valid()' straight
		// after getting a 'LoadRef<>' instance - which leaves no real window for an outside exception
		// to arrive first.
		if (InternalUtils::is_unwinding())
		{
			qWarning() << "Incorrect Scribe usage: 'LoadRef<>::is_valid()' was not called "
					"(not throwing, since an exception is already propagating).";
			return;
		}

		// Throw exception if 'LoadRef<>::is_valid()' has not been checked by the caller.
		//
		// If this assertion is triggered then it means:
		//   * A Scribe client has called 'Scribe::load()', or 'Scribe::load_reference()',
		//     but has not checked 'LoadRef<>::is_valid()' on the returned LoadRef.
		//
		// To fix this do something like:
		//
		//	GPlatesScribe::LoadRef<X> x = scribe.load<X>(TRANSCRIBE_SOURCE, "x");
		//	if (!x.is_valid())
		//	{
		//		return scribe.get_transcribe_result();
		//	}
		//
		GPlatesGlobal::Assert<Exceptions::ScribeTranscribeResultNotChecked>(
				false,
				GPLATES_ASSERTION_SOURCE);
	}


	template <typename ObjectType>
	LoadRef<ObjectType>::LoadRef(
			const GPlatesUtils::CallStack::Trace &transcribe_source,
			Scribe &scribe,
			ObjectType *object,
			bool release) :
		// Note that if 'release' is true then 'object' has been allocated by operator new()
		// via 'LoadConstructObjectOnHeap'...
		d_object(object, TrackingDeleter(transcribe_source, &scribe, release))
	{
		// Successful transcribe requires non-null object.
		GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
				object,
				GPLATES_ASSERTION_SOURCE,
				"Expected non-null object in LoadRef.");
	}


	template <typename ObjectType>
	bool
	LoadRef<ObjectType>::is_valid() const
	{
		if (!d_object)
		{
			return false;
		}

		// Mark the LoadRef as having been checked by the client.
		boost::get_deleter<TrackingDeleter>(d_object)->is_valid_called = true;

		return true;
	}


	template <typename ObjectType>
	ObjectType &
	LoadRef<ObjectType>::get() const
	{
		// Make sure there's a valid object to dereference.
		GPlatesGlobal::Assert<Exceptions::ScribeUserError>(
				d_object,
				GPLATES_ASSERTION_SOURCE,
				"Attempted to dereference null LoadRef.");

		TrackingDeleter *tracking_deleter = boost::get_deleter<TrackingDeleter>(d_object);

		// Throw exception if 'LoadRef<>::is_valid()' has not been checked by the caller.
		//
		// If this assertion is triggered then it means:
		//   * A Scribe client has called 'Scribe::load()', or 'Scribe::load_reference()',
		//     but has not first checked 'LoadRef<>::is_valid()' on the returned LoadRef
		//     before dereferencing it.
		//
		// To fix this do something like:
		//
		//	GPlatesScribe::LoadRef<X> x_ref = scribe.load<X>(TRANSCRIBE_SOURCE, "x");
		//	if (!x_ref.is_valid())
		//	{
		//		return scribe.get_transcribe_result();
		//	}
		//	X &x = x_ref.get();
		//
		if (!tracking_deleter->is_valid_called)
		{
			// Track the file/line of the call site for exception messages.
			// This is the file/line at which a load call was made which, in turn, returned a 'LoadRef<>'.
			GPlatesUtils::CallStackTracker call_stack_tracker(tracking_deleter->transcribe_source);

			// Tell '~LoadRef()' not to also throw when the exception below unwinds the call stack
			// (or, if it gets caught, when this 'LoadRef' is subsequently destroyed).
			tracking_deleter->exception_thrown = true;

			GPlatesGlobal::Assert<Exceptions::ScribeTranscribeResultNotChecked>(
					false,
					GPLATES_ASSERTION_SOURCE);
		}

		return *d_object;
	}

	template <typename ObjectType>
	void
	LoadRef<ObjectType>::untrack(
			Scribe *scribe,
			ObjectType *object,
			bool discard)
	{
		ScribeInternalAccess::untrack(*scribe, *object, discard);
	}
}


#endif // GPLATES_SCRIBE_SCRIBELOADREFIMPL_H
