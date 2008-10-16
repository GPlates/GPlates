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

		RenderedGeometryLayers():
				d_should_show_digitisation_layer(false),
				d_should_show_geometry_focus_layer(false),
				d_should_show_pole_manipulation_layer(false),
				d_should_show_plate_closure_layer(false)
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
		 * Whether the digitisation layer should be displayed.
		 */
		bool
		should_show_digitisation_layer()
		{
			return d_should_show_digitisation_layer;
		}

		/**
		 * Show (only) the digitisation layer.
		 *
		 * This will hide the geometry-focus layer, the pole-manipulation layer, and any
		 * other canvas-tool-specific layers.
		 */
		void
		show_only_digitisation_layer()
		{
			hide_all_canvas_tool_layers();
			d_should_show_digitisation_layer = true;
		}

		/**
		 * Geometries in this layer are interactive geometries which are populated from the
		 * contents of the geometry focus.
		 *
		 * Geometries in this layer should be drawn on top of everything except the
		 * geometries in the mouse-movement layer.  Geometries in this layer should be the
		 * first matched by a spatial (mouse-click) query.
		 */
		rendered_geometry_layer_type &
		geometry_focus_layer()
		{
			return d_geometry_focus_layer;
		}

		/**
		 * Whether the geometry-focus layer should be displayed.
		 */
		bool
		should_show_geometry_focus_layer()
		{
			return d_should_show_geometry_focus_layer;
		}

		/**
		 * Show (only) the geometry-focus layer.
		 *
		 * This will hide the digitisation layer, the pole-manipulation layer, and any
		 * other canvas-tool-specific layers.
		 */
		void
		show_only_geometry_focus_layer()
		{
			hide_all_canvas_tool_layers();
			d_should_show_geometry_focus_layer = true;
		}

		/**
		 * Show (only) the geometry-focus layer.
		 *
		 * This will hide the digitisation layer, the pole-manipulation layer, and any
		 * other canvas-tool-specific layers.
		 */
		void
		show_geometry_focus_layer()
		{
			d_should_show_geometry_focus_layer = true;
		}

		/**
		 * Geometries in this layer are interactive geometries which are populated from the
		 * contents of the pole-manipulation widget.
		 *
		 * Geometries in this layer should be drawn on top of everything except the
		 * geometries in the mouse-movement layer.  Geometries in this layer should be the
		 * first matched by a spatial (mouse-click) query.
		 */
		rendered_geometry_layer_type &
		pole_manipulation_layer()
		{
			return d_pole_manipulation_layer;
		}

		/**
		 * Whether the pole-manipulation layer should be displayed.
		 */
		bool
		should_show_pole_manipulation_layer()
		{
			return d_should_show_pole_manipulation_layer;
		}

		/**
		 * Show (only) the pole-manipulation layer.
		 *
		 * This will hide the digitisation layer, the geometry-focus layer, and any other
		 * canvas-tool-specific layers.
		 */
		void
		show_only_pole_manipulation_layer()
		{
			hide_all_canvas_tool_layers();
			d_should_show_pole_manipulation_layer = true;
		}

		/**
		 * Geometries in this layer are interactive geometries which are populated from the
		 * contents of the plate_closure widget.
		 *
		 * Geometries in this layer should be drawn on top of everything except the
		 * geometries in the mouse-movement layer.  Geometries in this layer should be the
		 * first matched by a spatial (mouse-click) query.
		 */
		rendered_geometry_layer_type &
		plate_closure_layer()
		{
			return d_plate_closure_layer;
		}

		/**
		 * Whether the plate_closure layer should be displayed.
		 */
		bool
		should_show_plate_closure_layer()
		{
			return d_should_show_plate_closure_layer;
		}

		/**
		 * Show (only) the plate_closure layer.
		 *
		 * This will hide the other canvas-tool-specific layers.
		 */
		void
		show_only_plate_closure_layer()
		{
			hide_all_canvas_tool_layers();
			d_should_show_plate_closure_layer = true;
		}

		void
		hide_all_canvas_tool_layers()
		{
			d_should_show_digitisation_layer = false;
			d_should_show_geometry_focus_layer = false;
			d_should_show_pole_manipulation_layer = false;
			d_should_show_plate_closure_layer = false;
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
		 * Whether the digitisation layer should be displayed.
		 */
		bool d_should_show_digitisation_layer;

		/**
		 * Geometries in this layer are interactive geometries which are populated from the
		 * contents of the geometry focus.
		 *
		 * Geometries in this layer should be drawn on top of everything except the
		 * geometries in the mouse-movement layer.  Geometries in this layer should be the
		 * first matched by a spatial (mouse-click) query.
		 */
		rendered_geometry_layer_type d_geometry_focus_layer;

		/**
		 * Whether the geometry focus layer should be displayed.
		 */
		bool d_should_show_geometry_focus_layer;


		/**
		 * Geometries in this layer are interactive geometries which are populated from the
		 * contents of the pole manipulation widget.
		 *
		 * Geometries in this layer should be drawn on top of everything except the
		 * geometries in the mouse-movement layer.  Geometries in this layer should be the
		 * first matched by a spatial (mouse-click) query.
		 */
		rendered_geometry_layer_type d_pole_manipulation_layer;

		/**
		 * Whether the pole manipulation layer should be displayed;
		 */
		bool d_should_show_pole_manipulation_layer;

		/**
		 * Geometries in this layer are interactive geometries which are populated from the
		 * contents of the plate closure widget.
		 *
		 * Geometries in this layer should be drawn on top of everything except the
		 * geometries in the mouse-movement layer.  Geometries in this layer should be the
		 * first matched by a spatial (mouse-click) query.
		 */
		rendered_geometry_layer_type d_plate_closure_layer;

		/**
		 * Whether the plate_closure layer should be displayed;
		 */
		bool d_should_show_plate_closure_layer;

		/**
		 * Geometries in this layer are interactive geometries which are populated from the
		 * ReconstructionGeometry contents of the Reconstruction.
		 */
		rendered_geometry_layer_type d_reconstruction_layer;



	};
}

#endif // GPLATES_GUI_RENDEREDGEOMETRYLAYERS_H
