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
 *   Hamish Law <hlaw@geosci.usyd.edu.au>
 */

#ifndef _GPLATES_FILEIO_XMLSTRING_H_
#define _GPLATES_FILEIO_XMLSTRING_H_

#include <iostream>
#include <xercesc/util/XMLString.hpp>

namespace GPlatesFileIO
{
	class XMLString
	{
		public:
			XMLString(const XMLCh *const str) 
				: _str(xercesc::XMLString::transcode(str)) {  }

			~XMLString() { xercesc::XMLString::release(&_str); }

			const char*
			c_str() const { return _str; }

		private:
			char *_str;
	};

	inline std::ostream&
	operator<<(std::ostream& os, const XMLString& str) {
		return os << str.c_str();
	}
}

#endif  // _GPLATES_FILEIO_XMLSTRING_H_
