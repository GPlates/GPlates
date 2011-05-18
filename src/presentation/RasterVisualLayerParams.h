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

#include <QString>

#include "VisualLayerParams.h"

#include "gui/RasterColourPalette.h"

#include "model/FeatureCollectionHandle.h"

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
				GPlatesAppLogic::LayerTaskParams &layer_task_params)
		{
			return new RasterVisualLayerParams(layer_task_params);
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
				const GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type &colour_palette);

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
		GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type
		get_colour_palette() const;

		/**
		 * Returns the type of the raster as an enumeration.
		 */
		GPlatesPropertyValues::RasterType::Type
		get_raster_type() const;

	protected:

		explicit
		RasterVisualLayerParams(
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
		 * CPT file), then variable this holds that filename.
		 * Otherwise, if the current colour palette was auto-generated, this is
		 * the empty string.
		 */
		QString d_colour_palette_filename;

		/**
		 * The current colour palette for this layer, whether set explicitly as
		 * loaded from a file, or auto-generated.
		 */
		GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type d_colour_palette;

		/**
		 * The type of raster the last time we examined it.
		 */
		GPlatesPropertyValues::RasterType::Type d_raster_type;
	};
}

#endif // GPLATES_PRESENTATION_RASTERVISUALLAYERPARAMS_H
