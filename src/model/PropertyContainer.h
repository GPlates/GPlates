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

#ifndef GPLATES_MODEL_PROPERTYCONTAINER_H
#define GPLATES_MODEL_PROPERTYCONTAINER_H

#include <unicode/unistr.h>
#include <boost/intrusive_ptr.hpp>


namespace GPlatesModel {

	class PropertyContainer {

	public:

		typedef long ref_count_type;

		virtual
		~PropertyContainer()
		{ }

		virtual
		boost::intrusive_ptr<PropertyContainer>
		clone() const = 0;

		ref_count_type
		ref_count() const {
			return d_ref_count;
		}

		void
		increment_ref_count() {
			++d_ref_count;
		}

		ref_count_type
		decrement_ref_count() {
			return --d_ref_count;
		}

		const UnicodeString &
		property_name() const {
			return d_property_name;
		}

		// FIXME: visitor accept method

	protected:

		explicit
		PropertyContainer(const UnicodeString &property_name_) :
			d_ref_count(0),
			d_property_name(property_name_)
		{ }

	private:

		ref_count_type d_ref_count;
		UnicodeString d_property_name;

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		PropertyContainer &
		operator=(PropertyContainer &);

	};


	void
	intrusive_ptr_add_ref(PropertyContainer *p) {
		p->increment_ref_count();
	}


	void
	intrusive_ptr_release(PropertyContainer *p) {
		if (p->decrement_ref_count() == 0) {
			delete p;
		}
	}

}

#endif  // GPLATES_MODEL_PROPERTYCONTAINER_H
