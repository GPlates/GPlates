/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#ifndef GPLATES_UTILS_SUBJECTOBSERVERTOKEN_H
#define GPLATES_UTILS_SUBJECTOBSERVERTOKEN_H

#include <boost/cstdint.hpp>


namespace GPlatesUtils
{
	namespace Implementation
	{
#ifdef BOOST_NO_INT64_T
		/**
		 * Just in case we happen to run into a compiler without 64-bit integers.
		 *
		 * Shouldn't really happen on any systems that Qt supports but better to be sure.
		 */
		class InvalidateCounterType
		{
		public:
			//! Constructor to instantiate from a 32-bit integer (defaults to zero).
			explicit
			InvalidateCounterType(
					boost::uint32_t low = 0) :
				d_high(0),
				d_low(low)
			{  }

			InvalidateCounterType &
			operator++()
			{
				++d_low;
				if (d_low == 0) // integer overflow
				{
					++d_high;
				}
				return *this;
			}

			bool
			operator==(
					const InvalidateCounterType &other) const
			{
				return d_low == other.d_low && d_high == other.d_high;
			}

		private:
			boost::uint32_t d_high, d_low; // This should be portable.
		};

		typedef InvalidateCounterType invalidate_counter_type;
#else
		// Use built-in 64-bit integers where available.
		typedef boost::uint64_t invalidate_counter_type;
#endif
	}


	/**
	 * Used to effect a simple polling version of the subject-observer pattern.
	 *
	 * This has the benefit of avoiding signals and callbacks which, in certain situations,
	 * can complicate matter and lead to circular dependencies and unknown ordering of callbacks.
	 *
	 * This solution involves incrementing a counter (in a @a SubjectToken object) every time a
	 * subject has updated itself - and observers can compare their @a ObserverToken objects with
	 * the subject's @a SubjectToken object to see if the observer needs updating.
	 *
	 * NOTE: This incrementing can have problems due to integer overflow and subsequent wraparound
	 * back to zero but we're using 64-bit integers which, if we incremented every CPU cycle
	 * (ie, the fastest possible incrementing) on a 3GHz system it would take 195 years to overflow.
	 * So we are safe as long as we are guaranteed to use 64-bit integers (and for this there is
	 * 64-bit simulation code for those systems that only support 32-bit integers -
	 * which should be very few). Use of 32-bit integers brings this down from 195 years to
	 * a couple of seconds so 64-bit must be used.
	 */
	class ObserverToken
	{
	public:
		ObserverToken() :
			d_invalidate_counter(0)
		{  }


		/**
		 * Resets this observer such that is *not* up-to-date with its subject.
		 *
		 * Unless the corresponding @a SubjectToken has just been created and has a
		 * non-default constructor argument of 'false'.
		 */
		void
		reset()
		{
			d_invalidate_counter = 0;
		}

	private:
		Implementation::invalidate_counter_type d_invalidate_counter;

		friend class SubjectToken;
	};


	/**
	 * The complement of the @a ObserverToken.
	 *
	 * See @a ObserverToken for details.
	 */
	class SubjectToken
	{
	public:
		/**
		 * If @a invalidate is 'true' then call @a invalidate on construction.
		 *
		 * This is useful to force any new observers to update themselves once before they are
		 * up-to-date with respect to this subject.
		 */
		explicit
		SubjectToken(
				bool invalidate_ = true) :
			d_invalidate_counter(0)
		{
			if (invalidate_)
			{
				invalidate();
			}
		}


		/**
		 * Returns true if the specified observer is up-to-date with this subject.
		 *
		 * If this returns false then the observer needs to update its state to reflect the latest subject state.
		 */
		bool
		is_observer_up_to_date(
				const ObserverToken &observer) const
		{
			return d_invalidate_counter == observer.d_invalidate_counter;
		}


		/**
		 * Updates the specified observer so it is valid with respect to this subject.
		 *
		 * This should be done after an observer has updated its state to reflect the latest subject state.
		 */
		void
		update_observer(
				ObserverToken &observer) const
		{
			observer.d_invalidate_counter = d_invalidate_counter;
		}


		/**
		 * Invalidates this subject.
		 *
		 * Any observers will then become invalid.
		 */
		void
		invalidate()
		{
			++d_invalidate_counter;
		}

	private:
		Implementation::invalidate_counter_type d_invalidate_counter;
	};
}

#endif // GPLATES_UTILS_SUBJECTOBSERVERTOKEN_H
