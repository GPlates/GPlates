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
		 * Returns the current iso-value of the iso-surface.
		 *
		 * Returns boost::none if value has not yet been set.
		 */
		boost::optional<float>
		get_iso_value() const
		{
			return d_iso_value;
		}

		/**
		 * Sets the current iso-value of the iso-surface.
		 */
		void
		set_iso_value(
				float iso_value)
		{
			d_iso_value = iso_value;
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

		boost::optional<float> d_iso_value;
		std::vector<float> d_shader_test_variables;

	};
}

#endif // GPLATES_PRESENTATION_SCALARFIELD3DVISUALLAYERPARAMS_H
