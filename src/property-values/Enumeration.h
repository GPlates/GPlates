/* $Id$ */

/**
 * \file 
 * Contains the definition of the class Enumeration.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2007, 2008, 2009, 2010 The University of Sydney, Australia
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

#ifndef GPLATES_PROPERTYVALUES_ENUMERATION_H
#define GPLATES_PROPERTYVALUES_ENUMERATION_H

#include "EnumerationContent.h"
#include "EnumerationType.h"

#include "feature-visitors/PropertyValueFinder.h"

#include "model/PropertyValue.h"

#include "utils/UnicodeStringUtils.h"


// Enable GPlatesFeatureVisitors::get_property_value() to work with this property value.
// First parameter is the namespace qualified property value class.
// Second parameter is the name of the feature visitor method that visits the property value.
DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::Enumeration, visit_enumeration)

namespace GPlatesPropertyValues
{

	class Enumeration :
			public GPlatesModel::PropertyValue
	{

	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<Enumeration> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const Enumeration> non_null_ptr_to_const_type;


		virtual
		~Enumeration()
		{  }

		static
		const non_null_ptr_type
		create(
				const EnumerationType &enum_type,
				const EnumerationContent &enum_content)
		{
			return non_null_ptr_type(new Enumeration(enum_type, enum_content));
		}

		static
		const non_null_ptr_type
		create(
				const EnumerationType &enum_type,
				const GPlatesUtils::UnicodeString &enum_content)
		{
			return non_null_ptr_type(new Enumeration(enum_type, EnumerationContent(enum_content)));
		}

		const non_null_ptr_type
		clone() const
		{
			return GPlatesUtils::dynamic_pointer_cast<Enumeration>(clone_impl());
		}

		const EnumerationContent &
		get_value() const
		{
			return get_current_revision<Revision>().value;
		}
		
		/**
		 * Set the content of this enumeration to @a new_value.
		 * EnumerationContent can be created by passing a UnicodeString in to
		 * EnumerationContent's constructor.
		 */
		void
		set_value(
				const EnumerationContent &new_value);

		// Note that no "setter" is provided:  The type of an Enumeration
		// instance should never be changed.
		const EnumerationType &
		get_type() const
		{
			return d_type;
		}

		/**
		 * Returns the structural type associated with this property value class.
		 */
		virtual
		StructuralType
		get_structural_type() const
		{
			return StructuralType(d_type);
		}

		/**
		 * NOTE: There is no static access to the structural type (eg, as Enumeration::STRUCTURAL_TYPE)
		 * because it depends on the enumeration type which is non-static data.
		 */
		//static const StructuralType STRUCTURAL_TYPE;


		virtual
		void
		accept_visitor(
				GPlatesModel::ConstFeatureVisitor &visitor) const
		{
			visitor.visit_enumeration(*this);
		}

		virtual
		void
		accept_visitor(
				GPlatesModel::FeatureVisitor &visitor)
		{
			visitor.visit_enumeration(*this);
		}

		virtual
		std::ostream &
		print_to(
				std::ostream &os) const;

	protected:

		explicit
		Enumeration(
				const EnumerationType &enum_type,
				const EnumerationContent &enum_content) :
			PropertyValue(Revision::non_null_ptr_type(new Revision(enum_content))),
			d_type(enum_type)
		{  }

		//! Constructor used when cloning.
		Enumeration(
				const Enumeration &other_,
				boost::optional<GPlatesModel::RevisionContext &> context_) :
			PropertyValue(
					Revision::non_null_ptr_type(
							new Revision(other_.get_current_revision<Revision>(), context_))),
			d_type(other_.d_type)
		{  }

		virtual
		const Revisionable::non_null_ptr_type
		clone_impl(
				boost::optional<GPlatesModel::RevisionContext &> context = boost::none) const
		{
			return non_null_ptr_type(new Enumeration(*this, context));
		}

		virtual
		bool
		equality(
				const Revisionable &other) const
		{
			const Enumeration &other_pv = dynamic_cast<const Enumeration &>(other);

			return d_type == other_pv.d_type &&
					// The revisioned data comparisons are handled here...
					Revisionable::equality(other);
		}

	private:

		/**
		 * Property value data that is mutable/revisionable.
		 */
		struct Revision :
				public PropertyValue::Revision
		{
			explicit
			Revision(
					const EnumerationContent &value_) :
				value(value_)
			{  }

			//! Clone constructor.
			Revision(
					const Revision &other_,
					boost::optional<GPlatesModel::RevisionContext &> context_) :
				PropertyValue::Revision(context_),
				value(other_.value)
			{  }

			virtual
			GPlatesModel::Revision::non_null_ptr_type
			clone_revision(
					boost::optional<GPlatesModel::RevisionContext &> context) const
			{
				return non_null_ptr_type(new Revision(*this, context));
			}

			virtual
			bool
			equality(
					const GPlatesModel::Revision &other) const
			{
				const Revision &other_revision = dynamic_cast<const Revision &>(other);

				return value == other_revision.value &&
						PropertyValue::Revision::equality(other);
			}

			EnumerationContent value;
		};

		EnumerationType d_type;

	};

}

#endif  // GPLATES_PROPERTYVALUES_ENUMERATION_H
