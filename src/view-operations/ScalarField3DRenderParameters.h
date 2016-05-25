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

#include <utility>
#include <vector>
#include <boost/optional.hpp>
#include <QString>

#include "presentation/RemappedColourPaletteParameters.h"


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
			RENDER_MODE_CROSS_SECTIONS,

			NUM_RENDER_MODES // This must be last.
		};

		/**
		 * The isosurface deviation window mode.
		 */
		enum IsosurfaceDeviationWindowMode
		{
			ISOSURFACE_DEVIATION_WINDOW_MODE_NONE,
			ISOSURFACE_DEVIATION_WINDOW_MODE_SINGLE,
			ISOSURFACE_DEVIATION_WINDOW_MODE_DOUBLE,

			NUM_ISOSURFACE_DEVIATION_WINDOW_MODES // This must be last.
		};

		/**
		 * The isosurface colouring mode.
		 */
		enum IsosurfaceColourMode
		{
			ISOSURFACE_COLOUR_MODE_DEPTH,
			ISOSURFACE_COLOUR_MODE_SCALAR,
			ISOSURFACE_COLOUR_MODE_GRADIENT,

			NUM_ISOSURFACE_COLOUR_MODES // This must be last.
		};

		/**
		 * The cross-sections colouring mode.
		 */
		enum CrossSectionColourMode
		{
			CROSS_SECTION_COLOUR_MODE_SCALAR,
			CROSS_SECTION_COLOUR_MODE_GRADIENT,

			NUM_CROSS_SECTION_COLOUR_MODES // This must be last.
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
		 * Deviation window render options.
		 */
		struct DeviationWindowRenderOptions
		{
			explicit
			DeviationWindowRenderOptions(
					float opacity_deviation_surfaces_ = 0.5f,
					bool deviation_window_volume_rendering_ = false,
					float opacity_deviation_window_volume_rendering_ = 0.5f,
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
			/**
			 * The default disables the polygons mask and does not treat polylines as polygons and
			 * shows walls (but only boundary walls).
			 */
			SurfacePolygonsMask(
					bool enable_surface_polygons_mask_ = false,
					bool treat_polylines_as_polygons_ = false,
					bool show_polygon_walls_ = true,
					bool only_show_boundary_walls_ = true);

			bool enable_surface_polygons_mask;
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
		 * Parameters affecting quality/performance trade-off.
		 */
		struct QualityPerformance
		{
			QualityPerformance(
					unsigned int sampling_rate_ = 50,
					unsigned int bisection_iterations_ = 5,
					bool enable_reduce_rate_during_drag_globe_ = false,
					unsigned int reduce_rate_during_drag_globe_ = 2);

			unsigned int sampling_rate;
			unsigned int bisection_iterations;
			// Enable improved performance (at cost of quality) at certain times such as
			// during globe rotation when the mouse is dragged...
			bool enable_reduce_rate_during_drag_globe;
			unsigned int reduce_rate_during_drag_globe;
		};


		ScalarField3DRenderParameters();

		ScalarField3DRenderParameters(
				RenderMode render_mode,
				IsosurfaceDeviationWindowMode isosurface_deviation_window_mode,
				IsosurfaceColourMode isosurface_colour_mode,
				CrossSectionColourMode cross_section_colour_mode,
				const GPlatesPresentation::RemappedColourPaletteParameters &scalar_colour_palette_parameters,
				const GPlatesPresentation::RemappedColourPaletteParameters &gradient_colour_palette_parameters,
				const IsovalueParameters &isovalue_parameters,
				const DeviationWindowRenderOptions &deviation_window_render_options,
				const SurfacePolygonsMask &surface_polygons_mask,
				const DepthRestriction &depth_restriction,
				const QualityPerformance &quality_performance,
				const std::vector<float> &shader_test_variables);


		RenderMode
		get_render_mode() const
		{
			return d_render_mode;
		}

		void
		set_render_mode(
				RenderMode render_mode)
		{
			d_render_mode = render_mode;
		}

		IsosurfaceDeviationWindowMode
		get_isosurface_deviation_window_mode() const
		{
			return d_isosurface_deviation_window_mode;
		}

		void
		set_isosurface_deviation_window_mode(
				IsosurfaceDeviationWindowMode isosurface_deviation_window_mode)
		{
			d_isosurface_deviation_window_mode = isosurface_deviation_window_mode;
		}

		IsosurfaceColourMode
		get_isosurface_colour_mode() const
		{
			return d_isosurface_colour_mode;
		}

		void
		set_isosurface_colour_mode(
				IsosurfaceColourMode isosurface_colour_mode)
		{
			d_isosurface_colour_mode = isosurface_colour_mode;
		}

		CrossSectionColourMode
		get_cross_section_colour_mode() const
		{
			return d_cross_section_colour_mode;
		}

		void
		set_cross_section_colour_mode(
				CrossSectionColourMode cross_section_colour_mode)
		{
			d_cross_section_colour_mode = cross_section_colour_mode;
		}

		/**
		 * The colour palette used to colour by scalar value.
		 */
		const GPlatesPresentation::RemappedColourPaletteParameters &
		get_scalar_colour_palette_parameters() const
		{
			return d_scalar_colour_palette_parameters;
		}

		void
		set_scalar_colour_palette_parameters(
				const GPlatesPresentation::RemappedColourPaletteParameters &scalar_colour_palette_parameters)
		{
			d_scalar_colour_palette_parameters = scalar_colour_palette_parameters;
		}

		/**
		 * The colour palette used to colour by gradient magnitude.
		 */
		const GPlatesPresentation::RemappedColourPaletteParameters &
		get_gradient_colour_palette_parameters() const
		{
			return d_gradient_colour_palette_parameters;
		}

		void
		set_gradient_colour_palette_parameters(
				const GPlatesPresentation::RemappedColourPaletteParameters &gradient_colour_palette_parameters)
		{
			d_gradient_colour_palette_parameters = gradient_colour_palette_parameters;
		}

		const IsovalueParameters &
		get_isovalue_parameters() const
		{
			return d_isovalue_parameters;
		}

		void
		set_isovalue_parameters(
				const IsovalueParameters &isovalue_parameters)
		{
			d_isovalue_parameters = isovalue_parameters;
		}

		const DeviationWindowRenderOptions &
		get_deviation_window_render_options() const
		{
			return d_deviation_window_render_options;
		}

		void
		set_deviation_window_render_options(
				const DeviationWindowRenderOptions &deviation_window_render_options)
		{
			d_deviation_window_render_options = deviation_window_render_options;
		}

		const SurfacePolygonsMask &
		get_surface_polygons_mask() const
		{
			return d_surface_polygons_mask;
		}

		void
		set_surface_polygons_mask(
				const SurfacePolygonsMask &surface_polygons_mask)
		{
			d_surface_polygons_mask = surface_polygons_mask;
		}

		const DepthRestriction &
		get_depth_restriction() const
		{
			return d_depth_restriction;
		}

		void
		set_depth_restriction(
				const DepthRestriction &depth_restriction)
		{
			d_depth_restriction = depth_restriction;
		}

		const QualityPerformance &
		get_quality_performance() const
		{
			return d_quality_performance;
		}

		void
		set_quality_performance(
				const QualityPerformance &quality_performance)
		{
			d_quality_performance = quality_performance;
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

		void
		set_shader_test_variables(
				const std::vector<float> &shader_test_variables)
		{
			d_shader_test_variables = shader_test_variables;
		}

	private:

		RenderMode d_render_mode;
		IsosurfaceDeviationWindowMode d_isosurface_deviation_window_mode;
		IsosurfaceColourMode d_isosurface_colour_mode;
		CrossSectionColourMode d_cross_section_colour_mode;

		//! The colour palette used to colour by scalar value.
		GPlatesPresentation::RemappedColourPaletteParameters d_scalar_colour_palette_parameters;

		//! The colour palette used to colour by gradient magnitude.
		GPlatesPresentation::RemappedColourPaletteParameters d_gradient_colour_palette_parameters;

		IsovalueParameters d_isovalue_parameters;

		DeviationWindowRenderOptions d_deviation_window_render_options;

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
