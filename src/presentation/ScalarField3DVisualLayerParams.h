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

#ifndef GPLATES_PRESENTATION_SCALARFIELD3DVISUALLAYERPARAMS_H
#define GPLATES_PRESENTATION_SCALARFIELD3DVISUALLAYERPARAMS_H

#include <utility>
#include <vector>
#include <boost/shared_ptr.hpp>
#include <QString>

#include "VisualLayerParams.h"

#include "view-operations/ScalarField3DRenderParameters.h"


namespace GPlatesPresentation
{
	class ViewState;

	class ScalarField3DVisualLayerParams :
			public VisualLayerParams
	{
	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<ScalarField3DVisualLayerParams> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const ScalarField3DVisualLayerParams> non_null_ptr_to_const_type;


		static
		non_null_ptr_type
		create(
				GPlatesAppLogic::LayerParams::non_null_ptr_type layer_params,
				GPlatesPresentation::ViewState &view_state)
		{
			return non_null_ptr_type(new ScalarField3DVisualLayerParams(layer_params, view_state));
		}

		/**
		 * Override of virtual method in VirtualLayerParams base.
		 */
		virtual
		void
		accept_visitor(
				ConstVisualLayerParamsVisitor &visitor) const
		{
			visitor.visit_scalar_field_3d_visual_layer_params(*this);
		}

		/**
		 * Override of virtual method in VirtualLayerParams base.
		 */
		virtual
		void
		accept_visitor(
				VisualLayerParamsVisitor &visitor)
		{
			visitor.visit_scalar_field_3d_visual_layer_params(*this);
		}

		/**
		 * Override of virtual method in VisualLayerParams base.
		 */
		virtual
		void
		handle_layer_modified(
				const GPlatesAppLogic::Layer &layer);


		/**
		 * Returns all parameters as a single @a ScalarField3DRenderParameters object for convenience.
		 */
		const GPlatesViewOperations::ScalarField3DRenderParameters &
		get_scalar_field_3d_render_parameters() const
		{
			return d_scalar_field_3d_render_parameters;
		}

		/**
		 * Sets all parameters as a single @a ScalarField3DRenderParameters object for convenience.
		 */
		void
		set_scalar_field_3d_render_parameters(
				const GPlatesViewOperations::ScalarField3DRenderParameters &scalar_field_3d_render_parameters)
		{
			d_scalar_colour_palette_parameters_initialised_from_scalar_field = true;
			d_gradient_colour_palette_parameters_initialised_from_scalar_field = true;
			d_isovalue_parameters_initialised_from_scalar_field = true;
			d_depth_restriction_initialised_from_scalar_field = true;

			d_scalar_field_3d_render_parameters = scalar_field_3d_render_parameters;

			// If surface polygons mask not supported then disable it.
			disable_surface_polygons_mask_if_not_supported();

			emit_modified();
		}


		/**
		 * Returns the current render mode.
		 */
		GPlatesViewOperations::ScalarField3DRenderParameters::RenderMode
		get_render_mode() const
		{
			return d_scalar_field_3d_render_parameters.get_render_mode();
		}

		/**
		 * Sets the current render mode.
		 */
		void
		set_render_mode(
				GPlatesViewOperations::ScalarField3DRenderParameters::RenderMode render_mode)
		{
			if (d_scalar_field_3d_render_parameters.get_render_mode() == render_mode)
			{
				return;
			}

			d_scalar_field_3d_render_parameters.set_render_mode(render_mode);
			emit_modified();
		}

		/**
		 * Returns the current iso-surface window deviation mode.
		 */
		GPlatesViewOperations::ScalarField3DRenderParameters::IsosurfaceDeviationWindowMode
		get_isosurface_deviation_window_mode() const
		{
			return d_scalar_field_3d_render_parameters.get_isosurface_deviation_window_mode();
		}

		/**
		 * Sets the current iso-surface window deviation mode.
		 */
		void
		set_isosurface_deviation_window_mode(
				GPlatesViewOperations::ScalarField3DRenderParameters::IsosurfaceDeviationWindowMode isosurface_deviation_window_mode)
		{
			if (d_scalar_field_3d_render_parameters.get_isosurface_deviation_window_mode() == isosurface_deviation_window_mode)
			{
				return;
			}

			d_scalar_field_3d_render_parameters.set_isosurface_deviation_window_mode(isosurface_deviation_window_mode);
			emit_modified();
		}

		/**
		 * Returns the current iso-surface colour mode.
		 */
		GPlatesViewOperations::ScalarField3DRenderParameters::IsosurfaceColourMode
		get_isosurface_colour_mode() const
		{
			return d_scalar_field_3d_render_parameters.get_isosurface_colour_mode();
		}

		/**
		 * Sets the current iso-surface colour mode.
		 */
		void
		set_isosurface_colour_mode(
				GPlatesViewOperations::ScalarField3DRenderParameters::IsosurfaceColourMode isosurface_colour_mode)
		{
			if (d_scalar_field_3d_render_parameters.get_isosurface_colour_mode() == isosurface_colour_mode)
			{
				return;
			}

			d_scalar_field_3d_render_parameters.set_isosurface_colour_mode(isosurface_colour_mode);
			emit_modified();
		}

		/**
		 * Returns the current cross-section colour mode.
		 */
		GPlatesViewOperations::ScalarField3DRenderParameters::CrossSectionColourMode
		get_cross_section_colour_mode() const
		{
			return d_scalar_field_3d_render_parameters.get_cross_section_colour_mode();
		}

		/**
		 * Sets the current cross-section colour mode.
		 */
		void
		set_cross_section_colour_mode(
				GPlatesViewOperations::ScalarField3DRenderParameters::CrossSectionColourMode cross_section_colour_mode)
		{
			if (d_scalar_field_3d_render_parameters.get_cross_section_colour_mode() == cross_section_colour_mode)
			{
				return;
			}

			d_scalar_field_3d_render_parameters.set_cross_section_colour_mode(cross_section_colour_mode);
			emit_modified();
		}


		/**
		 * The default scalar colour palette parameters.
		 */
		static
		GPlatesPresentation::RemappedColourPaletteParameters
		create_default_scalar_colour_palette_parameters()
		{
			return GPlatesViewOperations::ScalarField3DRenderParameters::create_default_scalar_colour_palette_parameters();
		}

		/**
		 * Returns the current scalar colour palette.
		 */
		const GPlatesPresentation::RemappedColourPaletteParameters &
		get_scalar_colour_palette_parameters() const
		{
			return d_scalar_field_3d_render_parameters.get_scalar_colour_palette_parameters();
		}

		/**
		 * Sets the current scalar colour palette.
		 */
		void
		set_scalar_colour_palette_parameters(
				const GPlatesPresentation::RemappedColourPaletteParameters &scalar_colour_palette_parameters)
		{
			d_scalar_colour_palette_parameters_initialised_from_scalar_field = true;

			d_scalar_field_3d_render_parameters.set_scalar_colour_palette_parameters(scalar_colour_palette_parameters);
			emit_modified();
		}


		/**
		 * The default gradient colour palette parameters.
		 */
		static
		GPlatesPresentation::RemappedColourPaletteParameters
		create_default_gradient_colour_palette_parameters()
		{
			return GPlatesViewOperations::ScalarField3DRenderParameters::create_default_gradient_colour_palette_parameters();
		}

		/**
		 * Returns the current gradient colour palette.
		 */
		const GPlatesPresentation::RemappedColourPaletteParameters &
		get_gradient_colour_palette_parameters() const
		{
			return d_scalar_field_3d_render_parameters.get_gradient_colour_palette_parameters();
		}

		/**
		 * Sets the current gradient colour palette.
		 */
		void
		set_gradient_colour_palette_parameters(
				const GPlatesPresentation::RemappedColourPaletteParameters &gradient_colour_palette_parameters)
		{
			d_gradient_colour_palette_parameters_initialised_from_scalar_field = true;

			d_scalar_field_3d_render_parameters.set_gradient_colour_palette_parameters(gradient_colour_palette_parameters);
			emit_modified();
		}


		/**
		 * Returns the current isovalue parameters.
		 */
		const GPlatesViewOperations::ScalarField3DRenderParameters::IsovalueParameters &
		get_isovalue_parameters() const
		{
			return d_scalar_field_3d_render_parameters.get_isovalue_parameters();
		}

		/**
		 * Sets the current isovalue parameters.
		 */
		void
		set_isovalue_parameters(
				const GPlatesViewOperations::ScalarField3DRenderParameters::IsovalueParameters &isovalue_parameters)
		{
			d_isovalue_parameters_initialised_from_scalar_field = true;

			d_scalar_field_3d_render_parameters.set_isovalue_parameters(isovalue_parameters);
			emit_modified();
		}

		const GPlatesViewOperations::ScalarField3DRenderParameters::DeviationWindowRenderOptions &
		get_deviation_window_render_options() const
		{
			return d_scalar_field_3d_render_parameters.get_deviation_window_render_options();
		}

		void
		set_deviation_window_render_options(
				const GPlatesViewOperations::ScalarField3DRenderParameters::DeviationWindowRenderOptions &deviation_window_render_options)
		{
			d_scalar_field_3d_render_parameters.set_deviation_window_render_options(deviation_window_render_options);
			emit_modified();
		}

		/**
		 * Returns whether the runtime graphics hardware can support surface polygons mask.
		 */
		bool
		is_surface_polygons_mask_supported() const
		{
			return d_is_surface_polygons_mask_supported;
		}

		const GPlatesViewOperations::ScalarField3DRenderParameters::SurfacePolygonsMask &
		get_surface_polygons_mask() const
		{
			return d_scalar_field_3d_render_parameters.get_surface_polygons_mask();
		}

		void
		set_surface_polygons_mask(
				GPlatesViewOperations::ScalarField3DRenderParameters::SurfacePolygonsMask surface_polygons_mask)
		{
			d_scalar_field_3d_render_parameters.set_surface_polygons_mask(surface_polygons_mask);

			// If surface polygons mask not supported then disable it.
			disable_surface_polygons_mask_if_not_supported();

			emit_modified();
		}

		const GPlatesViewOperations::ScalarField3DRenderParameters::DepthRestriction &
		get_depth_restriction() const
		{
			return d_scalar_field_3d_render_parameters.get_depth_restriction();
		}

		void
		set_depth_restriction(
				const GPlatesViewOperations::ScalarField3DRenderParameters::DepthRestriction &depth_restriction)
		{
			d_depth_restriction_initialised_from_scalar_field = true;

			d_scalar_field_3d_render_parameters.set_depth_restriction(depth_restriction);
			emit_modified();
		}

		const GPlatesViewOperations::ScalarField3DRenderParameters::QualityPerformance &
		get_quality_performance() const
		{
			return d_scalar_field_3d_render_parameters.get_quality_performance();
		}

		void
		set_quality_performance(
				const GPlatesViewOperations::ScalarField3DRenderParameters::QualityPerformance &quality_performance)
		{
			d_scalar_field_3d_render_parameters.set_quality_performance(quality_performance);
			emit_modified();
		}

		/**
		 * Returns the optional test variables to use during @a GLScalarField3D shader program development.
		 */
		const std::vector<float> &
		get_shader_test_variables() const
		{
			return d_scalar_field_3d_render_parameters.get_shader_test_variables();
		}

		/**
		 * Optional test variables to use during @a GLScalarField3D shader program development.
		 */
		void
		set_shader_test_variables(
				const std::vector<float> &shader_test_variables)
		{
			d_scalar_field_3d_render_parameters.set_shader_test_variables(shader_test_variables);
			emit_modified();
		}

	protected:

		explicit
		ScalarField3DVisualLayerParams(
				GPlatesAppLogic::LayerParams::non_null_ptr_type layer_params,
				GPlatesPresentation::ViewState &view_state);

	private:

		GPlatesViewOperations::ScalarField3DRenderParameters d_scalar_field_3d_render_parameters;

		bool d_is_surface_polygons_mask_supported;

		bool d_scalar_colour_palette_parameters_initialised_from_scalar_field;
		bool d_gradient_colour_palette_parameters_initialised_from_scalar_field;
		bool d_isovalue_parameters_initialised_from_scalar_field;
		bool d_depth_restriction_initialised_from_scalar_field;


		void
		disable_surface_polygons_mask_if_not_supported();
	};
}

#endif // GPLATES_PRESENTATION_SCALARFIELD3DVISUALLAYERPARAMS_H
