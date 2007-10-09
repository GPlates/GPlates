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

#ifndef GPLATES_PROPERTYVALUES_GMLORIENTABLECURVE_H
#define GPLATES_PROPERTYVALUES_GMLORIENTABLECURVE_H

#include <map>
#include "model/PropertyValue.h"
#include "model/XmlAttributeName.h"
#include "model/XmlAttributeValue.h"


namespace GPlatesPropertyValues {

	/**
	 * This class implements the PropertyValue which corresponds to "gml:OrientableCurve".
	 */
	class GmlOrientableCurve :
			public GPlatesModel::PropertyValue {

	public:

		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<GmlOrientableCurve>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<GmlOrientableCurve>
				non_null_ptr_type;

		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<const GmlOrientableCurve>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GmlOrientableCurve>
				non_null_ptr_to_const_type;

		virtual
		~GmlOrientableCurve() {  }

		/**
		 * Create a GmlOrientableCurve instance which contains a "base curve".
		 *
		 * The "base curve" is passed in the parameter @a base_curve_, which is meant to
		 * refer to an object which is substitutable for "gml:_Curve".  (This won't however
		 * be verified at construction-time.)
		 *
		 * This instance will also contain the XML attributes in @a xml_attributes_.
		 */
		// This creation function is here purely for the simple, hard-coded construction of
		// features.  It may not be necessary or appropriate later on when we're doing
		// everything properly, so don't look at this function and think "Uh oh, this
		// function doesn't look like it should be here, but I'm sure it's here for a
		// reason..."
		static
		const non_null_ptr_type
		create(
				GPlatesModel::PropertyValue::non_null_ptr_type base_curve_,
				const std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> &
						xml_attributes_) {
			non_null_ptr_type ptr(
					*(new GmlOrientableCurve(base_curve_, xml_attributes_)));
			return ptr;
		}

		/**
		 * Create a duplicate of this PropertyValue instance.
		 */
		virtual
		const GPlatesModel::PropertyValue::non_null_ptr_type
		clone() const {
			GPlatesModel::PropertyValue::non_null_ptr_type dup(
					*(new GmlOrientableCurve(*this)));
			return dup;
		}

		/**
		 * Access the PropertyValue which is the "base curve" of this instance.
		 *
		 * This is the overloading of this function for const GmlOrientableCurve instances;
		 * it returns a pointer to a const PropertyValue instance.
		 */
		const GPlatesModel::PropertyValue::non_null_ptr_to_const_type
		base_curve() const {
			return d_base_curve;
		}

		/**
		 * Access the PropertyValue which is the "base curve" of this instance.
		 *
		 * This is the overloading of this function for non-const GmlOrientableCurve
		 * instances; it returns a pointer to a non-const PropertyValue instance.
		 *
		 * Note that, because the copy-assignment operator of PropertyValue is private,
		 * the PropertyValue referenced by the return-value of this function cannot be
		 * assigned-to, which means that this function does not provide a means to directly
		 * switch the PropertyValue within this GmlOrientableCurve instance.  (This
		 * restriction is intentional.)
		 *
		 * To switch the PropertyValue within this GmlOrientableCurve instance, use the
		 * function @a set_base_curve below.
		 *
		 * (This overload is provided to allow the referenced PropertyValue instance to
		 * accept a FeatureVisitor instance.)
		 */
		const GPlatesModel::PropertyValue::non_null_ptr_type
		base_curve() {
			return d_base_curve;
		}

		/**
		 * Set the "base curve" of this instance to @a bc.
		 */
		void
		set_base_curve(
				GPlatesModel::PropertyValue::non_null_ptr_type bc) {
			d_base_curve = bc;
		}

		/**
		 * Return the map of XML attributes contained by this instance.
		 *
		 * This is the overloading of this function for const GmlOrientableCurve instances;
		 * it returns a reference to a const map, which in turn will only allow const
		 * access to its elements.
		 *
		 * @b FIXME:  Should this function be replaced with per-index const-access to
		 * elements of the XML attribute map?  (For consistency with the non-const
		 * overload...)
		 */
		const std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> &
		xml_attributes() const {
			return d_xml_attributes;
		}

		/**
		 * Return the map of XML attributes contained by this instance.
		 *
		 * This is the overloading of this function for non-const GmlOrientableCurve
		 * instances; it returns a reference to a non-const map, which in turn will allow
		 * non-const access to its elements.
		 *
		 * @b FIXME:  Should this function be replaced with per-index const-access to
		 * elements of the XML attribute map, as well as per-index assignment (setter) and
		 * removal operations?  This would ensure that revisioning is correctly handled...
		 */
		std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> &
		xml_attributes() {
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
				GPlatesModel::ConstFeatureVisitor &visitor) const {
			visitor.visit_gml_orientable_curve(*this);
		}

		/**
		 * Accept a FeatureVisitor instance.
		 *
		 * See the Visitor pattern (p.331) in Gamma95 for information on the purpose of
		 * this function.
		 */
		virtual
		void
		accept_visitor(
				GPlatesModel::FeatureVisitor &visitor) {
			visitor.visit_gml_orientable_curve(*this);
		}

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		GmlOrientableCurve(
				GPlatesModel::PropertyValue::non_null_ptr_type base_curve_,
				const std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> &
						xml_attributes_):
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

		GPlatesModel::PropertyValue::non_null_ptr_type d_base_curve;
		std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> d_xml_attributes;

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		GmlOrientableCurve &
		operator=(const GmlOrientableCurve &);

	};

}

#endif  // GPLATES_PROPERTYVALUES_GMLORIENTABLECURVE_H
