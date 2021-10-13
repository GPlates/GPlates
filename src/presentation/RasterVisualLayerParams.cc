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

#include <utility>
#include <boost/optional.hpp>
#include <boost/foreach.hpp>
 
#include "RasterVisualLayerParams.h"

#include "app-logic/ExtractRasterFeatureProperties.h"
#include "app-logic/Layer.h"
#include "app-logic/RasterLayerTask.h"

#include "maths/MathsUtils.h"


namespace
{
	/**
	 * First corresponds to 'mean', second corresponds to 'standard deviation'.
	 */
	typedef std::pair<double, double> default_raster_colour_palette_parameters;


	/**
	 * Extracts the mean and standard deviation from a RasterColourPalette, if it
	 * is indeed a DefaultRasterColourPalette.
	 */
	class ExtractDefaultRasterColourPaletteProperties :
			public boost::static_visitor<boost::optional<default_raster_colour_palette_parameters> >
	{
	public:

		boost::optional<default_raster_colour_palette_parameters>
		operator()(
				const GPlatesGui::RasterColourPalette::empty &) const
		{
			return boost::none;
		}

		template<class ColourPaletteType>
		boost::optional<default_raster_colour_palette_parameters>
		operator()(
				const GPlatesUtils::non_null_intrusive_ptr<ColourPaletteType> &colour_palette) const
		{
			DefaultRasterColourPalettePropertiesVisitor visitor;
			colour_palette->accept_visitor(visitor);
			return visitor.get_parameters();
		}

	private:

		class DefaultRasterColourPalettePropertiesVisitor :
				public GPlatesGui::ConstColourPaletteVisitor
		{
		public:

			const boost::optional<default_raster_colour_palette_parameters> &
			get_parameters() const
			{
				return d_parameters;
			}

			virtual
			void
			visit_default_raster_colour_palette(
					const GPlatesGui::DefaultRasterColourPalette &colour_palette)
			{
				d_parameters = std::make_pair(colour_palette.get_mean(), colour_palette.get_std_dev());
			}
		
		private:

			boost::optional<default_raster_colour_palette_parameters> d_parameters;
		};
	};
}


GPlatesPresentation::RasterVisualLayerParams::RasterVisualLayerParams(
		GPlatesAppLogic::LayerTaskParams &layer_task_params) :
	VisualLayerParams(layer_task_params),
	d_colour_palette_filename(QString()),
	d_colour_palette(GPlatesGui::RasterColourPalette::create()),
	d_raster_type(GPlatesPropertyValues::RasterType::UNKNOWN)
{
}


void
GPlatesPresentation::RasterVisualLayerParams::handle_layer_modified(
		const GPlatesAppLogic::Layer &layer)
{
	GPlatesAppLogic::RasterLayerTask::Params *raster_layer_task_params =
		dynamic_cast<GPlatesAppLogic::RasterLayerTask::Params *>(
				&get_layer_task_params());
	if (raster_layer_task_params && raster_layer_task_params->get_raster_feature())
	{
		update(true);
		return;
	}
	// ...there's no raster feature...

	if (d_colour_palette_filename.isEmpty()) // i.e. colour palette auto-generated
	{
		d_colour_palette = GPlatesGui::RasterColourPalette::create();
	}

	emit_modified();
}


