/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008, 2010 The University of Sydney, Australia
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

#ifndef GPLATES_CANVASTOOLS_MANIPULATEPOLE_H
#define GPLATES_CANVASTOOLS_MANIPULATEPOLE_H

#include "CanvasTool.h"

#include "gui/GlobeCanvasTool.h"

#include "qt-widgets/ModifyReconstructionPoleWidget.h"


namespace GPlatesViewOperations
{
	class RenderedGeometryCollection;
}

namespace GPlatesCanvasTools
{
	/**
	 * This is the canvas tool used to interactively manipulate absolute rotations.
	 */
	class ManipulatePole :
			public CanvasTool
	{
	public:

		/**
		 * Convenience typedef for GPlatesUtils::non_null_intrusive_ptr<ManipulatePole>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<ManipulatePole> non_null_ptr_type;

		static
		const non_null_ptr_type
		create(
				const status_bar_callback_type &status_bar_callback,
				GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
				GPlatesQtWidgets::ModifyReconstructionPoleWidget &pole_widget)
		{
			return new ManipulatePole(
					status_bar_callback,
					rendered_geom_collection,
					pole_widget);
		}

		virtual
		void
		handle_activation();

		virtual
		void
		handle_deactivation();

		virtual
		void
		handle_left_drag(
				const GPlatesMaths::PointOnSphere &initial_point_on_sphere,
				bool was_on_earth,
				double initial_proximity_inclusion_threshold,
				const GPlatesMaths::PointOnSphere &current_point_on_sphere,
				bool is_on_earth,
				double current_proximity_inclussion_threshold,
				const boost::optional<GPlatesMaths::PointOnSphere> &centre_of_viewport);

		virtual
		void
		handle_left_release_after_drag(
				const GPlatesMaths::PointOnSphere &initial_point_on_sphere,
				bool was_on_earth,
				double initial_proximity_inclusion_threshold,
				const GPlatesMaths::PointOnSphere &current_point_on_sphere,
				bool is_on_earth,
				double current_proximity_inclussion_threshold,
				const boost::optional<GPlatesMaths::PointOnSphere> &centre_of_viewport);

		virtual
		void
		handle_shift_left_drag(
				const GPlatesMaths::PointOnSphere &initial_point_on_sphere,
				bool was_on_earth,
				double initial_proximity_inclusion_threshold,
				const GPlatesMaths::PointOnSphere &current_point_on_sphere,
				bool is_on_earth,
				double current_proximity_inclussion_threshold,
				const boost::optional<GPlatesMaths::PointOnSphere> &centre_of_viewport);

		virtual
		void
		handle_shift_left_release_after_drag(
				const GPlatesMaths::PointOnSphere &initial_point_on_sphere,
				bool was_on_earth,
				double initial_proximity_inclusion_threshold,
				const GPlatesMaths::PointOnSphere &current_point_on_sphere,
				bool is_on_earth,
				double current_proximity_inclussion_threshold,
				const boost::optional<GPlatesMaths::PointOnSphere> &centre_of_viewport);

	private:

		ManipulatePole(
				const status_bar_callback_type &status_bar_callback,
				GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
				GPlatesQtWidgets::ModifyReconstructionPoleWidget &pole_widget);

		/**
		 * We need to change which canvas-tool layer is shown when this canvas-tool is
		 * activated.
		 */
		GPlatesViewOperations::RenderedGeometryCollection *d_rendered_geom_collection;

		/**
		 * This is the Modify Reconstruction Pole widget in the Task Panel.
		 * It accumulates the rotation adjustment for us, as well as other book-keeping.
		 */
		GPlatesQtWidgets::ModifyReconstructionPoleWidget *d_pole_widget_ptr;

		/**
		 * Whether or not this pole-manipulation tool is currently in the midst of a
		 * pole-manipulating drag.
		 */
		bool d_is_in_drag;
	};
}

#endif  // GPLATES_CANVASTOOLS_MANIPULATEPOLE_H
