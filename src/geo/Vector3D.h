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

#ifndef _GPLATES_GEO_VECTOR3D_H_
#define _GPLATES_GEO_VECTOR3D_H_

#include <iostream>
#include "GeneralisedData.h"
#include "global/types.h"  /* fpdata_t */

namespace GPlatesGeo
{
	using namespace GPlatesGlobal;

	/** 
	 * A three-dimensional variable.
	 */
	class Vector3D : public GeneralisedData
	{
		public:
			/** 
			 * Create the three-dimensional zero vector.
			 */
			Vector3D() 
				: GeneralisedData(), _x(0.0), _y(0.0), _z(0.0) { }
			
			/** 
			 * Create the vector (@a x , @a y , @a z ).
			 */
			Vector3D(const fpdata_t& x, const fpdata_t& y, const fpdata_t& z);

			/**
			 * @todo See Todo section from Vector2D::ReadIn().
			 * @role ConcreteClass::PrimitiveOperation1() in the Template
			 *   Method design pattern (p325).
			 */
			virtual void
			ReadIn(std::istream& is) {

				is >> _x >> _y >> _z;
			}

			/**
			 * @todo See Todo section from Vector2D::PrintOut().
			 * @role ConcreteClass::PrimitiveOperation2() in the Template
			 *   Method design pattern (p325).
			 */
			virtual void
			PrintOut(std::ostream& os) const;
			
			fpdata_t
			GetX() const { return _x; }

			fpdata_t
			GetY() const { return _y; }

			fpdata_t
			GetZ() const { return _z; }

		private:
			fpdata_t _x, _y, _z;
	};
}

#endif  // _GPLATES_GEO_VECTOR3D_H_
