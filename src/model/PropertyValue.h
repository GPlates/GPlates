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

#ifndef GPLATES_MODEL_PROPERTYVALUE_H
#define GPLATES_MODEL_PROPERTYVALUE_H

// Even though we could make do with a forward declaration inside this header, every derived class
// of 'PropertyValue' will need to #include "ConstFeatureVisitor.h" and "FeatureVisitor.h" anyway,
// so we may as well include them here.
#include "ConstFeatureVisitor.h"
#include "FeatureVisitor.h"
#include "utils/non_null_intrusive_ptr.h"
#include "utils/NullIntrusivePointerHandler.h"
#include "utils/ReferenceCount.h"

// This macro is used to define the template function 'assign_member' inside a class of type C.
// To define the function, invoke the macro in the class definition, supplying the class type as
// the argument to the macro.  The macro invocation will expand to a definition of the function.
#define DEFINE_FUNCTION_ASSIGN_MEMBER(C)  \
		template<typename MemberType>  \
		void  \
		assign_member(  \
				MemberType C::*member_ptr_lvalue,  \
				const MemberType &rvalue)  \
		{  \
			if (container() == NULL) {  \
				/* This property value is not contained, so assign to 'this'. */  \
				this->*member_ptr_lvalue = rvalue;  \
			} else {  \
				GPlatesModel::DummyTransactionHandle transaction(__FILE__, __LINE__);  \
				C::non_null_ptr_type dup = clone();  \
				dup.get()->*member_ptr_lvalue = rvalue;  \
				container()->bubble_up_change(this, dup, transaction);  \
				transaction.commit();  \
			}  \
		}


// This macro is used to define the virtual function 'deep_clone_as_prop_val' inside a class which
// derives from PropertyValue.  The function definition is exactly identical in every PropertyValue
// derivation, but the function must be defined in each derived class (rather than in the base)
// because it invokes the non-virtual member function 'deep_clone' of that specific derived class.
// (This function 'deep_clone' cannot be moved into the base class, because (i) its return type is
// the type of the derived class, and (ii) it must perform different actions in different classes.)
// To define the function, invoke the macro in the class definition.  The macro invocation will
// expand to a definition of the function.
#define DEFINE_FUNCTION_DEEP_CLONE_AS_PROP_VAL()  \
		virtual  \
		const GPlatesModel::PropertyValue::non_null_ptr_type  \
		deep_clone_as_prop_val() const  \
		{  \
			return deep_clone();  \
		}


namespace GPlatesModel
{
	class PropertyValueContainer;


	/**
	 * This class is the abstract base of all property values.
	 *
	 * It provides pure virtual function declarations for cloning and accepting visitors.  It
	 * also provides the functions to be used by boost::intrusive_ptr for reference-counting.
	 */
	class PropertyValue :
			public GPlatesUtils::ReferenceCount<PropertyValue>
	{
	public:
		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<PropertyValue,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<PropertyValue,
				GPlatesUtils::NullIntrusivePointerHandler> non_null_ptr_type;

		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<const PropertyValue,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const PropertyValue,
				GPlatesUtils::NullIntrusivePointerHandler>
				non_null_ptr_to_const_type;

		/**
		 * Construct a PropertyValue instance.
		 *
		 * Since this class is an abstract class, this constructor can never be invoked
		 * other than explicitly in the initialiser lists of derived classes. 
		 * Nevertheless, the initialiser lists of derived classes @em do need to invoke it
		 * explicitly, since this class contains members which need to be initialised.
		 */
		PropertyValue() :
			GPlatesUtils::ReferenceCount<PropertyValue>(),
			d_container(NULL)
		{  }

		/**
		 * Construct a PropertyValue instance which is a copy of @a other.
		 *
		 * Since this class is an abstract class, this constructor can never be invoked
		 * other than explicitly in the initialiser lists of derived classes. 
		 * Nevertheless, the initialiser lists of derived classes @em do need to invoke it
		 * explicitly, since this class contains members which need to be initialised.
		 *
		 * This ctor should only be invoked by the @a clone member function (pure virtual
		 * in this class; defined in derived classes), which will create a duplicate
		 * instance and return a new intrusive_ptr reference to the new duplicate.  Since
		 * initially the only reference to the new duplicate will be the one returned by
		 * the @a clone function, *before* the new intrusive_ptr is created, the ref-count
		 * of the new PropertyValue instance should be zero.
		 *
		 * Note that this ctor should act exactly the same as the default (auto-generated)
		 * copy-ctor, except that it should initialise the ref-count to zero.
		 */
		PropertyValue(
				const PropertyValue &other) :
			GPlatesUtils::ReferenceCount<PropertyValue>(),
			d_container(NULL)
		{  }

		virtual
		~PropertyValue()
		{  }

		/**
		 * Create a duplicate of this PropertyValue instance, including a recursive copy
		 * of any property values this instance might contain.
		 *
		 * The Bubble-Up revisioning system @em might make this function redundant
		 * when it's fully operational.  Until then, however...
		 */
		virtual
		const non_null_ptr_type
		deep_clone_as_prop_val() const = 0;

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
		 * Access the PropertyValueContainer which contains this PropertyValue.
		 *
		 * Client code should not use this function!
		 *
		 * Note that the return value may be a NULL pointer, which signifies that this
		 * PropertyValue is not contained within a PropertyValueContainer.
		 */
		PropertyValueContainer *
		container() const
		{
			return d_container;
		}

		/**
		 * Set the PropertyValueContainer which contains this PropertyValue.
		 *
		 * Client code should not use this function!
		 *
		 * Note that @a new_container may be a NULL pointer... but only if this
		 * PropertyValue instance will not be contained within a PropertyValueContainer.
		 */
		void
		set_container(
				PropertyValueContainer *new_container)
		{
			d_container = new_container;
		}

	private:
		/**
		 * The property value container which contains this PropertyValue instance.
		 *
		 * Note that this should be held via a (regular, raw) pointer rather than a
		 * ref-counting pointer (or any other type of smart pointer) because:
		 *  -# The PropertyValueContainer instance conceptually manages the instance of
		 * this class, not the other way around.
		 *  -# A PropertyValueContainer instance will outlive the PropertyValue instances
		 * it contains; thus, it doesn't make sense for a PropertyValueContainer to have
		 * its memory managed by its contained PropertyValue.
		 *  -# Each PropertyValueContainer derivation will contain a ref-counting pointer
		 * to class PropertyValue, and we don't want to set up a ref-counting loop (which
		 * would lead to memory leaks).
		 *
		 * This pointer may be NULL.  It will be NULL when this PropertyValue instance is
		 * not contained.
		 */
		PropertyValueContainer *d_container;

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		PropertyValue &
		operator=(
				const PropertyValue &);

	};

}

#endif  // GPLATES_MODEL_PROPERTYVALUE_H
