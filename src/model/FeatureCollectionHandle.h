/* $Id$ */

/**
 * \file 
 * Contains the definition of the class FeatureCollectionHandle.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006 The University of Sydney, Australia
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

#ifndef GPLATES_MODEL_FEATURECOLLECTIONHANDLE_H
#define GPLATES_MODEL_FEATURECOLLECTIONHANDLE_H

#include <boost/intrusive_ptr.hpp>
#include "FeatureCollectionRevision.h"
#include "HandleContainerIterator.h"

namespace GPlatesModel
{
	/**
	 * A feature collection handle is the part of a GML feature collection which does not
	 * change with revisions.
	 *
	 * This class represents the part of a GML feature collection which does not change with
	 * revisions (currently nothing), as well as acting as an everlasting handle to the
	 * FeatureCollectionRevision which contains the rest of the feature collection.
	 *
	 * A FeatureCollectionHandle instance is referenced by a FeatureStoreRoot.
	 */
	class FeatureCollectionHandle
	{
	public:
		/**
		 * The type used to store the reference-count of an instance of this class.
		 */
		typedef long ref_count_type;

		/**
		 * The type used for iterating over the collection of feature handles (const).
		 */
		typedef HandleContainerIterator<const FeatureCollectionHandle,
				const std::vector<boost::intrusive_ptr<FeatureHandle> >,
				boost::intrusive_ptr<const FeatureHandle> > const_iterator;

		/**
		 * The type used for iterating over the collection of feature handles (non-const).
		 */
		typedef HandleContainerIterator<FeatureCollectionHandle,
				std::vector<boost::intrusive_ptr<FeatureHandle> >,
				boost::intrusive_ptr<FeatureHandle> > iterator;

		~FeatureCollectionHandle()
		{  }

		/**
		 * Create a new FeatureCollectionHandle instance.
		 */
		static
		const boost::intrusive_ptr<FeatureCollectionHandle>
		create()
		{
			boost::intrusive_ptr<FeatureCollectionHandle> ptr(
					new FeatureCollectionHandle());
			return ptr;
		}

		/**
		 * Create a duplicate of this FeatureCollectionHandle instance.
		 *
		 * Note that this will perform a "shallow copy".
		 */
		const boost::intrusive_ptr<FeatureCollectionHandle>
		clone() const
		{
			boost::intrusive_ptr<FeatureCollectionHandle> dup(
					new FeatureCollectionHandle(*this));
			return dup;
		}

		/**
		 * Return the "begin" const-iterator to iterate over the collection of features.
		 */
		const const_iterator
		begin() const
		{
			return const_iterator::create_begin(*this);
		}

		/**
		 * Return the "begin" iterator to iterate over the collection of features.
		 */
		const iterator
		begin()
		{
			return iterator::create_begin(*this);
		}

		/**
		 * Return the "end" const-iterator used during iteration over the collection of
		 * features.
		 */
		const const_iterator
		end() const
		{
			return const_iterator::create_end(*this);
		}

		/**
		 * Return the "end" iterator used during iteration over the collection of features.
		 */
		const iterator
		end()
		{
			return iterator::create_end(*this);
		}

		/**
		 * Access the current revision of this feature collection.
		 *
		 * This is the overloading of this function for const FeatureCollectionHandle
		 * instances; it returns a pointer to a const FeatureCollectionRevision instance.
		 */
		const boost::intrusive_ptr<const FeatureCollectionRevision>
		current_revision() const
		{
			return d_current_revision;
		}

		/**
		 * Access the current revision of this feature collection.
		 *
		 * This is the overloading of this function for non-const FeatureCollectionHandle
		 * instances; it returns a C++ reference to a pointer to a non-const
		 * FeatureCollectionRevision instance.
		 *
		 * Note that, because the copy-assignment operator of FeatureCollectionRevision is
		 * private, the FeatureCollectionRevision referenced by the return-value of this
		 * function cannot be assigned-to, which means that this function does not provide
		 * a means to directly switch the FeatureCollectionRevision within this
		 * FeatureCollectionHandle instance.  (This restriction is intentional.)
		 *
		 * To switch the FeatureCollectionRevision within this FeatureCollectionHandle
		 * instance, use the function @a set_current_revision below.
		 */
		const boost::intrusive_ptr<FeatureCollectionRevision>
		current_revision()
		{
			return d_current_revision;
		}

		/**
		 * Set the current revision of this feature collection to @a rev.
		 *
		 * FIXME:  This pointer should not be allowed to be NULL.
		 */
		void
		set_current_revision(
				boost::intrusive_ptr<FeatureCollectionRevision> rev)
		{
			d_current_revision = rev;
		}

		/**
		 * Increment the reference-count of this instance.
		 *
		 * This function is used by boost::intrusive_ptr.
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
		 * This function is used by boost::intrusive_ptr.
		 */
		ref_count_type
		decrement_ref_count() const
		{
			return --d_ref_count;
		}

	private:

		/**
		 * The reference-count of this instance by intrusive-pointers.
		 */
		mutable ref_count_type d_ref_count;

		/**
		 * The current revision of this feature collection.
		 *
		 * FIXME:  This pointer should not be allowed to be NULL.
		 */
		boost::intrusive_ptr<FeatureCollectionRevision> d_current_revision;

		/**
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 */
		FeatureCollectionHandle():
			d_ref_count(0),
			d_current_revision(FeatureCollectionRevision::create())
		{  }

		/**
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 *
		 * This ctor should only be invoked by the 'clone' member function, which will
		 * create a duplicate instance and return a new intrusive_ptr reference to the new
		 * duplicate.  Since initially the only reference to the new duplicate will be the
		 * one returned by the 'clone' function, *before* the new intrusive_ptr is created,
		 * the ref-count of the new FeatureCollectionHandle instance should be zero.
		 *
		 * Note that this ctor should act exactly the same as the default (auto-generated)
		 * copy-ctor, except that it should initialise the ref-count to zero.
		 */
		FeatureCollectionHandle(
				const FeatureCollectionHandle &other) :
			d_ref_count(0),
			d_current_revision(other.d_current_revision)
		{  }

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		FeatureCollectionHandle &
		operator=(
				const FeatureCollectionHandle &);
	};


	inline
	void
	intrusive_ptr_add_ref(
			const FeatureCollectionHandle *p)
	{
		p->increment_ref_count();
	}


	inline
	void
	intrusive_ptr_release(
			const FeatureCollectionHandle *p)
	{
		if (p->decrement_ref_count() == 0) {
			delete p;
		}
	}

}

#endif  // GPLATES_MODEL_FEATURECOLLECTIONHANDLE_H
