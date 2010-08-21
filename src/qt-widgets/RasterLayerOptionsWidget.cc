/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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

#include <boost/foreach.hpp>
#include <QFileDialog>
#include <QDebug>

#include "RasterLayerOptionsWidget.h"

#include "FriendlyLineEdit.h"
#include "QtWidgetUtils.h"
#include "ReadErrorAccumulationDialog.h"

#include "app-logic/FeatureCollectionFileState.h"

#include "file-io/CptReader.h"
#include "file-io/File.h"
#include "file-io/ReadErrorAccumulation.h"

#include "gui/ColourPaletteAdapter.h"
#include "gui/CptColourPalette.h"
#include "gui/RasterColourSchemeMap.h"

#include "model/FeatureCollectionHandle.h"
#include "model/FeatureVisitor.h"

#include "presentation/ViewState.h"

#include "property-values/GmlFile.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlPiecewiseAggregation.h"
#include "property-values/GpmlRasterBandNames.h"
#include "property-values/RawRasterUtils.h"
#include "property-values/XsString.h"

#include "utils/UnicodeStringUtils.h"


namespace
{
	class ExtractRasterProperties :
			public GPlatesModel::ConstFeatureVisitor
	{
	public:

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
			d_raster_band_names = boost::none;

			return true;
		}

		virtual
		void
		visit_gpml_raster_band_names(
				const GPlatesPropertyValues::GpmlRasterBandNames &gpml_raster_band_names)
		{
			static const GPlatesModel::PropertyName BAND_NAMES =
				GPlatesModel::PropertyName::create_gpml("bandNames");

			const boost::optional<GPlatesModel::PropertyName> &propname = current_top_level_propname();
			if (propname && *propname == BAND_NAMES)
			{
				d_raster_band_names = gpml_raster_band_names.band_names();
			}
		}

	private:

		/**
		 * The list of band names - one for each proxied raster.
		 */
		boost::optional<GPlatesPropertyValues::GpmlRasterBandNames::band_names_list_type> d_raster_band_names;
	};
}


const QString
GPlatesQtWidgets::RasterLayerOptionsWidget::PALETTE_FILENAME_BLANK_TEXT("Default Palette");


GPlatesQtWidgets::RasterLayerOptionsWidget::RasterLayerOptionsWidget(
		GPlatesAppLogic::ApplicationState &application_state,
		GPlatesPresentation::ViewState &view_state,
		QString &open_file_path,
		ReadErrorAccumulationDialog *read_errors_dialog,
		QWidget *parent_) :
	d_application_state(application_state),
	d_view_state(view_state),
	d_open_file_path(open_file_path),
	d_read_errors_dialog(read_errors_dialog),
	d_palette_filename_lineedit(
			new FriendlyLineEdit(
				QString(),
				PALETTE_FILENAME_BLANK_TEXT,
				this))
{
	setupUi(this);

	d_palette_filename_lineedit->setReadOnly(true);
	QtWidgetUtils::add_widget_to_placeholder(
			d_palette_filename_lineedit,
			palette_filename_placeholder_widget);

	make_signal_slot_connections();
}


void
GPlatesQtWidgets::RasterLayerOptionsWidget::set_data(
		const GPlatesAppLogic::Layer &layer)
{
	d_current_layer = layer;

	// Get at the feature collection used as main input into the layer.
	QString main_channel = layer.get_main_input_feature_collection_channel();
	std::vector<GPlatesAppLogic::Layer::InputConnection> input_connections =
		layer.get_channel_inputs(main_channel);
	if (input_connections.size() != 1)
	{
		return;
	}
	boost::optional<GPlatesAppLogic::Layer::InputFile> input_file =
		input_connections[0].get_input_file();
	if (!input_file)
	{
		return;
	}
	const GPlatesFileIO::File::Reference &file = input_file->get_file().get_file();
	GPlatesModel::FeatureCollectionHandle::weak_ref feature_collection = file.get_feature_collection();
	if (feature_collection.is_valid() && feature_collection->size() != 1)
	{
		return;
	}

	// Get the raster bands out of the single feature.
	ExtractRasterProperties visitor;
	visitor.visit_feature(feature_collection->begin());
	typedef GPlatesPropertyValues::GpmlRasterBandNames::band_names_list_type band_names_list_type;
	const boost::optional<band_names_list_type> &raster_band_names_opt = visitor.get_raster_band_names();
	if (!raster_band_names_opt || raster_band_names_opt->size() == 0)
	{
		return;
	}
	const band_names_list_type &raster_band_names = *raster_band_names_opt;

	// Populate the band combobox.
	band_combobox->clear();
	BOOST_FOREACH(const GPlatesPropertyValues::XsString::non_null_ptr_to_const_type &band_name, raster_band_names)
	{
		band_combobox->addItem(GPlatesUtils::make_qstring_from_icu_string(band_name->value().get()));
	}

	GPlatesGui::RasterColourSchemeMap &map = d_view_state.get_raster_colour_scheme_map();
	boost::optional<GPlatesGui::RasterColourSchemeMap::layer_info_type> layer_info =
		map.get_layer_info(layer);
	if (layer_info)
	{
		// Find the currently selected band name in the list of band names
		// drawn from the raster.
		const GPlatesGui::RasterColourScheme::non_null_ptr_type &colour_scheme =
			layer_info->colour_scheme;
		const GPlatesPropertyValues::TextContent &selected_band_name =
			colour_scheme->get_band_name();
		unsigned int band_name_index = 0;
		for (unsigned int i = 0; i != raster_band_names.size(); ++i)
		{
			if (selected_band_name == raster_band_names[i]->value())
			{
				band_name_index = i;
				break;
			}
		}
		band_combobox->setCurrentIndex(band_name_index);

		// Populate the palette filename.
		d_palette_filename_lineedit->setText(layer_info->palette_file_name);
	}
	else
	{
		// Initialise to sensible defaults.
		band_combobox->setCurrentIndex(0);
		d_palette_filename_lineedit->setText(QString());
	}
}


