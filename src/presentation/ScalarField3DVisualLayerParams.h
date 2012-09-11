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

#include <vector>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <QString>

#include "VisualLayerParams.h"

#include "gui/ColourPalette.h"

#include "view-operations/ScalarField3DRenderParameters.h"


namespace GPlatesPresentation
{
	class ScalarField3DVisualLayerParams :
			public VisualLayerParams
	{
	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<ScalarField3DVisualLayerParams> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const ScalarField3DVisualLayerParams> non_null_ptr_to_const_type;


		static
		non_null_ptr_type
		create(
				GPlatesAppLogic::LayerTaskParams &layer_task_params)
		{
			return new ScalarField3DVisualLayerParams(layer_task_params);
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
		 * Returns the current render mode.
		 */
		GPlatesViewOperations::ScalarField3DRenderParameters::RenderMode
		get_render_mode() const
		{
			return d_render_mode;
		}

		/**
		 * Sets the current render mode.
		 */
		void
		set_render_mode(
				GPlatesViewOperations::ScalarField3DRenderParameters::RenderMode render_mode)
		{
			d_render_mode = render_mode;
			emit_modified();
		}

		/**
		 * Returns the current colour mode.
		 */
		GPlatesViewOperations::ScalarField3DRenderParameters::ColourMode
		get_colour_mode() const
		{
			return d_colour_mode;
		}

		/**
		 * Sets the current colour mode.
		 */
		void
		set_colour_mode(
				GPlatesViewOperations::ScalarField3DRenderParameters::ColourMode colour_mode)
		{
			d_colour_mode = colour_mode;
			emit_modified();
		}

		/**
		 * Returns the filename of the file from which the current colour
		 * palette was loaded, if it was loaded from a file.
		 * If the current colour palette is auto-generated, returns the empty string.
		 */
		const QString &
		get_colour_palette_filename() const;

		/**
		 * Sets the current colour palette to be one that is loaded from a file.
		 * @a filename must not be the empty string.
		 */
		void
		set_colour_palette(
				const QString &filename,
				const GPlatesGui::ColourPalette<double>::non_null_ptr_type &colour_palette);

		/**
		 * Causes the current colour palette to be auto-generated from the
		 * raster in the layer, and sets the filename field to be the empty string.
		 */
		void
		use_auto_generated_colour_palette();

		/**
		 * Returns the current colour palette, whether explicitly set as loaded
		 * from a file, or auto-generated.
		 */
		GPlatesGui::ColourPalette<double>::non_null_ptr_type
		get_colour_palette() const;

		/**
		 * Returns the current isovalue parameters.
		 *
		 * Returns boost::none if they have not been set yet.
		 */
		boost::optional<GPlatesViewOperations::ScalarField3DRenderParameters::IsovalueParameters>
		get_isovalue_parameters() const
		{
			return d_isovalue_parameters;
		}

		/**
		 * Sets the current isovalue parameters.
		 */
		void
		set_isovalue_parameters(
				const GPlatesViewOperations::ScalarField3DRenderParameters::IsovalueParameters &isovalue_parameters)
		{
			d_isovalue_parameters = isovalue_parameters;
			emit_modified();
		}

		const GPlatesViewOperations::ScalarField3DRenderParameters::DeviationWindowRenderOptions &
		get_deviation_window_render_options() const
		{
			return d_deviation_window_render_options;
		}

		void
		set_deviation_window_render_options(
				const GPlatesViewOperations::ScalarField3DRenderParameters::DeviationWindowRenderOptions &render_options)
		{
			d_deviation_window_render_options = render_options;
			emit_modified();
		}

		const GPlatesViewOperations::ScalarField3DRenderParameters::SurfacePolygonsMask &
		get_surface_polygons_mask() const
		{
			return d_surface_polygons_mask;
		}

		void
		set_surface_polygons_mask(
				const GPlatesViewOperations::ScalarField3DRenderParameters::SurfacePolygonsMask &surface_polygons_mask)
		{
			d_surface_polygons_mask = surface_polygons_mask;
			emit_modified();
		}

		boost::optional<GPlatesViewOperations::ScalarField3DRenderParameters::DepthRestriction>
		get_depth_restriction() const
		{
			return d_depth_restriction;
		}

		void
		set_depth_restriction(
				const GPlatesViewOperations::ScalarField3DRenderParameters::DepthRestriction &depth_restriction)
		{
			d_depth_restriction = depth_restriction;
			emit_modified();
		}

		const GPlatesViewOperations::ScalarField3DRenderParameters::QualityPerformance &
		get_quality_performance() const
		{
			return d_quality_performance;
		}

		void
		set_quality_performance(
				const GPlatesViewOperations::ScalarField3DRenderParameters::QualityPerformance &quality_performance)
		{
			d_quality_performance = quality_performance;
			emit_modified();
		}

		/**
		 * Returns the optional test variables to use during @a GLScalarField3D shader program development.
		 */
		const std::vector<float> &
		get_shader_test_variables() const
		{
			return d_shader_test_variables;
		}

		/**
		 * Optional test variables to use during @a GLScalarField3D shader program development.
		 */
		void
		set_shader_test_variables(
				const std::vector<float> &test_variables)
		{
			d_shader_test_variables = test_variables;
			emit_modified();
		}

	protected:

		explicit
		ScalarField3DVisualLayerParams(
				GPlatesAppLogic::LayerTaskParams &layer_task_params);

	private:

		/**
		 * See if any pertinent properties have changed.
		 */
		void
		update(
				bool always_emit_modified_signal = false);


		GPlatesViewOperations::ScalarField3DRenderParameters::RenderMode d_render_mode;
		GPlatesViewOperations::ScalarField3DRenderParameters::ColourMode d_colour_mode;

		/**
		 * If the current colour palette was loaded from a file (typically a
		 * CPT file), then this variable holds that filename.
		 * Otherwise, if the current colour palette was auto-generated, this is
		 * the empty string.
		 */
		QString d_colour_palette_filename;

		/**
		 * The current colour palette for this layer, whether set explicitly as
		 * loaded from a file, or auto-generated.
		 */
		GPlatesGui::ColourPalette<double>::non_null_ptr_type d_colour_palette;

		// This is optional because the default isovalue cannot be set until a scalar field feature
		// is available - the default isovalue is the mean scalar value in the scalar field.
		// The scalar field is not immediately available due to various signal/slot dependencies.
		boost::optional<GPlatesViewOperations::ScalarField3DRenderParameters::IsovalueParameters> d_isovalue_parameters;

		GPlatesViewOperations::ScalarField3DRenderParameters::DeviationWindowRenderOptions d_deviation_window_render_options;

		GPlatesViewOperations::ScalarField3DRenderParameters::SurfacePolygonsMask d_surface_polygons_mask;

		// This is optional because the default depth restriction range cannot be set until a
		// scalar field feature is available - the default depth restriction range is the
		// depth range in the scalar field.
		// The scalar field is not immediately available due to various signal/slot dependencies.
		boost::optional<GPlatesViewOperations::ScalarField3DRenderParameters::DepthRestriction> d_depth_restriction;

		GPlatesViewOperations::ScalarField3DRenderParameters::QualityPerformance d_quality_performance;

		std::vector<float> d_shader_test_variables;

	};
}

#endif // GPLATES_PRESENTATION_SCALARFIELD3DVISUALLAYERPARAMS_H
