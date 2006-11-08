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

#ifndef GPLATES_MODEL_XSSTRING_H
#define GPLATES_MODEL_XSSTRING_H

#include <boost/intrusive_ptr.hpp>
#include "PropertyValue.h"
#include "TextContent.h"


namespace GPlatesModel {

	class XsString :
			public PropertyValue {

	public:

		virtual
		~XsString() {  }

		static
		boost::intrusive_ptr<XsString>
		create(
				const UnicodeString &s) {
			boost::intrusive_ptr<XsString> ptr(new XsString(s));
			return ptr;
		}

		virtual
		boost::intrusive_ptr<PropertyValue>
		clone() const {
			boost::intrusive_ptr<PropertyValue> dup(new XsString(*this));
			return dup;
		}

		// FIXME: visitor accept method

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		explicit
		XsString(
				const UnicodeString &s) :
			PropertyValue(),
			d_tc(s)
		{  }

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		//
		// Note that this should act exactly the same as the default (auto-generated)
		// copy-constructor, except it should not be public.
		XsString(
				const XsString &other) :
			PropertyValue(other),
			d_tc(other.d_tc)
		{  }

	private:

		TextContent d_tc;

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		XsString &
		operator=(const XsString &);

	};

}

#endif  // GPLATES_MODEL_XSSTRING_H
