/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
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
 */

#ifndef _GPLATES_GEO_STRINGVALUE_H_
#define _GPLATES_GEO_STRINGVALUE_H_

#include <string>

namespace GPlatesGeo
{
	/**
	 * Represents the value that a key may take inside an attribute
	 * map.
	 */
	class StringValue
	{
		public:
			/**
			 * Return the string to which this StringValue refers.
			 */
			virtual std::string
			GetString() const = 0;

			virtual
			~StringValue() {  }
	
		protected:
			StringValue() {  }
	};
}

#endif  /* _GPLATES_GEO_STRINGVALUE_H_ */
