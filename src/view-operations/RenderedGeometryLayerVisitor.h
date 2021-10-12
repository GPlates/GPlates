/* $Id$ */

/**
 * \file 
 * Interface for visiting elements of a @a RenderedGeometryLayer.
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

#ifndef GPLATES_VIEWOPERATIONS_RENDEREDGEOMETRYLAYERVISITOR_H
#define GPLATES_VIEWOPERATIONS_RENDEREDGEOMETRYLAYERVISITOR_H

#include "RenderedGeometryVisitor.h"
#include "RenderedGeometryLayer.h"

namespace GPlatesViewOperations
{
	/**
	 * Interface for visiting a @a RenderedGeometryLayer object and its
	 * @a RenderedGeometry objects.
	 *
	 * Visits const objects.
	 */
	class ConstRenderedGeometryLayerVisitor :
		public ConstRenderedGeometryVisitor
	{
	public:
		/**
		 * Visit a rendered geometry layer.
		 *
		 * Return true if would like to visit the @a RenderedGeometry objects
		 * in the @a RenderedGeometryLayer specified by @a rendered_geometry_layer.
		 *
		 * Default is to only visit if layer is active.
		 *
		 * @param RenderedGeometryLayer the layer being visited.
		 * @return true if wish to visit the layer.
		 */
		virtual
		bool
		visit_rendered_geometry_layer(
				const RenderedGeometryLayer &rendered_geometry_layer)
		{
			// Default is to only visit if layer is active.
			return rendered_geometry_layer.is_active();
		}
	};

	/**
	 * Interface for visiting a @a RenderedGeometryLayer object and its
	 * @a RenderedGeometry objects.
	 *
	 * Visits non-const @a RenderedGeometryLayer but visits its
	 * const @a RenderedGeometry objects.
	 */
	class RenderedGeometryLayerVisitor :
		public ConstRenderedGeometryVisitor
	{
	public:
		/**
		 * Visit a rendered geometry layer.
		 *
		 * Return true if would like to visit the @a RenderedGeometry objects
		 * in the @a RenderedGeometryLayer specified by @a rendered_geometry_layer.
		 *
		 * Default is to only visit if layer is active.
		 *
		 * @param RenderedGeometryLayer the layer being visited.
		 * @return true if wish to visit the layer.
		 */
		virtual
		bool
		visit_rendered_geometry_layer(
				RenderedGeometryLayer &rendered_geometry_layer)
		{
			// Default is to only visit if layer is active.
			return rendered_geometry_layer.is_active();
		}
	};
}

#endif // GPLATES_VIEWOPERATIONS_RENDEREDGEOMETRYLAYERVISITOR_H
