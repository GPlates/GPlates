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

#ifndef GPLATES_GUI_GEOMETRYFOCUSHIGHLIGHT_H
#define GPLATES_GUI_GEOMETRYFOCUSHIGHLIGHT_H

#include <QObject>
#include <vector>
#include <boost/optional.hpp>

#include "app-logic/ReconstructedFeatureGeometry.h"
#include "gui/Symbol.h"
#include "model/FeatureHandle.h"


namespace GPlatesViewOperations
{
	class RenderedGeometryLayer;
	class RenderedGeometryCollection;
}

namespace GPlatesGui
{
	class FeatureFocus;

	/**
	 * This class is used to control the highlighting of the currently-focused reconstruction
	 * geometry.
	 */
	class GeometryFocusHighlight:
			public QObject
	{
		Q_OBJECT
	public:

		GeometryFocusHighlight(
			GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
			const GPlatesGui::symbol_map_type &symbol_map);

		virtual
		~GeometryFocusHighlight()
		{  }

	public slots:

		/**
		 * Change which reconstruction geometry is currently focused, also specifying an
		 * (optional) weak-ref to the feature which contains the geometry whose RG is the
		 * currently-focused reconstruction geometry.
		 *
		 * The counter-intuitive ordering of the arguments (feature first, followed by the
		 * reconstruction geometry, when the geometry is the "main" argument) is so that
		 * this slot matches the @a focus_changed signal emitted by FeatureFocus.
		 */
		void
		set_focus(
				GPlatesGui::FeatureFocus &feature_focus);

		/**
		 * Draw the focused geometry (if there is one) on the screen.
		 */
		void
		draw_focused_geometry();

	private:
		/**
		 * The feature which contains the geometry whose RFG is the currently-focused
		 * reconstruction geometry.
		 *
		 * Note that there might not be any such feature, in which case this would be an
		 * invalid weak-ref.
		 */
		GPlatesModel::FeatureHandle::weak_ref d_feature;

		/**
		 * The reconstruction geometry which is focused.
		 *
		 * Note that there may not be a focused reconstruction geometry, in which case this
		 * would be a null pointer.
		 */
		GPlatesAppLogic::ReconstructionGeometry::maybe_null_ptr_to_const_type d_focused_geometry;

		/**
		 * Used to find the visible reconstruction geometries as a short-list for searching
		 * for new reconstruction geometries for a new reconstruction because we don't want to
		 * pick up any old or invisible reconstruction geometries.
		 */
		GPlatesViewOperations::RenderedGeometryCollection &d_rendered_geom_collection;

		/**
		 * The layer of rendered geometries which is used for highlighting.
		 */
		GPlatesViewOperations::RenderedGeometryLayer *d_geometry_focus_highlight_layer_ptr;

		/**
		 * A reference to the viewstate's feature-type-to-symbol-map
		 */
		const GPlatesGui::symbol_map_type &d_symbol_map;
	};
}

#endif // GPLATES_GUI_GEOMETRYFOCUSHIGHLIGHT_H
