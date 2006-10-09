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
// #include "base/RevisionId.h"


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

		void
		decrement_ref_count() {
			--d_ref_count;
		}

		const UnicodeString &
		name() const {
			return d_name;
		}

		// FIXME: visitor accept method

	protected:

		explicit
		PropertyContainer(const UnicodeString &name_) :
				d_ref_count(0),
				d_name(name_)
		{ }

	private:

		ref_count_type d_ref_count;
		UnicodeString d_name;

		PropertyContainer &
		operator=(PropertyContainer &);

	};


	void
	intrusive_ptr_add_ref(PropertyContainer *p) {
		p->increment_ref_count();
	}


	void
	intrusive_ptr_release(PropertyContainer *p) {
		p->decrement_ref_count();
		if (p->ref_count() == 0) {
			delete p;
		}
	}

}

#endif  // GPLATES_MODEL_PROPERTYCONTAINER_H
