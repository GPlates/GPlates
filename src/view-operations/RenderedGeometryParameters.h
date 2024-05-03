/* $Id$ */

/**
 * \file 
 * Contains parameter values used when created @a RenderedGeometry objects.
 * $Revision$
 * $Date$
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

#ifndef GPLATES_VIEWOPERATIONS_RENDEREDGEOMETRYPARAMETERS_H
#define GPLATES_VIEWOPERATIONS_RENDEREDGEOMETRYPARAMETERS_H

#include <QObject>

#include "RenderedGeometryFactory.h"

#include "gui/Colour.h"

#include "maths/MathsUtils.h"


namespace GPlatesViewOperations
{
	/**
	 * Parameters that specify how to draw geometry in the various canvas tools, and also some aspects
	 * (not covered by symboling/colouring/etc) of drawing the main reconstruction rendered layer.
	 */
	class RenderedGeometryParameters :
			public QObject
	{
		Q_OBJECT

	public:

		//! Constructor sets the default parameter values.
		RenderedGeometryParameters() :
			d_reconstruction_layer_point_size_hint(4.0f),
			d_reconstruction_layer_line_width_hint(1.5f),
			d_reconstruction_layer_topology_size_multiplier(1.0f),
			d_reconstruction_layer_ratio_arrow_unit_vector_direction_to_globe_radius(0.05f),
			d_reconstruction_layer_ratio_arrowhead_size_to_globe_radius(
					RenderedGeometryFactory::DEFAULT_RATIO_ARROWHEAD_SIZE_TO_GLOBE_RADIUS),
			d_reconstruction_layer_arrow_spacing(0.175f),
			d_choose_feature_tool_point_size_hint(4.0f),
			d_choose_feature_tool_line_width_hint(2.5f),
			d_choose_feature_tool_clicked_geometry_of_focused_feature_colour(GPlatesGui::Colour::get_white()),
			d_choose_feature_tool_non_clicked_geometry_of_focused_feature_colour(GPlatesGui::Colour::get_grey()),
			d_topology_tool_focused_geometry_colour(GPlatesGui::Colour::get_white()),
			d_topology_tool_focused_geometry_point_size_hint(4.0f),
			d_topology_tool_focused_geometry_line_width_hint(2.5f),
			d_topology_tool_topological_sections_colour(GPlatesGui::Colour(0.05f, 0.05f, 0.05f, 1.0f)), // dark grey
			d_topology_tool_topological_sections_point_size_hint(4.0f),
			d_topology_tool_topological_sections_line_width_hint(2.5f)
		{  }


		//! Point size for reconstruction layer.
		float
		get_reconstruction_layer_point_size_hint() const
		{
			return d_reconstruction_layer_point_size_hint;
		}

		void
		set_reconstruction_layer_point_size_hint(
				float reconstruction_layer_point_size_hint)
		{
			d_reconstruction_layer_point_size_hint = reconstruction_layer_point_size_hint;
			Q_EMIT parameters_changed(*this);
		}


		//! Line width for reconstruction layer.
		float
		get_reconstruction_layer_line_width_hint() const
		{
			return d_reconstruction_layer_line_width_hint;
		}

		void
		set_reconstruction_layer_line_width_hint(
				float reconstruction_layer_line_width_hint)
		{
			d_reconstruction_layer_line_width_hint = reconstruction_layer_line_width_hint;
			Q_EMIT parameters_changed(*this);
		}

		//! Line width for topologies in reconstruction layer.
		float
		get_reconstruction_layer_topology_size_multiplier() const
		{
			return d_reconstruction_layer_topology_size_multiplier;
		}

		void
		set_reconstruction_layer_topology_size_multiplier(
				float reconstruction_layer_topology_size_multiplier)
		{
			d_reconstruction_layer_topology_size_multiplier = reconstruction_layer_topology_size_multiplier;
			Q_EMIT parameters_changed(*this);
		}


		//! Scaling for arrow bodies in reconstruction layer.
		float
		get_reconstruction_layer_ratio_arrow_unit_vector_direction_to_globe_radius() const
		{
			return d_reconstruction_layer_ratio_arrow_unit_vector_direction_to_globe_radius;
		}

		void
		set_reconstruction_layer_ratio_arrow_unit_vector_direction_to_globe_radius(
				float reconstruction_layer_ratio_arrow_unit_vector_direction_to_globe_radius)
		{
			d_reconstruction_layer_ratio_arrow_unit_vector_direction_to_globe_radius =
					reconstruction_layer_ratio_arrow_unit_vector_direction_to_globe_radius;
			Q_EMIT parameters_changed(*this);
		}


		//! Scaling for arrowheads in reconstruction layer.
		float
		get_reconstruction_layer_ratio_arrowhead_size_to_globe_radius() const
		{
			return d_reconstruction_layer_ratio_arrowhead_size_to_globe_radius;
		}

		void
		set_reconstruction_layer_ratio_arrowhead_size_to_globe_radius(
				float reconstruction_layer_ratio_arrowhead_size_to_globe_radius)
		{
			d_reconstruction_layer_ratio_arrowhead_size_to_globe_radius =
					reconstruction_layer_ratio_arrowhead_size_to_globe_radius;
			Q_EMIT parameters_changed(*this);
		}


		//! The screen-space spacing of rendered arrows in reconstruction layer.
		float
		get_reconstruction_layer_arrow_spacing() const
		{
			return d_reconstruction_layer_arrow_spacing;
		}

		void
		set_reconstruction_layer_arrow_spacing(
				float reconstruction_layer_arrow_spacing)
		{
			d_reconstruction_layer_arrow_spacing = reconstruction_layer_arrow_spacing;
			Q_EMIT parameters_changed(*this);
		}


		//! Point size for rendering the actual focus geometry clicked by user.
		float
		get_choose_feature_tool_point_size_hint() const
		{
			return d_choose_feature_tool_point_size_hint;
		}

		void
		set_choose_feature_tool_point_size_hint(
				float point_size)
		{
			d_choose_feature_tool_point_size_hint = point_size;
			Q_EMIT parameters_changed(*this);
		}


		//! Line width for rendering the actual focus geometry clicked by user.
		float
		get_choose_feature_tool_line_width_hint() const
		{
			return d_choose_feature_tool_line_width_hint;
		}

		void
		set_choose_feature_tool_line_width_hint(
				float line_width)
		{
			d_choose_feature_tool_line_width_hint = line_width;
			Q_EMIT parameters_changed(*this);
		}


		/**
		 * Colour to use for rendering the actual focus geometry clicked by user.
		 *
		 * Since there can be multiple geometry properties associated with a single feature
		 * only one of them (the clicked geometry) gets rendered in this colour.
		 */
		const GPlatesGui::Colour &
		get_choose_feature_tool_clicked_geometry_of_focused_feature_colour() const
		{
			return d_choose_feature_tool_clicked_geometry_of_focused_feature_colour;
		}

		void
		set_choose_feature_tool_clicked_geometry_of_focused_feature_colour(
				const GPlatesGui::Colour &colour)
		{
			d_choose_feature_tool_clicked_geometry_of_focused_feature_colour = colour;
			Q_EMIT parameters_changed(*this);
		}


		/**
		 * Colour to use for rendering the geometries of focused feature that the user did not click on.
		 *
		 * When the user clicks on a geometry it focuses the feature that the geometry
		 * belongs to. If there are other geometries associated with that feature then
		 * they will get rendered in this colour.
		 */
		const GPlatesGui::Colour &
		get_choose_feature_tool_non_clicked_geometry_of_focused_feature_colour() const
		{
			return d_choose_feature_tool_non_clicked_geometry_of_focused_feature_colour;
		}

		void
		set_choose_feature_tool_non_clicked_geometry_of_focused_feature_colour(
				const GPlatesGui::Colour &colour)
		{
			d_choose_feature_tool_non_clicked_geometry_of_focused_feature_colour = colour;
			Q_EMIT parameters_changed(*this);
		}


		//! Colour for rendering focus geometry in topology tools.
		const GPlatesGui::Colour &
		get_topology_tool_focused_geometry_colour() const
		{
			return d_topology_tool_focused_geometry_colour;
		}

		void
		set_topology_tool_focused_geometry_colour(
				const GPlatesGui::Colour &colour)
		{
			d_topology_tool_focused_geometry_colour = colour;
			Q_EMIT parameters_changed(*this);
		}


		//! Point size for rendering focus geometry in topology tools.
		float
		get_topology_tool_focused_geometry_point_size_hint() const
		{
			return d_topology_tool_focused_geometry_point_size_hint;
		}

		void
		set_topology_tool_focused_geometry_point_size_hint(
				float point_size_hint)
		{
			d_topology_tool_focused_geometry_point_size_hint = point_size_hint;
			Q_EMIT parameters_changed(*this);
		}


		//! Line width for rendering focus geometry in topology tools.
		float
		get_topology_tool_focused_geometry_line_width_hint() const
		{
			return d_topology_tool_focused_geometry_line_width_hint;
		}

		void
		set_topology_tool_focused_geometry_line_width_hint(
				float line_width_hint)
		{
			d_topology_tool_focused_geometry_line_width_hint = line_width_hint;
			Q_EMIT parameters_changed(*this);
		}


		//! Colour for rendering topological sections in topology tools.
		const GPlatesGui::Colour &
		get_topology_tool_topological_sections_colour() const
		{
			return d_topology_tool_topological_sections_colour;
		}

		void
		set_topology_tool_topological_sections_colour(
				const GPlatesGui::Colour &colour)
		{
			d_topology_tool_topological_sections_colour = colour;
			Q_EMIT parameters_changed(*this);
		}


		//! Point size for rendering topological sections in topology tools.
		float
		get_topology_tool_topological_sections_point_size_hint() const
		{
			return d_topology_tool_topological_sections_point_size_hint;
		}

		void
		set_topology_tool_topological_sections_point_size_hint(
				float point_size_hint)
		{
			d_topology_tool_topological_sections_point_size_hint = point_size_hint;
			Q_EMIT parameters_changed(*this);
		}


		//! Line width for rendering topological sections in topology tools.
		float
		get_topology_tool_topological_sections_line_width_hint() const
		{
			return d_topology_tool_topological_sections_line_width_hint;
		}

		void
		set_topology_tool_topological_sections_line_width_hint(
				float line_width_hint)
		{
			d_topology_tool_topological_sections_line_width_hint = line_width_hint;
			Q_EMIT parameters_changed(*this);
		}

	Q_SIGNALS:

		void
		parameters_changed(
				GPlatesViewOperations::RenderedGeometryParameters &);

	private:

		float d_reconstruction_layer_point_size_hint;
		float d_reconstruction_layer_line_width_hint;
		float d_reconstruction_layer_topology_size_multiplier;
		float d_reconstruction_layer_ratio_arrow_unit_vector_direction_to_globe_radius;
		float d_reconstruction_layer_ratio_arrowhead_size_to_globe_radius;
		float d_reconstruction_layer_arrow_spacing;

		float d_choose_feature_tool_point_size_hint;
		float d_choose_feature_tool_line_width_hint;
		GPlatesGui::Colour d_choose_feature_tool_clicked_geometry_of_focused_feature_colour;
		GPlatesGui::Colour d_choose_feature_tool_non_clicked_geometry_of_focused_feature_colour;

		GPlatesGui::Colour d_topology_tool_focused_geometry_colour;
		float d_topology_tool_focused_geometry_point_size_hint;
		float d_topology_tool_focused_geometry_line_width_hint;
		GPlatesGui::Colour d_topology_tool_topological_sections_colour;
		float d_topology_tool_topological_sections_point_size_hint;
		float d_topology_tool_topological_sections_line_width_hint;

	};
}

