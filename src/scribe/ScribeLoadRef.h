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

#ifndef GPLATES_SCRIBE_SCRIBELOADREF_H
#define GPLATES_SCRIBE_SCRIBELOADREF_H

#include <boost/shared_ptr.hpp>

#include "ScribeExceptions.h"

#include "utils/CallStackTracker.h"


namespace GPlatesScribe
{
	class Scribe;
	class ScribeInternalAccess;


	/**
	 * A shared reference to an object loaded from an archive using Scribe::load() or a reference
	 * to an object using Scribe::load_reference().
	 *
	 * If the loaded object (via Scribe::load()) is tracked then either:
	 *  1) The scribe client needs to relocate from the LoadRef to the object's final resting place, or
	 *  2) The client does not relocate and, when all LoadRef's to the tracked object go out of scope,
	 *     the object is automatically untracked/discarded. This assumes that the client decided
	 *     not to use the loaded object for some reason. If the client meant to relocate but forgot to
	 *     then it should still be OK unless a transcribed pointer references the discarded object
	 *     in which case loading will fail.
	 *
	 * ...note that if the object was not tracked in the first place then the above does not matter/apply.
	 */
	template <typename ObjectType>
	class LoadRef
	{
	public:

		/**
		 * A NULL reference (no object referenced).
		 */
		LoadRef()
		{  }


		/**
		 * Return whether this reference is valid to be dereferenced, or whether it's a NULL reference.
		 *
		 * To use:
		 *
		 *    	GPlatesScribe::LoadRef<int> x = scribe.load<int>(TRANSCRIBE_SOURCE, "x");
		 *    	if (!x.is_valid())
		 *    	{
		 *    		return scribe.get_transcribe_result();
		 *    	}
		 *    	
		 *		int x_deref = x.get();
		 *
		 * NOTE: If this method is not called then Exceptions::ScribeTranscribeResultNotChecked is
		 * thrown to notify the programmer to insert the check.
		 */
		bool
		is_valid() const;


		/**
		 * Get the referenced object.
		 *    
		 * @throws Exceptions::ScribeUserError if NULL reference (no object referenced).
		 */
		ObjectType &
		get() const;


		/**
		 * Convenient conversion operator.
		 *
		 * It enables a 'LoadRef<>' to be used directly in 'ConstructObject<>::construct_object()',
		 * in a similar manner to 'boost::ref()', for transporting *non-const* references via the
		 * *const* reference arguments of 'construct_object()' as shown in the following code:
		 *
		 *    GPlatesScribe::TranscribeResult
		 *    transcribe_construct_data(
		 *    		GPlatesScribe::Scribe &scribe,
		 *    		GPlatesScribe::ConstructObject<A> &a)
		 *    {
		 *    	if (scribe.is_saving())
		 *    	{
		 *    		scribe.save(TRANSCRIBE_SOURCE, a->x, "x");
		 *    	}
		 *    	else // loading...
		 *    	{
		 *    		GPlatesScribe::LoadRef<int> x = scribe.load<int>(TRANSCRIBE_SOURCE, "x");
		 *    		if (!x.is_valid())
		 *    		{
		 *    			return scribe.get_transcribe_result();
		 *    		}
		 *
		 *    		a.construct_object(x); // <----- can use 'x' instead of 'boost::ref(x.get())'
		 *    		scribe.relocated(TRANSCRIBE_SOURCE, a->x, x);
		 *    	}
		 *    	
		 *    	return GPlatesScribe::TRANSCRIBE_SUCCESS;
		 *    }
		 *    
		 * @throws Exceptions::ScribeUserError if NULL reference (no object referenced).
		 */
		operator ObjectType &() const
		{
			return get();
		}


		/**
		 * Indirection operator.
		 *
		 * To use:
		 *
		 *    	GPlatesScribe::LoadRef<A> a = scribe.load<A>(TRANSCRIBE_SOURCE, "a");
		 *    	if (!a.is_valid())
		 *    	{
		 *    		return scribe.get_transcribe_result();
		 *    	}
		 *    	
		 *		int x = a->x;
		 */
		ObjectType *
		operator->() const
		{
			return &get();
		}

	private:

		/**
		 * Custom boost::shared_ptr deleter that untracks the object if it's still being tracked.
		 */
		struct TrackingDeleter;


		/**
		 * Successful transcribe - @a object should be non-null.
		 *
		 * If @a release is true then ownership of the object is transferred to this LoadRef which
		 * must then release it once all LoadRef's go out of scope.
		 */
		LoadRef(
				const GPlatesUtils::CallStack::Trace &transcribe_source,
				Scribe &scribe,
				ObjectType *object,
				bool release);

		// Used by TrackingDeleter to get friend access to ScribeInternalAccess.
		static
		void
		untrack(
				Scribe *scribe,
				ObjectType *object,
				bool discard);


		boost::shared_ptr<ObjectType> d_object;


		friend class Scribe;
		friend class ScribeInternalAccess;
	};
}

#endif // GPLATES_SCRIBE_SCRIBELOADREF_H
