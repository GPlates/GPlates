/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

#ifndef GPLATES_VIEW_OPERATIONS_SCALARFIELD3DRENDERPARAMETERS_H
#define GPLATES_VIEW_OPERATIONS_SCALARFIELD3DRENDERPARAMETERS_H

#include <vector>
#include <boost/optional.hpp>

#include "gui/ColourPalette.h"


namespace GPlatesViewOperations
{
	class ScalarField3DRenderParameters
	{
	public:

		/**
		 * The scalar field rendering mode.
		 */
		enum RenderMode
		{
			RENDER_MODE_ISOSURFACE,
			RENDER_MODE_SINGLE_DEVIATION_WINDOW,
			RENDER_MODE_DOUBLE_DEVIATION_WINDOW,
			RENDER_MODE_CROSS_SECTIONS,

			NUM_RENDER_MODES // This must be last.
		};

		/**
		 * The scalar field colouring mode.
		 */
		enum ColourMode
		{
			COLOUR_MODE_DEPTH,
			COLOUR_MODE_ISOVALUE,
			COLOUR_MODE_GRADIENT,

			NUM_COLOUR_MODES // This must be last.
		};


		/**
		 * Isovalue(s) and associated deviation windows.
		 */
		struct IsovalueParameters
		{
			//! Sets both isovalues the same and deviations to zero and symmetric deviation to true.
			explicit
			IsovalueParameters(
					float isovalue_ = 0);

			float isovalue1;
			float lower_deviation1;
			float upper_deviation1;

			float isovalue2;
			float lower_deviation2;
			float upper_deviation2;

			bool symmetric_deviation;
		};


		/**
		 * Rendering options.
		 */
		struct RenderOptions
		{
			explicit
			RenderOptions(
					float opacity_deviation_surfaces_ = 1,
					bool deviation_window_volume_rendering_ = false,
					float opacity_deviation_window_volume_rendering_ = 1,
					bool surface_deviation_window_ = false,
					unsigned int surface_deviation_window_isoline_frequency_ = 0);

			float opacity_deviation_surfaces;
			bool deviation_window_volume_rendering;
			float opacity_deviation_window_volume_rendering;
			bool surface_deviation_window;
			unsigned int surface_deviation_window_isoline_frequency;
		};


		/**
		 * Surface polygons mask parameters.
		 */
		struct SurfacePolygonsMask
		{
			SurfacePolygonsMask(
					bool treat_polylines_as_polygons_ = false,
					bool show_polygon_walls_ = true,
					bool only_show_boundary_walls_ = true);

			bool treat_polylines_as_polygons;
			bool show_polygon_walls;
			bool only_show_boundary_walls;
		};


		/**
		 * Restricting depth range visualised for scalar field.
		 */
		struct DepthRestriction
		{
			DepthRestriction(
					float min_depth_radius_restriction_ = 0,
					float max_depth_radius_restriction_ = 1);

			float min_depth_radius_restriction;
			float max_depth_radius_restriction;
		};


		/**
		 * Parameters affecting quality/performance tradeoff.
		 */
		struct QualityPerformance
		{
			QualityPerformance(
					unsigned int sampling_rate_ = 20,
					unsigned int bisection_iterations_ = 3);

			unsigned int sampling_rate;
			unsigned int bisection_iterations;
		};


		ScalarField3DRenderParameters();

		ScalarField3DRenderParameters(
				RenderMode render_mode,
				ColourMode colour_mode,
				const GPlatesGui::ColourPalette<double>::non_null_ptr_to_const_type &colour_palette,
				const IsovalueParameters &isovalue_parameters,
				const RenderOptions &render_options,
				const SurfacePolygonsMask &surface_polygons_mask,
				const DepthRestriction &depth_restriction,
				const QualityPerformance &quality_performance,
				const std::vector<float> &shader_test_variables);


		RenderMode
		get_render_mode() const
		{
			return d_render_mode;
		}

		ColourMode
		get_colour_mode() const
		{
			return d_colour_mode;
		}

		/**
		 * A colour palette in the [0,1] range used to colour by scalar value or gradient magnitude.
		 */
		GPlatesGui::ColourPalette<double>::non_null_ptr_to_const_type
		get_colour_palette() const
		{
			return d_colour_palette;
		}

		const IsovalueParameters &
		get_isovalue_parameters() const
		{
			return d_isovalue_parameters;
		}

		const RenderOptions &
		get_render_options() const
		{
			return d_render_options;
		}

		const SurfacePolygonsMask &
		get_surface_polygons_mask() const
		{
			return d_surface_polygons_mask;
		}

		const DepthRestriction &
		get_depth_restriction() const
		{
			return d_depth_restriction;
		}

		const QualityPerformance &
		get_quality_performance() const
		{
			return d_quality_performance;
		}

		/**
		 * A set of arbitrary shader variables.
		 *
		 * This is a temporary solution used during development of scalar field rendering.
		 */
		const std::vector<float> &
		get_shader_test_variables() const
		{
			return d_shader_test_variables;
		}

	private:

		RenderMode d_render_mode;
		ColourMode d_colour_mode;

		/**
		 * The colour palette used to colour the scalar field.
		 */
		GPlatesGui::ColourPalette<double>::non_null_ptr_to_const_type d_colour_palette;

		IsovalueParameters d_isovalue_parameters;

		RenderOptions d_render_options;

		SurfacePolygonsMask d_surface_polygons_mask;

		DepthRestriction d_depth_restriction;

		QualityPerformance d_quality_performance;

		/**
		 * Used during test/development of the scalar field shader program.
		 */
		std::vector<float> d_shader_test_variables;
	};
}

#endif // GPLATES_VIEW_OPERATIONS_SCALARFIELD3DRENDERPARAMETERS_H
