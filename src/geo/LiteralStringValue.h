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

#ifndef _GPLATES_GEO_LITERALSTRINGVALUE_H_
#define _GPLATES_GEO_LITERALSTRINGVALUE_H_

#include "StringValue.h"

namespace GPlatesGeo
{
	/**
	 * Represents an *actual* string value.
	 */
	class LiteralStringValue : public StringValue
	{
		public:
			explicit
			LiteralStringValue(const std::string& str)
				: _str(str) {  }
	
			virtual std::string
			GetString() const { return _str; }

		private:
			std::string _str;
	};
}

#endif  /* _GPLATES_GEO_LITERALSTRINGVALUE_H_ */
