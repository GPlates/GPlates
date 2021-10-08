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

#ifndef GPLATES_MODEL_PROPERTYCONTAINER_H
#define GPLATES_MODEL_PROPERTYCONTAINER_H

#include <map>
// Even though we could make do with a forward declaration inside this header, every derived class
// of 'PropertyContainer' will need to #include "ConstFeatureVisitor.h" and "FeatureVisitor.h"
// anyway, so we may as well include them here.
#include "ConstFeatureVisitor.h"
#include "FeatureVisitor.h"
#include "PropertyName.h"
#include "XmlAttributeName.h"
#include "XmlAttributeValue.h"
#include "utils/non_null_intrusive_ptr.h"


namespace GPlatesModel
{
	class PropertyContainer
	{
	public:
		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<PropertyContainer>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<PropertyContainer> non_null_ptr_type;

		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<const PropertyContainer>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const PropertyContainer>
				non_null_ptr_to_const_type;

		typedef long ref_count_type;

		virtual
		~PropertyContainer()
		{  }

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
		{  }

		/**
		 * Construct a PropertyContainer instance which is a copy of @a other.
		 *
		 * Since this class is an abstract class, this constructor can never be invoked
		 * other than explicitly in the initialiser lists of derived classes. 
		 * Nevertheless, the initialiser lists of derived classes @em do need to invoke it
		 * explicitly, since this class contains members which need to be initialised.
		 *
		 * This ctor should only be invoked by the @a clone member function (pure virtual
		 * in this class; defined in derived classes), which will create a duplicate
		 * instance and return a new non_null_intrusive_ptr reference to the new duplicate.
		 * Since initially the only reference to the new duplicate will be the one returned
		 * by the @a clone function, *before* the new non_null_intrusive_ptr is created,
		 * the ref-count of the new PropertyContainer instance should be zero.
		 *
		 * Note that this ctor should act exactly the same as the default (auto-generated)
		 * copy-ctor, except that it should initialise the ref-count to zero.
		 */
		PropertyContainer(
				const PropertyContainer &other) :
			d_ref_count(0),
			d_property_name(other.d_property_name),
			d_xml_attributes(other.d_xml_attributes)
		{  }

		/**
		 * Create a duplicate of this PropertyContainer instance.
		 */
		virtual
		const non_null_ptr_type
		clone() const = 0;

		// Note that no "setter" is provided:  The property name of a PropertyContainer
		// instance should never be changed.
		const PropertyName &
		property_name() const
		{
			return d_property_name;
		}

		// @b FIXME:  Should this function be replaced with per-index const-access to
		// elements of the XML attribute map?  (For consistency with the non-const
		// overload...)
		const std::map<XmlAttributeName, XmlAttributeValue> &
		xml_attributes() const
		{
			return d_xml_attributes;
		}

		// @b FIXME:  Should this function be replaced with per-index const-access to
		// elements of the XML attribute map, as well as per-index assignment (setter) and
		// removal operations?  This would ensure that revisioning is correctly handled...
		std::map<XmlAttributeName, XmlAttributeValue> &
		xml_attributes()
		{
			return d_xml_attributes;
		}

		/**
		 * Accept a ConstFeatureVisitor instance.
		 *
		 * See the Visitor pattern (p.331) in Gamma95 for information on the purpose of
		 * this function.
		 */
		virtual
		void
		accept_visitor(
				ConstFeatureVisitor &visitor) const = 0;

		/**
		 * Accept a FeatureVisitor instance.
		 *
		 * See the Visitor pattern (p.331) in Gamma95 for information on the purpose of
		 * this function.
		 */
		virtual
		void
		accept_visitor(
				FeatureVisitor &visitor) = 0;

		/**
		 * Increment the reference-count of this instance.
		 *
		 * This function is used by boost::intrusive_ptr and
		 * GPlatesUtils::non_null_intrusive_ptr.
		 */
		void
		increment_ref_count() const
		{
			++d_ref_count;
		}

		/**
		 * Decrement the reference-count of this instance, and return the new
		 * reference-count.
		 *
		 * This function is used by boost::intrusive_ptr and
		 * GPlatesUtils::non_null_intrusive_ptr.
		 */
		ref_count_type
		decrement_ref_count() const
		{
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
			const PropertyContainer *p)
	{
		p->increment_ref_count();
	}


	inline
	void
	intrusive_ptr_release(
			const PropertyContainer *p)
	{
		if (p->decrement_ref_count() == 0) {
			delete p;
		}
	}

}

#endif  // GPLATES_MODEL_PROPERTYCONTAINER_H
