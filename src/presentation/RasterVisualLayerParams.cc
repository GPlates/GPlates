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

#include "app-logic/Layer.h"

#include "maths/MathsUtils.h"

#include "model/FeatureVisitor.h"

#include "property-values/GmlFile.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlPiecewiseAggregation.h"
#include "property-values/GpmlTimeWindow.h"
#include "property-values/RasterStatistics.h"
#include "property-values/RawRaster.h"
#include "property-values/RawRasterUtils.h"
#include "property-values/XsString.h"


namespace
{
	/**
	 * Visits a raster feature and extracts the following properties from it:
	 *  - The first GmlFile inside a GpmlConstantValue or a GpmlPiecewiseAggregation
	 *    inside a gpml:rangeSet top-level property.
	 *  - GpmlRasterBandNames (not inside any time-dependent structure) inside a
	 *    gpml:bandNames top-level property.
	 */
	class ExtractRasterProperties :
			public GPlatesModel::ConstFeatureVisitor
	{
	public:

		ExtractRasterProperties() :
			d_inside_constant_value(false),
			d_inside_piecewise_aggregation(false)
		{  }

		const boost::optional<std::vector<GPlatesPropertyValues::RawRaster::non_null_ptr_type> > &
		get_proxied_raster() const
		{
			return d_proxied_rasters;
		}

		const boost::optional<GPlatesPropertyValues::GpmlRasterBandNames::band_names_list_type> &
		get_raster_band_names() const
		{
			return d_raster_band_names;
		}

		virtual
		bool
		initialise_pre_feature_properties(
				const GPlatesModel::FeatureHandle &feature_handle)
		{
			d_proxied_rasters = boost::none;
			d_raster_band_names = boost::none;

			return true;
		}

		virtual
		void
		visit_gpml_constant_value(
				const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
		{
			d_inside_constant_value = true;
			gpml_constant_value.value()->accept_visitor(*this);
			d_inside_constant_value = false;
		}

		virtual
		void
		visit_gpml_piecewise_aggregation(
				const GPlatesPropertyValues::GpmlPiecewiseAggregation &gpml_piecewise_aggregation)
		{
			d_inside_piecewise_aggregation = true;
			std::vector<GPlatesPropertyValues::GpmlTimeWindow> time_windows =
				gpml_piecewise_aggregation.time_windows();
			if (!time_windows.empty())
			{
				time_windows.begin()->time_dependent_value()->accept_visitor(*this);
			}
			d_inside_piecewise_aggregation = false;
		}

		virtual
		void
		visit_gml_file(
				const GPlatesPropertyValues::GmlFile &gml_file)
		{
			static const GPlatesModel::PropertyName RANGE_SET =
				GPlatesModel::PropertyName::create_gpml("rangeSet");

			if (d_inside_constant_value || d_inside_piecewise_aggregation)
			{
				const boost::optional<GPlatesModel::PropertyName> &propname = current_top_level_propname();
				if (propname && *propname == RANGE_SET)
				{
					d_proxied_rasters = gml_file.proxied_raw_rasters();
				}
			}
		}

		virtual
		void
		visit_gpml_raster_band_names(
				const GPlatesPropertyValues::GpmlRasterBandNames &gpml_raster_band_names)
		{
			static const GPlatesModel::PropertyName BAND_NAMES =
				GPlatesModel::PropertyName::create_gpml("bandNames");

			if (!d_inside_constant_value && !d_inside_piecewise_aggregation)
			{
				const boost::optional<GPlatesModel::PropertyName> &propname = current_top_level_propname();
				if (propname && *propname == BAND_NAMES)
				{
					d_raster_band_names = gpml_raster_band_names.band_names();
				}
			}
		}

	private:

		/**
		 * The proxied rasters of the first GmlFile encountered.
		 *
		 * The reason why we are only interested in the first GmlFile encountered is
		 * that the auto-generated raster colour palette is created based on the first
		 * frame of a time-dependent raster sequence.
		 */
		boost::optional<std::vector<GPlatesPropertyValues::RawRaster::non_null_ptr_type> >
			d_proxied_rasters;

		/**
		 * The list of band names - one for each proxied raster.
		 */
		boost::optional<GPlatesPropertyValues::GpmlRasterBandNames::band_names_list_type> d_raster_band_names;

		bool d_inside_constant_value;
		bool d_inside_piecewise_aggregation;
	};


	/**
	 * Returns the index of @a band_name inside @a band_names_list if present.
	 * If not present, returns boost::none.
	 */
	boost::optional<std::size_t>
	find_band_name(
			const GPlatesPropertyValues::GpmlRasterBandNames::band_names_list_type &band_names_list,
			const GPlatesPropertyValues::TextContent &band_name)
	{
		for (std::size_t i = 0; i != band_names_list.size(); ++i)
		{
			if (band_names_list[i]->value() == band_name)
			{
				return i;
			}
		}
		return boost::none;
	}


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


GPlatesPresentation::RasterVisualLayerParams::RasterVisualLayerParams() :
	d_band_name(GPlatesUtils::UnicodeString()),
	d_colour_palette_filename(QString()),
	d_colour_palette(GPlatesGui::RasterColourPalette::create()),
	d_raster_type(GPlatesPropertyValues::RasterType::UNKNOWN)
{
}


void
GPlatesPresentation::RasterVisualLayerParams::handle_layer_modified(
		const GPlatesAppLogic::Layer &layer)
{
	// Get at the feature collection used as main input into the layer.
	QString main_channel = layer.get_main_input_feature_collection_channel();
	std::vector<GPlatesAppLogic::Layer::InputConnection> input_connections =
		layer.get_channel_inputs(main_channel);
	bool raster_feature_collection_found = false;
	if (input_connections.size() > 0)
	{
		boost::optional<GPlatesAppLogic::Layer::InputFile> input_file =
			input_connections[0].get_input_file();
		if (input_file)
		{
			const GPlatesFileIO::File::Reference &file = input_file->get_file().get_file();
			GPlatesModel::FeatureCollectionHandle::weak_ref feature_collection = file.get_feature_collection();
			if (feature_collection.is_valid())
			{
				raster_feature_collection_found = true;

				// Is it a different feature collection compared with the last time we
				// looked at the layer?
				if (d_raster_feature_collection != feature_collection)
				{
					d_raster_feature_collection = feature_collection;
					d_raster_feature_collection.attach_callback(
							new RasterFeatureCollectionCallback(this));
					rescan_raster_feature_collection();
				}
			}
		}
	}

	if (!raster_feature_collection_found)
	{
		d_band_name = GPlatesUtils::UnicodeString();
		d_band_names.clear();
		if (d_colour_palette_filename.isEmpty()) // i.e. colour palette auto-generated
		{
			d_colour_palette = GPlatesGui::RasterColourPalette::create();
		}
		d_raster_feature_collection = GPlatesModel::FeatureCollectionHandle::weak_ref();
		d_raster_feature = GPlatesModel::FeatureHandle::weak_ref();

		emit_modified();
	}
}


void
GPlatesPresentation::RasterVisualLayerParams::rescan_raster_feature_collection()
{
	// Get at the raster feature inside the feature collection.
	if (d_raster_feature_collection->size() == 1)
	{
		GPlatesModel::FeatureHandle::weak_ref feature = (*d_raster_feature_collection->begin())->reference();

		// Is it a different feature compared with the last time we looked at the
		// feature collection?
		if (d_raster_feature != feature)
		{
			d_raster_feature = feature;
			d_raster_feature.attach_callback(
					new RasterFeatureCallback(this));
			rescan_raster_feature();
		}
	}
	else
	{
		d_band_name = GPlatesUtils::UnicodeString();
		d_band_names.clear();
		if (d_colour_palette_filename.isEmpty()) // i.e. colour palette auto-generated
		{
			d_colour_palette = GPlatesGui::RasterColourPalette::create();
		}
		d_raster_feature = GPlatesModel::FeatureHandle::weak_ref();

		emit_modified();
	}
}


void
GPlatesPresentation::RasterVisualLayerParams::rescan_raster_feature(
		bool always_emit_modified_signal)
{
	ExtractRasterProperties visitor;
	visitor.visit_feature(d_raster_feature);

	bool band_name_changed = false;
	bool band_names_list_changed = false;
	bool band_name_valid = false;
	std::size_t band_name_index = 0;
	if (visitor.get_raster_band_names())
	{
		// Is the selected band name one of the available bands in the raster?
		// If not, then change the band name to be the first of the available bands.
		// At the same time, record the index of the band names in the band names list
		// in the variable @a band_name_index.
		const GPlatesPropertyValues::GpmlRasterBandNames::band_names_list_type &
			raster_band_names = *(visitor.get_raster_band_names());
		if (!raster_band_names.empty())
		{
			boost::optional<std::size_t> band_name_index_opt = find_band_name(raster_band_names, d_band_name);
			if (band_name_index_opt)
			{
				band_name_index = *band_name_index_opt;
			}
			else
			{
				d_band_name = raster_band_names[0]->value();
				band_name_changed = true;
			}

			band_name_valid = true;
		}

		// Update @a d_band_names if necessary.
		if (raster_band_names != d_band_names)
		{
			d_band_names = raster_band_names;
			band_names_list_changed = true;
		}
	}
	else
	{
		d_band_names.clear();
		band_names_list_changed = true;
	}

	if (!band_name_valid)
	{
		d_band_name = GPlatesUtils::UnicodeString();
		band_name_changed = true;
	}

	bool colour_palette_changed = false;
	bool colour_palette_valid = false;
	GPlatesPropertyValues::RasterType::Type raster_type = GPlatesPropertyValues::RasterType::UNKNOWN;
	if (visitor.get_proxied_raster())
	{
		const std::vector<GPlatesPropertyValues::RawRaster::non_null_ptr_type> &
			proxied_raster = *(visitor.get_proxied_raster());
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
			band_name_changed ||
			band_names_list_changed ||
			colour_palette_changed ||
			raster_type_changed)
	{
		emit_modified();
	}
}


const GPlatesPropertyValues::TextContent &
GPlatesPresentation::RasterVisualLayerParams::get_band_name() const
{
	return d_band_name;
}


void
GPlatesPresentation::RasterVisualLayerParams::set_band_name(
		const GPlatesPropertyValues::TextContent &band_name)
{
	d_band_name = band_name;
	rescan_raster_feature(true /* emit modified signal for us */);
}


const GPlatesPropertyValues::GpmlRasterBandNames::band_names_list_type &
GPlatesPresentation::RasterVisualLayerParams::get_band_names() const
{
	return d_band_names;
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
	rescan_raster_feature(true /* emit modified signal for us */);
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

