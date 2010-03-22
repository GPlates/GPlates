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

#include "TopLevelProperty.h"
#include "PropertyValueContainer.h"
#include "PropertyValue.h"


namespace GPlatesModel
{
	/**
	 * This class represents a top-level property of a feature, which is containing its
	 * property-value inline.
	 */
	class TopLevelPropertyInline:
			public TopLevelProperty, public PropertyValueContainer
	{
	public:

		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<TopLevelPropertyInline,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<TopLevelPropertyInline,
				GPlatesUtils::NullIntrusivePointerHandler> non_null_ptr_type;

		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<const TopLevelPropertyInline,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const TopLevelPropertyInline,
				GPlatesUtils::NullIntrusivePointerHandler>
				non_null_ptr_to_const_type;

		typedef std::vector<PropertyValue::non_null_ptr_type> container_type;

		typedef container_type::iterator iterator;
		typedef container_type::const_iterator const_iterator;


		virtual
		~TopLevelPropertyInline()
		{  }

		static
		const non_null_ptr_type
		create(
				const PropertyName &property_name_,
				const container_type &values_,
				const std::map<XmlAttributeName, XmlAttributeValue> &xml_attributes_)
		{
			non_null_ptr_type ptr(
					new TopLevelPropertyInline(property_name_, values_, xml_attributes_),
					GPlatesUtils::NullIntrusivePointerHandler());
			return ptr;
		}

		static
		const non_null_ptr_type
		create(
				const PropertyName &property_name_,
				PropertyValue::non_null_ptr_type value_,
				const std::map<XmlAttributeName, XmlAttributeValue> &xml_attributes_)
		{
			non_null_ptr_type ptr(
					new TopLevelPropertyInline(property_name_, value_, xml_attributes_),
					GPlatesUtils::NullIntrusivePointerHandler());
			return ptr;
		}

		virtual
		const TopLevelProperty::non_null_ptr_type
		clone() const
		{
			TopLevelProperty::non_null_ptr_type dup(
					new TopLevelPropertyInline(*this),
					GPlatesUtils::NullIntrusivePointerHandler());
			return dup;
		}

		virtual
		const TopLevelProperty::non_null_ptr_type
		deep_clone() const;

		const_iterator
		begin() const
		{
			return d_values.begin();
		}

		const_iterator
		end() const
		{
			return d_values.end();
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

		virtual
		void
		accept_visitor(
				ConstFeatureVisitor &visitor) const;

		virtual
		void
		accept_visitor(
				FeatureVisitor &visitor);

		/**
		 * Client code should not use this function!
		 *
		 * This is part of the mechanism which tracks whether a feature collection contains
		 * unsaved changes, and (later) part of the Bubble-Up mechanism.
		 */
		virtual
		void
		bubble_up_change(
				const PropertyValue *old_value,
				PropertyValue::non_null_ptr_type new_value,
				DummyTransactionHandle &transaction);

	protected:
		TopLevelPropertyInline(
				const PropertyName &property_name_,
				const container_type &values_,
				const std::map<XmlAttributeName, XmlAttributeValue> &xml_attributes_) :
			TopLevelProperty(property_name_, xml_attributes_),
			d_values(values_)
		{  }

		TopLevelPropertyInline(
				const PropertyName &property_name_,
				PropertyValue::non_null_ptr_type value_,
				const std::map<XmlAttributeName, XmlAttributeValue> &xml_attributes_) :
			TopLevelProperty(property_name_, xml_attributes_)
		{
			d_values.push_back(value_);
		}

		TopLevelPropertyInline(
				const TopLevelPropertyInline &other) :
			TopLevelProperty(other),
			PropertyValueContainer(other),
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

}

#endif  // GPLATES_MODEL_TOPLEVELPROPERTYINLINE_H
