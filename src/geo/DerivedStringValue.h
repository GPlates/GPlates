/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Author$
 *   $Date$
 * 
 * Copyright (C) 2003 The GPlates Consortium
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Authors:
 *   Hamish Ivey-Law <hlaw@geosci.usyd.edu.au>
 */

#ifndef _GPLATES_GEO_DERIVEDSTRINGVALUE_H_
#define _GPLATES_GEO_DERIVEDSTRINGVALUE_H_

#include "StringValue.h"

namespace GPlatesGeo
{
	/**
	 * Represents a string that is defined further up the
	 * hierarchy.  The parent should be from one of the 
	 * enclosing DataGroups.  When this parent list is 
	 * followed, we should (in theory) wind up at a 
	 * LiteralStringValue.
	 */
	class DerivedStringValue : public StringValue
	{
		public:
			explicit
			DerivedStringValue(const StringValue* parent)
				: _parent(parent) {  }

			virtual std::string
			GetString() const { return _parent->GetString(); }

		private:
			const StringValue* _parent;
	};
}

#endif  /* _GPLATES_GEO_DERIVEDSTRINGVALUE_H_ */
