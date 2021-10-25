/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007, 2008, 2009, 2010 The University of Sydney, Australia
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

#include "GmlLineString.h"

#include "feature-visitors/PropertyValueFinder.h"

#include "model/PropertyValue.h"
#include "model/XmlAttributeName.h"
#include "model/XmlAttributeValue.h"


// Enable GPlatesFeatureVisitors::get_property_value() to work with this property value.
// First parameter is the namespace qualified property value class.
// Second parameter is the name of the feature visitor method that visits the property value.
DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GmlOrientableCurve, visit_gml_orientable_curve)

namespace GPlatesPropertyValues
{

	/**
	 * This class implements the PropertyValue which corresponds to "gml:OrientableCurve".
	 */
	class GmlOrientableCurve:
			public GPlatesModel::PropertyValue
	{

	public:

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<GmlOrientableCurve>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<GmlOrientableCurve> non_null_ptr_type;

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<const GmlOrientableCurve>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GmlOrientableCurve> non_null_ptr_to_const_type;


		virtual
		~GmlOrientableCurve()
		{  }

		/**
		 * Create a GmlOrientableCurve instance which contains a "base curve".
		 *
		 * The "base curve" is passed in the parameter @a base_curve_, which is meant to
		 * refer to an object which is substitutable for "gml:_Curve".  (This won't however
		 * be verified at construction-time.)
		 *
		 * This instance will also contain the XML attributes in @a xml_attributes_.
		 */
		static
		const non_null_ptr_type
		create(
				GmlLineString::non_null_ptr_type base_curve_,
				const std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> &
						xml_attributes_)
		{
			return non_null_ptr_type(
					new GmlOrientableCurve(base_curve_, xml_attributes_));
		}

		const non_null_ptr_type
		clone() const
		{
			return non_null_ptr_type(new GmlOrientableCurve(*this));
		}

		const non_null_ptr_type
		deep_clone() const;

		DEFINE_FUNCTION_DEEP_CLONE_AS_PROP_VAL()

		/**
		 * Access the 'const' PropertyValue which is the "base curve" of this instance.
		 */
		const GmlLineString::non_null_ptr_to_const_type
		base_curve() const
		{
			return d_base_curve;
		}

		/**
		 * Access the 'non-const' PropertyValue which is the "base curve" of this instance.
		 */
		const GmlLineString::non_null_ptr_type
		base_curve()
		{
			return d_base_curve;
		}

		/**
		 * Set the "base curve" of this instance to @a bc.
		 */
		void
		set_base_curve(
				GmlLineString::non_null_ptr_type bc)
		{
			d_base_curve = bc;
			update_instance_id();
		}

		/**
		 * Return the map of XML attributes contained by this instance.
		 *
		 * This is the overloading of this function for const GmlOrientableCurve instances;
		 * it returns a reference to a const map, which in turn will only allow const
		 * access to its elements.
		 */
		const std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> &
		xml_attributes() const
		{
			return d_xml_attributes;
		}

		/**
		 * Set the map of XML attributes contained by this instance.
		 */
		std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> &
		xml_attributes()
		{
			return d_xml_attributes;
		}

		/**
		 * Returns the structural type associated with this property value class.
		 */
		virtual
		StructuralType
		get_structural_type() const
		{
			static const StructuralType STRUCTURAL_TYPE = StructuralType::create_gml("OrientableCurve");
			return STRUCTURAL_TYPE;
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
				GPlatesModel::ConstFeatureVisitor &visitor) const
		{
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
				GPlatesModel::FeatureVisitor &visitor)
		{
			visitor.visit_gml_orientable_curve(*this);
		}

		virtual
		std::ostream &
		print_to(
				std::ostream &os) const;

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		GmlOrientableCurve(
				GmlLineString::non_null_ptr_type base_curve_,
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
			PropertyValue(other), /* share instance id */
			d_base_curve(other.d_base_curve),
			d_xml_attributes(other.d_xml_attributes)
		{  }

		virtual
		bool
		directly_modifiable_fields_equal(
				const GPlatesModel::PropertyValue &other) const;

	private:

		GmlLineString::non_null_ptr_type d_base_curve;
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
