/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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
 
#ifndef GPLATES_PRESENTATION_RASTERVISUALLAYERPARAMS_H
#define GPLATES_PRESENTATION_RASTERVISUALLAYERPARAMS_H

#include <boost/optional.hpp>
#include <QString>

#include "RemappedColourPaletteParameters.h"
#include "VisualLayerParams.h"

#include "gui/Colour.h"
#include "gui/RasterColourPalette.h"

#include "property-values/RasterType.h"


namespace GPlatesPresentation
{
	class RasterVisualLayerParams :
			public VisualLayerParams
	{
	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<RasterVisualLayerParams> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const RasterVisualLayerParams> non_null_ptr_to_const_type;

		static
		non_null_ptr_type
		create(
				GPlatesAppLogic::LayerParams::non_null_ptr_type layer_params)
		{
			return new RasterVisualLayerParams(layer_params);
		}

		/**
		 * Override of virtual method in VisualLayerParams base.
		 */
		virtual
		void
		accept_visitor(
				ConstVisualLayerParamsVisitor &visitor) const
		{
			visitor.visit_raster_visual_layer_params(*this);
		}

		/**
		 * Override of virtual method in VisualLayerParams base.
		 */
		virtual
		void
		accept_visitor(
				VisualLayerParamsVisitor &visitor)
		{
			visitor.visit_raster_visual_layer_params(*this);
		}

		/**
		 * Override of virtual method in VisualLayerParams base.
		 */
		virtual
		void
		handle_layer_modified(
				const GPlatesAppLogic::Layer &layer);


		/**
		 * The default colour palette parameters.
		 */
		static
		GPlatesPresentation::RemappedColourPaletteParameters
		create_default_colour_palette_parameters();

		/**
		 * Returns the current colour palette.
		 */
		const GPlatesPresentation::RemappedColourPaletteParameters &
		get_colour_palette_parameters() const
		{
			return d_colour_palette_parameters;
		}

		/**
		 * Sets the current colour palette.
		 */
		void
		set_colour_palette_parameters(
				const GPlatesPresentation::RemappedColourPaletteParameters &colour_palette_parameters);


		/**
		 * Returns the type of the raster as an enumeration.
		 */
		GPlatesPropertyValues::RasterType::Type
		get_raster_type() const;

		/**
		 * Sets the opacity of the raster.
		 */
		void
		set_opacity(
				const double &opacity);

		/**
		 * Gets the opacity of the raster.
		 */
		double
		get_opacity() const
		{
			return d_opacity;
		}

		/**
		 * Sets the intensity of the raster.
		 */
		void
		set_intensity(
				const double &intensity);

		/**
		 * Gets the intensity of the raster.
		 */
		double
		get_intensity() const
		{
			return d_intensity;
		}

		/**
		 * Returns the raster modulate colour.
		 *
		 * This is a combination of the opacity and intensity as (I, I, I, O) where
		 * 'I' is intensity and 'O' is opacity.
		 */
		GPlatesGui::Colour
		get_modulate_colour() const;

		/**
		 * Sets the height field scale factor adjustment to use for normal map.
		 */
		void
		set_surface_relief_scale(
				float surface_relief_scale);

		/**
		 * Gets the height field scale factor adjustment to use for normal map.
		 */
		float
		get_surface_relief_scale() const
		{
			return d_surface_relief_scale;
		}

	protected:

		explicit
		RasterVisualLayerParams(
				GPlatesAppLogic::LayerParams::non_null_ptr_type layer_params);

	private:

		bool d_colour_palette_parameters_initialised_from_raster;
		/**
		 * The current colour palette for this layer, whether set explicitly as loaded from a file,
		 * or auto-generated.
		 */
		GPlatesPresentation::RemappedColourPaletteParameters d_colour_palette_parameters;

		/**
		 * The type of raster the last time we examined it.
		 */
		GPlatesPropertyValues::RasterType::Type d_raster_type;

		/**
		 * The opacity of the raster in the range [0,1].
		 */
		double d_opacity;

		/**
		 * The intensity of the raster in the range [0,1].
		 */
		double d_intensity;

		/**
		 * Gets the height field scale factor adjustment to use for normal map.
		 */
		float d_surface_relief_scale;
	};
}

#endif // GPLATES_PRESENTATION_RASTERVISUALLAYERPARAMS_H
