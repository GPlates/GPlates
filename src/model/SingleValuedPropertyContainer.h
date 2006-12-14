/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006 The University of Sydney, Australia
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

#ifndef GPLATES_MODEL_SINGLEVALUEDPROPERTYCONTAINER_H
#define GPLATES_MODEL_SINGLEVALUEDPROPERTYCONTAINER_H

#include "PropertyContainer.h"
#include "PropertyValue.h"


namespace GPlatesModel {

	class SingleValuedPropertyContainer :
			public PropertyContainer {

	public:

		virtual
		~SingleValuedPropertyContainer() {  }

		static
		const boost::intrusive_ptr<SingleValuedPropertyContainer>
		create(
				const PropertyName &property_name_,
				boost::intrusive_ptr<PropertyValue> value_,
				const std::map<XmlAttributeName, XmlAttributeValue> &xml_attributes_,
				bool value_is_optional_) {
			boost::intrusive_ptr<SingleValuedPropertyContainer> ptr(
					new SingleValuedPropertyContainer(
							property_name_, value_, xml_attributes_, value_is_optional_));
			return ptr;
		}

		virtual
		const boost::intrusive_ptr<PropertyContainer>
		clone() const {
			boost::intrusive_ptr<PropertyContainer> dup(new SingleValuedPropertyContainer(*this));
			return dup;
		}

		const boost::intrusive_ptr<const PropertyValue>
		value() const {
			return d_value;
		}

		const boost::intrusive_ptr<PropertyValue>
		value() {
			return d_value;
		}

		bool
		value_is_optional() const {
			return d_value_is_optional;
		}

		virtual
		void
		accept_visitor(
				ConstFeatureVisitor &visitor) const {
			visitor.visit_single_valued_property_container(*this);
		}

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		SingleValuedPropertyContainer(
				const PropertyName &property_name_,
				boost::intrusive_ptr<PropertyValue> value_,
				const std::map<XmlAttributeName, XmlAttributeValue> &xml_attributes_,
				bool value_is_optional_) :
			PropertyContainer(property_name_, xml_attributes_),
			d_value(value_),
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
			d_value_is_optional(other.d_value_is_optional)
		{ }

	private:

		/*
		 * Note that this pointer can be NULL.
		 *
		 * It is quite valid for this pointer to be NULL if the property is an optional
		 * property, and the value is absent.
		 *
		 * Of course, even if the property is NOT optional, we may have to handle
		 * situations in which the value is absent...
		 */
		boost::intrusive_ptr<PropertyValue> d_value;
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
