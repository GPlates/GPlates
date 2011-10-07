/* $Id$ */

/**
 * \file Reference count class that can be used with boost::intrusive_ptr.
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#ifndef GPLATES_UTILS_REFERENCECOUNT_H
#define GPLATES_UTILS_REFERENCECOUNT_H

#include <boost/bind.hpp>
#include <boost/checked_delete.hpp>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>

#include "global/GPlatesAssert.h"
#include "global/IntrusivePointerZeroRefCountException.h"

#include "utils/non_null_intrusive_ptr.h"


namespace GPlatesUtils
{
	/**
	 * Forward declaration of base reference count class.
	 */
	template <class Derived> class ReferenceCount;


	/**
	 * Creates a non-null shared intrusive pointer to @a reference_count_derived
	 * which is assumed to derive directly or indirectly from @a ReferenceCount.
	 *
	 * @a reference_count_derived must point to an object with non-zero reference count.
	 *
	 * @throws @a IntrusivePointerZeroRefCountException if the reference count is zero.
	 */
	template <class U>
	GPlatesUtils::non_null_intrusive_ptr<U>
	get_non_null_pointer(
			U *reference_count_derived);

	/**
	 * Creates a boost::shared_ptr to @a reference_count_derived which is assumed to
	 * derive directly or indirectly from @a ReferenceCount.
	 *
	 * The returned shared pointer and any @a non_null_intrusive_ptr pointers currently
	 * referencing @a reference_count_derived all share ownership of it.
	 *
	 * @a reference_count_derived must point to an object with non-zero reference count.
	 *
	 * @throws @a IntrusivePointerZeroRefCountException if the reference count is zero.
	 */
	template <class U>
	boost::shared_ptr<U>
	make_shared_from_intrusive(
			U *reference_count_derived);

	/**
	 * Creates a boost::shared_ptr to the object referenced by @a non_null_ptr.
	 *
	 * The returned shared pointer and @a non_null_ptr both share ownership of the referenced object.
	 * This includes any other @a non_null_intrusive_ptr intrusive pointers referencing that same object.
	 */
	template <class U>
	boost::shared_ptr<U>
	make_shared_from_intrusive(
			const GPlatesUtils::non_null_intrusive_ptr<U> &non_null_ptr);


	/**
	 * Allows incrementing, decrementing and retrieving a reference count.
	 * Useful for boost::intrusive_ptr.
	 * Classes should derive publicly from @a ReferenceCount and pass the type of
	 * the derived class into the template parameter 'Derived'
	 *
	 * NOTE: the template parameter 'Derived' should be the type of class that
	 * is deriving from 'ReferenceCount'. This is the curiously recurring template pattern.
	 * For example,
	 *    class ProximityHitDetail :
	 *       public GPlatesUtils::ReferenceCount<ProximityHitDetail>
	 *    ...
	 * This is used by 'intrusive_ptr_release()' below. See that function's comments
	 * for details of why this is done.
	 */
	template <class Derived>
	class ReferenceCount :
			private boost::noncopyable
	{
	public:
		/**
		 * The type used to store the reference-count of an instance of this class.
		 */
		typedef int ref_count_type;

		/**
		 * Constructor.
		 */
		ReferenceCount() :
			d_ref_count(0)
		{  }

		/**
		 * Increment the reference-count of this instance.
		 *
		 * Client code should not use this function when used with boost::intrusive_ptr or
		 * GPlatesUtils::non_null_intrusive_ptr!
		 */
		void
		increment_ref_count() const
		{
			++d_ref_count;
		}

		/**
		 * Decrement the reference-count of this instance, and return the new
		 * reference-count.
		 *
		 * Client code should not use this function when used with boost::intrusive_ptr or
		 * GPlatesUtils::non_null_intrusive_ptr!
		 */
		ref_count_type
		decrement_ref_count() const
		{
			return --d_ref_count;
		}

		/**
		 * Returns the current reference count.
		 */
		ref_count_type
		get_reference_count() const
		{
			return d_ref_count;
		}

	private:
		/**
		 * The reference-count of this instance by intrusive-pointers.
		 */
		mutable ref_count_type d_ref_count;
	};

	template <class Derived>
	inline
	void
	intrusive_ptr_add_ref(
			const ReferenceCount<Derived> *p)
	{
		p->increment_ref_count();
	}

	template <class Derived>
	inline
	void
	intrusive_ptr_release(
			const ReferenceCount<Derived> *p)
	{
		if (p->decrement_ref_count() == 0)
		{
			// Use the curiously recurring template pattern to delete the class derived
			// from @a ReferenceCount.
			//
			// By deleting the derived class pointer instead of the @a ReferenceCount pointer we
			// avoid requiring a virtual destructor and the associated increase in size of class
			// @a ReferenceCount due to the virtual table pointer stored in a @a ReferenceCount object.
			// This assumes that each base class (of a derived class) that has virtual functions
			// will result in an additional virtual table pointer stored in the object itself.
			//
			// A static_cast is fine here (no need for dynamic_cast which would require a
			// virtual destructor again) since the compiler knows the 'Derived' type at
			// compile-time and can perform any pointer-fixups required for multiple-inheritance.
			//
			// If class Derived has further classes derived from it then class Derived will
			// have a virtual destructor so that deleting a pointer to Derived will destroy
			// the full object properly.
			//
			// We use boost::checked_delete() to make sure we have the class definition of
			// 'Derived' otherwise if 'Derived' has a non-trivial destructor then it might
			// not get called.
			boost::checked_delete( static_cast<const Derived *>(p) );
		}
	}


	template <class U>
	GPlatesUtils::non_null_intrusive_ptr<U>
	get_non_null_pointer(
			U *reference_count_derived)
	{
		GPlatesGlobal::Assert<GPlatesGlobal::IntrusivePointerZeroRefCountException>(
				reference_count_derived->get_reference_count() != 0,
				GPLATES_ASSERTION_SOURCE,
				reference_count_derived);

		// This instance is already managed by intrusive-pointers, so we can simply return
		// another intrusive-pointer to this instance.
		return GPlatesUtils::non_null_intrusive_ptr<U>(reference_count_derived);
	}


	template <class U>
	boost::shared_ptr<U>
	make_shared_from_intrusive(
			U *reference_count_derived)
	{
		GPlatesGlobal::Assert<GPlatesGlobal::IntrusivePointerZeroRefCountException>(
				reference_count_derived->get_reference_count() != 0,
				GPLATES_ASSERTION_SOURCE,
				reference_count_derived);

		intrusive_ptr_add_ref(reference_count_derived);

		return boost::shared_ptr<U>(
				reference_count_derived,
				boost::bind(&intrusive_ptr_release<U>, _1));
	}


	template <class U>
	boost::shared_ptr<U>
	make_shared_from_intrusive(
			const GPlatesUtils::non_null_intrusive_ptr<U> &non_null_ptr)
	{
		U *reference_count_derived = non_null_ptr.get();

		intrusive_ptr_add_ref(reference_count_derived);

		return boost::shared_ptr<U>(
				reference_count_derived,
				boost::bind(&intrusive_ptr_release<U>, _1));
	}
}

#endif // GPLATES_UTILS_REFERENCECOUNT_H
