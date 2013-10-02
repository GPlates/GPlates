/* $Id$ */

/**
 * @file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2013 Geological Survey of Norway
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

#ifndef GPLATES_CANVASTOOLS_FITTOPOLE_H
#define GPLATES_CANVASTOOLS_FITTOPOLE_H

#include <QObject>

#include "CanvasTool.h"

#include "view-operations/RenderedGeometryCollection.h"
#include "view-operations/RenderedGeometryVisitor.h"
#include "view-operations/RenderedCrossSymbol.h"
#include "view-operations/RenderedPointOnSphere.h"


// TODO: Check if we need any of these "inherited" includes/forward-declarations.
namespace GPlatesGui
{
	class Colour;
}

namespace GPlatesMaths
{
	class PointOnSphere;
}

namespace GPlatesQtWidgets
{
	class HellingerDialog;
}

namespace GPlatesViewOperations
{
	class RenderedGeometryLayer;
}

namespace GPlatesCanvasTools
{
	/**
	 * Canvas tool used for fitting points to a rotation pole.
	 */
	class FitToPole :
			public QObject,
			public CanvasTool
	{
		Q_OBJECT

	public:

		/**
		 * Visitor to find a rendered geometry's point-on-sphere, if it has one.
		 */
		class GeometryFinder:
			public GPlatesViewOperations::ConstRenderedGeometryVisitor
		{
		public:

			GeometryFinder()
			{  }

			virtual
			void
			visit_rendered_point_on_sphere(
				const GPlatesViewOperations::RenderedPointOnSphere &rendered_point_on_sphere)
			{
				d_geometry.reset(
					rendered_point_on_sphere.get_point_on_sphere().get_non_null_pointer());
			}


			virtual
			void
			visit_rendered_cross_symbol(
					const GPlatesViewOperations::RenderedCrossSymbol &rendered_cross_symbol)
			{
				d_geometry.reset(
							rendered_cross_symbol.get_centre().get_non_null_pointer());
			}

			boost::optional<GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type>
			get_geometry()
			{
				return d_geometry;
			}

		private:

			boost::optional<GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type>
				d_geometry;

		};




		/**
		 * Convenience typedef for GPlatesUtils::non_null_intrusive_ptr<FitToPole>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<FitToPole> non_null_ptr_type;

		static
		const non_null_ptr_type
		create(
				const status_bar_callback_type &status_bar_callback,
				GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
				GPlatesViewOperations::RenderedGeometryCollection::MainLayerType main_rendered_layer_type,
				GPlatesQtWidgets::HellingerDialog &hellinger_dialog)
		{
			return new FitToPole(
					status_bar_callback,
					rendered_geom_collection,
					main_rendered_layer_type,
					hellinger_dialog);
		}

		virtual
		void
		handle_activation();


		virtual
		void
		handle_deactivation();

		virtual
		void
		handle_left_click(
				const GPlatesMaths::PointOnSphere &point_on_sphere,
				bool is_on_earth,
				double proximity_inclusion_threshold);

		virtual
		void
		handle_move_without_drag(	
				const GPlatesMaths::PointOnSphere &point_on_sphere,
				bool is_on_earth,
				double proximity_inclusion_threshold);

		virtual
		void
		handle_shift_left_click(
				const GPlatesMaths::PointOnSphere &point_on_sphere,
				bool is_on_earth,
				double proximity_inclusion_threshold);

		virtual
		void
		handle_left_drag(
				const GPlatesMaths::PointOnSphere &initial_point_on_sphere,
				bool was_on_earth,
				double initial_proximity_inclusion_threshold,
				const GPlatesMaths::PointOnSphere &current_point_on_sphere,
				bool is_on_earth, double current_proximity_inclusion_threshold,
				const boost::optional<GPlatesMaths::PointOnSphere> &centre_of_viewport);

		virtual
		void
		handle_left_release_after_drag(
				const GPlatesMaths::PointOnSphere &initial_point_on_sphere,
				bool was_on_earth,
				double initial_proximity_inclusion_threshold,
				const GPlatesMaths::PointOnSphere &current_point_on_sphere,
				bool is_on_earth,
				double current_proximity_inclusion_threshold,
				const boost::optional<GPlatesMaths::PointOnSphere> &centre_of_viewport);

		virtual
		void
		handle_left_press(
				const GPlatesMaths::PointOnSphere &point_on_sphere,
				bool is_on_earth,
				double proximity_inclusion_threshold);

	private:

		void
		paint();

		FitToPole(
				const status_bar_callback_type &status_bar_callback,
				GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
				GPlatesViewOperations::RenderedGeometryCollection::MainLayerType main_rendered_layer_type,
				GPlatesQtWidgets::HellingerDialog &hellinger_dialog);

		//! For rendering purposes
		GPlatesViewOperations::RenderedGeometryCollection *d_rendered_geom_collection_ptr;

		GPlatesQtWidgets::HellingerDialog *d_hellinger_dialog_ptr;

		bool d_mouse_is_over_editable_pick;
		bool d_mouse_is_over_selectable_pick;
		bool d_pick_is_being_dragged;
	};
}

#endif  // GPLATES_CANVASTOOLS_FITTOPOLE_H
