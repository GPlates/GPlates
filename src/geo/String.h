/* $Id$ */

/**
 * \file
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
 *   Hamish Law <hlaw@es.usyd.edu.au>
 */

#ifndef _GPLATES_GEO_STRING_H_
#define _GPLATES_GEO_STRING_H_

#include <string>
#include <iostream>
#include "GeneralisedData.h"

namespace GPlatesGeo
{
	/** 
	 * A string of characters.
	 */
	class String : public GeneralisedData
	{
		public:
			explicit
			String(const std::string& str = "")
				: GeneralisedData(), _string(str) {}

			/**
			 * Read one word from a stream.
			 * @todo Provide some way to alter how much is read into the
			 *   String.
			 * @role ConcreteClass::PrimitiveOperation1() in the Template
			 *   Method design pattern (p325).
			 */
			virtual void
			ReadIn(std::istream& is) { is >> _string; }
			
			/**
			 * @role ConcreteClass::PrimitiveOperation2() in the Template
			 *   Method design pattern (p325).
			 */
			virtual void
			PrintOut(std::ostream& os) const { os << _string; }
			
			std::string
			GetValue() const { return _string; }

		private:
			std::string _string;
	};
}

#endif  // _GPLATES_GEO_STRING_H_
