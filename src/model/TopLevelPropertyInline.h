/* $Id$ */

/**
 * \file 
 * Contains the definition of TopLevelPropertyInline.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007 The University of Sydney, Australia
 *  (under the name "InlinePropertyContainer.h")
 * Copyright (C) 2009, 2010 The University of Sydney, Australia
 *  (under the name "TopLevelPropertyInline.h")
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

#ifndef GPLATES_MODEL_TOPLEVELPROPERTYINLINE_H
#define GPLATES_MODEL_TOPLEVELPROPERTYINLINE_H

#include <iterator>
#include <vector>
#include <boost/function.hpp>
#include <boost/mpl/if.hpp>
#include <boost/operators.hpp>
#include <boost/optional.hpp>
#include <boost/type_traits/is_const.hpp>

#include "TopLevelProperty.h"
#include "PropertyValue.h"

#include "global/PointerTraits.h"
#include "global/unicode.h"

namespace GPlatesModel
{
	/**
	 * This class represents a top-level property of a feature, which is containing its
	 * property-value inline.
	 */
	class TopLevelPropertyInline:
			public TopLevelProperty
	{
	private:

		/**
		 * The type of our container of PropertyValue revisioned references.
		 */
		typedef std::vector<PropertyValue::RevisionedReference> container_type;

	public:

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<TopLevelPropertyInline>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<TopLevelPropertyInline> non_null_ptr_type;

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<const TopLevelPropertyInline>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const TopLevelPropertyInline> non_null_ptr_to_const_type;


		/**
		 * Iterator implementation.
		 *
		 * PropertyValueQualifiedType can be 'PropertyValue' or 'const PropertyValue'.
		 *
		 * This iterator can also work across revisions (eg, if change a property value during
		 * iteration then can continue iteration afterwards even though a new revision was created).
		 */
		template <class PropertyValueQualifiedType>
		class Iterator:
				public std::iterator<
						std::bidirectional_iterator_tag,
						// Dereferencing returns a temporary property value pointer...
						typename GPlatesGlobal::PointerTraits<PropertyValueQualifiedType>::non_null_ptr_type,
						ptrdiff_t,
						// The 'pointer' inner type is set to void, because the dereference operator
						// returns a temporary, and it is not desirable to take a pointer to a temporary...
						void,
						// The 'reference' inner type is not a reference, because the dereference operator
						// returns a temporary, and it is not desirable to take a reference to a temporary...
						typename GPlatesGlobal::PointerTraits<PropertyValueQualifiedType>::non_null_ptr_type>,
				public boost::incrementable<
						Iterator<PropertyValueQualifiedType>,
						boost::decrementable<
								Iterator<PropertyValueQualifiedType>,
								boost::equality_comparable<Iterator<PropertyValueQualifiedType> > > >
		{
		public:

			//! Typedef for the top level property pointer.
			typedef typename boost::mpl::if_<
					boost::is_const<PropertyValueQualifiedType>,
								const TopLevelPropertyInline *,
								TopLevelPropertyInline *>::type
										top_level_property_ptr_type;

			explicit
			Iterator(
					top_level_property_ptr_type top_level_property_inline,
					std::size_t index) :
				d_top_level_property_inline(top_level_property_inline),
				d_index(index)
			{  }

			/**
			 * Dereference operator.
			 *
			 * Note that this does not return a *reference* to a property value non_null_ptr_type and
			 * hence clients cannot use '*iter = new_ptr' to swap one property value pointer for another.
			 *
			 * 'operator->()' is not provided since address of a temporary object is not desirable.
			 */
			typename GPlatesGlobal::PointerTraits<PropertyValueQualifiedType>::non_null_ptr_type
			operator*() const
			{
				return d_top_level_property_inline->get_current_revision<Revision>()
						.values[d_index].get_property_value();
			}

			//! Post-increment operator provided by base class boost::incrementable.
			Iterator &
			operator++()
			{
				++d_index;
				return *this;
			}

			//! Post-decrement operator provided by base class boost::decrementable.
			Iterator &
			operator--()
			{
				--d_index;
				return *this;
			}

			//! Inequality operator provided by base class boost::equality_comparable.
			friend
			bool
			operator==(
					const Iterator &lhs,
					const Iterator &rhs)
			{
				return lhs.d_top_level_property_inline == rhs.d_top_level_property_inline &&
						lhs.d_index == rhs.d_index;
			}

		private:
			top_level_property_ptr_type d_top_level_property_inline;
			std::size_t d_index;
		};


		/**
		 * Non-const iterator type.
		 *
		 * Dereferencing will return a PropertyValue::non_null_ptr_type.
		 */
		typedef Iterator<PropertyValue> iterator;

		/**
		 * Const iterator type.
		 *
		 * Dereferencing will return a PropertyValue::non_null_ptr_to_const_type.
		 */
		typedef Iterator<const PropertyValue> const_iterator;


		virtual
		~TopLevelPropertyInline();

		template<class PropertyValueIterator>
		static
		const non_null_ptr_type
		create(
				const PropertyName &property_name_,
				const PropertyValueIterator &values_begin_,
				const PropertyValueIterator &values_end_,
				const xml_attributes_type &xml_attributes_ = xml_attributes_type())
		{
			return non_null_ptr_type(
					new TopLevelPropertyInline(
						property_name_,
						values_begin_,
						values_end_,
						xml_attributes_));
		}

		static
		const non_null_ptr_type
		create(
				const PropertyName &property_name_,
				PropertyValue::non_null_ptr_type value_,
				const xml_attributes_type &xml_attributes_ = xml_attributes_type())
		{
			return create(property_name_, &value_, &value_ + 1, xml_attributes_);
		}

		static
		const non_null_ptr_type
		create(
				const PropertyName &property_name_,
				PropertyValue::non_null_ptr_type value_,
				const GPlatesUtils::UnicodeString &attribute_name_string,
				const GPlatesUtils::UnicodeString &attribute_value_string);

		template<class AttributeIterator>
		static
		const non_null_ptr_type
		create(
				const PropertyName &property_name_,
				PropertyValue::non_null_ptr_type value_,
				const AttributeIterator &attributes_begin,
				const AttributeIterator &attributes_end)
		{
			return create(
					property_name_,
					value_,
					xml_attributes_type(attributes_begin, attributes_end));
		}

		const non_null_ptr_type
		clone() const
		{
			return GPlatesUtils::dynamic_pointer_cast<TopLevelPropertyInline>(clone_impl());
		}

		/**
		 * Const iterator dereferences to give PropertyValue::non_null_ptr_to_const_type.
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
		 * Non-const iterator dereferences to give PropertyValue::non_null_ptr_type.
		 *
		 * Note that this iterator cannot be used to replace property value pointers in the internal
		 * sequence using '*iter = new_ptr', but it is possible to modify the property value since
		 * the iterator dereferences to give a pointer to a *non-const* property value.
		 * This is supported since property values have their own revisioning and so modifications
		 * to the property value will bubble up to this top-level property and get revisioned properly.
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

		std::size_t
		size() const
		{
			return get_current_revision<Revision>().values.size();
		}

		virtual
		void
		accept_visitor(
				ConstFeatureVisitor &visitor) const;

		virtual
		void
		accept_visitor(
				FeatureVisitor &visitor);

		virtual
		std::ostream &
		print_to(
				std::ostream &os) const;

	protected:

		template<class PropertyValueIterator>
		TopLevelPropertyInline(
				const PropertyName &property_name_,
				const PropertyValueIterator &values_begin_,
				const PropertyValueIterator &values_end_,
				const xml_attributes_type &xml_attributes_) :
			TopLevelProperty(
					property_name_,
					Revision::non_null_ptr_type(
							new Revision(values_begin_, values_end_, xml_attributes_)))
		{
			// Enable property value modifications to bubble up to us.
			set_parent_on_child_property_values();
		}

		/**
		 * Copy constructor does a deep copy.
		 */
		TopLevelPropertyInline(
				const TopLevelPropertyInline &other) :
			TopLevelProperty(other)
		{
			// Enable property value modifications to bubble up to us.
			set_parent_on_child_property_values();
		}

		virtual
		const TopLevelProperty::non_null_ptr_type
		clone_impl() const
		{
			return non_null_ptr_type(new TopLevelPropertyInline(*this));
		}

	private:

		/**
		 * Top level property data that is mutable/revisionable.
		 */
		struct Revision :
				public TopLevelProperty::Revision
		{
			template<class PropertyValueIterator>
			Revision(
					const PropertyValueIterator &values_begin_,
					const PropertyValueIterator &values_end_,
					const xml_attributes_type &xml_attributes_) :
				TopLevelProperty::Revision(xml_attributes_)
			{
				PropertyValueIterator values_iter = values_begin_;
				for ( ; values_iter != values_end_; ++values_iter)
				{
					PropertyValue::non_null_ptr_type property_value = *values_iter;

					// A revisioned reference to the property value enables us to switch to its
					// revision later (eg, during undo/redo).
					PropertyValue::RevisionedReference revisioned_property_value =
							property_value->get_current_revisioned_reference();

					values.push_back(revisioned_property_value);
				}
			}

			//! Shallow copy of revisioned values.
			Revision(
					const container_type &values_,
					const xml_attributes_type &xml_attributes_) :
				TopLevelProperty::Revision(xml_attributes_),
				values(values_)
			{  }

			//! Deep copy constructor.
			Revision(
					const Revision &other);

			virtual
			TopLevelProperty::Revision::non_null_ptr_type
			clone() const
			{
				return non_null_ptr_type(new Revision(*this));
			}

			virtual
			TopLevelProperty::Revision::non_null_ptr_type
			clone_for_bubble_up_modification() const
			{
				// Don't clone the property values.
				// This can be achieved using our regular constructor which does shallow copying.
				return non_null_ptr_type(new Revision(values, xml_attributes));
			}

			virtual
			void
			reference_bubbled_up_property_value_revision(
					const PropertyValue::RevisionedReference &property_value_revisioned_reference);

			virtual
			bool
			equality(
					const TopLevelProperty::Revision &other) const;

			container_type values;
		};


		/**
		 * Set 'this' as the parent of the child property values.
		 */
		void
		set_parent_on_child_property_values();

		/**
		 * Unset the parent of the child property values.
		 */
		void
		unset_parent_on_child_property_values();


		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		TopLevelPropertyInline &
		operator=(
				const TopLevelPropertyInline &);

	};
}

#endif  // GPLATES_MODEL_TOPLEVELPROPERTYINLINE_H
