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

#ifndef GPLATES_GUI_RENDEREDGEOMETRYLAYERS_H
#define GPLATES_GUI_RENDEREDGEOMETRYLAYERS_H

#include <vector>
#include "RenderedGeometry.h"


namespace GPlatesGui
{
	/**
	 * This class contains the rendered geometry layers for a given Reconstruction View.
	 */
	class RenderedGeometryLayers
	{
	public:
		typedef std::vector<RenderedGeometry> rendered_geometry_layer_type;

		RenderedGeometryLayers()
		{  }

		~RenderedGeometryLayers()
		{  }

		/**
		 * Geometries in this layer are non-interactive geometries which are drawn and
		 * updated in response to mouse movement.
		 *
		 * For example: the selection rectangle whose corner follows the mouse pointer
		 * position (when the rectangle-selection canvas tool is in use); the line-segment
		 * whose endpoint follows the mouse pointer position (when the digitisation tool is
		 * in use).
		 *
		 * Geometries in this layer should be drawn on top of everything else.
		 */
		rendered_geometry_layer_type &
		mouse_movement_layer()
		{
			return d_mouse_movement_layer;
		}

		/**
		 * Geometries in this layer are interactive geometries which are populated from the
		 * contents of the digitisation widget.
		 *
		 * Geometries in this layer should be drawn on top of everything except the
		 * geometries in the mouse-movement layer.  Geometries in this layer should be the
		 * first matched by a spatial (mouse-click) query.
		 */
		rendered_geometry_layer_type &
		digitisation_layer()
		{
			return d_digitisation_layer;
		}

		/**
		 * Geometries in this layer are interactive geometries which are populated from the
		 * ReconstructionGeometry contents of the Reconstruction.
		 */
		rendered_geometry_layer_type &
		reconstruction_layer()
		{
			return d_reconstruction_layer;
		}
	private:
		/**
		 * Geometries in this layer are non-interactive geometries which are drawn and
		 * updated in response to mouse movement.
		 *
		 * For example: the selection rectangle whose corner follows the mouse pointer
		 * position (when the rectangle-selection canvas tool is in use); the line-segment
		 * whose endpoint follows the mouse pointer position (when the digitisation tool is
		 * in use).
		 *
		 * Geometries in this layer should be drawn on top of everything else.
		 */
		rendered_geometry_layer_type d_mouse_movement_layer;

		/**
		 * Geometries in this layer are interactive geometries which are populated from the
		 * contents of the digitisation widget.
		 *
		 * Geometries in this layer should be drawn on top of everything except the
		 * geometries in the mouse-movement layer.  Geometries in this layer should be the
		 * first matched by a spatial (mouse-click) query.
		 */
		rendered_geometry_layer_type d_digitisation_layer;

		/**
		 * Geometries in this layer are interactive geometries which are populated from the
		 * ReconstructionGeometry contents of the Reconstruction.
		 */
		rendered_geometry_layer_type d_reconstruction_layer;
	};
}

#endif // GPLATES_GUI_RENDEREDGEOMETRYLAYERS_H
