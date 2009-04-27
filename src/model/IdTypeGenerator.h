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

#ifndef GPLATES_MODEL_IDTYPEGENERATOR_H
#define GPLATES_MODEL_IDTYPEGENERATOR_H

#include <vector>
#include <unicode/unistr.h>
#include <boost/intrusive_ptr.hpp>
#include "StringSetSingletons.h"
#include "utils/UniqueId.h"
#include "utils/ReferenceCount.h"


namespace GPlatesModel
{
	// These using-declarations are necessary for successful lookup of 'intrusive_ptr_add_ref'
	// and 'intrusive_ptr_release' in G++ 4.1.2.  See the commit message for changeset 5634 for
	// details.  (The code compiles fine in MSVC 8 with and without the using-declarations.)
	using GPlatesUtils::intrusive_ptr_add_ref;
	using GPlatesUtils::intrusive_ptr_release;

	/**
	 * This class provides an efficient means of containing an ID, which is a Unicode string.
	 *
	 * Since the strings are unique in the IdStringSet, comparison for equality of ID instances
	 * is as simple as comparing a pair of iterators for equality.
	 *
	 * Since IdStringSet uses a 'std::set' for storage, testing whether an arbitrary Unicode
	 * string is a member of the IdStringSet has O(log n) cost.  Further, since all loaded ID
	 * instances are stored within the IdStringSet, it is inexpensive to test whether a desired
	 * ID instance is even loaded, without needing to iterate through all features.
	 *
	 * This class is for strings which are IDs.  To store strings which are qualified XML names
	 * (such as "gml:Point" or "gpml:Isochron") use QualifiedXmlName.  To store other strings,
	 * use StringContentTypeGenerator.
	 */
	template<typename SingletonType, typename BackRefTargetType>
	class IdTypeGenerator
	{
	public:
		typedef BackRefTargetType back_ref_target_type;

		typedef GPlatesUtils::IdStringSet::back_ref_list_type back_ref_list_type;

		typedef GPlatesUtils::IdStringSet::SharedIterator shared_iterator_type;

		class BackRef:
				public GPlatesUtils::IdStringSet::AbstractBackRef,
				public GPlatesUtils::ReferenceCount<BackRef>
		{
		public:
			BackRef(
					back_ref_target_type &target,
					shared_iterator_type &sh_iter):
				d_target_ptr(&target),
				d_node_for_back_ref_registration(this)
			{
				sh_iter.back_refs().append(d_node_for_back_ref_registration);
			}

			virtual
			~BackRef()
			{  }

			back_ref_target_type *
			target_ptr() const
			{
				return d_target_ptr;
			}

			back_ref_list_type::Node &
			node() const
			{
				return d_node_for_back_ref_registration;
			}

		private:
			/**
			 * A pointer to the target of the back-reference.
			 */
			back_ref_target_type *d_target_ptr;

			/**
			 * The smart node which is linked into the list of back-references.
			 *
			 * Linking this node into the list of back-references is what registers
			 * this BackRef instance as a back-reference for the ID.
			 *
			 * This data member is mutable because the node must be modified when it's
			 * spliced into or out of the list.
			 */
			mutable back_ref_list_type::Node d_node_for_back_ref_registration;

			// Disallow copy-construction.
			BackRef(
					const BackRef &);

			// Disallow copy-assignment.
			BackRef &
			operator=(
					const BackRef &);
		};


		/**
		 * Determine whether an arbitrary Unicode string is a member of the collection of
		 * loaded ID instances (without inserting the Unicode string into the collection).
		 */
		static
		bool
		is_loaded(
				const UnicodeString &s);

		IdTypeGenerator() :
			sh_iter(SingletonType::instance().insert(GPlatesUtils::generate_unique_id()))
		{  }

