/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007 The University of Sydney, Australia
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

#ifndef GPLATES_MODEL_INLINEPROPERTYCONTAINER_H
#define GPLATES_MODEL_INLINEPROPERTYCONTAINER_H

#include "PropertyContainer.h"
#include "PropertyValue.h"


namespace GPlatesModel
{
	class InlinePropertyContainer :
			public PropertyContainer
	{
	public:

		/**
		 * A convenience typedef for
		 * GPlatesContrib::non_null_intrusive_ptr<InlinePropertyContainer>.
		 */
		typedef GPlatesContrib::non_null_intrusive_ptr<InlinePropertyContainer>
				non_null_ptr_type;

		/**
		 * A convenience typedef for
		 * GPlatesContrib::non_null_intrusive_ptr<const InlinePropertyContainer>.
		 */
		typedef GPlatesContrib::non_null_intrusive_ptr<const InlinePropertyContainer>
				non_null_ptr_to_const_type;

		typedef std::vector<PropertyValue::non_null_ptr_type> container_type;

		typedef container_type::iterator iterator;
		typedef container_type::const_iterator const_iterator;


		virtual
		~InlinePropertyContainer()
		{  }

		static
		const non_null_ptr_type
		create(
				const PropertyName &property_name_,
				const container_type &values_,
				const std::map<XmlAttributeName, XmlAttributeValue> &xml_attributes_)
		{
			non_null_ptr_type ptr(
					*(new InlinePropertyContainer(property_name_, values_,
							xml_attributes_)));
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
					*(new InlinePropertyContainer(property_name_, value_, xml_attributes_)));
			return ptr;
		}

		virtual
		const PropertyContainer::non_null_ptr_type
		clone() const 
		{
			PropertyContainer::non_null_ptr_type dup(
					*(new InlinePropertyContainer(*this)));
			return dup;
		}

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
				ConstFeatureVisitor &visitor) const 
		{
			visitor.visit_inline_property_container(*this);
		}

		virtual
		void
		accept_visitor(
				FeatureVisitor &visitor) 
		{
			visitor.visit_inline_property_container(*this);
		}

	protected:
		InlinePropertyContainer(
				const PropertyName &property_name_,
				const container_type &values_,
				const std::map<XmlAttributeName, XmlAttributeValue> &xml_attributes_) :
			PropertyContainer(property_name_, xml_attributes_),
			d_values(values_)
		{  }

		InlinePropertyContainer(
				const PropertyName &property_name_,
				PropertyValue::non_null_ptr_type value_,
				const std::map<XmlAttributeName, XmlAttributeValue> &xml_attributes_) :
			PropertyContainer(property_name_, xml_attributes_)
		{
			d_values.push_back(value_);
		}

		InlinePropertyContainer(
				const InlinePropertyContainer &other) :
			PropertyContainer(other),
			d_values(other.d_values)
		{  }

	private:
		container_type d_values;

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		InlinePropertyContainer &
		operator=(
				const InlinePropertyContainer &);

	};

}

#endif  // GPLATES_MODEL_INLINEPROPERTYCONTAINER_H
