/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007 The University of Sydney, Australia
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
#include "contrib/non_null_intrusive_ptr.h"


namespace GPlatesModel {

	/**
	 * This class is the abstract base of all property values.
	 *
	 * It provides pure virtual function declarations for cloning and accepting visitors.  It
	 * also provides the functions to be used by boost::intrusive_ptr for reference-counting.
	 */
	class PropertyValue {

	public:
		/**
		 * A convenience typedef for GPlatesContrib::non_null_intrusive_ptr<PropertyValue>.
		 */
		typedef GPlatesContrib::non_null_intrusive_ptr<PropertyValue> non_null_ptr_type;

		/**
		 * A convenience typedef for
		 * GPlatesContrib::non_null_intrusive_ptr<const PropertyValue>.
		 */
		typedef GPlatesContrib::non_null_intrusive_ptr<const PropertyValue>
				non_null_ptr_to_const_type;

		/**
		 * The type used to store the reference-count of an instance of this class.
		 */
		typedef long ref_count_type;

		virtual
		~PropertyValue()
		{ }

		/**
		 * Construct a PropertyValue instance.
		 *
		 * Since this class is an abstract class, this constructor can never be invoked
		 * other than explicitly in the initialiser lists of derived classes. 
		 * Nevertheless, the initialiser lists of derived classes @em do need to invoke it
		 * explicitly, since this class contains members which need to be initialised.
		 */
		PropertyValue() :
			d_ref_count(0)
		{ }

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
		 * of the new PropertyContainer instance should be zero.
		 *
		 * Note that this ctor should act exactly the same as the default (auto-generated)
		 * copy-ctor, except that it should initialise the ref-count to zero.
		 */
		PropertyValue(
				const PropertyValue &other) :
			d_ref_count(0)
		{ }

		/**
		 * Create a duplicate of this PropertyValue instance.
		 */
		virtual
		const non_null_ptr_type
		clone() const = 0;

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
		 * Increment the reference-count of this instance.
		 *
		 * This function is used by boost::intrusive_ptr and
		 * GPlatesContrib::non_null_intrusive_ptr.
		 */
		void
		increment_ref_count() const {
			++d_ref_count;
		}

		/**
		 * Decrement the reference-count of this instance, and return the new
		 * reference-count.
		 *
		 * This function is used by boost::intrusive_ptr and
		 * GPlatesContrib::non_null_intrusive_ptr.
		 */
		ref_count_type
		decrement_ref_count() const {
			return --d_ref_count;
		}

	private:

		mutable ref_count_type d_ref_count;

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		PropertyValue &
		operator=(
				const PropertyValue &);

	};


	inline
	void
	intrusive_ptr_add_ref(
			const PropertyValue *p) {
		p->increment_ref_count();
	}


	inline
	void
	intrusive_ptr_release(
			const PropertyValue *p) {
		if (p->decrement_ref_count() == 0) {
			delete p;
		}
	}

}

#endif  // GPLATES_MODEL_PROPERTYVALUE_H
