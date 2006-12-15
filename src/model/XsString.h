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

#ifndef GPLATES_MODEL_XSSTRING_H
#define GPLATES_MODEL_XSSTRING_H

#include "PropertyValue.h"
#include "TextContent.h"


namespace GPlatesModel {

	class XsString :
			public PropertyValue {

	public:

		virtual
		~XsString() {  }

		static
		const boost::intrusive_ptr<XsString>
		create(
				const UnicodeString &s) {
			boost::intrusive_ptr<XsString> ptr(new XsString(s));
			return ptr;
		}

		virtual
		const boost::intrusive_ptr<PropertyValue>
		clone() const {
			boost::intrusive_ptr<PropertyValue> dup(new XsString(*this));
			return dup;
		}

		const TextContent &
		value() const {
			return d_value;
		}

		virtual
		void
		accept_visitor(
				ConstFeatureVisitor &visitor) const {
			visitor.visit_xs_string(*this);
		}

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		explicit
		XsString(
				const UnicodeString &s) :
			PropertyValue(),
			d_value(s)
		{  }

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		//
		// Note that this should act exactly the same as the default (auto-generated)
		// copy-constructor, except it should not be public.
		XsString(
				const XsString &other) :
			PropertyValue(other),
			d_value(other.d_value)
		{  }

	private:

		TextContent d_value;

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		XsString &
		operator=(const XsString &);

	};

}

#endif  // GPLATES_MODEL_XSSTRING_H