void
GPlatesQtWidgets::RasterLayerOptionsWidget::handle_band_combobox_activated(
		const QString &text)
{
	UnicodeString band_name = GPlatesUtils::make_icu_string_from_qstring(text);
	QString palette_file_name = d_palette_filename_lineedit->text();

	GPlatesGui::RasterColourSchemeMap &map = d_view_state.get_raster_colour_scheme_map();
	boost::optional<GPlatesGui::RasterColourSchemeMap::layer_info_type> layer_info =
		map.get_layer_info(d_current_layer);
	if (layer_info)
	{
		GPlatesGui::RasterColourScheme::non_null_ptr_type new_colour_scheme =
			GPlatesGui::RasterColourScheme::create_from_existing(
					layer_info->colour_scheme, band_name);
		map.set_colour_scheme(
				d_current_layer,
				new_colour_scheme,
				palette_file_name);
	}
	else
	{
		GPlatesGui::RasterColourScheme::non_null_ptr_type new_colour_scheme =
			GPlatesGui::RasterColourScheme::create(band_name);
		map.set_colour_scheme(
				d_current_layer,
				new_colour_scheme,
				palette_file_name);
	}
}


void
GPlatesQtWidgets::RasterLayerOptionsWidget::handle_select_palette_filename_button_clicked()
{
	// FIXME: This currently just reads in regular CPT files.
	QString palette_file_name = QFileDialog::getOpenFileName(
			this,
			"Open CPT File",
			QString(),
			"CPT file (*.cpt);;All files (*)");
	if (!palette_file_name.isEmpty())
	{
		GPlatesFileIO::ReadErrorAccumulation &read_errors = d_read_errors_dialog->read_errors();
		GPlatesFileIO::ReadErrorAccumulation::size_type num_initial_errors = read_errors.size();

		GPlatesFileIO::RegularCptReader cpt_reader;
		GPlatesGui::RegularCptColourPalette::maybe_null_ptr_type colour_palette_opt =
			cpt_reader.read_file(palette_file_name, read_errors);

		if (colour_palette_opt)
		{
			GPlatesGui::ColourPalette<double>::non_null_ptr_type colour_palette =
				GPlatesGui::convert_colour_palette<
					GPlatesMaths::Real,
					double
				>(colour_palette_opt.get(), GPlatesGui::RealToBuiltInConverter<double>());
			UnicodeString band_name = GPlatesUtils::make_icu_string_from_qstring(
					band_combobox->currentText());
			GPlatesGui::RasterColourScheme::non_null_ptr_type colour_scheme =
				GPlatesGui::RasterColourScheme::create<double>(band_name, colour_palette);

			GPlatesGui::RasterColourSchemeMap &map = d_view_state.get_raster_colour_scheme_map();
			map.set_colour_scheme(
					d_current_layer,
					colour_scheme,
					palette_file_name);

			d_palette_filename_lineedit->setText(palette_file_name);
		}

		d_read_errors_dialog->update();
		GPlatesFileIO::ReadErrorAccumulation::size_type num_final_errors = read_errors.size();
		if (num_initial_errors != num_final_errors)
		{
			d_read_errors_dialog->show();
		}
	}
}


void
GPlatesQtWidgets::RasterLayerOptionsWidget::handle_use_default_palette_button_clicked()
{
	QString palette_file_name; // empty string.
	UnicodeString band_name = GPlatesUtils::make_icu_string_from_qstring(
			band_combobox->currentText());
	GPlatesGui::RasterColourScheme::non_null_ptr_type colour_scheme =
		GPlatesGui::RasterColourScheme::create(band_name);

	GPlatesGui::RasterColourSchemeMap &map = d_view_state.get_raster_colour_scheme_map();
	map.set_colour_scheme(
			d_current_layer,
			colour_scheme,
			palette_file_name);

	d_palette_filename_lineedit->setText(QString());
}


void
GPlatesQtWidgets::RasterLayerOptionsWidget::make_signal_slot_connections()
{
	QObject::connect(
			band_combobox,
			SIGNAL(activated(const QString &)),
			this,
			SLOT(handle_band_combobox_activated(const QString &)));
	QObject::connect(
			select_palette_filename_button,
			SIGNAL(clicked()),
			this,
			SLOT(handle_select_palette_filename_button_clicked()));
	QObject::connect(
			use_default_palette_button,
			SIGNAL(clicked()),
			this,
			SLOT(handle_use_default_palette_button_clicked()));
}

