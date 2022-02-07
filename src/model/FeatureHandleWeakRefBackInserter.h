/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
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

#ifndef GPLATES_MODEL_FEATUREHANDLEWEAKREFBACKINSERTER_H
#define GPLATES_MODEL_FEATUREHANDLEWEAKREFBACKINSERTER_H

#include <iterator>
#include "FeatureHandle.h"


namespace GPlatesModel {

	/**
	 * A back inserter for FeatureHandle weak-refs.
	 *
	 * This is a back inserter which converts the arguments of the assignment to FeatureHandle
	 * weak-refs and then inserts the weak-refs at the back of the supplied container.
	 *
	 * Consult Josuttis p.272 for more information on back inserters.
	 * Consult Josuttis p.253 for more information on output iterators.
	 * Consult Josuttis p.289 for an example insert iterator.
	 */
	template<typename C>
	class FeatureHandleWeakRefBackInserter:
			public std::iterator<std::output_iterator_tag, void, void, void, void>
	{
	public:
		/**
		 * The type of the target container.
		 */
		typedef C container_type;

		/**
		 * The type of this class.
		 */
		typedef FeatureHandleWeakRefBackInserter this_type;

		/**
		 * Construct an instance of this class which will insert into @a target_container.
		 */
		explicit
		FeatureHandleWeakRefBackInserter(
				container_type &target_container):
			d_target_container_ptr(&target_container)
		{  }

		/**
		 * Copy-constructor.
		 *
		 * Construct an instance of this class which will insert into the same container as
		 * @a other.
		 */
		FeatureHandleWeakRefBackInserter(
				const FeatureHandleWeakRefBackInserter &other):
			d_target_container_ptr(other.d_target_container_ptr)
		{  }

		/**
		 * Insert @a fh_ptr.
		 */
		this_type &
		operator=(
				FeatureHandle *fh_ptr)
		{
			d_target_container_ptr->push_back(fh_ptr->reference());
			return *this;
		}

		/**
		 * No-op.
		 */
		this_type &
		operator*()
		{
			return *this;
		}

		/**
		 * No-op.
		 */
		this_type &
		operator++()
		{
			return *this;
		}

		/**
		 * No-op.
		 *
		 * Note that the non-standard return type of this post-increment function (that is,
		 * @c this_type @c & rather than @c const @c this_type) is copied from the example
		 * in Josuttis, p289.
		 */
		this_type &
		operator++(int)
		{
			return *this;
		}

	private:
		container_type *d_target_container_ptr;
	};


	/**
	 * Convenience function to create an instance of the inserter.
	 */
	template<typename C>
	inline
	FeatureHandleWeakRefBackInserter<C>
	append_as_weak_refs(
			C &container)
	{
		return FeatureHandleWeakRefBackInserter<C>(container);
	}
}

#endif  // GPLATES_MODEL_FEATUREHANDLEWEAKREFBACKINSERTER_H
