/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007, 2009, 2010 The University of Sydney, Australia
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

#ifndef GPLATES_PROPERTYVALUES_GMLTIMEINSTANT_H
#define GPLATES_PROPERTYVALUES_GMLTIMEINSTANT_H

#include <map>

#include "GeoTimeInstant.h"

#include "feature-visitors/PropertyValueFinder.h"
#include "model/PropertyValue.h"
#include "model/XmlAttributeName.h"
#include "model/XmlAttributeValue.h"


// Enable GPlatesFeatureVisitors::get_property_value() to work with this property value.
// First parameter is the namespace qualified property value class.
// Second parameter is the name of the feature visitor method that visits the property value.
DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GmlTimeInstant, visit_gml_time_instant)

namespace GPlatesPropertyValues
{

	class GmlTimeInstant :
			public GPlatesModel::PropertyValue
	{

	public:

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<GmlTimeInstant>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<GmlTimeInstant> non_null_ptr_type;

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<const GmlTimeInstant>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GmlTimeInstant> non_null_ptr_to_const_type;


		//! Typedef for an XML attribute (name/value) map.
		typedef std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> xml_attribute_map_type;


		virtual
		~GmlTimeInstant()
		{  }

		static
		const non_null_ptr_type
		create(
				const GeoTimeInstant &time_position_,
				const xml_attribute_map_type &time_position_xml_attributes_ = xml_attribute_map_type())
		{
			return non_null_ptr_type(new GmlTimeInstant(time_position_, time_position_xml_attributes_));
		}

		const non_null_ptr_type
		clone() const
		{
			return non_null_ptr_type(new GmlTimeInstant(*this));
		}

		const GmlTimeInstant::non_null_ptr_type
		deep_clone() const
		{
			// This class doesn't reference any mutable objects by pointer, so there's
			// no need for any recursive cloning.  Hence, regular clone will suffice.
			return clone();
		}

		DEFINE_FUNCTION_DEEP_CLONE_AS_PROP_VAL()

		/**
		 * Access the GeoTimeInstant which encodes the temporal position of this GmlTimeInstant.
		 *
		 * Note that there is no accessor provided which returns a non-const
		 * GeoTimeInstant. This is intentional. To modify this GmlTimeInstant,
		 * set a new GeoTimeInstant using the method @a set_time_position()
		 */
		const GeoTimeInstant &
		time_position() const
		{
			return d_time_position;
		}

		/**
		 * Set the temporal position of this GmlTimeInstant to @a tp.
		 */
		void
		set_time_position(
				const GeoTimeInstant &tp)
		{
			d_time_position = tp;
			update_instance_id();
		}

		// @b FIXME:  Should this function be replaced with per-index const-access to
		// elements of the XML attribute map?  (For consistency with the non-const
		// overload...)
		const xml_attribute_map_type &
		time_position_xml_attributes() const
		{
			return d_time_position_xml_attributes;
		}

		// @b FIXME:  Should this function be replaced with per-index const-access to
		// elements of the XML attribute map, as well as per-index assignment (setter) and
		// removal operations?  This would ensure that revisioning is correctly handled...
		xml_attribute_map_type &
		time_position_xml_attributes()
		{
			return d_time_position_xml_attributes;
		}

		/**
		 * Returns the structural type associated with this property value class.
		 */
		virtual
		StructuralType
		get_structural_type() const
		{
			static const StructuralType STRUCTURAL_TYPE = StructuralType::create_gml("TimeInstant");
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
			visitor.visit_gml_time_instant(*this);
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
			visitor.visit_gml_time_instant(*this);
		}

		virtual
		std::ostream &
		print_to(
				std::ostream &os) const;

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		GmlTimeInstant(
				const GeoTimeInstant &time_position_,
				const xml_attribute_map_type &
						time_position_xml_attributes_);

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		//
		// Note that this should act exactly the same as the default (auto-generated)
		// copy-constructor, except it should not be public.
		GmlTimeInstant(
				const GmlTimeInstant &other);

		virtual
		bool
		directly_modifiable_fields_equal(
				const PropertyValue &other) const;

	private:

		GeoTimeInstant d_time_position;
		xml_attribute_map_type d_time_position_xml_attributes;

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		GmlTimeInstant &
		operator=(const GmlTimeInstant &);

	};

}

#endif  // GPLATES_PROPERTYVALUES_GMLTIMEINSTANT_H
