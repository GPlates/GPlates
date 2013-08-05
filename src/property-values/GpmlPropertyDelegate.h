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

#ifndef GPLATES_PROPERTYVALUES_GPMLPROPERTYDELEGATE_H
#define GPLATES_PROPERTYVALUES_GPMLPROPERTYDELEGATE_H

#include "StructuralType.h"

#include "feature-visitors/PropertyValueFinder.h"
#include "model/PropertyValue.h"
#include "model/FeatureId.h"
#include "model/PropertyName.h"
#include "utils/UnicodeStringUtils.h"


// Enable GPlatesFeatureVisitors::get_property_value() to work with this property value.
// First parameter is the namespace qualified property value class.
// Second parameter is the name of the feature visitor method that visits the property value.
DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GpmlPropertyDelegate, visit_gpml_property_delegate)

namespace GPlatesPropertyValues
{

	class GpmlPropertyDelegate:
			public GPlatesModel::PropertyValue
	{

	public:

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<GpmlPropertyDelegate>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<GpmlPropertyDelegate> non_null_ptr_type;

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<const GpmlPropertyDelegate>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GpmlPropertyDelegate> non_null_ptr_to_const_type;


		virtual
		~GpmlPropertyDelegate()
		{  }

		static
		const non_null_ptr_type
		create(
				const GPlatesModel::FeatureId &feature_,
				const GPlatesModel::PropertyName &property_name_,
				const StructuralType &value_type_)
		{
			return non_null_ptr_type(new GpmlPropertyDelegate(feature_, property_name_, value_type_));
		}

		const non_null_ptr_type
		clone() const
		{
			return GPlatesUtils::dynamic_pointer_cast<GpmlPropertyDelegate>(clone_impl());
		}

		const GPlatesModel::FeatureId &
		get_feature_id() const
		{
			return d_feature;
		}

		const GPlatesModel::PropertyName &
		get_target_property_name() const
		{
			return d_property_name;
		}

		// Note that no "setter" is provided:  The value type of a GpmlPropertyDelegate
		// instance should never be changed.
		const StructuralType &
		get_value_type() const
		{
			return d_value_type;
		}

		/**
		 * Returns the structural type associated with this property value class.
		 */
		virtual
		StructuralType
		get_structural_type() const
		{
			static const StructuralType STRUCTURAL_TYPE = StructuralType::create_gpml("PropertyDelegate");
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
			visitor.visit_gpml_property_delegate(*this);
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
			visitor.visit_gpml_property_delegate(*this);
		}

		virtual
		std::ostream &
		print_to(
				std::ostream &os) const;

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		GpmlPropertyDelegate(
				const GPlatesModel::FeatureId &feature_,
				const GPlatesModel::PropertyName &property_name_,
				const StructuralType &value_type_):
			// We don't actually need revisioning so just create an empty base class revision...
			PropertyValue(GPlatesModel::PropertyValue::Revision::non_null_ptr_type(new Revision())),
			d_feature(feature_),
			d_property_name(property_name_),
			d_value_type(value_type_)
		{  }

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		//
		// Note that this should act exactly the same as the default (auto-generated)
		// copy-constructor, except it should not be public.
		GpmlPropertyDelegate(
				const GpmlPropertyDelegate &other) :
			PropertyValue(other),
			d_feature(other.d_feature),
			d_property_name(other.d_property_name),
			d_value_type(other.d_value_type)
		{  }

		virtual
		const GPlatesModel::PropertyValue::non_null_ptr_type
		clone_impl() const
		{
			return non_null_ptr_type(new GpmlPropertyDelegate(*this));
		}

		virtual
		bool
		equality(
				const PropertyValue &other) const
		{
			const GpmlPropertyDelegate &other_pv = dynamic_cast<const GpmlPropertyDelegate &>(other);

			return d_feature == other_pv.d_feature &&
					d_property_name == other_pv.d_property_name &&
					d_value_type == other_pv.d_value_type &&
					GPlatesModel::PropertyValue::equality(other);
		}

	private:

		GPlatesModel::FeatureId d_feature;
		GPlatesModel::PropertyName d_property_name;
		StructuralType d_value_type;

	};

}

#endif  // GPLATES_PROPERTYVALUES_GPMLPROPERTYDELEGATE_H
