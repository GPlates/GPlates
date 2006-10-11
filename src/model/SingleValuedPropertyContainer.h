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

#ifndef GPLATES_MODEL_SINGLEVALUEDPROPERTY_H
#define GPLATES_MODEL_SINGLEVALUEDPROPERTY_H

#include <unicode/unistr.h>
#include <boost/intrusive_ptr.hpp>
#include "model/PropertyContainer.h"


namespace GPlatesModel {

	// forward references
	class PropertyValue;

	class SingleValuedPropertyContainer :
			public PropertyContainer {

	public:

		static
		boost::intrusive_ptr<PropertyContainer>
		create(const UnicodeString &property_name_) {
			boost::intrusive_ptr<PropertyContainer> ptr(new SingleValuedPropertyContainer(property_name_));
			return ptr;
		}

		boost::intrusive_ptr<PropertyContainer>
		clone() const {
			boost::intrusive_ptr<PropertyContainer> dup(new SingleValuedPropertyContainer(*this));
			return dup;
		}

		// FIXME: visitor accept method

	protected:

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

		SingleValuedPropertyContainer &
		operator=(SingleValuedPropertyContainer &);

	};

}

#endif  // GPLATES_MODEL_SINGLEVALUEDPROPERTY_H
