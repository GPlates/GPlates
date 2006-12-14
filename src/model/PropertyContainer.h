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

#ifndef GPLATES_MODEL_PROPERTYCONTAINER_H
#define GPLATES_MODEL_PROPERTYCONTAINER_H

#include <map>
#include <boost/intrusive_ptr.hpp>
// Even though we could make do with a forward declaration inside this header, every derived class
// of 'PropertyContainer' will need to #include "ConstFeatureVisitor.h" anyway, so we may as well
// include it here.
#include "ConstFeatureVisitor.h"
#include "PropertyName.h"
#include "XmlAttributeName.h"
#include "XmlAttributeValue.h"


namespace GPlatesModel {

	class PropertyContainer {

	public:

		typedef long ref_count_type;

		virtual
		~PropertyContainer()
		{ }

		/**
		 * Construct a PropertyContainer instance with the given property name.
		 *
		 * Since this class is an abstract class, this constructor can never be invoked
		 * other than explicitly in the initialiser lists of derived classes. 
		 * Nevertheless, the initialiser lists of derived classes @em do need to invoke it
		 * explicitly, since this class contains members which need to be initialised.
		 */
		PropertyContainer(
				const PropertyName &property_name_,
				const std::map<XmlAttributeName, XmlAttributeValue> &xml_attributes_):
			d_ref_count(0),
			d_property_name(property_name_),
			d_xml_attributes(xml_attributes_)
		{ }

		/**
		 * Construct a PropertyContainer instance which is a copy of @a other.
		 *
		 * Since this class is an abstract class, this constructor can never be invoked
		 * other than explicitly in the initialiser lists of derived classes. 
		 * Nevertheless, the initialiser lists of derived classes @em do need to invoke it
		 * explicitly, since this class contains members which need to be initialised.
		 */
		PropertyContainer(
				const PropertyContainer &other) :
			d_ref_count(other.d_ref_count),
			d_property_name(other.d_property_name),
			d_xml_attributes(other.d_xml_attributes)
		{ }

		virtual
		const boost::intrusive_ptr<PropertyContainer>
		clone() const = 0;

		const PropertyName &
		property_name() const {
			return d_property_name;
		}

		const std::map<XmlAttributeName, XmlAttributeValue> &
		xml_attributes() const {
			return d_xml_attributes;
		}

		std::map<XmlAttributeName, XmlAttributeValue> &
		xml_attributes() {
			return d_xml_attributes;
		}

		virtual
		void
		accept_visitor(
				ConstFeatureVisitor &visitor) const = 0;

		void
		increment_ref_count() const {
			++d_ref_count;
		}

		ref_count_type
		decrement_ref_count() const {
			return --d_ref_count;
		}

	private:

		mutable ref_count_type d_ref_count;
		PropertyName d_property_name;
		std::map<XmlAttributeName, XmlAttributeValue> d_xml_attributes;

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		PropertyContainer &
		operator=(
				const PropertyContainer &);

	};


	inline
	void
	intrusive_ptr_add_ref(
			const PropertyContainer *p) {
		p->increment_ref_count();
	}


	inline
	void
	intrusive_ptr_release(
			const PropertyContainer *p) {
		if (p->decrement_ref_count() == 0) {
			delete p;
		}
	}

}

#endif  // GPLATES_MODEL_PROPERTYCONTAINER_H
