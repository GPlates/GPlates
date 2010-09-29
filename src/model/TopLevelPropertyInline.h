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

#include <vector>
#include <boost/optional.hpp>
#include <boost/function.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <boost/lambda/construct.hpp>
#include <unicode/unistr.h>

#include "TopLevelProperty.h"
#include "PropertyValue.h"

namespace GPlatesModel
{
	/**
	 * This class represents a top-level property of a feature, which is containing its
	 * property-value inline.
	 */
	class TopLevelPropertyInline:
			public TopLevelProperty
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
		 * The type of our container of PropertyValue pointers.
		 */
		typedef std::vector<PropertyValue::non_null_ptr_type> container_type;

		/**
		 * The type of an iterator that iterates over the PropertyValue instances
		 * contained in this TopLevelPropertyInline.
		 */
		typedef container_type::iterator iterator;

	private:

		/**
		 * The type of a function that converts a pointer to non-const
		 * PropertyValue to a pointer to const.
		 */
		typedef boost::function< PropertyValue::non_null_ptr_to_const_type (
				PropertyValue::non_null_ptr_type) > make_const_ptr_fn_type;

	public:

		/**
		 * The type of an iterator that const-iterates over the PropertyValue instances
		 * contained in this TopLevelPropertyInline.
		 */
		typedef boost::transform_iterator<make_const_ptr_fn_type, container_type::const_iterator> const_iterator;

		virtual
		~TopLevelPropertyInline()
		{  }

		static
		const non_null_ptr_type
		create(
				const PropertyName &property_name_,
				const container_type &values_,
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
				const UnicodeString &attribute_name_string,
				const UnicodeString &attribute_value_string);

		template<class AttributeIterator>
		static
		const non_null_ptr_type
		create(
				const PropertyName &property_name_,
				PropertyValue::non_null_ptr_type value_,
				const AttributeIterator &attributes_begin,
				const AttributeIterator &attributes_end);

		virtual
		const TopLevelProperty::non_null_ptr_type
		clone() const;

		virtual
		const TopLevelProperty::non_null_ptr_type
		deep_clone() const;

		const_iterator
		begin() const
		{
			return boost::make_transform_iterator(
					d_values.begin(),
					make_const_ptr_fn_type(boost::lambda::constructor<PropertyValue::non_null_ptr_to_const_type>()));
		}

		const_iterator
		end() const
		{
			return boost::make_transform_iterator(
					d_values.end(),
					make_const_ptr_fn_type(boost::lambda::constructor<PropertyValue::non_null_ptr_to_const_type>()));
		}

		iterator
		begin()
		{
			return d_values.begin();
		}

		iterator
		end()
		{
			return d_values.end();
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
		operator==(
				const TopLevelProperty &other) const;

	protected:

		TopLevelPropertyInline(
				const PropertyName &property_name_,
				const container_type &values_,
				const xml_attributes_type &xml_attributes_) :
			TopLevelProperty(property_name_, xml_attributes_),
			d_values(values_)
		{  }

		TopLevelPropertyInline(
				const PropertyName &property_name_,
				PropertyValue::non_null_ptr_type value_,
				const xml_attributes_type &xml_attributes_) :
			TopLevelProperty(property_name_, xml_attributes_)
		{
			d_values.push_back(value_);
		}

		TopLevelPropertyInline(
				const TopLevelPropertyInline &other) :
			TopLevelProperty(other),
			d_values(other.d_values)
		{  }

	private:

		container_type d_values;

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		TopLevelPropertyInline &
		operator=(
				const TopLevelPropertyInline &);

	};


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

}

#endif  // GPLATES_MODEL_TOPLEVELPROPERTYINLINE_H