namespace GPlatesViewOperations
{
	/**
	 * Parameters that specify how to draw geometry for the different rendered layers.
	 */
	namespace RenderedLayerParameters
	{
		/**
		 * Default point size hint used by most (or all) layers.
		 */
		const float DEFAULT_POINT_SIZE_HINT = 4.0f;

		/**
		 * Default line width hint used by most (or all) layers.
		 */
		const float DEFAULT_LINE_WIDTH_HINT = 1.5f;

		//! Point size for reconstruction layer.
		const float POLE_MANIPULATION_POINT_SIZE_HINT = DEFAULT_POINT_SIZE_HINT;

		//! Line width for reconstruction layer.
		const float POLE_MANIPULATION_LINE_WIDTH_HINT = 1.5f;

		//! Line width for reconstruction layer.
		const float TOPOLOGY_TOOL_LINE_WIDTH_HINT = 4.0f;
	}

	/**
	 * Parameters that specify how geometry operations should draw geometry.
	 */
	namespace GeometryOperationParameters
	{
		/////////////////
		// Line widths //
		/////////////////

		/**
		 * Width of lines to render in the most general case.
		 */
		const float LINE_WIDTH_HINT = 2.5f;

		/**
		 * Width of lines for rendering those parts of geometry that need highlighting
		 * to indicate, to the user, that an operation is possible.
		 */
		const float HIGHLIGHT_LINE_WIDTH_HINT = 3.0f;
		
	
		//! Line width for move-vertex secondary geometries.
		const float SECONDARY_LINE_WIDTH_HINT = 2.0f;

