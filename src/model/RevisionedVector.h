/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
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

#ifndef GPLATES_MODEL_REVISIONEDVECTOR_H
#define GPLATES_MODEL_REVISIONEDVECTOR_H

#include <vector>
#include <boost/foreach.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/optional.hpp>
#include <boost/type_traits/is_convertible.hpp>
#include <boost/utility/enable_if.hpp>

#include "ModelTransaction.h"
#include "Revisionable.h"
#include "RevisionedReference.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "utils/non_null_intrusive_ptr.h"


namespace GPlatesModel
{
	/**
	 * A vector of elements that maintains revisions, where each revision is a snapshot
	 * of the sequence of elements contained by the vector.
	 */
	template <typename ElementType>
	class RevisionedVector :
			public Revisionable
	{
	public:

		//! Typedef for the vector's element type.
		typedef ElementType element_type;

		//! A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<RevisionedVector>.
		typedef GPlatesUtils::non_null_intrusive_ptr<RevisionedVector> non_null_ptr_type;

		//! A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<const RevisionedVector>.
		typedef GPlatesUtils::non_null_intrusive_ptr<const RevisionedVector> non_null_ptr_to_const_type;


		/**
		 * Create a revisioned vector with the initial sequence of specified elements.
		 *
		 * NOTE: If 'ElementType' is not revisionable (convertible to 'Revisionable::non_null_ptr_to_const')
		 * you should not modify the elements once they are stored in this vector (eg, in the
		 * case where elements are shared pointers) otherwise previous revision snapshots will get
		 * corrupted (the snapshots should essentially be immutable).
		 */
		static
		const non_null_ptr_type
		create(
				const std::vector<ElementType> &elements)
		{
			ModelTransaction transaction;
			non_null_ptr_type ptr(new RevisionedVector(transaction, elements));
			transaction.commit();
			return ptr;
		}


		/**
		 * Create a duplicate of this RevisionedVector instance.
		 *
		 * NOTE: If 'ElementType' is not revisionable (convertible to 'Revisionable::non_null_ptr_to_const')
		 * the elements will be copy-constructed - in the case where 'ElementType' is a shared pointer
		 * this means only the pointer is copied - however, as noted in @a create, un-revisionable
		 * elements should not be modified anyway as this corrupts the immutable revision snapshots.
		 */
		const non_null_ptr_type
		clone() const
		{
			return GPlatesUtils::dynamic_pointer_cast<RevisionedVector>(clone_impl());
		}

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		RevisionedVector(
				ModelTransaction &transaction_,
				const std::vector<ElementType> &elements) :
			PropertyValue(
					Revision::non_null_ptr_type(
							new Revision(transaction_, *this, elements)))
		{  }

		//! Constructor used when cloning.
		RevisionedVector(
				const RevisionedVector &other_,
				boost::optional<RevisionContext &> context_) :
			PropertyValue(
					Revision::non_null_ptr_type(
							// Use deep-clone constructor...
							new Revision(other_.get_current_revision<Revision>(), context_, *this)))
		{  }

		virtual
		const Revisionable::non_null_ptr_type
		clone_impl(
				boost::optional<RevisionContext &> context = boost::none) const
		{
			return non_null_ptr_type(new RevisionedVector(*this, context));
		}

	private:

		/**
		 * Helper metafunction inherits from 'boost::true_type' if 'ElementType' is revisionable
		 * (convertible to 'Revisionable::non_null_ptr_to_const_type').
		 */
		struct IsElementTypeRevisionable :
				// Use 'const' since non-const elements are convertible to const elements...
				boost::is_convertible<ElementType, Revisionable::non_null_ptr_to_const_type>
		{  };

		/**
		 * Helper metafunction to get the @a RevisionedReferenced type from a
		 * GPlatesUtils::non_null_intrusive_ptr<RevisionableType> type where 'RevisionableType'
		 * derives from @a Revisionable. This helper class is needed, along with 'mpl::eval_if',
		 * to avoid a compile error when 'ElementType' is not related to @a Revisionable (eg, an integer).
		 * In other words this template is only instantiated by 'mpl::eval_if' if 'ElementType'
		 * has the nested type 'element_type'.
		 */
		template <class Type>
		struct GetRevisionedReferenceType
		{
			//! 'GPlatesUtils::non_null_intrusive_ptr' has the nested type 'element_type'.
			typedef RevisionedReference<typename Type::element_type> type;
		};


		/**
		 * Used when modifications bubble up to us.
		 *
		 * Inherited from @a RevisionContext.
		 */
		virtual
		GPlatesModel::Revision::non_null_ptr_type
		bubble_up(
				ModelTransaction &transaction,
				const Revisionable::non_null_ptr_to_const_type &child_revisionable)
		{
			// Delegate depending on whether 'ElementType' is revisionable or not.
			return bubble_up(transaction, child_revisionable, typename IsElementTypeRevisionable::type())
		}

		//! Bubble-up for revisioning element types.
		GPlatesModel::Revision::non_null_ptr_type
		bubble_up(
				ModelTransaction &transaction,
				const Revisionable::non_null_ptr_to_const_type &child_revisionable,
				boost::mpl::true_/*'ElementType' is revisionable*/);

		//! Bubble-up for non-revisioning element types.
		GPlatesModel::Revision::non_null_ptr_type
		bubble_up(
				ModelTransaction &transaction,
				const Revisionable::non_null_ptr_to_const_type &child_revisionable,
				boost::mpl::false_/*'ElementType' is not revisionable*/);


		/**
		 * Inherited from @a RevisionContext.
		 */
		virtual
		boost::optional<Model &>
		get_model()
		{
			return PropertyValue::get_model();
		}


		/**
		 * Property value data that is mutable/revisionable.
		 */
		struct Revision :
				public GPlatesModel::Revision
		{
			/**
			 * Typedef for the revisioned element.
			 *
			 * It's either just the ElementType itself (eg, 'int') or
			 * RevisionReference<ElementType::element_type> depending on whether the element type
			 * is revisionable or not.
			 */
			typedef typename boost::mpl::eval_if<
					// Use 'const' since non-const elements are convertible to const elements...
					IsElementTypeRevisionable,
							// Lazy evaluation to avoid compile error if no nested type 'element_type'...
							GetRevisionedReferenceType<ElementType>,
							boost::mpl::identity<ElementType> >::type
									revisioned_element_type;


			//! Constructor for revisionable elements.
			Revision(
					ModelTransaction &transaction_,
					RevisionContext &child_context_,
					const std::vector<ElementType> &elements_,
					typename boost::enable_if<IsElementTypeRevisionable>::type *dummy = 0)
			{
				BOOST_FOREACH(const ElementType &element_, elements_)
				{
					elements.push_back(
							// Revisioned elements bubble up to us...
							revisioned_element_type::attach(transaction_, child_context_, element_));
				}
			}

			//! Constructor for non-revisionable elements.
			Revision(
					ModelTransaction &transaction_,
					RevisionContext &child_context_,
					const std::vector<ElementType> &elements_,
					typename boost::disable_if<IsElementTypeRevisionable>::type *dummy = 0) :
				elements(elements_)
			{  }

			//! Deep-clone constructor for revisionable elements.
			Revision(
					const Revision &other_,
					boost::optional<RevisionContext &> context_,
					RevisionContext &child_context_,
					typename boost::enable_if<IsElementTypeRevisionable>::type *dummy = 0) :
				GPlatesModel::Revision(context_),
				elements(other_.elements)
			{
				// Clone data members that were not deep copied.
				BOOST_FOREACH(revisioned_element_type &element, elements)
				{
					element.clone(child_context_));
				}
			}

			//! Deep-clone constructor for non-revisionable elements.
			Revision(
					const Revision &other_,
					boost::optional<RevisionContext &> context_,
					RevisionContext &child_context_,
					typename boost::disable_if<IsElementTypeRevisionable>::type *dummy = 0) :
				GPlatesModel::Revision(context_),
				elements(other_.elements)
			{
				// NOTE: What if the element type is a shared pointer ?
				// We would need to clone the pointed-to object.
				// However, if the client has chosen not to have revisionable elements then
				// they should not be modifying these elements since that would bypass
				// revisioning anyway, so if they are not writing to the elements then
				// they're essentially immutable snapshots and we don't need to deep copy them.
			}

			//! Shallow-clone constructor (same for revisionable and non-revisionable elements).
			Revision(
					const Revision &other_,
					boost::optional<RevisionContext &> context_) :
				GPlatesModel::Revision(context_),
				elements(other_.elements)
			{  }

			virtual
			GPlatesModel::Revision::non_null_ptr_type
			clone_revision(
					boost::optional<RevisionContext &> context) const
			{
				// Use shallow-clone constructor.
				return non_null_ptr_type(new Revision(*this, context));
			}

			virtual
			bool
			equality(
					const GPlatesModel::Revision &other) const
			{
				// Delegate depending on whether 'ElementType' is revisionable or not.
				return equality(other, typename IsElementTypeRevisionable::type());
			}

			bool
			equality(
					const GPlatesModel::Revision &other,
					boost::mpl::true_/*'ElementType' is revisionable*/) const;

			bool
			equality(
					const GPlatesModel::Revision &other,
					boost::mpl::false_/*'ElementType' is not revisionable*/) const;


			std::vector<revisioned_element_type> elements;
		};

	};
}

