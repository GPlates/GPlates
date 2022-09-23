/* $Id$ */

/**
 * @file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2014 Geological Survey of Norway
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

#ifndef GPLATES_CANVASTOOLS_ADJUSTFITTEDPOLEESTIMATE_H
#define GPLATES_CANVASTOOLS_ADJUSTFITTEDPOLEESTIMATE_H

#include <QObject>

#include "boost/optional.hpp"

#include "CanvasTool.h"

#include "maths/GeometryOnSphere.h"
#include "maths/GreatCircleArc.h"
#include "maths/MultiPointOnSphere.h"

#include "view-operations/RenderedArrow.h"
#include "view-operations/RenderedCircleSymbol.h"
#include "view-operations/RenderedCrossSymbol.h"
#include "view-operations/RenderedGeometryCollection.h"
#include "view-operations/RenderedGeometryVisitor.h"
#include "view-operations/RenderedMultiPointOnSphere.h"
#include "view-operations/RenderedPointOnSphere.h"
#include "view-operations/RenderedPolylineOnSphere.h"
#include "view-operations/RenderedSquareSymbol.h"
#include "view-operations/RenderedStrainMarkerSymbol.h"
#include "view-operations/RenderedTriangleSymbol.h"


namespace GPlatesMaths
{
	class PointOnSphere;
}

namespace GPlatesQtWidgets
{
	class HellingerDialog;
}

namespace GPlatesCanvasTools
{
	/**
	 * Canvas tool used for adjusting the initial pole estimates for the hellinger tool.
	 */
	class AdjustFittedPoleEstimate :
			public QObject,
			public CanvasTool
	{
		Q_OBJECT

	public:

		enum ActivePoleType{
			PLATES_1_2_POLE_TYPE,
			PLATES_1_3_POLE_TYPE,

			NO_ACTIVE_POLE_TYPE
		};

		/**
		 * Visitor to find a rendered geometry's underlying geometry-on-sphere, if it has one.
		 * TODO: this class has been copied from SelectHellingerGeometry tool; we may want to
		 * put it somewhere accessible by both tools.
		 *
		 * There are several variations of GeometryFinders in different parts of GPlates, with
		 * subtly different modes of use - I'm sure there was a reason for me making a new one
		 * here (and in SelectHellingerGeometry....), but TODO: check if we can use existing
		 * finders.
		 */
		class GeometryFinder:
			public GPlatesViewOperations::ConstRenderedGeometryVisitor
		{
		public:

			GeometryFinder(
					boost::optional<unsigned int> vertex_index):
				d_vertex_index(vertex_index)
			{  }


			virtual
			void
			visit_rendered_arrow(
					const GPlatesViewOperations::RenderedArrow &rendered_arrow)
			{
				d_geometry.reset(
						rendered_arrow.get_start_position().get_geometry_on_sphere());
			}

			virtual
			void
			visit_rendered_point_on_sphere(
				const GPlatesViewOperations::RenderedPointOnSphere &rendered_point_on_sphere)
			{
				d_geometry.reset(
						rendered_point_on_sphere.get_point_on_sphere().get_geometry_on_sphere());
			}

			virtual
			void
			visit_rendered_multi_point_on_sphere(
				const GPlatesViewOperations::RenderedMultiPointOnSphere &rendered_multi_point_on_sphere)
			{
				if (d_vertex_index)
				{
					if (*d_vertex_index >= rendered_multi_point_on_sphere.get_multi_point_on_sphere()->number_of_points())
					{
						return;
					}

					GPlatesMaths::MultiPointOnSphere::const_iterator
						it = rendered_multi_point_on_sphere.get_multi_point_on_sphere()->begin();
					std::advance(it,*d_vertex_index);

					d_geometry.reset(it->get_geometry_on_sphere());
				}
			}

			virtual
			void
			visit_rendered_circle_symbol(const GPlatesViewOperations::RenderedCircleSymbol &rendered_circle_symbol)
			{
				d_geometry.reset(
							rendered_circle_symbol.get_centre().get_geometry_on_sphere());
			}

			virtual
			void
			visit_rendered_cross_symbol(
					const GPlatesViewOperations::RenderedCrossSymbol &rendered_cross_symbol)
			{
				d_geometry.reset(
							rendered_cross_symbol.get_centre().get_geometry_on_sphere());
			}

			virtual
			void
			visit_rendered_square_symbol(
					const GPlatesViewOperations::RenderedSquareSymbol &rendered_square_symbol)
			{
				d_geometry.reset(
							rendered_square_symbol.get_centre().get_geometry_on_sphere());
			}

			virtual
			void
			visit_rendered_triangle_symbol(
					const GPlatesViewOperations::RenderedTriangleSymbol &rendered_triangle_symbol)
			{
				d_geometry.reset(
							rendered_triangle_symbol.get_centre().get_geometry_on_sphere());
			}

			virtual
			void
			visit_rendered_polyline_on_sphere(
					const GPlatesViewOperations::RenderedPolylineOnSphere &rendered_polyline)
			{
				d_geometry.reset(
							rendered_polyline.get_polyline_on_sphere());
			}

			boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>
			get_geometry()
			{
				return d_geometry;
			}

		private:

			boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>
				d_geometry;

			boost::optional<unsigned int> d_vertex_index;

		};


		//! Convenience typedef for GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type
		typedef GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type child_layer_ptr_type;


		static
		const non_null_ptr_type
		create(
				const status_bar_callback_type &status_bar_callback,
				GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
				GPlatesViewOperations::RenderedGeometryCollection::MainLayerType main_rendered_layer_type,
				GPlatesQtWidgets::HellingerDialog &hellinger_dialog)
		{
			return new AdjustFittedPoleEstimate(
					status_bar_callback,
					rendered_geom_collection,
					main_rendered_layer_type,
					hellinger_dialog);
		}

		virtual
		void
		handle_activation() override;


		virtual
		void
		handle_deactivation() override;

		virtual
		void
		handle_move_without_drag(	
				const GPlatesMaths::PointOnSphere &point_on_sphere,
				bool is_on_earth,
				double proximity_inclusion_threshold) override;


		virtual
		void
		handle_left_drag(
				const GPlatesMaths::PointOnSphere &initial_point_on_sphere,
				bool was_on_earth,
				double initial_proximity_inclusion_threshold,
				const GPlatesMaths::PointOnSphere &current_point_on_sphere,
				bool is_on_earth, double current_proximity_inclusion_threshold,
				const GPlatesMaths::PointOnSphere &centre_of_viewport) override;

		virtual
		void
		handle_left_release_after_drag(
				const GPlatesMaths::PointOnSphere &initial_point_on_sphere,
				bool was_on_earth,
				double initial_proximity_inclusion_threshold,
				const GPlatesMaths::PointOnSphere &current_point_on_sphere,
				bool is_on_earth,
				double current_proximity_inclusion_threshold,
				const GPlatesMaths::PointOnSphere &centre_of_viewport) override;

		virtual
		void
		handle_left_press(
				const GPlatesMaths::PointOnSphere &point_on_sphere,
				bool is_on_earth,
				double proximity_inclusion_threshold) override;



	private Q_SLOTS:

		void
		handle_pole_estimate_12_lat_lon_changed(
				double lat,
				double lon);

		void
		handle_pole_estimate_12_angle_changed(
				double angle);

		void
		handle_pole_estimate_13_lat_lon_changed(
				double lat,
				double lon);

		void
		handle_pole_estimate_13_angle_changed(
				double angle);


	private:


		// This enum is used in keeping track of which geometry in the pole_estimate_layer we're hovered over.
		enum GeometryTypeIndex
		{
			POLE_12_GEOMETRY_INDEX,
			REFERENCE_ARC_ENDPOINT_12_GEOMETRY_INDEX,
			RELATIVE_ARC_ENDPOINT_12_GEOMETRY_INDEX,
			REFERENCE_ARC_12_GEOMETRY_INDEX,
			RELATIVE_ARC_12_GEOMETRY_INDEX,

			POLE_13_GEOMETRY_INDEX,
			REFERENCE_ARC_ENDPOINT_13_GEOMETRY_INDEX,
			RELATIVE_ARC_ENDPOINT_13_GEOMETRY_INDEX,
			REFERENCE_ARC_13_GEOMETRY_INDEX,
			RELATIVE_ARC_13_GEOMETRY_INDEX,
		};

		void
		update_local_values_from_hellinger_dialog();

		void
		update_hellinger_dialog_from_local_values();

		void
		update_current_pole_arrow_layer();

		void
		update_current_pole_and_angle_layer();

		void
		update_pole_estimate_and_arc_highlight(
				const GPlatesMaths::PointOnSphere &pole,
				const GPlatesMaths::PointOnSphere &reference_arc_end_point,
				const GPlatesMaths::PointOnSphere &relative_arc_end_point);

		void
		update_arc_and_end_point_highlight(
				const GPlatesMaths::PointOnSphere &end_point,
				const GPlatesMaths::PointOnSphere &pole);

		void
		update_angle();


		void
		paint();

		bool
		mouse_is_over_a_highlight_geometry();

		AdjustFittedPoleEstimate(
				const status_bar_callback_type &status_bar_callback,
				GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
				GPlatesViewOperations::RenderedGeometryCollection::MainLayerType main_rendered_layer_type,
				GPlatesQtWidgets::HellingerDialog &hellinger_dialog);

		//! For rendering purposes
		GPlatesViewOperations::RenderedGeometryCollection *d_rendered_geom_collection_ptr;

		GPlatesQtWidgets::HellingerDialog *d_hellinger_dialog_ptr;

		bool d_mouse_is_over_pole_estimate;
		bool d_pole_is_being_dragged;
		bool d_mouse_is_over_reference_arc;
		bool d_reference_arc_is_being_draggged;
		bool d_mouse_is_over_reference_arc_end_point;
		bool d_reference_arc_end_point_is_being_dragged;
		bool d_mouse_is_over_relative_arc;
		bool d_relative_arc_is_being_dragged;
		bool d_mouse_is_over_relative_arc_end_point;
		bool d_relative_arc_end_point_is_being_dragged;

		/**
		 * @brief d_pole_arrow_layer_ptr
		 * layer for drawing the current pole arrow
		 */
		child_layer_ptr_type d_current_pole_arrow_layer_ptr;

		/**
		 * @brief d_current_pole_and_angle_layer_ptr
		 * layer for drawing the vertices arcs of the current pole and angle
		 */
		child_layer_ptr_type d_current_pole_and_angle_layer_ptr;

		/**
		 * @brief d_highlight_layer_ptr
		 * layer for highlighting whichever geometry (pole, reference-arc,or relative-arc) is
		 * hovered over and hence draggable / adjustable.
		 */
		child_layer_ptr_type d_highlight_layer_ptr;

		// Coordinates, angles etc of geometries related to the initial pole estimates.
		// "12" denotes variables associated with the pole representing the rotation between
		// plate indices 1 and 2
		// "13" denotes those related to plate indices 1 and 3.
		GPlatesMaths::PointOnSphere d_current_pole_12;
		double d_current_angle_12;
		GPlatesMaths::PointOnSphere d_end_point_of_reference_arc_12;
		GPlatesMaths::PointOnSphere d_end_point_of_relative_arc_12;

		GPlatesMaths::PointOnSphere d_current_pole_13;
		double d_current_angle_13;
		GPlatesMaths::PointOnSphere d_end_point_of_reference_arc_13;
		GPlatesMaths::PointOnSphere d_end_point_of_relative_arc_13;

		bool d_has_been_activated;

		/**
		 * @brief d_active_pole_type - the pole type which is currently or most recently
		 * selected/highlighted/dragged.
		 */
		ActivePoleType d_active_pole_type;

	};
}

#endif  // GPLATES_CANVASTOOLS_ADJUSTFITTEDPOLEESTIMATE_H
