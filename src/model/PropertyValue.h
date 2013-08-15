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
#include <boost/operators.hpp>
#include <boost/optional.hpp>

#include "PropertyValueRevision.h"

#include "property-values/StructuralType.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/QtStreamable.h"
#include "utils/ReferenceCount.h"


namespace GPlatesModel
{
	// Forward declarations.
	class FeatureHandle;
	template<class H> class FeatureVisitorBase;
	typedef FeatureVisitorBase<FeatureHandle> FeatureVisitor;
	typedef FeatureVisitorBase<const FeatureHandle> ConstFeatureVisitor;
	class Model;
	class ModelTransaction;
	class PropertyValueBubbleUpRevisionHandler;
	class PropertyValueRevisionContext;
	template <class PropertyValueType> class PropertyValueRevisionedReference;


	/**
	 * This class is the abstract base of all property values.
	 *
	 * It provides pure virtual function declarations for cloning and accepting visitors.  It
	 * also provides the functions to be used by boost::intrusive_ptr for reference-counting.
	 */
	class PropertyValue :
			public GPlatesUtils::ReferenceCount<PropertyValue>,
			// Gives us "operator<<" for qDebug(), etc and QTextStream, if we provide for std::ostream...
			public GPlatesUtils::QtStreamable<PropertyValue>,
			public boost::equality_comparable<PropertyValue>
	{
	public:
		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<PropertyValue>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<PropertyValue> non_null_ptr_type;

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<const PropertyValue>.
		 */
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
			return clone_impl();
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

		/**
		 * Value equality comparison operator.
		 *
		 * Returns false if the types of @a other and 'this' aren't the same type, otherwise
		 * returns true if their values (tested recursively as needed) compare equal.
		 *
		 * Inequality provided by boost equality_comparable.
		 */
		bool
		operator==(
				const PropertyValue &other) const;

		/**
		 * Returns a (non-const) reference to the Model to which this property value belongs.
		 *
		 * Returns none if this property value is not currently attached to the model - this can happen
		 * if this property value has no parent (eg, top-level property) or if the parent has no parent, etc.
		 */
		boost::optional<Model &>
		get_model();

		/**
		 * Returns a const reference to the Model to which this property value belongs.
		 *
		 * Returns none if this property value is not currently attached to the model - this can happen
		 * if this property value has no parent (eg, top-level property) or if the parent has no parent, etc.
		 */
		boost::optional<const Model &>
		get_model() const;

	protected:

		/**
		 * Construct a PropertyValue instance.
		 */
		PropertyValue(
				const PropertyValueRevision::non_null_ptr_to_const_type &revision) :
			d_current_revision(revision)
		{  }

		/**
		 * Copy-construct a PropertyValue instance.
		 *
		 * This ctor should only be invoked by the @a clone member function.
		 * We don't copy the parent feature reference because a cloned property value does
		 * not belong to the original feature (so initially it has no parent).
		 */
		PropertyValue(
				const PropertyValue &other) :
			d_current_revision(other.d_current_revision->clone())
		{  }

		/**
		 * Returns the current immutable revision as the base revision type.
		 *
		 * Revisions are immutable - use @a MutableRevisionHandler to modify revisions.
		 */
		PropertyValueRevision::non_null_ptr_to_const_type
		get_current_revision() const
		{
			return d_current_revision;
		}

		/**
		 * Returns the current immutable revision as the specified derived revision type.
		 *
		 * Revisions are immutable - use @a MutableRevisionHandler to modify revisions.
		 */
		template <class RevisionType>
		const RevisionType &
		get_current_revision() const
		{
			return dynamic_cast<const RevisionType &>(*d_current_revision);
		}

		/**
		 * Create a new bubble-up revision by delegating to the (parent) revision context
		 * if there is one, otherwise create a new revision without any context.
		 */
		PropertyValueRevision::non_null_ptr_type
		create_bubble_up_revision(
				ModelTransaction &transaction) const;

		/**
		 * Same as the other overload of @a create_bubble_up_revision but downcasts to
		 * the specified derived revision type.
		 */
		template <class RevisionType>
		RevisionType &
		create_bubble_up_revision(
				ModelTransaction &transaction) const
		{
			// The returned revision is kept alive by either the model transaction (if uncommitted),
			// or 'this' property value (if committed).
			return dynamic_cast<RevisionType &>(*create_bubble_up_revision(transaction));
		}

		void
		clone_impl(
				PropertyValueRevisionContext &revision_context)
		{
		}

		/**
		 * Create a duplicate of this PropertyValue instance, including a recursive copy
		 * of any property values this instance might contain.
		 */
		virtual
		const non_null_ptr_type
		clone_impl() const = 0;

		/**
		 * Determine if two property value instances ('this' and 'other') value compare equal.
		 *
		 * This should recursively test for equality as needed.
		 * Note that the revision testing is done by PropertyValue::equality(), since the revisions
		 * are contained in class PropertyValue, so derived property values classes only need to test
		 * any non-revisioned data that they may contain - and if there is none then this method
		 * does not need to be implemented by that derived property value class.
		 *
		 * A precondition of this method is that the type of 'this' is the same as the type of 'object'
		 * so static_cast can be used instead of dynamic_cast.
		 */
		virtual
		bool
		equality(
				const PropertyValue &other) const;

	private:

		/**
		 * The current revision of this property value.
		 *
		 * The current revision is immutable since it has already been initialised and once
		 * initialised it cannot be modified. A modification involves creating a new revision object.
		 * Keeping the current revision 'const' prevents inadvertent modifications by derived
		 * property value classes.
		 *
		 * The revision also contains the current parent reference such that when a different
		 * revision is swapped in (due to undo/redo) it will automatically reference the correct parent.
		 *
		 * The pointer is declared 'mutable' so that revisions can be swapped in 'const' property values.
		 */
		mutable PropertyValueRevision::non_null_ptr_to_const_type d_current_revision;


		friend class ModelTransaction;
		friend class PropertyValueBubbleUpRevisionHandler;

		template <class PropertyValueType>
		friend class PropertyValueRevisionedReference;
	};


	// operator<< for PropertyValue.
	std::ostream &
	operator<<(
			std::ostream &os,
			const PropertyValue &property_value);

}

#endif  // GPLATES_MODEL_PROPERTYVALUE_H
