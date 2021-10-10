/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_RENDEREDGEOMETRY_H
#define GPLATES_GUI_RENDEREDGEOMETRY_H

#include "maths/GeometryOnSphere.h"
#include "PlatesColourTable.h"


namespace GPlatesGui
{
	/**
	 * This class describes a geometry which hase been rendered for display.
	 */
	class RenderedGeometry
	{
	public:
		RenderedGeometry(
				GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry_,
				PlatesColourTable::const_iterator colour_):
			d_geometry(geometry_),
			d_colour(colour_)
		{  }

		virtual
		~RenderedGeometry()
		{  }

		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
		geometry() const
		{
			return d_geometry;
		}

		PlatesColourTable::const_iterator
		colour() const
		{
			return d_colour;
		}
	private:
		/**
		 * This is the geometry which is to be displayed.
		 */
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type d_geometry;

		/**
		 * This is the colour in which the geometry is to be drawn.
		 */
		PlatesColourTable::const_iterator d_colour;
	};
}

#endif // GPLATES_GUI_RENDEREDGEOMETRY_H
