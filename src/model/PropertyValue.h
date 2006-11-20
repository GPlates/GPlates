/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006 The University of Sydney, Australia
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

#include <boost/intrusive_ptr.hpp>
// Even though we could make do with a forward declaration inside this header, every derived class
// of 'PropertyValue' will need to #include "ConstFeatureVisitor.h" anyway, so we may as well
// include it here.
#include "ConstFeatureVisitor.h"


namespace GPlatesModel {

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
