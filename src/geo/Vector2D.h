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
 *   James Boyden <jboyden@geosci.usyd.edu.au>
 */

#ifndef _GPLATES_GEO_VECTOR2D_H_
#define _GPLATES_GEO_VECTOR2D_H_

#include <iostream>
#include "GeneralisedData.h"
#include "global/types.h"  /* fpdata_t */

namespace GPlatesGeo
{
	using namespace GPlatesGlobal;

	/** 
	 * A two-dimensional variable.
	 */
	class Vector2D : public GeneralisedData
	{
		public:
			/**
			 * Create the two-dimensional zero-vector.
			 */
			Vector2D() : GeneralisedData(), _x(0.0), _y(0.0) { }
			
			/**
			 * Create the vector (@a x , @a y ).
			 */
			Vector2D(const fpdata_t& x, const fpdata_t& y);

			/**
			 * @todo Provide some kind of parsing/checking.
			 * @role ConcreteClass::PrimitiveOperation1() in the Template
			 *   Method design pattern (p325).
			 */
			virtual void
			ReadIn(std::istream& is) { is >> _x >> _y; }

			/**
			 * @todo Provide a way to specify formatting of values.
			 * @role ConcreteClass::PrimitiveOperation2() in the Template
			 *   Method design pattern (p325).
			 */
			virtual void
			PrintOut(std::ostream& os) const { os << GetX() << " " << GetY(); }
			
			fpdata_t
			GetX() const { return _x; }

			fpdata_t
			GetY() const { return _y; }

		private:
			fpdata_t _x, _y;
	};
}

#endif  // _GPLATES_GEO_VECTOR2D_H_
