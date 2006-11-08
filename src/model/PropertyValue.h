/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006 The GPlates Consortium
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef GPLATES_MODEL_PROPERTYVALUE_H
#define GPLATES_MODEL_PROPERTYVALUE_H

#include <boost/intrusive_ptr.hpp>


namespace GPlatesModel {

	// Forward declaration for the function 'accept_visitor'.
	class ConstFeatureVisitor;

	class PropertyValue {

	public:

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
		 */
		PropertyValue(
				const PropertyValue &other) :
			d_ref_count(other.d_ref_count)
		{ }

		virtual
		boost::intrusive_ptr<PropertyValue>
		clone() const = 0;

		virtual
		void
		accept_visitor(
				ConstFeatureVisitor &visitor) const = 0;

		void
		increment_ref_count() const {
			++d_ref_count;
		}

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
