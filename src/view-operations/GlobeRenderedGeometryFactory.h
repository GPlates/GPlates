/* $Id$ */

/**
 * \file 
 * Extends @a RenderedGeometryFactory to handle creation of @a RenderedGeometry objects
 * that are dependent on the 3D Globe view.
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

#ifndef GPLATES_VIEWOPERATIONS_GLOBERENDEREDGEOMETRYFACTORY_H
#define GPLATES_VIEWOPERATIONS_GLOBERENDEREDGEOMETRYFACTORY_H

#include "RenderedGeometryFactory.h"


namespace GPlatesViewOperations
{
	class GlobeRenderedGeometryFactory :
		public RenderedGeometryFactory
	{
	public:
		//@{
		/**
		 * Inherited from @a RenderedGeometryFactory.
		 */
		virtual
		RenderedGeometry
		create_rendered_dashed_polyline(
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type,
				const GPlatesGui::Colour &colour);

		virtual
		rendered_geometry_seq_type
		create_rendered_dashed_polyline_segments_on_sphere(
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type,
				const GPlatesGui::Colour &colour);
		//@}

	private:
	};
}

#endif // GPLATES_VIEWOPERATIONS_GLOBERENDEREDGEOMETRYFACTORY_H
