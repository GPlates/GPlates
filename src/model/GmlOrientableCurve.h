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

#ifndef GPLATES_MODEL_GMLORIENTABLECURVE_H
#define GPLATES_MODEL_GMLORIENTABLECURVE_H

#include <map>
#include "PropertyValue.h"
#include "XmlAttributeName.h"
#include "XmlAttributeValue.h"


namespace GPlatesModel {

	class GmlOrientableCurve :
			public PropertyValue {

	public:

		virtual
		~GmlOrientableCurve() {  }

		// This creation function is here purely for the simple, hard-coded construction of
		// features.  It may not be necessary or appropriate later on when we're doing
		// everything properly, so don't look at this function and think "Uh oh, this
		// function doesn't look like it should be here, but I'm sure it's here for a
		// reason..."
		static
		boost::intrusive_ptr<GmlOrientableCurve>
		create(
				boost::intrusive_ptr<PropertyValue> base_curve_,
				const std::map<XmlAttributeName, XmlAttributeValue> &xml_attributes_) {
			boost::intrusive_ptr<GmlOrientableCurve> ptr(new GmlOrientableCurve(base_curve_, xml_attributes_));
			return ptr;
		}

		virtual
		boost::intrusive_ptr<PropertyValue>
		clone() const {
			boost::intrusive_ptr<PropertyValue> dup(new GmlOrientableCurve(*this));
			return dup;
		}

		boost::intrusive_ptr<const PropertyValue>
		base_curve() const {
			return d_base_curve;
		}

		boost::intrusive_ptr<PropertyValue>
		base_curve() {
			return d_base_curve;
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
				ConstFeatureVisitor &visitor) const {
			visitor.visit_gml_orientable_curve(*this);
		}

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		GmlOrientableCurve(
				boost::intrusive_ptr<PropertyValue> base_curve_,
				const std::map<XmlAttributeName, XmlAttributeValue> &xml_attributes_):
			PropertyValue(),
			d_base_curve(base_curve_),
			d_xml_attributes(xml_attributes_)
		{  }

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		//
		// Note that this should act exactly the same as the default (auto-generated)
		// copy-constructor, except it should not be public.
		GmlOrientableCurve(
				const GmlOrientableCurve &other) :
			PropertyValue(),
			d_base_curve(other.d_base_curve),
			d_xml_attributes(d_xml_attributes)
		{  }

	private:

		// FIXME:  Is it valid for this pointer to be NULL?  I don't think so...
		boost::intrusive_ptr<PropertyValue> d_base_curve;
		std::map<XmlAttributeName, XmlAttributeValue> d_xml_attributes;

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		GmlOrientableCurve &
		operator=(const GmlOrientableCurve &);

	};

}

#endif  // GPLATES_MODEL_GMLORIENTABLECURVE_H