void
GPlatesPresentation::RasterVisualLayerParams::update(
		bool always_emit_modified_signal)
{
	// Get the raster band index.
	std::size_t band_name_index = 0;

	GPlatesAppLogic::RasterLayerTask::Params *raster_layer_task_params =
		dynamic_cast<GPlatesAppLogic::RasterLayerTask::Params *>(
				&get_layer_task_params());
	if (raster_layer_task_params)
	{
		const GPlatesPropertyValues::GpmlRasterBandNames::band_names_list_type &raster_band_names =
				raster_layer_task_params->get_band_names();

		boost::optional<std::size_t> band_name_index_opt =
				GPlatesAppLogic::find_raster_band_name(
						raster_band_names,
						raster_layer_task_params->get_band_name());
		if (band_name_index_opt)
		{
			band_name_index = *band_name_index_opt;
		}
	}


	GPlatesAppLogic::ExtractRasterFeatureProperties visitor;
	if (raster_layer_task_params && raster_layer_task_params->get_raster_feature())
	{
		visitor.visit_feature(raster_layer_task_params->get_raster_feature().get());
	}

	bool colour_palette_changed = false;
	bool colour_palette_valid = false;
	GPlatesPropertyValues::RasterType::Type raster_type = GPlatesPropertyValues::RasterType::UNKNOWN;
	if (visitor.get_proxied_rasters())
	{
		const std::vector<GPlatesPropertyValues::RawRaster::non_null_ptr_type> &
			proxied_raster = *(visitor.get_proxied_rasters());
		if (band_name_index < proxied_raster.size())
		{
			// Get the proxied raster corresponding to the chosen band name.
			const GPlatesPropertyValues::RawRaster::non_null_ptr_type &raw_raster =
				proxied_raster[band_name_index];

			// Create a new colour palette if the colour palette currently in place was not
			// loaded from a file.
			if (d_colour_palette_filename.isEmpty())
			{
				GPlatesPropertyValues::RasterStatistics *raster_statistics =
					GPlatesPropertyValues::RawRasterUtils::get_raster_statistics(*raw_raster);
				if (raster_statistics &&
						raster_statistics->mean &&
						raster_statistics->standard_deviation)
				{
					// Compare the statistics of the chosen raster band with those used to
					// generate the existing colour palette; if they differ, generate a new
					// colour palette.
					double mean = *(raster_statistics->mean);
					double std_dev = *(raster_statistics->standard_deviation);

					boost::optional<default_raster_colour_palette_parameters> existing_parameters =
						d_colour_palette->apply_visitor(ExtractDefaultRasterColourPaletteProperties());
					if (!existing_parameters ||
							!GPlatesMaths::are_almost_exactly_equal(mean, existing_parameters->first) ||
							!GPlatesMaths::are_almost_exactly_equal(std_dev, existing_parameters->second))
					{
						d_colour_palette = GPlatesGui::RasterColourPalette::create<double>(
								GPlatesGui::DefaultRasterColourPalette::create(mean, std_dev));
						colour_palette_changed = true;
					}

					colour_palette_valid = true;
				}
			}
			else
			{
				colour_palette_valid = true;
			}

			// Get the raster type as an enumeration.
			raster_type = GPlatesPropertyValues::RawRasterUtils::get_raster_type(*raw_raster);
		}
	}

	if (!colour_palette_valid)
	{
		d_colour_palette = GPlatesGui::RasterColourPalette::create();
		colour_palette_changed = true;
	}

	bool raster_type_changed = false;
	if (raster_type != d_raster_type)
	{
		d_raster_type = raster_type;
		raster_type_changed = true;
	}

	if (always_emit_modified_signal ||
			colour_palette_changed ||
			raster_type_changed)
	{
		emit_modified();
	}
}


const QString &
GPlatesPresentation::RasterVisualLayerParams::get_colour_palette_filename() const
{
	return d_colour_palette_filename;
}


void
GPlatesPresentation::RasterVisualLayerParams::set_colour_palette(
		const QString &filename,
		const GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type &colour_palette)
{
	d_colour_palette_filename = filename;
	d_colour_palette = colour_palette;
	emit_modified();
}


void
GPlatesPresentation::RasterVisualLayerParams::use_auto_generated_colour_palette()
{
	d_colour_palette_filename = QString();
	update(true /* emit modified signal for us */);
}


GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type
GPlatesPresentation::RasterVisualLayerParams::get_colour_palette() const
{
	return d_colour_palette;
}


GPlatesPropertyValues::RasterType::Type
GPlatesPresentation::RasterVisualLayerParams::get_raster_type() const
{
	return d_raster_type;
}

