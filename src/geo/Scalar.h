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
 *   James Boyden <jboyden@es.usyd.edu.au>
 */

#ifndef _GPLATES_GEO_SCALAR_H_
#define _GPLATES_GEO_SCALAR_H_

#include <iostream>
#include "GeneralisedData.h"
#include "global/types.h"  /* real_t */

namespace GPlatesMaths
{
	using namespace GPlatesGlobal;

	/** 
	 * A one-dimensional variable.
	 */
	class Scalar : GeneralisedData
	{
		public:
			/**
			 * Create a one-dimensional variable with the given
			 * magnitude.
			 */
			explicit
			Scalar(const real_t& value = 0.0)
				: GeneralisedData(), _value(value) { }

			/**
			 * @role ConcreteClass::PrimitiveOperation1() in the Template
			 *   Method design pattern (p325).
			 */
			virtual void
			ReadIn(std::istream& is) { is >> _value; }

			/**
			 * @role ConcreteClass::PrimitiveOperation2() in the Template
			 *   Method design pattern (p325).
			 */
			virtual void
			PrintOut(std::ostream& os) const { os << _value; }

			real_t
			GetValue() const { return _value; }

		private:
			real_t _value;  /** Magnitude of the variable. */
	};
}

#endif  // _GPLATES_GEO_SCALAR_H_
