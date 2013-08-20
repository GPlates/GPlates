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
			return GPlatesUtils::dynamic_pointer_cast<OldVersionPropertyValue>(clone_impl());
		}


		/**
		 * Returns the value.
		 *
		 * Note: Since there are no setters methods on this class we don't need revisioning.
		 */
		const value_type &
		get_value() const
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
			PropertyValue(Revision::non_null_ptr_type(new Revision())),
			d_structural_type(structural_type),
			d_value(value_)
		{  }

		//! Constructor used when cloning.
		OldVersionPropertyValue(
				const OldVersionPropertyValue &other_,
				boost::optional<GPlatesModel::PropertyValueRevisionContext &> context_) :
			PropertyValue(
					Revision::non_null_ptr_type(
							new Revision(other_.get_current_revision<Revision>(), context_))),
			d_structural_type(other_.d_structural_type),
			d_value(other_.d_value)
		{  }

		virtual
		const PropertyValue::non_null_ptr_type
		clone_impl(
				boost::optional<GPlatesModel::PropertyValueRevisionContext &> context = boost::none) const
		{
			return non_null_ptr_type(new OldVersionPropertyValue(*this, context));
		}

		virtual
		bool
		equality(
				const PropertyValue &other) const
		{
			// TODO: Compare values, but this requires knowing the value type.
			return false;
		}

	private:

		/**
		 * Property value data that is mutable/revisionable.
		 */
		struct Revision :
				public GPlatesModel::PropertyValueRevision
		{
			Revision()
			{  }

			//! Clone constructor.
			Revision(
					const Revision &other_,
					boost::optional<GPlatesModel::PropertyValueRevisionContext &> context_) :
				PropertyValueRevision(context_)
			{  }

			virtual
			PropertyValueRevision::non_null_ptr_type
			clone_revision(
					boost::optional<GPlatesModel::PropertyValueRevisionContext &> context) const
			{
				return non_null_ptr_type(new Revision(*this, context));
			}
		};

		//! The structural type of the old property value type.
		StructuralType d_structural_type;

		//! The arbitrary user-defined property 'value'.
		value_type d_value;

	};

}

#endif // GPLATES_PROPERTY_VALUES_OLDVERSIONPROPERTYVALUE_H