		/**
		 * Instantiate a new ID instance from a UnicodeString instance.
		 *
		 * The string should conform to the XML NCName production (see the class comment
		 * for FeatureId for justification).  (Note however that this constructor won't
		 * validate the contents of the input string.)
		 *
		 * This constructor is intended for use when parsing features from file which
		 * already possess this type of ID.
		 */
		explicit
		IdTypeGenerator(
				const UnicodeString &s) :
			sh_iter(SingletonType::instance().insert(s))
		{  }

		/**
		 * Access the Unicode string of the text content for this instance.
		 */
		const UnicodeString &
		get() const {
			return *sh_iter;
		}

		/**
		 * Set the back-reference target for this ID instance.
		 *
		 * Only the object in which an ID is @em defined (eg, in a FeatureHandle) should be
		 * the target of a back-ref; an object in which an ID is only @em used (eg, in a
		 * PropertyDelegate) should @em not be the target of a back-ref.
		 */
		void
		set_back_ref_target(
				back_ref_target_type &target)
		{
			d_back_ref_ptr = new BackRef(target, sh_iter);
		}


		/**
		 * Find all the back-reference targets for this ID.
		 *
		 * The template parameter @a Inserter is the type of an insert iterator; the
		 * function parameter @a inserter is an instance of type Inserter.
		 *
		 * Consult Josuttis p.272 for more information on back inserters.
		 * Consult Josuttis p.253 for more information on output iterators.
		 * Consult Josuttis p.289 for an example insert iterator.
		 *
		 * A suitable insert iterator, intended for use with feature IDs with back-refs to
		 * FeatureHandles, is FeatureHandleWeakRefBackInserter.  This class provides a
		 * convenience function @a append_as_weak_refs to create an instance of the
		 * inserter.  This inserter will populate a container such as @c std::vector with
		 * FeatureHandle::weak_ref instances for all the target FeatureHandles.
		 *
		 * An example usage for a feature ID would be:
		 * @code
		 * std::vector<FeatureHandle::weak_ref> back_ref_targets;
		 * feature_id.find_back_ref_targets(append_as_weak_refs(back_ref_targets));
		 * @endcode
		 */
		template<typename Inserter>
		void
		find_back_ref_targets(
				Inserter inserter) const
		{
			back_ref_list_type::iterator iter = sh_iter.back_refs().begin();
			back_ref_list_type::iterator end = sh_iter.back_refs().end();
			for ( ; iter != end; ++iter) {
				BackRef *back_ref = dynamic_cast<BackRef *>(*iter);
				if (back_ref != NULL) {
					*inserter = back_ref->target_ptr();
				}
			}
		}

		/**
		 * Determine whether another Id instance contains the same text content
		 * as this instance.
		 */
		bool
		is_equal_to(
				const IdTypeGenerator &other) const {
			return sh_iter == other.sh_iter;
		}

	private:

		shared_iterator_type sh_iter;

		// FIXME:  Should this instead be a scoped_ptr, with IdTypeGenerator's copy-ctor
		// and copy-assignment not copying it, or should it be left as intrusive_ptr?
		boost::intrusive_ptr<const BackRef> d_back_ref_ptr;
	};


	template<typename SingletonType, typename BackRefTargetType>
	inline
	bool
	GPlatesModel::IdTypeGenerator<SingletonType, BackRefTargetType>::is_loaded(
			const UnicodeString &s) {
		return SingletonType::instance().contains(s);
	}

	template<typename SingletonType, typename BackRefTargetType>
	inline
	bool
	operator==(
			const IdTypeGenerator<SingletonType, BackRefTargetType> &c1,
			const IdTypeGenerator<SingletonType, BackRefTargetType> &c2) {
		return c1.is_equal_to(c2);
	}

	template<typename SingletonType, typename BackRefTargetType>
	inline
	bool
	operator!=(
			const IdTypeGenerator<SingletonType, BackRefTargetType> &c1,
			const IdTypeGenerator<SingletonType, BackRefTargetType> &c2) {
		return ! c1.is_equal_to(c2);
	}

}

#endif  // GPLATES_MODEL_IDTYPEGENERATOR_H