////////////////////
// Implementation //
////////////////////

namespace GPlatesModel
{
	template <typename ElementType>
	GPlatesModel::Revision::non_null_ptr_type
	RevisionedVector<ElementType>::bubble_up(
			ModelTransaction &transaction,
			const Revisionable::non_null_ptr_to_const_type &child_revisionable,
			boost::mpl::true_/*'ElementType' is revisionable*/)
	{
		// Bubble up to our (parent) context (if any) which creates a new revision for us.
		Revision &revision = create_bubble_up_revision<Revision>(transaction);

		// In this method we are operating on a (bubble up) cloned version of the current revision.

		// Find which element bubbled up.
		BOOST_FOREACH(revisioned_element_type &element, elements)
		{
			if (child_revisionable == element.get_revisionable())
			{
				return element.clone_revision(transaction);
			}
		}

		// The child property value that bubbled up the modification should be one of our children.
		GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);

		// To keep compiler happy - won't be able to get past 'Abort()'.
		return GPlatesModel::Revision::non_null_ptr_type(NULL);
	}


	template <typename ElementType>
	GPlatesModel::Revision::non_null_ptr_type
	RevisionedVector<ElementType>::bubble_up(
			ModelTransaction &transaction,
			const Revisionable::non_null_ptr_to_const_type &child_revisionable,
			boost::mpl::false_/*'ElementType' is not revisionable*/)
	{
		// Shouldn't be able to get here since non-revisionable elements don't bubble-up.
		GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);

		// To keep compiler happy - won't be able to get past 'Abort()'.
		return GPlatesModel::Revision::non_null_ptr_type(NULL);
	}


	template <typename ElementType>
	bool
	RevisionedVector<ElementType>::Revision::equality(
			const GPlatesModel::Revision &other,
			boost::mpl::true_/*'ElementType' is revisionable*/) const
	{
		const Revision &other_revision = dynamic_cast<const Revision &>(other);

		if (elements.size() != other_revision.elements.size())
		{
			return false;
		}

		for (unsigned int n = 0; n < elements.size(); ++n)
		{
			// Compare the pointed-to revisionable objects.
			if (*elements[n].get_revisionable() != *other_revision.elements[n].get_revisionable())
			{
				return false;
			}
		}

		return GPlatesModel::Revision::equality(other);
	}


	template <typename ElementType>
	bool
	RevisionedVector<ElementType>::Revision::equality(
			const GPlatesModel::Revision &other,
			boost::mpl::false_/*'ElementType' is not revisionable*/) const
	{
		const Revision &other_revision = dynamic_cast<const Revision &>(other);

		return elements == other_revision.elements &&
				GPlatesModel::Revision::equality(other);
	}
}

#endif // GPLATES_MODEL_REVISIONEDVECTOR_H
