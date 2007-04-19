/* $Id$ */

/**
 * \file 
 * Contains the definition of the class FeatureStore.
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

#ifndef GPLATES_MODEL_FEATURESTORE_H
#define GPLATES_MODEL_FEATURESTORE_H

#include "FeatureStoreRootHandle.h"
#include "contrib/non_null_intrusive_ptr.h"

namespace GPlatesModel
{
	/**
	 * The feature store contains (directly or indirectly) all the currently-loaded features
	 * and feature collections, as well as all past and future states (which are reachable by
	 * the undo and redo operations) of those features and feature collections.
	 *
	 * The feature store contains a three-tiered conceptual hierarchy of revisioned objects: 
	 * The top layer/component is the feature store root, the singleton "root" vertex of the
	 * tree of revisioned objects contained within the feature store; the feature store root
	 * contains all the currently-loaded feature collections (each of which corresponds to a
	 * single data file); and each feature collection contains zero or more features.
	 */
	class FeatureStore
	{
	public:
		/**
		 * A convenience typedef for GPlatesContrib::non_null_intrusive_ptr<FeatureStore>.
		 */
		typedef GPlatesContrib::non_null_intrusive_ptr<FeatureStore> non_null_ptr_type;

		/**
		 * A convenience typedef for
		 * GPlatesContrib::non_null_intrusive_ptr<const FeatureStore>.
		 */
		typedef GPlatesContrib::non_null_intrusive_ptr<const FeatureStore>
				non_null_ptr_to_const_type;

		/**
		 * The type used to store the reference-count of an instance of this class.
		 */
		typedef long ref_count_type;

		/**
		 * Create a new FeatureStore instance.
		 */
		static
		const non_null_ptr_type
		create()
		{
			non_null_ptr_type ptr(*(new FeatureStore()));
			return ptr;
		}

		~FeatureStore()
		{  }

		/**
		 * Access the feature store root contained within this feature store.
		 *
		 * Note that, because the copy-assignment operator of FeatureStoreRootHandle is
		 * private, the FeatureStoreRootHandle instance referenced by the return-value of
		 * this function cannot be assigned-to, which means that this function does not
		 * provide a means to switch the FeatureStoreRootHandle instance within this
		 * FeatureStore instance.  (This restriction is intentional.)
		 */
		const FeatureStoreRootHandle::non_null_ptr_type
		root()
		{
			return d_root;
		}

		/**
		 * Increment the reference-count of this instance.
		 *
		 * Client code should not use this function!
		 *
		 * This function is used by boost::intrusive_ptr and
		 * GPlatesContrib::non_null_intrusive_ptr.
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
		 * Client code should not use this function!
		 *
		 * This function is used by boost::intrusive_ptr and
		 * GPlatesContrib::non_null_intrusive_ptr.
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
		 * The feature store root contained within this feature store.
		 */
		FeatureStoreRootHandle::non_null_ptr_type d_root;

		/**
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 */
		FeatureStore():
			d_ref_count(0),
			d_root(FeatureStoreRootHandle::create())
		{  }

		/**
		 * This constructor should never be defined, because we don't want to allow
		 * copy-construction.
		 */
		FeatureStore(
				const FeatureStore &);

		/**
		 * This operator should never be defined, because we don't want to allow
		 * copy-assignment.
		 *
		 * All "copy-assignment" should really only be the assignment of one intrusive_ptr
		 * to another.
		 */
		FeatureStore &
		operator=(
				const FeatureStore &);
	};


	inline
	void
	intrusive_ptr_add_ref(
			const FeatureStore *p)
	{
		p->increment_ref_count();
	}


	inline
	void
	intrusive_ptr_release(
			const FeatureStore *p)
	{
		if (p->decrement_ref_count() == 0) {
			delete p;
		}
	}

}

#endif  // GPLATES_MODEL_FEATURESTORE_H
