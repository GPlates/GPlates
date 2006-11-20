/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006 The University of Sydney, Australia
 *
 * This file is part of GPlates.
 *
 * GPlates is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 2, as published by
 * the Free Software Foundation.
 *
 * GPlates is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef _GPLATES_GEO_GRIDELEMENT_H_
#define _GPLATES_GEO_GRIDELEMENT_H_

#include "GeologicalData.h"

namespace GPlatesGeo
{
	/**
	 * An element of gridded data.
	 */
	class GridElement
	{
		typedef GPlatesGeo::GeologicalData::Attributes_t Attributes_t;

		public:
			explicit
			GridElement (float val, const Attributes_t &attrs =
						GeologicalData::NO_ATTRIBUTES)
				: _value (val), _attributes(attrs)
			{ }

			float getValue () const { return _value; }
			void setValue (float val) { _value = val; }

		private:
			float _value;
			Attributes_t _attributes;
	};
}

#endif  // _GPLATES_GEO_GRIDELEMENT_H_
