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

#include <cstddef> // std::size_t
#include <iterator>
#include <vector>
#include <boost/foreach.hpp>
#include <boost/iterator/iterator_facade.hpp>
#include <boost/mpl/bool.hpp>
#include <boost/mpl/eval_if.hpp>
#include <boost/mpl/identity.hpp>
#include <boost/mpl/if.hpp>
#include <boost/optional.hpp>
#include <boost/pointee.hpp>
#include <boost/type_traits/add_const.hpp>
#include <boost/type_traits/is_convertible.hpp>
#include <boost/type_traits/is_same.hpp>
#include <boost/utility/enable_if.hpp>

#include "BubbleUpRevisionHandler.h"
#include "ModelTransaction.h"
#include "Revisionable.h"
#include "RevisionContext.h"
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
			public Revisionable,
			public RevisionContext
	{
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
		 * Helper metafunction to convert from 'RevisionableType::non_null_ptr_type' to
		 * 'RevisionableType::non_null_ptr_to_const_type'.
		 */
		template <class Type>
		struct GetElementAsNonNullPtrToConstType
		{
			//! 'GPlatesUtils::non_null_intrusive_ptr' has the nested type 'element_type'.
			typedef GPlatesUtils::non_null_intrusive_ptr<
					typename boost::add_const<
							typename boost::pointee<Type>::type>::type> type;
		};

		/**
		 * Typedef for a 'const' element.
		 *
		 * It's either just 'const ElementType' (eg, 'const int') or
		 * GPlatesUtils::non_null_intrusive_ptr<const ElementType::element_type>
		 * (eg, PropertyValue::non_null_ptr_to_const_type) depending on whether the element type
		 * is revisionable or not.
		 */
		typedef typename boost::mpl::eval_if<
				IsElementTypeRevisionable,
						// Lazy evaluation to avoid compile error if no nested type 'element_type'...
						GetElementAsNonNullPtrToConstType<ElementType>,
						boost::add_const<ElementType>
								>::type const_element_type;

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
			typedef RevisionedReference<typename boost::pointee<Type>::type> type;
		};

		/**
		 * Typedef for the revisioned element.
		 *
		 * It's either just the ElementType itself (eg, 'int') or
		 * RevisionReference<ElementType::element_type> depending on whether the element type
		 * is revisionable or not.
		 */
		typedef typename boost::mpl::eval_if<
				IsElementTypeRevisionable,
						// Lazy evaluation to avoid compile error if no nested type 'element_type'...
						GetRevisionedReferenceType<ElementType>,
						boost::mpl::identity<ElementType>
								>::type revisioned_element_type;

	public:

		//! Typedef for the vector's element type.
		typedef ElementType element_type;

		//! A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<RevisionedVector>.
		typedef GPlatesUtils::non_null_intrusive_ptr<RevisionedVector> non_null_ptr_type;

		//! A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<const RevisionedVector>.
		typedef GPlatesUtils::non_null_intrusive_ptr<const RevisionedVector> non_null_ptr_to_const_type;


		/**
		 * Reference (proxied) implementation for a reference to a 'const' element.
		 */
		class ConstReference
		{
		public:

			explicit
			ConstReference(
					const RevisionedVector<ElementType> *revisioned_vector,
					std::size_t index) :
				d_revisioned_vector(revisioned_vector),
				d_index(index)
			{  }

			/**
			 * Access 'const' element.
			 */
			const_element_type
			get() const
			{
				return d_revisioned_vector->get_element(d_index);
			}

			/**
			 * Conversion to 'const' element.
			 */
			operator const_element_type() const
			{
				return d_revisioned_vector->get_element(d_index);
			}

		private:

			const RevisionedVector<ElementType> *d_revisioned_vector;
			std::size_t d_index;
		};

		/**
		 * Const reference type.
		 *
		 * Returns a 'const_element_type' which is either 'const ElementType', or
		 * RevisionableType::non_null_ptr_to_const_type if 'ElementType' is
		 * RevisionableType::non_null_ptr_type (where RevisionableType inherits from @a Revisionable).
		 */
		typedef ConstReference const_reference;


		/**
		 * Reference (proxied) implementation for a reference to a 'non-const' element.
		 *
		 * This is essentially the same as @a ConstReference but adds assignment operator so
		 * client can write '*iter = new_element'.
		 *
		 * Using a proxy implementation enables us to remain revision-aware and enables
		 * us to use '*iter = new_element' to replace elements in-place in the vector while
		 * maintaining revisioning in the process.
		 */
		class Reference
		{
		public:

			explicit
			Reference(
					RevisionedVector<ElementType> *revisioned_vector,
					std::size_t index) :
				d_revisioned_vector(revisioned_vector),
				d_index(index)
			{  }

			/**
			 * Element assignment operator.
			 *
			 * Can set the element in-place in the vector as in:
			 *   *iter = new_element; // Where '*iter' returns 'Reference'.
			 */
			Reference &
			operator=(
					const ElementType &element)
			{
				d_revisioned_vector->set_element(element, d_index);
				return *this;
			}

			/**
			 * Copy assignment operator.
			 */
			Reference &
			operator=(
					const Reference &other)
			{
				*this = ElementType(other); // Use element assignment operator.
				return *this;
			}

			/**
			 * Access 'non-const' element.
			 *
			 * Note that a 'const ElementType' is returned to ensure the returned temporary
			 * is not modified since this is probably not the intention of the client.
			 * However, it's still possible to modify a revisionable element because
			 * 'const RevisionableType::non_null_ptr_type' is returned and the pointed-to
			 * revisionable object can be modified (as opposed to the non_null_intrusive_ptr).
			 */
			const ElementType
			get() const
			{
				return d_revisioned_vector->get_element(d_index);
			}

			/**
			 * Conversion to 'non-const' element.
			 */
			operator ElementType() const
			{
				return d_revisioned_vector->get_element(d_index);
			}

		private:

			RevisionedVector<ElementType> *d_revisioned_vector;
			std::size_t d_index;
		};

		/**
		 * Non-const reference type.
		 *
		 * Returns a 'element_type'.
		 */
		typedef Reference reference;


		/**
		 * Iterator implementation.
		 *
		 * ElementQualifiedType can be 'element_type' or 'const_element_type'.
		 *
		 * This iterator can also work across revisions (eg, if change an element during iteration
		 * then can continue iteration afterwards even though a new vector revision was created).
		 */
		template <typename ElementQualifiedType>
		class Iterator :
				public boost::iterator_facade<
						Iterator<ElementQualifiedType>,
						ElementQualifiedType,
						boost::random_access_traversal_tag,
						typename boost::mpl::if_<
								boost::is_same<ElementQualifiedType, const_element_type>,
										// Use a proxied reference for const elements...
										ConstReference,
										// Use a proxied reference for non-const elements...
										Reference>::type>
		{
		public:

			//! Typedef for the const or non-const revisioned vector pointer.
			typedef typename boost::mpl::if_<
					boost::is_same<ElementQualifiedType, const_element_type>,
								const RevisionedVector<ElementType> *,
								RevisionedVector<ElementType> *
										>::type revisioned_vector_ptr_type;

			Iterator() :
				d_revisioned_vector(NULL),
				d_index(0)
			{  }

			explicit
			Iterator(
					revisioned_vector_ptr_type revisioned_vector,
					std::size_t index) :
				d_revisioned_vector(revisioned_vector),
				d_index(index)
			{  }

			// Template copy constructor conversion from 'iterator' to 'const_iterator'.
			template <class OtherElementQualifiedType>
			Iterator(
					const Iterator<OtherElementQualifiedType> &other,
					// Only allow conversion from 'iterator' to 'const_iterator', not vice versa...
					typename boost::enable_if<
							boost::is_convertible<OtherElementQualifiedType, ElementQualifiedType>
									>::type *dummy = 0) :
				d_revisioned_vector(other.d_revisioned_vector),
				d_index(other.d_index)
			{  }

		private:

			revisioned_vector_ptr_type d_revisioned_vector;
			std::size_t d_index;


			typename Iterator::reference
			dereference() const
			{
				return dereference(typename boost::is_same<ElementQualifiedType, const_element_type>::type());
			}

			ConstReference
			dereference(
					boost::mpl::true_/*'ElementQualifiedType' is 'const_element_type'*/) const
			{
				return ConstReference(d_revisioned_vector, d_index);
			}

			Reference
			dereference(
					boost::mpl::false_/*'ElementQualifiedType' is 'const_element_type'*/) const
			{
				return Reference(d_revisioned_vector, d_index);
			}

			// Templated because enables comparison of 'const_iterator' and 'iterator'.
			template <class OtherElementQualifiedType>
			bool
			equal(
					const Iterator<OtherElementQualifiedType> &other) const
			{
				return d_revisioned_vector == other.d_revisioned_vector &&
						d_index == other.d_index;
			}

			void
			increment()
			{
				++d_index;
			}

			void
			decrement()
			{
				--d_index;
			}

			void
			advance(
					typename Iterator::difference_type n)
			{
				d_index += n;
			}

			// Templated because enables distance between 'const_iterator' and 'iterator'.
			template <class OtherElementQualifiedType>
			typename Iterator::difference_type
			distance_to(
					const Iterator<OtherElementQualifiedType> &other) const
			{
				return Iterator::difference_type(other.d_index) - Iterator::difference_type(d_index);
			}

			friend class boost::iterator_core_access;
			template <class OtherElementQualifiedType> friend class Iterator;
			friend class RevisionedVector<ElementType>;

			//
			// Workaround for bug in boost 1.47 -> 1.50 when using proxy references.
			// Fixed in 1.51 - see https://svn.boost.org/trac/boost/changeset/77723
			//
			// Workaround involves defining our own 'operator->()' and 'operator_arrow_proxy'.
			//

		private:

			template <class ReferenceType>
			struct operator_arrow_proxy
			{
				explicit
				operator_arrow_proxy(
						const ReferenceType &reference_) :
					reference(reference_)
				{  }

				ReferenceType *
				operator->()
				{
					return &reference;
				}

				operator ReferenceType *()
				{
					return &reference;
				}

				ReferenceType reference;
			};

		public:

			// Overrides 'operator->()' in base class 'boost::iterator_facade'.
			operator_arrow_proxy<typename Iterator::reference>
			operator->() const
			{
				// '**this' uses 'operator*()' defined in base class 'boost::iterator_facade'.
				return operator_arrow_proxy<typename Iterator::reference>(**this);
			}
		};

		/**
		 * Non-const iterator type.
		 *
		 * Dereferencing will return a 'reference'.
		 */
		typedef Iterator<element_type> iterator;

		/**
		 * Const iterator type.
		 *
		 * Dereferencing will return a 'const_reference' which references either a
		 * 'const element_type', or RevisionableType::non_null_ptr_to_const_type if 'ElementType' is
		 * RevisionableType::non_null_ptr_type (where RevisionableType inherits from @a Revisionable).
		 */
		typedef Iterator<const_element_type> const_iterator;


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
				const std::vector<ElementType> &elements = std::vector<ElementType>())
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


		/**
		 * Const iterator dereferences to give 'const_reference' which references a 'const ElementType', or
		 * RevisionableType::non_null_ptr_to_const_type if 'ElementType' is
		 * RevisionableType::non_null_ptr_type (where RevisionableType inherits from @a Revisionable).
		 */
		const_iterator
		begin() const
		{
			return const_iterator(this, 0);
		}

		const_iterator
		end() const
		{
			return const_iterator(this, size());
		}

		/**
		 * Non-const iterator dereferences to give 'reference'.
		 *
		 * Note that this non-const iterator can also be used to replace elements in the
		 * internal sequence using '*iter = new_element'.
		 */
		iterator
		begin()
		{
			return iterator(this, 0);
		}

		iterator
		end()
		{
			return iterator(this, size());
		}

		bool
		empty() const
		{
			return get_current_revision<Revision>().elements.empty();
		}

		void
		clear()
		{
			erase(begin(), end());
		}

		template <typename Iter>
		void
		assign(
				Iter first,
				Iter last)
		{
			erase(begin(), end());
			insert(begin(), first, last);
		}

		std::size_t
		size() const
		{
			return get_current_revision<Revision>().elements.size();
		}

		const_reference
		front() const
		{
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					!empty(),
					GPLATES_ASSERTION_SOURCE);
			return const_reference(this, 0);
		}

		reference
		front()
		{
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					!empty(),
					GPLATES_ASSERTION_SOURCE);
			return reference(this, 0);
		}

		const_reference
		back() const
		{
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					!empty(),
					GPLATES_ASSERTION_SOURCE);
			return const_reference(this, size() - 1);
		}

		reference
		back()
		{
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					!empty(),
					GPLATES_ASSERTION_SOURCE);
			return reference(this, size() - 1);
		}

		const_reference
		operator[](
				std::size_t index) const
		{
			return const_reference(this, index);
		}

		reference
		operator[](
				std::size_t index)
		{
			return reference(this, index);
		}

		void
		push_back(
				const ElementType &elem)
		{
			push_back(elem, typename IsElementTypeRevisionable::type());
		}

		void
		pop_back()
		{
			pop_back(typename IsElementTypeRevisionable::type());
		}

		iterator
		insert(
				iterator pos,
				const ElementType &elem)
		{
			return insert(pos, elem, typename IsElementTypeRevisionable::type());
		}

		template <typename Iter>
		void
		insert(
				iterator pos,
				Iter first,
				Iter last)
		{
			return insert(pos, first, last, typename IsElementTypeRevisionable::type());
		}

		iterator
		erase(
				iterator pos)
		{
			return erase(pos, typename IsElementTypeRevisionable::type());
		}

		iterator
		erase(
				iterator first,
				iterator last)
		{
			return erase(first, last, typename IsElementTypeRevisionable::type());
		}

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		RevisionedVector(
				ModelTransaction &transaction_,
				const std::vector<ElementType> &elements) :
			Revisionable(
					Revision::non_null_ptr_type(
							new Revision(transaction_, *this, elements)))
		{  }

		//! Constructor used when cloning.
		RevisionedVector(
				const RevisionedVector &other_,
				boost::optional<RevisionContext &> context_) :
			Revisionable(
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
			return bubble_up(transaction, child_revisionable, typename IsElementTypeRevisionable::type());
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
			return Revisionable::get_model();
		}


		/**
		 * Returns the 'const' element at the specified index.
		 */
		const_element_type
		get_element(
				std::size_t element_index) const
		{
			return get_element(element_index, typename IsElementTypeRevisionable::type());
		}

		const_element_type
		get_element(
				std::size_t element_index,
				boost::mpl::true_/*'ElementType' is revisionable*/) const;

		const_element_type
		get_element(
				std::size_t element_index,
				boost::mpl::false_/*'ElementType' is not revisionable*/) const;


		/**
		 * Returns the 'non-const' element at the specified index.
		 */
		ElementType
		get_element(
				std::size_t element_index)
		{
			return get_element(element_index, typename IsElementTypeRevisionable::type());
		}

		ElementType
		get_element(
				std::size_t element_index,
				boost::mpl::true_/*'ElementType' is revisionable*/);

		ElementType
		get_element(
				std::size_t element_index,
				boost::mpl::false_/*'ElementType' is not revisionable*/);


		/**
		 * Set an element in-place.
		 */
		void
		set_element(
				const ElementType &element,
				std::size_t element_index)
		{
			set_element(element, element_index, typename IsElementTypeRevisionable::type());
		}

		void
		set_element(
				const ElementType &element,
				std::size_t element_index,
				boost::mpl::true_/*'ElementType' is revisionable*/);

		void
		set_element(
				const ElementType &element,
				std::size_t element_index,
				boost::mpl::false_/*'ElementType' is not revisionable*/);


		void
		push_back(
				const ElementType &elem,
				boost::mpl::true_/*'ElementType' is revisionable*/);

		void
		push_back(
				const ElementType &elem,
				boost::mpl::false_/*'ElementType' is not revisionable*/);


		void
		pop_back(
				boost::mpl::true_/*'ElementType' is revisionable*/);

		void
		pop_back(
				boost::mpl::false_/*'ElementType' is not revisionable*/);


		iterator
		insert(
				iterator pos,
				const ElementType &elem,
				boost::mpl::true_/*'ElementType' is revisionable*/);

		iterator
		insert(
				iterator pos,
				const ElementType &elem,
				boost::mpl::false_/*'ElementType' is not revisionable*/);


		template <typename Iter>
		void
		insert(
				iterator pos,
				Iter first,
				Iter last,
				boost::mpl::true_/*'ElementType' is revisionable*/);

		template <typename Iter>
		void
		insert(
				iterator pos,
				Iter first,
				Iter last,
				boost::mpl::false_/*'ElementType' is not revisionable*/);


		iterator
		erase(
				iterator pos,
				boost::mpl::true_/*'ElementType' is revisionable*/);

		iterator
		erase(
				iterator pos,
				boost::mpl::false_/*'ElementType' is not revisionable*/);


		iterator
		erase(
				iterator first,
				iterator last,
				boost::mpl::true_/*'ElementType' is revisionable*/);

		iterator
		erase(
				iterator first,
				iterator last,
				boost::mpl::false_/*'ElementType' is not revisionable*/);


		//! Convert to 'std::vector<revisioned_element_type>::const_iterator' from wrapped iterator.
		typename std::vector<revisioned_element_type>::const_iterator
		to_internal_iterator(
				std::vector<revisioned_element_type> &elements,
				const_iterator iter) const
		{
			typename std::vector<revisioned_element_type>::const_iterator internal_iter = elements.begin();
			std::advance(internal_iter, iter.d_index);
			return internal_iter;
		}

		//! Convert to 'std::vector<revisioned_element_type>::iterator' from wrapped iterator.
		typename std::vector<revisioned_element_type>::iterator
		to_internal_iterator(
				std::vector<revisioned_element_type> &elements,
				iterator iter)
		{
			typename std::vector<revisioned_element_type>::iterator internal_iter = elements.begin();
			std::advance(internal_iter, iter.d_index);
			return internal_iter;
		}

		//! Convert from 'std::vector<revisioned_element_type>::const_iterator' iterator to wrapped iterator.
		const_iterator
		from_internal_iterator(
				std::vector<revisioned_element_type> &elements,
				typename std::vector<revisioned_element_type>::const_iterator internal_iter) const
		{
			return const_iterator(this, std::distance(elements.begin(), internal_iter));
		}

		//! Convert from 'std::vector<revisioned_element_type>::const_iterator' iterator to wrapped iterator.
		iterator
		from_internal_iterator(
				std::vector<revisioned_element_type> &elements,
				typename std::vector<revisioned_element_type>::iterator internal_iter)
		{
			return iterator(this, std::distance(elements.begin(), internal_iter));
		}


		/**
		 * Property value data that is mutable/revisionable.
		 */
		struct Revision :
				public GPlatesModel::Revision
		{
			Revision(
					ModelTransaction &transaction_,
					RevisionContext &child_context_,
					const std::vector<ElementType> &elements_)
			{
				initialise_elements(transaction_, child_context_, elements_, typename IsElementTypeRevisionable::type());
			}

			//! Initialise revisionable elements.
			void
			initialise_elements(
					ModelTransaction &transaction_,
					RevisionContext &child_context_,
					const std::vector<ElementType> &elements_,
					boost::mpl::true_/*'ElementType' is revisionable*/)
			{
				BOOST_FOREACH(const ElementType &element_, elements_)
				{
					elements.push_back(
							// Revisioned elements bubble up to us...
							revisioned_element_type::attach(transaction_, child_context_, element_));
				}
			}

			//! Initialise non-revisionable elements.
			void
			initialise_elements(
					ModelTransaction &transaction_,
					RevisionContext &child_context_,
					const std::vector<ElementType> &elements_,
					boost::mpl::false_/*'ElementType' is not revisionable*/)
			{
				elements = elements_;
			}

			//! Deep-clone constructor.
			Revision(
					const Revision &other_,
					boost::optional<RevisionContext &> context_,
					RevisionContext &child_context_) :
				GPlatesModel::Revision(context_),
				elements(other_.elements)
			{
				clone_elements(child_context_, typename IsElementTypeRevisionable::type());
			}

			//! Clone revisionable elements.
			void
			clone_elements(
					RevisionContext &child_context_,
					boost::mpl::true_/*'ElementType' is revisionable*/)
			{
				// Clone data members that were not deep copied.
				BOOST_FOREACH(revisioned_element_type &element, elements)
				{
					element.clone(child_context_);
				}
			}

			//! Clone non-revisionable elements.
			void
			clone_elements(
					RevisionContext &child_context_,
					boost::mpl::false_/*'ElementType' is not revisionable*/)
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
		BOOST_FOREACH(revisioned_element_type &element, revision.elements)
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
	typename RevisionedVector<ElementType>::const_element_type
	RevisionedVector<ElementType>::get_element(
			std::size_t element_index,
			boost::mpl::true_/*'ElementType' is revisionable*/) const
	{
		const Revision &revision = get_current_revision<Revision>();

		// Make sure we're not dereferencing out-of-bounds.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				element_index < revision.elements.size(),
				GPLATES_ASSERTION_SOURCE);

		return revision.elements[element_index].get_revisionable();
	}


	template <typename ElementType>
	typename RevisionedVector<ElementType>::const_element_type
	RevisionedVector<ElementType>::get_element(
			std::size_t element_index,
			boost::mpl::false_/*'ElementType' is not revisionable*/) const
	{
		const Revision &revision = get_current_revision<Revision>();

		// Make sure we're not dereferencing out-of-bounds.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				element_index < revision.elements.size(),
				GPLATES_ASSERTION_SOURCE);

		return revision.elements[element_index];
	}


	template <typename ElementType>
	ElementType
	RevisionedVector<ElementType>::get_element(
			std::size_t element_index,
			boost::mpl::true_/*'ElementType' is revisionable*/)
	{
		const Revision &revision = get_current_revision<Revision>();

		// Make sure we're not dereferencing out-of-bounds.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				element_index < revision.elements.size(),
				GPLATES_ASSERTION_SOURCE);

		return revision.elements[element_index].get_revisionable();
	}


	template <typename ElementType>
	ElementType
	RevisionedVector<ElementType>::get_element(
			std::size_t element_index,
			boost::mpl::false_/*'ElementType' is not revisionable*/)
	{
		const Revision &revision = get_current_revision<Revision>();

		// Make sure we're not dereferencing out-of-bounds.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				element_index < revision.elements.size(),
				GPLATES_ASSERTION_SOURCE);

		return revision.elements[element_index];
	}


	template <typename ElementType>
	void
	RevisionedVector<ElementType>::set_element(
			const ElementType &element,
			std::size_t element_index,
			boost::mpl::true_/*'ElementType' is revisionable*/)
	{
		BubbleUpRevisionHandler revision_handler(this);
		Revision &revision = revision_handler.get_revision<Revision>();

		// Make sure we're not dereferencing out-of-bounds.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				element_index < revision.elements.size(),
				GPLATES_ASSERTION_SOURCE);
		revision.elements[element_index].change(
				revision_handler.get_model_transaction(), element);

		revision_handler.commit();
	}


	template <typename ElementType>
	void
	RevisionedVector<ElementType>::set_element(
			const ElementType &element,
			std::size_t element_index,
			boost::mpl::false_/*'ElementType' is not revisionable*/)
	{
		BubbleUpRevisionHandler revision_handler(this);
		Revision &revision = revision_handler.get_revision<Revision>();

		// Make sure we're not dereferencing out-of-bounds.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				element_index < revision.elements.size(),
				GPLATES_ASSERTION_SOURCE);
		revision.elements[element_index] = element;

		revision_handler.commit();
	}


	template <typename ElementType>
	void
	RevisionedVector<ElementType>::push_back(
			const ElementType &elem,
			boost::mpl::true_/*'ElementType' is revisionable*/)
	{
		BubbleUpRevisionHandler revision_handler(this);
		Revision &revision = revision_handler.get_revision<Revision>();

		revision.elements.push_back(
				revisioned_element_type::attach(revision_handler.get_model_transaction(), *this, elem));

		revision_handler.commit();
	}


	template <typename ElementType>
	void
	RevisionedVector<ElementType>::push_back(
			const ElementType &elem,
			boost::mpl::false_/*'ElementType' is not revisionable*/)
	{
		BubbleUpRevisionHandler revision_handler(this);
		Revision &revision = revision_handler.get_revision<Revision>();

		revision.elements.push_back(elem);

		revision_handler.commit();
	}


	template <typename ElementType>
	void
	RevisionedVector<ElementType>::pop_back(
			boost::mpl::true_/*'ElementType' is revisionable*/)
	{
		BubbleUpRevisionHandler revision_handler(this);
		Revision &revision = revision_handler.get_revision<Revision>();

		// Detach the element before erasing it.
		revision.elements.back().detach(revision_handler.get_model_transaction());
		revision.elements.pop_back();

		revision_handler.commit();
	}


	template <typename ElementType>
	void
	RevisionedVector<ElementType>::pop_back(
			boost::mpl::false_/*'ElementType' is not revisionable*/)
	{
		BubbleUpRevisionHandler revision_handler(this);
		Revision &revision = revision_handler.get_revision<Revision>();

		revision.elements.pop_back();

		revision_handler.commit();
	}


	template <typename ElementType>
	typename RevisionedVector<ElementType>::iterator
	RevisionedVector<ElementType>::insert(
			iterator pos,
			const ElementType &elem,
			boost::mpl::true_/*'ElementType' is revisionable*/)
	{
		BubbleUpRevisionHandler revision_handler(this);
		Revision &revision = revision_handler.get_revision<Revision>();

		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				pos.d_revisioned_vector == this &&
					pos.d_index <= revision.elements.size(),
				GPLATES_ASSERTION_SOURCE);

		typename std::vector<revisioned_element_type>::iterator ret =
				revision.elements.insert(
						to_internal_iterator(revision.elements, pos),
						revisioned_element_type::attach(revision_handler.get_model_transaction(), *this, elem));

		revision_handler.commit();

		return from_internal_iterator(revision.elements, ret);
	}


	template <typename ElementType>
	typename RevisionedVector<ElementType>::iterator
	RevisionedVector<ElementType>::insert(
			iterator pos,
			const ElementType &elem,
			boost::mpl::false_/*'ElementType' is not revisionable*/)
	{
		BubbleUpRevisionHandler revision_handler(this);
		Revision &revision = revision_handler.get_revision<Revision>();

		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				pos.d_revisioned_vector == this &&
					pos.d_index <= revision.elements.size(),
				GPLATES_ASSERTION_SOURCE);

		typename std::vector<revisioned_element_type>::iterator ret =
				revision.elements.insert(
						to_internal_iterator(revision.elements, pos),
						elem);

		revision_handler.commit();

		return from_internal_iterator(revision.elements, ret);
	}


	template <typename ElementType>
	template <typename Iter>
	void
	RevisionedVector<ElementType>::insert(
			iterator pos,
			Iter first,
			Iter last,
			boost::mpl::true_/*'ElementType' is revisionable*/)
	{
		BubbleUpRevisionHandler revision_handler(this);
		Revision &revision = revision_handler.get_revision<Revision>();

		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				pos.d_revisioned_vector == this &&
					pos.d_index <= revision.elements.size(),
				GPLATES_ASSERTION_SOURCE);

		// Attach the elements before inserting them.
		std::vector<revisioned_element_type> elements;
		for (Iter iter = first; iter != last; ++iter)
		{
			elements.push_back(
					revisioned_element_type::attach(
							revision_handler.get_model_transaction(), *this, *iter));
		}

		// Insert the elements.
		revision.elements.insert(
				to_internal_iterator(revision.elements, pos),
				elements.begin(),
				elements.end());

		revision_handler.commit();
	}


	template <typename ElementType>
	template <typename Iter>
	void
	RevisionedVector<ElementType>::insert(
			iterator pos,
			Iter first,
			Iter last,
			boost::mpl::false_/*'ElementType' is not revisionable*/)
	{
		BubbleUpRevisionHandler revision_handler(this);
		Revision &revision = revision_handler.get_revision<Revision>();

		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				pos.d_revisioned_vector == this &&
					pos.d_index <= revision.elements.size(),
				GPLATES_ASSERTION_SOURCE);

		// Insert the elements.
		revision.elements.insert(
				to_internal_iterator(revision.elements, pos),
				first,
				last);

		revision_handler.commit();
	}


	template <typename ElementType>
	typename RevisionedVector<ElementType>::iterator
	RevisionedVector<ElementType>::erase(
			iterator pos,
			boost::mpl::true_/*'ElementType' is revisionable*/)
	{
		BubbleUpRevisionHandler revision_handler(this);
		Revision &revision = revision_handler.get_revision<Revision>();

		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				pos.d_revisioned_vector == this &&
					pos.d_index < revision.elements.size(),
				GPLATES_ASSERTION_SOURCE);

		// Detach the element before erasing it.
		typename std::vector<revisioned_element_type>::iterator erase_iter =
				to_internal_iterator(revision.elements, pos);
		erase_iter->detach(revision_handler.get_model_transaction());

		typename std::vector<revisioned_element_type>::iterator ret =
				revision.elements.erase(erase_iter);

		revision_handler.commit();

		return from_internal_iterator(revision.elements, ret);
	}


	template <typename ElementType>
	typename RevisionedVector<ElementType>::iterator
	RevisionedVector<ElementType>::erase(
			iterator pos,
			boost::mpl::false_/*'ElementType' is not revisionable*/)
	{
		BubbleUpRevisionHandler revision_handler(this);
		Revision &revision = revision_handler.get_revision<Revision>();

		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				pos.d_revisioned_vector == this &&
					pos.d_index < revision.elements.size(),
				GPLATES_ASSERTION_SOURCE);

		typename std::vector<revisioned_element_type>::iterator ret =
				revision.elements.erase(to_internal_iterator(revision.elements, pos));

		revision_handler.commit();

		return from_internal_iterator(revision.elements, ret);
	}


	template <typename ElementType>
	typename RevisionedVector<ElementType>::iterator
	RevisionedVector<ElementType>::erase(
			iterator first,
			iterator last,
			boost::mpl::true_/*'ElementType' is revisionable*/)
	{
		BubbleUpRevisionHandler revision_handler(this);
		Revision &revision = revision_handler.get_revision<Revision>();

		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				first.d_revisioned_vector == this &&
					last.d_revisioned_vector == this &&
					first.d_index < revision.elements.size() &&
					last.d_index < revision.elements.size() &&
					first.d_index <= last.d_index,
				GPLATES_ASSERTION_SOURCE);

		// Detach the elements before erasing them.
		for (iterator iter = first; iter != last; ++iter)
		{
			typename std::vector<revisioned_element_type>::iterator erase_iter =
					to_internal_iterator(revision.elements, iter);

			erase_iter->detach(revision_handler.get_model_transaction());
		}

		// Erase the elements.
		typename std::vector<revisioned_element_type>::iterator ret =
				revision.elements.erase(
						to_internal_iterator(revision.elements, first),
						to_internal_iterator(revision.elements, last));

		revision_handler.commit();

		return from_internal_iterator(revision.elements, ret);
	}


	template <typename ElementType>
	typename RevisionedVector<ElementType>::iterator
	RevisionedVector<ElementType>::erase(
			iterator first,
			iterator last,
			boost::mpl::false_/*'ElementType' is not revisionable*/)
	{
		BubbleUpRevisionHandler revision_handler(this);
		Revision &revision = revision_handler.get_revision<Revision>();

		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				first.d_revisioned_vector == this &&
					last.d_revisioned_vector == this &&
					first.d_index < revision.elements.size() &&
					last.d_index < revision.elements.size() &&
					first.d_index <= last.d_index,
				GPLATES_ASSERTION_SOURCE);

		// Erase the elements.
		typename std::vector<revisioned_element_type>::iterator ret =
				revision.elements.erase(
						to_internal_iterator(revision.elements, first),
						to_internal_iterator(revision.elements, last));

		revision_handler.commit();

		return from_internal_iterator(revision.elements, ret);
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
