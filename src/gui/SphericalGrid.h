/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006, 2008 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_SPHERICALGRID_H
#define GPLATES_GUI_SPHERICALGRID_H

#include <boost/noncopyable.hpp>

#include "opengl/GLDrawable.h"
#include "opengl/GLRenderGraphInternalNode.h"
#include "opengl/GLStateSet.h"
#include "opengl/GLUNurbsRenderer.h"


namespace GPlatesGui
{
	class Colour;


	class SphericalGrid :
			private boost::noncopyable
	{
	public:
		SphericalGrid(
				unsigned num_circles_lat,
				unsigned num_circles_lon);

		~SphericalGrid()
		{  }

		void
		paint(
				const GPlatesOpenGL::GLRenderGraphInternalNode::non_null_ptr_type &render_graph_parent_node,
				const Colour &colour);

		/**
		 * Paint the circumference for vector output.
		 */
		void
		paint_circumference(
				const GPlatesOpenGL::GLRenderGraphInternalNode::non_null_ptr_type &render_graph_parent_node,
				const Colour &colour);

	private:
		GPlatesOpenGL::GLUNurbsRenderer::non_null_ptr_type d_nurbs;
		GPlatesOpenGL::GLStateSet::non_null_ptr_to_const_type d_state_set;

		unsigned d_num_circles_lat;
		unsigned d_num_circles_lon;

		/**
		 * The delta (in radians) between adjacent circles of latitude.
		 */
		double d_lat_delta;

		/**
		 * The delta (in radians) between adjacent circles of latitude.
		 */
		double d_lon_delta;

		/**
		 * The value of pi.
		 *
		 * FIXME:  Pi is often defined in the C Standard Library -- but is it @em always
		 * defined on @em every platform? and to the @em same precision?
		 */
		static const double s_pi;

		/**
		 * Draw a line of latitude for latitude @a lat.
		 * The angle is in radians.
		 */
		GPlatesOpenGL::GLDrawable::non_null_ptr_to_const_type
		draw_line_of_lat(
				const double &lat,
				const Colour &colour);

		/**
		 * Draw a line of longitude for longitude @a lon.
		 * The angle is in radians.
		 */
		GPlatesOpenGL::GLDrawable::non_null_ptr_to_const_type
		draw_line_of_lon(
				const double &lon,
				const Colour &colour);

		/**
		 * Creates an OpenGL state set that defines the appearance of the grid lines.
		 */
		static
		GPlatesOpenGL::GLStateSet::non_null_ptr_to_const_type
		create_state_set();
	};
}

#endif  // GPLATES_GUI_SPHERICALGRID_H
