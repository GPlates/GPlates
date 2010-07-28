/* $Id$ */

/**
 * \file 
 * Contains the definition of the class TopLevelPropertyRef.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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

#ifndef GPLATES_MODEL_TOPLEVELPROPERTYREF_H
#define GPLATES_MODEL_TOPLEVELPROPERTYREF_H

#include <boost/scoped_ptr.hpp>

#include "types.h"

#include "global/PointerTraits.h"

#include "utils/SafeBool.h"


namespace GPlatesModel
{
	class FeatureHandle;
	class TopLevelProperty;

	/**
	 * A TopLevelPropertyRef object is returned by the RevisionAwareIterator on
	 * dereference to capture attempts to set a (non-const) Feature's TopLevelProperty child.
	 * Using this mechanism, we will know when a FeatureCollection has unsaved
	 * changes and we can also generate a Transaction for undo-redo purposes.
	 *
	 * Example usage:
	 *
	 *		FeatureHandle::weak_ref feature = ...;
	 *		FeatureHandle::children_iterator iter = feature->children_begin();
	 *
	 *		TopLevelProperty::non_null_ptr_to_const_type tlp = *iter;
	 *			// *iter returns TopLevelPropertyRef, which can be converted into a TopLevelProperty::non_null_ptr_to_const_type
	 *		TopLevelProperty::non_null_ptr_type tlp2 = tlp->clone(); // deep clone
	 *
	 *		// ... do some stuff to tlp2 ...
	 *
	 *		*iter = tlp2;
	 *			// *iter returns TopLevelPropertyRef, which has an overloaded assignment operator.
	 *			// This clones the old FeatureRevision, clones tlp2, sets the appropriate child in the new FeatureRevision to be
	 *			// the clone of tlp2, creates a Transaction and then commits the Transaction.
	 *			// Note that tlp and tlp2 are now "invalid", in the sense that they point to old data.
	 *			// Note that we needed to clone tlp2, otherwise it would be possible to alter the Feature's property via tlp2
	 *			// while bypassing the undo-redo mechanism.
	 *
	 * What if we have a const Feature?
	 *
	 *		FeatureHandle::const_weak_ref feature = ...;
	 *		FeatureHandle::const_children_iterator iter = feature->children_begin();
	 *
	 *		TopLevelProperty::non_null_ptr_to_const_type tlp = *iter;
	 *			// It is still possible to get the value out from the iterator.
	 *		TopLevelProperty::non_null_ptr_type tlp2 = tlp->clone();
	 *
	 *		*iter = tlp2;
	 *			// This will fail because the Transaction object will insist on being given a non-const reference to FeatureHandle,
	 *			// which is something we can't provide since we only have a const_weak_ref to a FeatureHandle.
	 *
	 * As is clear from the examples, the use of TopLevelPropertyRef is meant to
	 * be transparent to client code. There should never be any need to declare a
	 * variable of type TopLevelPropertyRef in client code.
	 *
	 * Iterators over FeatureCollectionHandles and FeatureStoreRootHandles both
	 * return boost::intrusive_ptrs to child elements on dereference; this is also
	 * the case for const FeatureHandles.
	 */
	class TopLevelPropertyRef :
			public GPlatesUtils::SafeBool<TopLevelPropertyRef>
	{

	public:

		TopLevelPropertyRef(
				const HandleTraits<FeatureHandle>::iterator &iterator);

		TopLevelPropertyRef(
				const TopLevelPropertyRef &other);

		~TopLevelPropertyRef();

		//! Gives access to the TopLevelProperty. Undefined behaviour if index is invalid.
		operator GPlatesGlobal::PointerTraits<const TopLevelProperty>::non_null_ptr_type() const;

		// Undefined behaviour if index is invalid.
		const TopLevelProperty *
		operator->() const;

		// The same as operator->().
		const TopLevelProperty *
		get() const;

		// Undefined behaviour if index is invalid.
		const TopLevelProperty &
		operator*() const;

		//! Allows the TopLevelProperty to be changed. Does nothing if index is invalid.
		void
		operator=(
				GPlatesGlobal::PointerTraits<const TopLevelProperty>::non_null_ptr_type new_property);

		// operator bool() provided by SafeBool.
		bool
		boolean_test() const;

	private:

		//! Gets a pointer to the TopLevelProperty from the iterator.
		GPlatesGlobal::PointerTraits<const TopLevelProperty>::non_null_ptr_type
		pointer() const;

		//! An iterator that points to the TopLevelProperty that we're interested in.
		boost::scoped_ptr<HandleTraits<FeatureHandle>::iterator> d_iterator_ptr;

	};

}


#endif  // GPLATES_MODEL_TOPLEVELPROPERTYREF_H
