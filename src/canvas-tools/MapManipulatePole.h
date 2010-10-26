/* $Id: MapInsertVertex.h 5447 2009-04-02 16:01:17Z rwatson $ */

/**
 * \file Derived @a CanvasTool to insert vertices into temporary or focused feature geometry.
 * $Revision: 5447 $
 * $Date: 2009-04-03 03:01:17 +1100 (fr, 03 apr 2009) $
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#ifndef GPLATES_CANVASTOOLS_MAPMANIPULATEPOLE_H
#define GPLATES_CANVASTOOLS_MAPMANIPULATEPOLE_H

#include "gui/MapCanvasTool.h"
#include "qt-widgets/ModifyReconstructionPoleWidget.h"


namespace GPlatesGui
{
	class ChooseCanvasTool;
	class MapTransform;
}

namespace GPlatesQtWidgets
{
	class ViewportWindow;
}

namespace GPlatesViewOperations
{
	class ActiveGeometryOperation;
	class InsertVertexGeometryOperation;
	class GeometryOperationTarget;
	class QueryProximityThreshold;
	class RenderedGeometryCollection;
}

namespace GPlatesCanvasTools
{
	/**
	 * This is the canvas tool used to insert vertices into geometry.
	 */
	class MapManipulatePole:
			public GPlatesGui::MapCanvasTool
	{
	public:
		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<MapInsertVertex,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<MapManipulatePole,
				GPlatesUtils::NullIntrusivePointerHandler> non_null_ptr_type;

		virtual
		~MapManipulatePole();

		/**
		 * Create a MapInsertVertex instance.
		 */
		static
		const non_null_ptr_type
		create(
				GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
				GPlatesQtWidgets::MapCanvas &map_canvas,
				GPlatesQtWidgets::MapView &map_view,
				GPlatesQtWidgets::ViewportWindow &view_state,
				GPlatesQtWidgets::ModifyReconstructionPoleWidget &pole_widget,
				GPlatesGui::MapTransform &map_transform_)
		{
			return MapManipulatePole::non_null_ptr_type(
					new MapManipulatePole(
							rendered_geometry_collection,
							map_canvas,
							map_view,
							view_state,
							pole_widget,
							map_transform_),
					GPlatesUtils::NullIntrusivePointerHandler());
		}
		
		
		virtual
		void
		handle_activation();


		virtual
		void
		handle_deactivation();


	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		MapManipulatePole(
				GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
				GPlatesQtWidgets::MapCanvas &map_canvas,
				GPlatesQtWidgets::MapView &map_view,
				GPlatesQtWidgets::ViewportWindow &view_state,
				GPlatesQtWidgets::ModifyReconstructionPoleWidget &pole_widget,
				GPlatesGui::MapTransform &map_transform_);

	private:


		/**
		 * Used to set main rendered layer.
		 */
		GPlatesViewOperations::RenderedGeometryCollection *d_rendered_geometry_collection;
		
		/**
		 * This is the view state used to update the viewport window status bar.
		 */
		GPlatesQtWidgets::ViewportWindow *d_view_state_ptr;

		/**
		 * This is the Reconstruction Pole widget in the Task Panel.
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

#endif // GPLATES_CANVASTOOLS_MAPMANIPULATEPOLE_H
