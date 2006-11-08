/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006 The GPlates Consortium
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef GPLATES_MODEL_SINGLEVALUEDPROPERTYCONTAINER_H
#define GPLATES_MODEL_SINGLEVALUEDPROPERTYCONTAINER_H

#include <map>
#include <unicode/unistr.h>
#include <boost/intrusive_ptr.hpp>
#include "PropertyContainer.h"
#include "PropertyValue.h"
#include "XmlAttributeName.h"
#include "XmlAttributeValue.h"


namespace GPlatesModel {

	class SingleValuedPropertyContainer :
			public PropertyContainer {

	public:

		virtual
		~SingleValuedPropertyContainer() {  }

		static
		boost::intrusive_ptr<SingleValuedPropertyContainer>
		create(
				const PropertyName &property_name_,
				boost::intrusive_ptr<PropertyValue> value_,
				const std::map<XmlAttributeName, XmlAttributeValue> xml_attributes_,
				bool value_is_optional_) {
			boost::intrusive_ptr<SingleValuedPropertyContainer> ptr(
					new SingleValuedPropertyContainer(
							property_name_, value_, xml_attributes_, value_is_optional_));
			return ptr;
		}

		virtual
		boost::intrusive_ptr<PropertyContainer>
		clone() const {
			boost::intrusive_ptr<PropertyContainer> dup(new SingleValuedPropertyContainer(*this));
			return dup;
		}

		// FIXME: visitor accept method

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		SingleValuedPropertyContainer(
				const PropertyName &property_name_,
				boost::intrusive_ptr<PropertyValue> value_,
				const std::map<XmlAttributeName, XmlAttributeValue> xml_attributes_,
				bool value_is_optional_) :
			PropertyContainer(property_name_),
			d_value(value_),
			d_xml_attributes(xml_attributes_),
			d_value_is_optional(value_is_optional_)
		{ }

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		//
		// Note that this should act exactly the same as the default (auto-generated)
		// copy-constructor, except it should not be public.
		SingleValuedPropertyContainer(
				const SingleValuedPropertyContainer &other) :
			PropertyContainer(other),
			d_value(other.d_value),
			d_xml_attributes(other.d_xml_attributes),
			d_value_is_optional(other.d_value_is_optional)
		{ }

	private:

		boost::intrusive_ptr<PropertyValue> d_value;
		std::map<XmlAttributeName, XmlAttributeValue> d_xml_attributes;
		bool d_value_is_optional;

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		SingleValuedPropertyContainer &
		operator=(const SingleValuedPropertyContainer &);

	};

}

#endif  // GPLATES_MODEL_SINGLEVALUEDPROPERTYCONTAINER_H
