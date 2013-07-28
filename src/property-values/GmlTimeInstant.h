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


		virtual
		~GmlTimeInstant()
		{  }

		static
		const non_null_ptr_type
		create(
				const GeoTimeInstant &time_position_,
				const std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> &
						time_position_xml_attributes_)
		{
			return non_null_ptr_type(new GmlTimeInstant(time_position_, time_position_xml_attributes_));
		}

		const non_null_ptr_type
		clone() const
		{
			return GPlatesUtils::dynamic_pointer_cast<GmlTimeInstant>(clone_impl());
		}

		/**
		 * Access the GeoTimeInstant which encodes the temporal position of this GmlTimeInstant.
		 *
		 * Note that there is no accessor provided which returns a non-const
		 * GeoTimeInstant. This is intentional. To modify this GmlTimeInstant,
		 * set a new GeoTimeInstant using the method @a set_time_position()
		 */
		const GeoTimeInstant &
		get_time_position() const
		{
			return get_current_revision<Revision>().time_position;
		}

		/**
		 * Set the temporal position of this GmlTimeInstant to @a tp.
		 */
		void
		set_time_position(
				const GeoTimeInstant &tp);

		// @b FIXME:  Should this function be replaced with per-index const-access to
		// elements of the XML attribute map?  (For consistency with the non-const
		// overload...)
		const std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> &
		get_time_position_xml_attributes() const
		{
			return get_current_revision<Revision>().time_position_xml_attributes;
		}

		// @b FIXME:  Should this function be replaced with per-index const-access to
		// elements of the XML attribute map, as well as per-index assignment (setter) and
		// removal operations?  This would ensure that revisioning is correctly handled...
		void
		set_time_position_xml_attributes(
				const std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> &tpxa);

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
				const std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> &
						time_position_xml_attributes_) :
			PropertyValue(Revision::non_null_ptr_type(new Revision(time_position_, time_position_xml_attributes_)))
		{  }

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		//
		// Note that this should act exactly the same as the default (auto-generated)
		// copy-constructor, except it should not be public.
		GmlTimeInstant(
				const GmlTimeInstant &other) :
			PropertyValue(other)
		{  }

		virtual
		const GPlatesModel::PropertyValue::non_null_ptr_type
		clone_impl() const
		{
			return non_null_ptr_type(new GmlTimeInstant(*this));
		}

	private:

		/**
		 * Property value data that is mutable/revisionable.
		 */
		struct Revision :
				public GPlatesModel::PropertyValue::Revision
		{
			Revision(
					const GeoTimeInstant &time_position_,
					const std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> &
							time_position_xml_attributes_) :
				time_position(time_position_),
				time_position_xml_attributes(time_position_xml_attributes_)
			{  }

			Revision(
					const Revision &other);

			virtual
			GPlatesModel::PropertyValue::Revision::non_null_ptr_type
			clone() const
			{
				return non_null_ptr_type(new Revision(*this));
			}

			virtual
			bool
			equality(
					const GPlatesModel::PropertyValue::Revision &other) const
			{
				const Revision &other_revision = static_cast<const Revision &>(other);

				return time_position == other_revision.time_position &&
						time_position_xml_attributes == other_revision.time_position_xml_attributes &&
						GPlatesModel::PropertyValue::Revision::equality(other);
			}

			GeoTimeInstant time_position;
			std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue>
					time_position_xml_attributes;
		};


		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		GmlTimeInstant &
		operator=(const GmlTimeInstant &);

	};

}

#endif  // GPLATES_PROPERTYVALUES_GMLTIMEINSTANT_H
