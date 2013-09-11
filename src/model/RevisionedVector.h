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
#include <boost/mpl/if.hpp>
#include <boost/optional.hpp>
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
	 * A vector of revisionable object that maintains revisions, where each vector revision is a
	 * snapshot of the sequence of revisionable elements contained by the vector.
	 *
	 * Template parameter 'RevisionableType' is @a Revisionable or one of its derived types and
	 * can be const or non-const (eg, 'GpmlPlateId' or 'const GpmlPlateId').
	 * Although typically it should be the 'non-const' version (since the python bindings use
	 * non-const for mutable types, since python has no real concept of const and non-const methods).
	 *
	 * Note: Previously @a RevisionedVector accepted non-revisionable types also.
	 * But this was removed since it became very difficult to bind this to python.
	 * Approaches such as boost::python::vector_indexing_suite come close to working with its
	 * proxying to ensure, for example, that deleting a slice from the middle of the sequence from
	 * the python side will result in any element references (again on python side) having their
	 * sequence indices adjusted so that they point to the correct location within the vector.
	 * However the two main problems with this approach are:
	 *  (1) the boost python proxying system uses C++ references into the vector (although these are
	 *      only short-lived during the period in which the vector is actually accessed from python)
	 *      and our revisioned vector cannot really allow direct references into the internal
	 *      vector because of revisioning (which is why we have our own proxying - see nested classes
	 *      Reference and ConstReference below), and
	 *  (2) there's also the danger of modifying the vector from the C++ side which bypasses the
	 *      proxy adjustments in boost::python::indexing_suite essentially invalidating any
	 *      python references into the vector resulting in errors or crashing.
	 * It turns out to be much easier if we just use shared pointers for everything - it matches
	 * up much better with the reference-semantics of python (rather than trying to map the
	 * value-semantics side of C++ to python). And things like deleting a slice in the middle of a
	 * vector sequence just work without any extra logic.
	 * So since @a Revisionable uses shared pointers this is not a problem.
	 * Also we don't allow just any type (ie, we restrict to @a Revisionable and its derived types)
	 * because @a Revisionable vector elements have their own internal revisioning and hence we
	 * can return the same @a Revisionable *instance* from two *different* revisions of the vector.
	 * We could also do this for non-revisionable elements (if using shared pointers) but we can't
	 * then modify a non-revisionable element (we could store a pre-modified copy in one vector
	 * revision snapshot and the post-modified copy in another snapshot - but the element is
	 * non-revisionable and so it has no bubble-up mechanism to tell the vector to do this).
	 * In any case, making a class revisionable is not too difficult, so that's the price to pay
	 * for being able to store it in a @a RevisionedVector.
	 */
	template <class RevisionableType>
	class RevisionedVector :
			public Revisionable,
			public RevisionContext
	{
	public:

		//! Typedef for the vector's element revisionable type.
		typedef RevisionableType revisionable_type;

		//! Typedef for a revisionable element - all @a Revisionable types use non_null_intrusive_ptr.
		typedef GPlatesUtils::non_null_intrusive_ptr<RevisionableType> element_type;

		//! Typedef for a revisionable element - all @a Revisionable types use non_null_intrusive_ptr.
		typedef GPlatesUtils::non_null_intrusive_ptr<const RevisionableType> const_element_type;


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
					const RevisionedVector<RevisionableType> *revisioned_vector,
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

			const RevisionedVector<RevisionableType> *d_revisioned_vector;
			std::size_t d_index;
		};

		/**
		 * Const reference type - returns a 'const_element_type'.
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
					RevisionedVector<RevisionableType> *revisioned_vector,
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
					const element_type &element)
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
				*this = element_type(other); // Use element assignment operator.
				return *this;
			}

			/**
			 * Access 'non-const' element.
			 *
			 * Note that a 'const element_type' is returned to ensure the returned temporary
			 * (non_null_intrusive_ptr) is not modified since this is probably not the intention of
			 * the client. However, it's still possible to modify a revisionable element because
			 * 'const element_type' is the same as 'const RevisionableType::non_null_ptr_type' and
			 * so the pointed-to revisionable object can be modified (as opposed to the non_null_intrusive_ptr).
			 */
			const element_type
			get() const
			{
				return d_revisioned_vector->get_element(d_index);
			}

			/**
			 * Conversion to 'non-const' element.
			 */
			operator element_type() const
			{
				return d_revisioned_vector->get_element(d_index);
			}

		private:

			RevisionedVector<RevisionableType> *d_revisioned_vector;
			std::size_t d_index;
		};

		/**
		 * Non-const reference type - returns a 'element_type'.
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
						// NOTE: This must not be 'boost::random_access_iterator_tag', otherwise std::advance()
						// won't work properly (for negative values) - because our proxy reference is not a real
						// reference and std::advance cannot yet take advantage of the new style iterator concepts
						// (see - http://www.boost.org/doc/libs/1_34_1/libs/iterator/doc/new-iter-concepts.html)...
						std::random_access_iterator_tag,
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
								const RevisionedVector<RevisionableType> *,
								RevisionedVector<RevisionableType> *
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
				d_index = typename Iterator::difference_type(d_index) + n;
			}

			// Templated because enables distance between 'const_iterator' and 'iterator'.
			template <class OtherElementQualifiedType>
			typename Iterator::difference_type
			distance_to(
					const Iterator<OtherElementQualifiedType> &other) const
			{
				return typename Iterator::difference_type(other.d_index) -
						typename Iterator::difference_type(d_index);
			}

			friend class boost::iterator_core_access;
			template <class OtherElementQualifiedType> friend class Iterator;
			friend class RevisionedVector<RevisionableType>;

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
		 * Dereferencing will return a 'const_reference'.
		 */
		typedef Iterator<const_element_type> const_iterator;


		/**
		 * Create a revisioned vector with the initial sequence of specified elements.
		 *
		 * Note that 'element_type' is the same as 'RevisionableType::non_null_ptr_type'.
		 */
		static
		const non_null_ptr_type
		create(
				const std::vector<element_type> &elements = std::vector<element_type>())
		{
			return create(elements.begin(), elements.end());
		}

		/**
		 * Create a revisioned vector with the initial sequence of elements in the specified
		 * iteration range (where iterator dereferences to 'element_type' which is the same as
		 * 'RevisionableType::non_null_ptr_type').
		 */
		template <typename ElementIter>
		static
		const non_null_ptr_type
		create(
				ElementIter elements_begin,
				ElementIter elements_end)
		{
			ModelTransaction transaction;
			non_null_ptr_type ptr(new RevisionedVector(transaction, elements_begin, elements_end));
			transaction.commit();
			return ptr;
		}


		/**
		 * Create a duplicate of this RevisionedVector instance.
		 *
		 * This also duplicates (clones) the contained revisionable elements.
		 */
		const non_null_ptr_type
		clone() const
		{
			return GPlatesUtils::dynamic_pointer_cast<RevisionedVector>(clone_impl());
		}


		/**
		 * Const iterator dereferences to give 'const_reference', which references a
		 * 'const_element_type' (which is the same as 'RevisionableType::non_null_ptr_to_const_type').
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
		 * Non-const iterator dereferences to give 'reference', which references a
		 * 'element_type' (which is the same as 'RevisionableType::non_null_ptr_type').
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
				const element_type &elem);

		void
		pop_back();

		iterator
		insert(
				iterator pos,
				const element_type &elem);

		template <typename Iter>
		void
		insert(
				iterator pos,
				Iter first,
				Iter last);

		iterator
		erase(
				iterator pos);

		iterator
		erase(
				iterator first,
				iterator last);

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		template <typename ElementIter>
		RevisionedVector(
				ModelTransaction &transaction_,
				ElementIter elements_begin,
				ElementIter elements_end) :
			Revisionable(
					typename Revision::non_null_ptr_type(
							new Revision(transaction_, *this, elements_begin, elements_end)))
		{  }

		//! Constructor used when cloning.
		RevisionedVector(
				const RevisionedVector &other_,
				boost::optional<RevisionContext &> context_) :
			Revisionable(
					typename Revision::non_null_ptr_type(
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
				const Revisionable::non_null_ptr_to_const_type &child_revisionable);


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
				std::size_t element_index) const;


		/**
		 * Returns the 'non-const' element at the specified index.
		 */
		element_type
		get_element(
				std::size_t element_index);


		/**
		 * Set an element in-place.
		 */
		void
		set_element(
				const element_type &element,
				std::size_t element_index);


		//! Typedef for a revisioned reference to an element (revisionable).
		typedef RevisionedReference<RevisionableType> element_revisioned_reference_type;

		//! Typedef for the internal vector of elements (stored in each vector revision snapshot).
		typedef std::vector<element_revisioned_reference_type> vector_element_revisioned_reference_type;


		//! Convert to 'vector_element_revisioned_reference_type::const_iterator' from wrapped iterator.
		typename vector_element_revisioned_reference_type::const_iterator
		to_internal_iterator(
				vector_element_revisioned_reference_type &elements,
				const_iterator iter) const
		{
			typename vector_element_revisioned_reference_type::const_iterator internal_iter = elements.begin();
			std::advance(internal_iter, iter.d_index);
			return internal_iter;
		}

		//! Convert to 'vector_element_revisioned_reference_type::iterator' from wrapped iterator.
		typename vector_element_revisioned_reference_type::iterator
		to_internal_iterator(
				vector_element_revisioned_reference_type &elements,
				iterator iter)
		{
			typename vector_element_revisioned_reference_type::iterator internal_iter = elements.begin();
			std::advance(internal_iter, iter.d_index);
			return internal_iter;
		}

		//! Convert from 'vector_element_revisioned_reference_type::const_iterator' iterator to wrapped iterator.
		const_iterator
		from_internal_iterator(
				vector_element_revisioned_reference_type &elements,
				typename vector_element_revisioned_reference_type::const_iterator internal_iter) const
		{
			return const_iterator(this, std::distance(elements.begin(), internal_iter));
		}

		//! Convert from 'vector_element_revisioned_reference_type::const_iterator' iterator to wrapped iterator.
		iterator
		from_internal_iterator(
				vector_element_revisioned_reference_type &elements,
				typename vector_element_revisioned_reference_type::iterator internal_iter)
		{
			return iterator(this, std::distance(elements.begin(), internal_iter));
		}


		/**
		 * Property value data that is mutable/revisionable.
		 */
		struct Revision :
				public GPlatesModel::Revision
		{
			template <typename ElementIter>
			Revision(
					ModelTransaction &transaction_,
					RevisionContext &child_context_,
					ElementIter elements_begin_,
					ElementIter elements_end_)
			{
				BOOST_FOREACH(
						const element_type &element_,
						std::make_pair(elements_begin_, elements_end_))
				{
					elements.push_back(
							// Revisioned elements bubble up to us...
							element_revisioned_reference_type::attach(transaction_, child_context_, element_));
				}
			}

			//! Deep-clone constructor.
			Revision(
					const Revision &other_,
					boost::optional<RevisionContext &> context_,
					RevisionContext &child_context_) :
				GPlatesModel::Revision(context_),
				elements(other_.elements)
			{
				// Clone data members that were not deep copied.
				BOOST_FOREACH(element_revisioned_reference_type &element, elements)
				{
					element.clone(child_context_);
				}
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
					const GPlatesModel::Revision &other) const;


			vector_element_revisioned_reference_type elements;
		};

	};
}

