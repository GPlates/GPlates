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

#ifndef _GPLATES_GRIDELEMENT_H_
#define _GPLATES_GRIDELEMENT_H_

#include "Grid.h"
#include "maths/GlobalDefinitions.h"

namespace GPlatesGeo
{
	class Grid::GridElement
	{
		public:
			GridElement(index_t row, index_t col) 
				: _row(row), _col(col) { }

		private:
			index_t _row, _col;
			GeologicalData::Attributes_t _attributes;
	};
}

#endif  // _GPLATES_GRIDELEMENT_H_
