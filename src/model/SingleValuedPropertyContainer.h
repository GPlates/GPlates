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

#include <unicode/unistr.h>
#include <boost/intrusive_ptr.hpp>
#include "model/PropertyContainer.h"


namespace GPlatesModel {

	// Forward declaration for intrusive-pointer.
	class PropertyValue;

	class SingleValuedPropertyContainer :
			public PropertyContainer {

	public:

		virtual
		~SingleValuedPropertyContainer() {  }

		static
		boost::intrusive_ptr<PropertyContainer>
		create(const UnicodeString &property_name_) {
			boost::intrusive_ptr<PropertyContainer> ptr(new SingleValuedPropertyContainer(property_name_));
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

		// This operator should not be public, because we don't want to allow instantiation
		// of this type on the stack.
		explicit
		SingleValuedPropertyContainer(const UnicodeString &property_name_) :
			PropertyContainer(property_name_),
			d_value(NULL),
			d_value_is_optional(false)
		{ }

	private:

		boost::intrusive_ptr<PropertyValue> d_value;
		std::map<UnicodeString, UnicodeString> d_xml_attributes;
		bool d_value_is_optional;

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		SingleValuedPropertyContainer &
		operator=(SingleValuedPropertyContainer &);

	};

}

#endif  // GPLATES_MODEL_SINGLEVALUEDPROPERTYCONTAINER_H
