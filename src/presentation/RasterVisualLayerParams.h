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
#include "model/FeatureHandle.h"
#include "model/WeakReferenceCallback.h"

#include "property-values/GpmlRasterBandNames.h"
#include "property-values/RasterType.h"
#include "property-values/TextContent.h"


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
		create()
		{
			return new RasterVisualLayerParams();
		}

		/**
		 * Override of virtual method in VirtualLayerParams base.
		 */
		virtual
		void
		accept_visitor(
				ConstVisualLayerParamsVisitor &visitor) const
		{
			visitor.visit_raster_visual_layer_params(*this);
		}

		/**
		 * Override of virtual method in VirtualLayerParams base.
		 */
		virtual
		void
		accept_visitor(
				VisualLayerParamsVisitor &visitor)
		{
			visitor.visit_raster_visual_layer_params(*this);
		}

		/**
		 * Override of virtual method in VirtualLayerParams base.
		 */
		virtual
		void
		handle_layer_modified(
				const GPlatesAppLogic::Layer &layer);

		/** 
		 * Returns the name of the band of the raster to be visualised.
		 */
		const GPlatesPropertyValues::TextContent &
		get_band_name() const;

		/**
		 * Sets the name of the band of the raster to be visualised.
		 */
		void
		set_band_name(
				const GPlatesPropertyValues::TextContent &band_name);

		/**
		 * Returns the list of band names that are in the raster feature.
		 */
		const GPlatesPropertyValues::GpmlRasterBandNames::band_names_list_type &
		get_band_names() const;

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

		RasterVisualLayerParams();

	private:

		/**
		 * Listens to modified events from the feature collection in the main input channel.
		 */
		class RasterFeatureCollectionCallback :
				public GPlatesModel::WeakReferenceCallback<GPlatesModel::FeatureCollectionHandle>
		{
		public:

			RasterFeatureCollectionCallback(
					RasterVisualLayerParams *params) :
				d_params(params)
			{  }

			virtual
			void
			publisher_modified(
					const modified_event_type &event)
			{
				d_params->rescan_raster_feature_collection();
			}

		private:

			RasterVisualLayerParams *d_params;
		};

		/**
		 * Rescan @a d_raster_feature_collection to see if the feature(s) it contains
		 * has changed.
		 */
		void
		rescan_raster_feature_collection();

		// RasterFeatureCollectionCallback calls @a rescan_raster_feature_collection.
		friend class RasterFeatureCollectionCallback;
		
		/**
		 * Listens to modified events from the raster feature.
		 */
		class RasterFeatureCallback :
				public GPlatesModel::WeakReferenceCallback<GPlatesModel::FeatureHandle>
		{
		public:

			RasterFeatureCallback(
					RasterVisualLayerParams *params) :
				d_params(params)
			{  }

			virtual
			void
			publisher_modified(
					const modified_event_type &event)
			{
				d_params->rescan_raster_feature();
			}

		private:

			RasterVisualLayerParams *d_params;
		};

		/**
		 * Rescan @a d_raster_feature to see if any pertinent properties have changed.
		 */
		void
		rescan_raster_feature(
				bool always_emit_modified_signal = false);

		// RasterFeatureCallback calls @a rescan_raster_feature.
		friend class RasterFeatureCallback;

		/**
		 * The name of the band of the raster to be visualised.
		 */
		GPlatesPropertyValues::TextContent d_band_name;

		/**
		 * The list of band names that were in the raster feature the last time
		 * we examined it.
		 */
		GPlatesPropertyValues::GpmlRasterBandNames::band_names_list_type d_band_names;

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

		/**
		 * The feature collection that was used as the input into the main input
		 * channel in the corresponding reconstructed raster layer.
		 * This weak-ref is used to track changes to that feature collection.
		 */
		GPlatesModel::FeatureCollectionHandle::weak_ref d_raster_feature_collection;

		/**
		 * The raster feature in @a d_raster_feature_collection.
		 * This weak-ref is used to track changes to that feature.
		 */
		GPlatesModel::FeatureHandle::weak_ref d_raster_feature;
	};
}

#endif // GPLATES_PRESENTATION_RASTERVISUALLAYERPARAMS_H
