/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, 2009, 2010 The University of Sydney, Australia
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

#ifndef GPLATES_PROPERTYVALUES_GPMLMEASURE_H
#define GPLATES_PROPERTYVALUES_GPMLMEASURE_H

#include <map>

#include "feature-visitors/PropertyValueFinder.h"

#include "model/PropertyValue.h"
#include "model/XmlAttributeName.h"
#include "model/XmlAttributeValue.h"


// Enable GPlatesFeatureVisitors::get_property_value() to work with this property value.
// First parameter is the namespace qualified property value class.
// Second parameter is the name of the feature visitor method that visits the property value.
DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GpmlMeasure, visit_gpml_measure)

namespace GPlatesPropertyValues
{

	class GpmlMeasure :
			public GPlatesModel::PropertyValue
	{

	public:

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<GpmlMeasure>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<GpmlMeasure> non_null_ptr_type;

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<const GpmlMeasure>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GpmlMeasure> non_null_ptr_to_const_type;


		virtual
		~GpmlMeasure()
		{  }

		static
		const non_null_ptr_type
		create(
				const double &quantity,
				const std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> &
						quantity_xml_attributes_)
		{
			return non_null_ptr_type(new GpmlMeasure(quantity, quantity_xml_attributes_));
		}

		const non_null_ptr_type
		clone() const
		{
			return GPlatesUtils::dynamic_pointer_cast<GpmlMeasure>(clone_impl());
		}

		/**
		 * Access the quantity contained in this GpmlMeasure.
		 *
		 * Note that this accessor does not allow you to modify the
		 * double contained in this GpmlMeasure - to do that,
		 * set a new double value using the method @a set_quantity
		 * below.
		 */
		const double &
		get_quantity() const
		{
			return get_current_revision<Revision>().quantity;
		}

		/**
		 * Set the quantity of this GpmlMeasure to @a q.
		 */
		void
		set_quantity(
				const double &q);

		// @b FIXME:  Should this function be replaced with per-index const-access to
		// elements of the XML attribute map?  (For consistency with the non-const
		// overload...)
		const std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> &
		get_quantity_xml_attributes() const
		{
			return get_current_revision<Revision>().quantity_xml_attributes;
		}

		// @b FIXME:  Should this function be replaced with per-index const-access to
		// elements of the XML attribute map, as well as per-index assignment (setter) and
		// removal operations?  This would ensure that revisioning is correctly handled...
		void
		set_quantity_xml_attributes(
				const std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> &qxa);

		/**
		 * Returns the structural type associated with this property value class.
		 */
		virtual
		StructuralType
		get_structural_type() const
		{
			static const StructuralType STRUCTURAL_TYPE = StructuralType::create_gpml("measure");
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
			visitor.visit_gpml_measure(*this);
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
			visitor.visit_gpml_measure(*this);
		}

		virtual
		std::ostream &
		print_to(
				std::ostream &os) const;

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		GpmlMeasure(
				const double &quantity_,
				const std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> &
						quantity_xml_attributes_):
			PropertyValue(Revision::non_null_ptr_type(new Revision(quantity_, quantity_xml_attributes_)))
		{  }

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		//
		// Note that this should act exactly the same as the default (auto-generated)
		// copy-constructor, except it should not be public.
		GpmlMeasure(
				const GpmlMeasure &other) :
			PropertyValue(other)
		{  }

		virtual
		const GPlatesModel::PropertyValue::non_null_ptr_type
		clone_impl() const
		{
			return non_null_ptr_type(new GpmlMeasure(*this));
		}

	private:

		/**
		 * Property value data that is mutable/revisionable.
		 */
		struct Revision :
				public GPlatesModel::PropertyValue::Revision
		{
			Revision(
					const double &quantity_,
					const std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue>
						quantity_xml_attributes_) :
				quantity(quantity_),
				quantity_xml_attributes(quantity_xml_attributes_)
			{  }

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
				const Revision &other_revision = dynamic_cast<const Revision &>(other);

				return quantity == other_revision.quantity &&
						quantity_xml_attributes == other_revision.quantity_xml_attributes &&
						GPlatesModel::PropertyValue::Revision::equality(other);
			}

			double quantity;
			std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> quantity_xml_attributes;
		};

	};

}

#endif  // GPLATES_PROPERTYVALUES_GPMLMEASURE_H