		/////////////////
		// Point sizes //
		/////////////////

		/**
		 * Regular size of point to render at each point/vertex.
		 * Used when it is not desired to have the point/vertex stick out.
		 */
		const float REGULAR_POINT_SIZE_HINT = 2.0f;

		/**
		 * Large size of point to render at each point/vertex.
		 * Used to make point/vertex more visible or to emphasise it.
		 */
		const float LARGE_POINT_SIZE_HINT = 4.0f;

		/**
		 * Extra large size of point to render at each point/vertex.
		 * Used to make point/vertex even more visible or to emphasise it even more.
		 */
		const float EXTRA_LARGE_POINT_SIZE_HINT = 8.0f;

		/////////////
		// Colours //
		/////////////

		/**
		 * Colour to use for rendering those parts of geometry that are in focus.
		 */
		const GPlatesGui::Colour FOCUS_COLOUR = GPlatesGui::Colour::get_white();

		/**
		 * Colour to be used for rendering points for "split feature" tool.
		 */
		const GPlatesGui::Colour SPLIT_FEATURE_START_POINT_COLOUR = 
				GPlatesGui::Colour::get_green();
		const GPlatesGui::Colour SPLIT_FEATURE_MIDDLE_POINT_COLOUR = 
				GPlatesGui::Colour::get_yellow();
		const GPlatesGui::Colour SPLIT_FEATURE_END_POINT_COLOUR = 
				GPlatesGui::Colour::get_red();

		/**
		 * Colour to use for rendering those parts of geometry that are not in focus.
		 */
		const GPlatesGui::Colour NOT_IN_FOCUS_COLOUR = GPlatesGui::Colour::get_grey();

		/**
		 * Colour to use for rendering those parts of geometry that need highlighting
		 * to indicate, to the user, that an operation is possible.
		 */
		const GPlatesGui::Colour HIGHLIGHT_COLOUR = GPlatesGui::Colour::get_yellow();

		/**
		 * Colour to use for rendering those parts of geometry that can be deleted.
		 */
		const GPlatesGui::Colour DELETE_COLOUR = GPlatesGui::Colour::get_red();
	}
}

#endif // GPLATES_VIEWOPERATIONS_RENDEREDGEOMETRYPARAMETERS_H
