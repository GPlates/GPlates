/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

#ifndef GPLATES_PROPERTY_VALUES_OLDVERSIONPROPERTYVALUE_H
#define GPLATES_PROPERTY_VALUES_OLDVERSIONPROPERTYVALUE_H

#include <boost/any.hpp>

#include "feature-visitors/PropertyValueFinder.h"

#include "model/PropertyValue.h"

#include "utils/UnicodeStringUtils.h"


// Enable GPlatesFeatureVisitors::get_property_value() to work with this property value.
// First parameter is the namespace qualified property value class.
// Second parameter is the name of the feature visitor method that visits the property value.
DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::OldVersionPropertyValue, visit_old_version_property_value)

namespace GPlatesPropertyValues
{
	/**
	 * This class implements an old version PropertyValue.
	 *
	 * This property value can be used to read old version GPML files that contain property values
	 * that have been deprecated. This provides an opportunity, during import, to then convert
	 * to a one, or more, existing property values. To assist with this the property value contains
	 * arbitrary user-defined data (used during import) in the form of boost::any which reflects
	 * the 'value' of the property.
	 *
	 * This is similar to GpmlUninterpretedPropertyValue except that, instead of retaining the
	 * 'uninterpreted' XML element node, it stores a client-specific interpretation of the
	 * old version property value read from an XML (GPML) file.
	 */
	class OldVersionPropertyValue :
			public GPlatesModel::PropertyValue
	{

	public:

		//! A convenience typedef for a shared pointer to a non-const @a OldVersionPropertyValue.
		typedef GPlatesUtils::non_null_intrusive_ptr<OldVersionPropertyValue> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a OldVersionPropertyValue.
		typedef GPlatesUtils::non_null_intrusive_ptr<const OldVersionPropertyValue> non_null_ptr_to_const_type;

		//! Typedef for the user-defined arbitrary property 'value'.
		typedef boost::any value_type;


		virtual
		~OldVersionPropertyValue()
		{  }


		static
		const non_null_ptr_type
		create(
				const StructuralType &structural_type,
				const value_type &value_)
		{
			return non_null_ptr_type(new OldVersionPropertyValue(structural_type, value_));
		}

		const non_null_ptr_type
		clone() const
		{
			return non_null_ptr_type(new OldVersionPropertyValue(*this));
		}

		const non_null_ptr_type
		deep_clone() const
		{
			// This class doesn't reference any mutable objects by pointer, so there's
			// no need for any recursive cloning.  Hence, regular clone will suffice.
			return clone();
		}

		DEFINE_FUNCTION_DEEP_CLONE_AS_PROP_VAL()


		/**
		 * Returns the value.
		 *
		 * Note: Since there are no setters methods on this class we don't need revisioning.
		 */
		const value_type &
		value() const
		{
			return d_value;
		}

		/**
		 * Returns the structural type associated with this property value class.
		 */
		virtual
		StructuralType
		get_structural_type() const
		{
			return d_structural_type;
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
			visitor.visit_old_version_property_value(*this);
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
			visitor.visit_old_version_property_value(*this);
		}

		virtual
		std::ostream &
		print_to(
				std::ostream &os) const;

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		explicit
		OldVersionPropertyValue(
				const StructuralType &structural_type,
				const value_type &value_) :
			PropertyValue(),
			d_structural_type(structural_type),
			d_value(value_)
		{  }

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		//
		// Note that this should act exactly the same as the default (auto-generated)
		// copy-constructor, except it should not be public.
		OldVersionPropertyValue(
				const OldVersionPropertyValue &other) :
			PropertyValue(other), /* share instance id */
			d_structural_type(other.d_structural_type),
			d_value(other.d_value)
		{  }

	private:

		//! The structural type of the old property value type.
		StructuralType d_structural_type;

		//! The arbitrary user-defined property 'value'.
		value_type d_value;


		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		OldVersionPropertyValue &
		operator=(
				const OldVersionPropertyValue &);

	};

}

#endif // GPLATES_PROPERTY_VALUES_OLDVERSIONPROPERTYVALUE_H
