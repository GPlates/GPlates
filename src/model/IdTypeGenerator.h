/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2007, 2009, 2010 The University of Sydney, Australia
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
#include <boost/operators.hpp>
#include <boost/scoped_ptr.hpp>

#include "StringSetSingletons.h"

#include "global/unicode.h"

#include "utils/UniqueId.h"


namespace GPlatesModel
{
	/**
	 * This class provides an efficient means of containing an ID, which is a Unicode string.
	 *
	 * An ID may also be associated with an object which defines the ID (such as a feature
	 * which defines a feature ID).  This is enabled by an optional "back-reference" to the
	 * object which defines the ID -- for example, a FeatureId would contain an optional
	 * back-reference to a FeatureHandle.  In this example, the FeatureHandle would be the
	 * "target" of the back-reference.  The back-reference is optional, as -- again, for
	 * example -- not all FeatureId instances are contained within a feature; a FeatureId
	 * instance might also be contained within a GpmlPropertyDelegate instance.
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
	class IdTypeGenerator :
			// NOTE: Base class chaining is used to avoid increasing sizeof(IdTypeGenerator) due to multiple
			// inheritance from several empty base classes...
			public boost::less_than_comparable< IdTypeGenerator<SingletonType, BackRefTargetType>,
					boost::equality_comparable< IdTypeGenerator<SingletonType, BackRefTargetType> > >
	{
	public:
		typedef BackRefTargetType back_ref_target_type;
		typedef GPlatesUtils::IdStringSet::back_ref_list_type back_ref_list_type;
		typedef GPlatesUtils::IdStringSet::SharedIterator shared_iterator_type;

		/**
		 * An RAII class which encapsulates the idea of being a back-reference in a list of
		 * registered back-references for a given ID.
		 *
		 * When this class is destroyed, it automatically unsubscribes itself from the list
		 * of back-references.  The list of back-references is contained in an element of
		 * IdStringSet; the automatic-unsubscription behaviour is provided by a smart node
		 * from SmartNodeLinkedList.
		 *
		 * This class is non-copyable.  It is intended that instances of this class be
		 * allocated on the heap.
		 */
		class BackRef:
				public GPlatesUtils::IdStringSet::AbstractBackRef
		{
		public:
			/**
			 * Constructor.
			 *
			 * @a target is the target of the back-reference.  @a sh_iter indicates the
			 * element in IdStringSet.
			 *
			 * When this constructor is complete, it will be a registered back-ref for
			 * the ID indicated by @a sh_iter.
			 *
			 * This constructor won't throw.
			 */
			BackRef(
					back_ref_target_type &target,
					shared_iterator_type &sh_iter):
				d_target_ptr(&target)
			{
				// For some reason, VS2008 is giving us warning C4355 ('this'
				// used in base member initializer list) - which makes no sense.
				// That's why we initialise it here.
				d_node_for_back_ref_registration.reset(
						new back_ref_list_type::Node(this));

				// Register this BackRef as a back-reference for this ID.
				sh_iter.back_refs().append(*d_node_for_back_ref_registration);
			}

			virtual
			~BackRef()
			{  }

			/**
			 * Access the target of this back-reference, an object which defines this
			 * ID.
			 */
			back_ref_target_type *
			target_ptr() const
			{
				return d_target_ptr;
			}

			/**
			 * Access the smart node which is linked into the list of back-references.
			 */
			back_ref_list_type::Node &
			node() const
			{
				return *d_node_for_back_ref_registration;
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
			 *
			 * When this BackRef instance is destroyed, the contained Node will also be
			 * destroyed.  When the Node is destroyed, its destructor will splice it
			 * out of the list, which will de-register this BackRef instance from the
			 * list of back-references for the ID.  Thus, the lifetime of the Node must
			 * be the same as the lifetime of this BackRef instance.
			 */
			boost::scoped_ptr<back_ref_list_type::Node> d_node_for_back_ref_registration;

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
				const GPlatesUtils::UnicodeString &s);

		IdTypeGenerator() :
			d_sh_iter(SingletonType::instance().insert(GPlatesUtils::generate_unique_id()))
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
				const GPlatesUtils::UnicodeString &s):
			d_sh_iter(SingletonType::instance().insert(s))
		{  }

		/**
		 * Copy constructor.
		 *
		 * Note that we don't copy the back-reference, since the back-reference in @a other
		 * (if there is one) points to the object which defined (and contains) @a other,
		 * which may not necessarily be the back-reference which defines or contains this.
		 */
		IdTypeGenerator(
				const IdTypeGenerator &other):
			d_sh_iter(other.d_sh_iter),
			d_back_ref_ptr()
		{  }

		/**
		 * Copy-assignment operator.
		 *
		 * Note that we don't copy the back-reference, since the back-reference in @a other
		 * (if there is one) points to the object which defined (and contains) @a other,
		 * which may not necessarily be the back-reference which defines or contains this.
		 */
		IdTypeGenerator &
		operator=(
				const IdTypeGenerator &other)
		{
			// Note that since the back-reference is reset inside this block, this
			// if-test is quite necessary...
			if (this != &other) {
				d_sh_iter = other.d_sh_iter;
				// Don't copy-assign the back-reference; instead, reset it.
				d_back_ref_ptr.reset();
			}
			return *this;
		}

		/**
		 * Access the Unicode string of the text content for this instance.
		 */
		const GPlatesUtils::UnicodeString &
		get() const {
			return *d_sh_iter;
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
			d_back_ref_ptr.reset(new BackRef(target, d_sh_iter));
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
			back_ref_list_type::iterator iter = d_sh_iter.back_refs().begin();
			back_ref_list_type::iterator end = d_sh_iter.back_refs().end();
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
			return d_sh_iter == other.d_sh_iter;
		}

		/**
		 * Equality comparison operator - inequality operator provided by 'boost::equality_comparable'.
		 */
		bool
		operator==(
				const IdTypeGenerator &other) const {
			return is_equal_to(other);
		}

		/**
		 * Less-than operator - provided so @a IdTypeGenerator can be used as a key in std::map.
		 *
		 * The other comparison operators are provided by boost::less_than_comparable.
		 */
		bool
		operator<(
				const IdTypeGenerator &other) const
		{
			return *d_sh_iter < *other.d_sh_iter;
		}

	private:

		shared_iterator_type d_sh_iter;

		// This is a scoped_ptr, so that it cannot be shared between IdTypeGenerator
		// instances which are copied or copy-assigned.
		boost::scoped_ptr<const BackRef> d_back_ref_ptr;
	};


	template<typename SingletonType, typename BackRefTargetType>
	inline
	bool
	GPlatesModel::IdTypeGenerator<SingletonType, BackRefTargetType>::is_loaded(
			const GPlatesUtils::UnicodeString &s) {
		return SingletonType::instance().contains(s);
	}

}

#endif  // GPLATES_MODEL_IDTYPEGENERATOR_H
