/* $Id$ */

/**
 * \file 
 * Contains the definition of the class PropertyValue.
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

#ifndef GPLATES_MODEL_PROPERTYVALUE_H
#define GPLATES_MODEL_PROPERTYVALUE_H

#include <iosfwd>
#include <boost/optional.hpp>

#include "Revisionable.h"

#include "property-values/StructuralType.h"

#include "utils/QtStreamable.h"
#include "utils/ReferenceCount.h"


namespace GPlatesModel
{
	// Forward declarations.
	class FeatureHandle;
	template<class H> class FeatureVisitorBase;
	typedef FeatureVisitorBase<FeatureHandle> FeatureVisitor;
	typedef FeatureVisitorBase<const FeatureHandle> ConstFeatureVisitor;


	/**
	 * This class is the abstract base of all property values.
	 *
	 * It provides pure virtual function declarations for accepting visitors.
	 */
	class PropertyValue :
			public Revisionable,
			// Gives us "operator<<" for qDebug(), etc and QTextStream, if we provide for std::ostream...
			public GPlatesUtils::QtStreamable<PropertyValue>
	{
	public:

		//! A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<PropertyValue>.
		typedef GPlatesUtils::non_null_intrusive_ptr<PropertyValue> non_null_ptr_type;

		//! A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<const PropertyValue>.
		typedef GPlatesUtils::non_null_intrusive_ptr<const PropertyValue> non_null_ptr_to_const_type;


		virtual
		~PropertyValue()
		{  }

		/**
		 * Create a duplicate of this PropertyValue instance, including a recursive copy
		 * of any property values this instance might contain.
		 */
		const non_null_ptr_type
		clone() const
		{
			return GPlatesUtils::dynamic_pointer_cast<PropertyValue>(clone_impl());
		}

		/**
		 * Returns the structural type associated with the type of the derived property value class.
		 *
		 * NOTE: This is actually a per-class, rather than per-instance, method but
		 * it's most accessible when implemented as a virtual method.
		 * Derived property value classes ideally should return a 'static' variable rather than
		 * an instance variable (data member) in order to reduce memory usage.
		 */
		virtual
		GPlatesPropertyValues::StructuralType
		get_structural_type() const = 0;

		/**
		 * Accept a ConstFeatureVisitor instance.
		 *
		 * See the Visitor pattern (p.331) in Gamma95 for information on the purpose of
		 * this function.
		 */
		virtual
		void
		accept_visitor(
				ConstFeatureVisitor &visitor) const = 0;

		/**
		 * Accept a FeatureVisitor instance.
		 *
		 * See the Visitor pattern (p.331) in Gamma95 for information on the purpose of
		 * this function.
		 */
		virtual
		void
		accept_visitor(
				FeatureVisitor &visitor) = 0;

		/**
		 * Prints the contents of this PropertyValue to the stream @a os.
		 *
		 * Note: This function is not called operator<< because operator<< needs to
		 * be a non-member operator, but we would like polymorphic behaviour.
		 */
		virtual
		std::ostream &
		print_to(
				std::ostream &os) const = 0;

	protected:

		/**
		 * Construct a PropertyValue instance.
		 */
		explicit
		PropertyValue(
				const Revision::non_null_ptr_to_const_type &revision) :
			Revisionable(revision)
		{  }


		/**
		 * Top-level property data that is mutable/revisionable.
		 */
		class Revision :
				public GPlatesModel::Revision
		{
		public:

			typedef GPlatesUtils::non_null_intrusive_ptr<Revision> non_null_ptr_type;
			typedef GPlatesUtils::non_null_intrusive_ptr<const Revision> non_null_ptr_to_const_type;

		protected:

			/**
			 * Constructor specified optional (parent) context in which this property value (revision) is nested.
			 */
			explicit
			Revision(
					boost::optional<RevisionContext &> context = boost::none) :
				GPlatesModel::Revision(context)
			{  }

			/**
			 * Construct a Revision instance using another Revision.
			 */
			Revision(
					const Revision &other,
					boost::optional<RevisionContext &> context) :
				GPlatesModel::Revision(context)
			{  }

		};

	};


	// operator<< for PropertyValue.
	std::ostream &
	operator<<(
			std::ostream &os,
			const PropertyValue &property_value);

}

#endif  // GPLATES_MODEL_PROPERTYVALUE_H
