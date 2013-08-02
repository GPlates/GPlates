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
		 */
		template <class PropertyValueQualifiedType>
		class Iterator:
				public std::iterator<std::bidirectional_iterator_tag, PropertyValueQualifiedType>,
				public boost::bidirectional_iteratable<Iterator<PropertyValueQualifiedType>, PropertyValueQualifiedType *>
		{
		public:

			//! Typedef for the element list iterator.
			typedef typename boost::mpl::if_<
					boost::is_const<PropertyValueQualifiedType>,
								typename container_type::const_iterator,
								typename container_type::iterator>::type
										iterator_type;

			explicit
			Iterator(
					iterator_type iterator_) :
				d_iterator(iterator_)
			{  }

			/**
			 * Dereference operator.
			 *
			 * Note that this does not return a *reference* to a property value non_null_ptr_type and
			 * hence clients cannot use '*iter = new_ptr' to swap one property value pointer for another.
			 *
			 * 'operator->()' provided by base class boost::bidirectional_iteratable.
			 */
			typename GPlatesGlobal::PointerTraits<PropertyValueQualifiedType>::non_null_ptr_type
			operator*() const
			{
				return d_iterator->get_property_value();
			}

			//! Post-increment operator provided by base class boost::bidirectional_iteratable.
			Iterator &
			operator++()
			{
				++d_iterator;
				return *this;
			}

			//! Inequality operator provided by base class boost::bidirectional_iteratable.
			friend
			bool
			operator==(
					const Iterator &lhs,
					const Iterator &rhs)
			{
				return lhs.d_iterator == rhs.d_iterator;
			}

		private:
			iterator_type d_iterator;
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
				const xml_attributes_type &xml_attributes_ = xml_attributes_type());

		static
		const non_null_ptr_type
		create(
				const PropertyName &property_name_,
				PropertyValue::non_null_ptr_type value_,
				const xml_attributes_type &xml_attributes_ = xml_attributes_type());

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
				const AttributeIterator &attributes_end);

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
			return const_iterator(d_values.begin());
		}

		const_iterator
		end() const
		{
			return const_iterator(d_values.end());
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
			return iterator(d_values.begin());
		}

		iterator
		end()
		{
			return iterator(d_values.end());
		}

		size_t
		size() const
		{
			return d_values.size();
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

		virtual
		bool
		equality(
				const TopLevelProperty &other) const;

	protected:

		template<class PropertyValueIterator>
		TopLevelPropertyInline(
				const PropertyName &property_name_,
				const PropertyValueIterator &values_begin_,
				const PropertyValueIterator &values_end_,
				const xml_attributes_type &xml_attributes_);

		/**
		 * Copy constructor does a deep copy.
		 */
		TopLevelPropertyInline(
				const TopLevelPropertyInline &other);

		virtual
		const GPlatesModel::TopLevelProperty::non_null_ptr_type
		clone_impl() const
		{
			return non_null_ptr_type(new TopLevelPropertyInline(*this));
		}

	private:

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		TopLevelPropertyInline &
		operator=(
				const TopLevelPropertyInline &);


		container_type d_values;

	};


	template<class PropertyValueIterator>
	const TopLevelPropertyInline::non_null_ptr_type
	TopLevelPropertyInline::create(
			const PropertyName &property_name_,
			const PropertyValueIterator &values_begin_,
			const PropertyValueIterator &values_end_,
			const xml_attributes_type &xml_attributes_)
	{
		return non_null_ptr_type(
				new TopLevelPropertyInline(
					property_name_,
					values_begin_,
					values_end_,
					xml_attributes_));
	}


	template<class AttributeIterator>
	const TopLevelPropertyInline::non_null_ptr_type
	TopLevelPropertyInline::create(
			const PropertyName &property_name_,
			PropertyValue::non_null_ptr_type value_,
			const AttributeIterator &attributes_begin,
			const AttributeIterator &attributes_end)
	{
		xml_attributes_type xml_attributes_(
				attributes_begin,
				attributes_end);
		return create(
				property_name_,
				value_,
				xml_attributes_);
	}


	template<class PropertyValueIterator>
	TopLevelPropertyInline::TopLevelPropertyInline(
			const PropertyName &property_name_,
			const PropertyValueIterator &values_begin_,
			const PropertyValueIterator &values_end_,
			const xml_attributes_type &xml_attributes_) :
		TopLevelProperty(property_name_, xml_attributes_)
	{
		PropertyValueIterator values_iter = values_begin_;
		for ( ; values_iter != values_end_; ++values_iter)
		{
			const PropertyValue::non_null_ptr_type &property_value = *values_iter;

			// A revisioned reference to the property value enables us to switch to its
			// revision later (eg, during undo/redo).
			PropertyValue::RevisionedReference revisioned_property_value =
					property_value->get_current_revisioned_reference();

			// Enable property value modifications to bubble up to us.
			// Note that we didn't clone the property value - so an external client could modify
			// it (if it has a pointer to the property value) and this modification will bubble up
			// to us and cause us to create a new TopLevelProperty revision.
			revisioned_property_value.get_property_value()->set_parent(*this);

			d_values.push_back(revisioned_property_value);
		}
	}
}

#endif  // GPLATES_MODEL_TOPLEVELPROPERTYINLINE_H
