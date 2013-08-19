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

#include "ModelTransaction.h"
#include "PropertyValue.h"
#include "PropertyValueRevisionContext.h"
#include "PropertyValueRevisionedReference.h"
#include "TopLevelProperty.h"

#include "global/PointerTraits.h"
#include "global/unicode.h"

namespace GPlatesModel
{
	/**
	 * This class represents a top-level property of a feature, which is containing its
	 * property-value inline.
	 */
	class TopLevelPropertyInline:
			public TopLevelProperty,
			public PropertyValueRevisionContext
	{
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
		~TopLevelPropertyInline()
		{  }

		template<class PropertyValueIterator>
		static
		const non_null_ptr_type
		create(
				const PropertyName &property_name_,
				const PropertyValueIterator &values_begin_,
				const PropertyValueIterator &values_end_,
				const xml_attributes_type &xml_attributes_ = xml_attributes_type())
		{
			ModelTransaction transaction;
			non_null_ptr_type ptr(
					new TopLevelPropertyInline(
							transaction, property_name_, values_begin_, values_end_, xml_attributes_));
			transaction.commit();
			return ptr;
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
				ModelTransaction &transaction_,
				const PropertyName &property_name_,
				const PropertyValueIterator &values_begin_,
				const PropertyValueIterator &values_end_,
				const xml_attributes_type &xml_attributes_) :
			TopLevelProperty(
					property_name_,
					Revision::non_null_ptr_type(
							new Revision(transaction_, *this, values_begin_, values_end_, xml_attributes_)))
		{  }

		//! Constructor used when cloning.
		TopLevelPropertyInline(
				const TopLevelPropertyInline &other_,
				boost::optional<FeatureHandle &> context_) :
			TopLevelProperty(
					other_,
					Revision::non_null_ptr_type(
							// Use deep-clone constructor...
							new Revision(other_.get_current_revision<Revision>(), context_, *this)))
		{  }

		virtual
		const TopLevelProperty::non_null_ptr_type
		clone_impl(
				boost::optional<FeatureHandle &> context = boost::none) const
		{
			return non_null_ptr_type(new TopLevelPropertyInline(*this, context));
		}

	private:

		/**
		 * Used when modifications bubble up to us.
		 *
		 * Inherited from @a PropertyValueRevisionContext.
		 */
		virtual
		PropertyValueRevision::non_null_ptr_type
		bubble_up(
				ModelTransaction &transaction,
				const PropertyValue::non_null_ptr_to_const_type &child_property_value);

		/**
		 * Inherited from @a PropertyValueRevisionContext.
		 */
		virtual
		boost::optional<Model &>
		get_model()
		{
			return TopLevelProperty::get_model();
		}


		/**
		 * The type of our container of PropertyValue revisioned references.
		 */
		typedef std::vector<PropertyValueRevisionedReference<PropertyValue> > property_value_container_type;


		/**
		 * Top level property data that is mutable/revisionable.
		 */
		struct Revision :
				public TopLevelPropertyRevision
		{
			template<class PropertyValueIterator>
			Revision(
					ModelTransaction &transaction_,
					PropertyValueRevisionContext &child_context_,
					const PropertyValueIterator &values_begin_,
					const PropertyValueIterator &values_end_,
					const xml_attributes_type &xml_attributes_) :
				TopLevelPropertyRevision(xml_attributes_)
			{
				PropertyValueIterator values_iter = values_begin_;
				for ( ; values_iter != values_end_; ++values_iter)
				{
					PropertyValue::non_null_ptr_type property_value = *values_iter;

					// A revisioned reference to the property value enables us to switch to its
					// revision later (eg, during undo/redo).
					PropertyValueRevisionedReference<PropertyValue> revisioned_property_value =
							PropertyValueRevisionedReference<PropertyValue>::attach(
									transaction_, child_context_, property_value);

					values.push_back(revisioned_property_value);
				}
			}

			//! Deep-clone constructor.
			Revision(
					const Revision &other_,
					boost::optional<FeatureHandle &> context_,
					PropertyValueRevisionContext &child_context_) :
				TopLevelPropertyRevision(other_, context_),
				values(other_.values)
			{
				// Clone data members that were not deep copied.
				property_value_container_type::iterator values_iter = values.begin();
				property_value_container_type::iterator values_end = values.end();
				for ( ; values_iter != values_end; ++values_iter)
				{
					values_iter->clone(child_context_);
				}
			}

			//! Shallow-clone constructor.
			Revision(
					const Revision &other_,
					boost::optional<FeatureHandle &> context_) :
				TopLevelPropertyRevision(other_, context_),
				values(other_.values)
			{  }

			virtual
			TopLevelPropertyRevision::non_null_ptr_type
			clone_revision(
					boost::optional<FeatureHandle &> context) const
			{
				// Use shallow-clone constructor.
				return non_null_ptr_type(new Revision(*this, context));
			}

			virtual
			bool
			equality(
					const TopLevelPropertyRevision &other) const;

			property_value_container_type values;
		};

	};
}

#endif  // GPLATES_MODEL_TOPLEVELPROPERTYINLINE_H
