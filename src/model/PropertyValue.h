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
#if _MSC_VER == 1600
#	undef UINT8_C
#endif
#include <boost/cstdint.hpp>
#if _MSC_VER == 1600 // For Boost 1.44 and Visual Studio 2010.
#	undef UINT8_C
#	include <cstdint>
#endif

#include "utils/non_null_intrusive_ptr.h"
#include "utils/NullIntrusivePointerHandler.h"
#include "utils/ReferenceCount.h"


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
	// Forward declarations.
	class FeatureHandle;
	template<class H> class FeatureVisitorBase;
	typedef FeatureVisitorBase<FeatureHandle> FeatureVisitor;
	typedef FeatureVisitorBase<const FeatureHandle> ConstFeatureVisitor;

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
			d_instance_id(s_next_instance_id)
		{
			++s_next_instance_id;
		}

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
			d_instance_id(other.d_instance_id)
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
		 * Prints the contents of this PropertyValue to the stream @a os.
		 *
		 * Note: This function is not called operator<< because operator<< needs to
		 * be a non-member operator, but we would like polymorphic behaviour.
		 */
		virtual
		std::ostream &
		print_to(
				std::ostream &os) const = 0;

		bool
		operator==(
				const PropertyValue &other) const;

	protected:

		/**
		 * Give this PropertyValue instance a new instance id. If this shared
		 * an instance id with another PropertyValue instance because this is a
		 * clone of the other instance, the link between the instances is
		 * thereby broken by getting a new instance id here.
		 */
		void
		update_instance_id()
		{
			d_instance_id = s_next_instance_id;
			++s_next_instance_id;
		}

		/**
		 * Reimplement in derived classes where there are instance variables that can
		 * be modified by client code without using a set_*() function.
		 * For example, if a derived class has an XML attributes map that can be
		 * retrieved by non-const reference by client code, or if a derived class has
		 * nested PropertyValues returned to client code as a non_null_intrusive_ptr,
		 * it is necessary to reimplement this function, because these instance
		 * variables may have been modified without the clear_cloned_from() function
		 * getting called.
		 */
		virtual
		bool
		directly_modifiable_fields_equal(
				const PropertyValue &other) const
		{
			return true;
		}

	private:

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		PropertyValue &
		operator=(
				const PropertyValue &);

#ifdef BOOST_NO_INT64_T
		/**
		 * Just in case we happen to run into a compiler without 64-bit integers.
		 */
		class instance_id_type
		{
		public:
			instance_id_type() :
				d_high(0),
				d_low(0)
			{
			}

			instance_id_type &
			operator++()
			{
				++d_low;
				if (d_low == 0) // integer overflow
				{
					++d_high;
				}
				return *this;
			}

			bool
			operator==(
					const instance_id_type &other) const
			{
				return d_low == other.d_low && d_high == other.d_high;
			}

		private:
			boost::uint32_t d_high, d_low; // This should be portable.
		};
#else
		// Use built-in 64-bit integers where available.
		typedef boost::uint64_t instance_id_type;
#endif

		// Assists in speeding up operator==.
		instance_id_type d_instance_id;

		static instance_id_type s_next_instance_id;

	};


	// operator<< for PropertyValue.
	std::ostream &
	operator<<(
			std::ostream &os,
			const PropertyValue &property_value);

}

#endif  // GPLATES_MODEL_PROPERTYVALUE_H