////////////////////
// Implementation //
////////////////////

namespace GPlatesModel
{
	template <typename RevisionableType>
	GPlatesModel::Revision::non_null_ptr_type
	RevisionedVector<RevisionableType>::bubble_up(
			ModelTransaction &transaction,
			const Revisionable::non_null_ptr_to_const_type &child_revisionable)
	{
		// Bubble up to our (parent) context (if any) which creates a new revision for us.
		Revision &revision = create_bubble_up_revision<Revision>(transaction);

		// In this method we are operating on a (bubble up) cloned version of the current revision.

		// Find which element bubbled up.
		BOOST_FOREACH(element_revisioned_reference_type &element, revision.elements)
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


	template <typename RevisionableType>
	typename RevisionedVector<RevisionableType>::const_element_type
	RevisionedVector<RevisionableType>::get_element(
			std::size_t element_index) const
	{
		const Revision &revision = get_current_revision<Revision>();

		// Make sure we're not dereferencing out-of-bounds.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				element_index < revision.elements.size(),
				GPLATES_ASSERTION_SOURCE);

		return revision.elements[element_index].get_revisionable();
	}


	template <typename RevisionableType>
	typename RevisionedVector<RevisionableType>::element_type
	RevisionedVector<RevisionableType>::get_element(
			std::size_t element_index)
	{
		const Revision &revision = get_current_revision<Revision>();

		// Make sure we're not dereferencing out-of-bounds.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				element_index < revision.elements.size(),
				GPLATES_ASSERTION_SOURCE);

		return revision.elements[element_index].get_revisionable();
	}


	template <typename RevisionableType>
	void
	RevisionedVector<RevisionableType>::set_element(
			const element_type &element,
			std::size_t element_index)
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


	template <typename RevisionableType>
	void
	RevisionedVector<RevisionableType>::push_back(
			const element_type &elem)
	{
		BubbleUpRevisionHandler revision_handler(this);
		Revision &revision = revision_handler.get_revision<Revision>();

		revision.elements.push_back(
				element_revisioned_reference_type::attach(
						revision_handler.get_model_transaction(), *this, elem));

		revision_handler.commit();
	}



	template <typename RevisionableType>
	void
	RevisionedVector<RevisionableType>::pop_back()
	{
		BubbleUpRevisionHandler revision_handler(this);
		Revision &revision = revision_handler.get_revision<Revision>();

		// Detach the element before erasing it.
		revision.elements.back().detach(revision_handler.get_model_transaction());
		revision.elements.pop_back();

		revision_handler.commit();
	}


	template <typename RevisionableType>
	typename RevisionedVector<RevisionableType>::iterator
	RevisionedVector<RevisionableType>::insert(
			iterator pos,
			const element_type &elem)
	{
		BubbleUpRevisionHandler revision_handler(this);
		Revision &revision = revision_handler.get_revision<Revision>();

		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				pos.d_revisioned_vector == this &&
					pos.d_index <= revision.elements.size(),
				GPLATES_ASSERTION_SOURCE);

		typename vector_element_revisioned_reference_type::iterator ret =
				revision.elements.insert(
						to_internal_iterator(revision.elements, pos),
						element_revisioned_reference_type::attach(
								revision_handler.get_model_transaction(), *this, elem));

		revision_handler.commit();

		return from_internal_iterator(revision.elements, ret);
	}


	template <typename RevisionableType>
	template <typename Iter>
	void
	RevisionedVector<RevisionableType>::insert(
			iterator pos,
			Iter first,
			Iter last)
	{
		BubbleUpRevisionHandler revision_handler(this);
		Revision &revision = revision_handler.get_revision<Revision>();

		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				pos.d_revisioned_vector == this &&
					pos.d_index <= revision.elements.size(),
				GPLATES_ASSERTION_SOURCE);

		// Attach the elements before inserting them.
		vector_element_revisioned_reference_type elements;
		for (Iter iter = first; iter != last; ++iter)
		{
			elements.push_back(
					element_revisioned_reference_type::attach(
							revision_handler.get_model_transaction(), *this, *iter));
		}

		// Insert the elements.
		revision.elements.insert(
				to_internal_iterator(revision.elements, pos),
				elements.begin(),
				elements.end());

		revision_handler.commit();
	}


	template <typename RevisionableType>
	typename RevisionedVector<RevisionableType>::iterator
	RevisionedVector<RevisionableType>::erase(
			iterator pos)
	{
		BubbleUpRevisionHandler revision_handler(this);
		Revision &revision = revision_handler.get_revision<Revision>();

		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				pos.d_revisioned_vector == this &&
					pos.d_index < revision.elements.size(),
				GPLATES_ASSERTION_SOURCE);

		// Detach the element before erasing it.
		typename vector_element_revisioned_reference_type::iterator erase_iter =
				to_internal_iterator(revision.elements, pos);
		erase_iter->detach(revision_handler.get_model_transaction());

		typename vector_element_revisioned_reference_type::iterator ret =
				revision.elements.erase(erase_iter);

		revision_handler.commit();

		return from_internal_iterator(revision.elements, ret);
	}


	template <typename RevisionableType>
	typename RevisionedVector<RevisionableType>::iterator
	RevisionedVector<RevisionableType>::erase(
			iterator first,
			iterator last)
	{
		BubbleUpRevisionHandler revision_handler(this);
		Revision &revision = revision_handler.get_revision<Revision>();

		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				first.d_revisioned_vector == this &&
					last.d_revisioned_vector == this &&
					first.d_index < revision.elements.size() &&
					last.d_index <= revision.elements.size() &&
					first.d_index <= last.d_index,
				GPLATES_ASSERTION_SOURCE);

		// Detach the elements before erasing them.
		for (iterator iter = first; iter != last; ++iter)
		{
			typename vector_element_revisioned_reference_type::iterator erase_iter =
					to_internal_iterator(revision.elements, iter);

			erase_iter->detach(revision_handler.get_model_transaction());
		}

		// Erase the elements.
		typename vector_element_revisioned_reference_type::iterator ret =
				revision.elements.erase(
						to_internal_iterator(revision.elements, first),
						to_internal_iterator(revision.elements, last));

		revision_handler.commit();

		return from_internal_iterator(revision.elements, ret);
	}


	template <typename RevisionableType>
	bool
	RevisionedVector<RevisionableType>::Revision::equality(
			const GPlatesModel::Revision &other) const
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
}

#endif // GPLATES_MODEL_REVISIONEDVECTOR_H
