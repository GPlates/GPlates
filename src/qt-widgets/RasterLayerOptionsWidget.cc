/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2010, 2011 The University of Sydney, Australia
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

#include <boost/shared_ptr.hpp>
#include <QFileInfo>
#include <QDir>
#include <QPalette>

#include "RasterLayerOptionsWidget.h"

#include "ColourScaleWidget.h"
#include "FriendlyLineEdit.h"
#include "QtWidgetUtils.h"
#include "ReadErrorAccumulationDialog.h"
#include "ViewportWindow.h"

#include "file-io/CptReader.h"
#include "file-io/ReadErrorAccumulation.h"

#include "gui/ColourPaletteAdapter.h"
#include "gui/CptColourPalette.h"

#include "presentation/RasterVisualLayerParams.h"
#include "presentation/VisualLayer.h"

#include "property-values/XsString.h"


GPlatesQtWidgets::RasterLayerOptionsWidget::RasterLayerOptionsWidget(
		GPlatesAppLogic::ApplicationState &application_state,
		GPlatesPresentation::ViewState &view_state,
		ViewportWindow *viewport_window,
		QWidget *parent_) :
	LayerOptionsWidget(parent_),
	d_application_state(application_state),
	d_view_state(view_state),
	d_viewport_window(viewport_window),
	d_palette_filename_lineedit(
			new FriendlyLineEdit(
				QString(),
				tr("Default Palette"),
				this)),
	d_open_file_dialog(
			this,
			tr("Open CPT File"),
			tr("Regular CPT file (*.cpt);;All files (*)"),
			view_state),
	d_colour_scale_widget(
			new ColourScaleWidget(
				view_state,
				viewport_window,
				this))
{
	setupUi(this);
	band_combobox->setCursor(QCursor(Qt::ArrowCursor));
	select_palette_filename_button->setCursor(QCursor(Qt::ArrowCursor));
	use_default_palette_button->setCursor(QCursor(Qt::ArrowCursor));

	d_palette_filename_lineedit->setReadOnly(true);
	QtWidgetUtils::add_widget_to_placeholder(
			d_palette_filename_lineedit,
			palette_filename_placeholder_widget);

	QtWidgetUtils::add_widget_to_placeholder(
			d_colour_scale_widget,
			colour_scale_placeholder_widget);
	QPalette colour_scale_palette = d_colour_scale_widget->palette();
	colour_scale_palette.setColor(QPalette::Window, Qt::white);
	d_colour_scale_widget->setPalette(colour_scale_palette);

	make_signal_slot_connections();
}


GPlatesQtWidgets::LayerOptionsWidget *
GPlatesQtWidgets::RasterLayerOptionsWidget::create(
		GPlatesAppLogic::ApplicationState &application_state,
		GPlatesPresentation::ViewState &view_state,
		ViewportWindow *viewport_window,
		QWidget *parent_)
{
	return new RasterLayerOptionsWidget(
			application_state,
			view_state,
			viewport_window,
			parent_);
}


void
GPlatesQtWidgets::RasterLayerOptionsWidget::set_data(
		const boost::weak_ptr<GPlatesPresentation::VisualLayer> &visual_layer)
{
	d_current_visual_layer = visual_layer;

	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			visual_layer.lock())
	{
		GPlatesPresentation::RasterVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::RasterVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (params)
		{
			// Populate the band combobox with the list of band names, and ensure that
			// the correct one is selected.
			int band_name_index = -1;
			const GPlatesPropertyValues::TextContent &selected_band_name = params->get_band_name();

			band_combobox->clear();
			const GPlatesPropertyValues::GpmlRasterBandNames::band_names_list_type &band_names =
				params->get_band_names();
			for (int i = 0; i != static_cast<int>(band_names.size()); ++i)
			{
				const GPlatesPropertyValues::TextContent &curr_band_name = band_names[i]->value();
				if (curr_band_name == selected_band_name)
				{
					band_name_index = i;
				}
				band_combobox->addItem(curr_band_name.get().qstring());
			}

			band_combobox->setCurrentIndex(band_name_index);

			// Hide colour palette-related widgets if raster type is RGBA8.
			bool is_rgba8 = (params->get_raster_type() == GPlatesPropertyValues::RasterType::RGBA8);
			palette_label->setVisible(!is_rgba8);
			palette_widget->setVisible(!is_rgba8);
			bool show_colour_scale = false;
			if (!is_rgba8)
			{
				show_colour_scale = d_colour_scale_widget->populate(params->get_colour_palette());
			}
			colour_scale_placeholder_widget->setVisible(show_colour_scale);

			if (!is_rgba8)
			{
				// Populate the palette filename.
				d_palette_filename_lineedit->setText(params->get_colour_palette_filename());
			}
		}
	}
}


const QString &
GPlatesQtWidgets::RasterLayerOptionsWidget::get_title()
{
	static const QString TITLE = tr("Raster options");
	return TITLE;
}


void
GPlatesQtWidgets::RasterLayerOptionsWidget::handle_band_combobox_activated(
		const QString &text)
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::RasterVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::RasterVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (params)
		{
			params->set_band_name(GPlatesUtils::UnicodeString(text));
		}
	}
}


void
GPlatesQtWidgets::RasterLayerOptionsWidget::handle_select_palette_filename_button_clicked()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::RasterVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::RasterVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (!params)
		{
			return;
		}

		QString palette_file_name = d_open_file_dialog.get_open_file_name();
		if (palette_file_name.isEmpty())
		{
			return;
		}

		ReadErrorAccumulationDialog &read_errors_dialog = d_viewport_window->read_errors_dialog();
		GPlatesFileIO::ReadErrorAccumulation &read_errors = read_errors_dialog.read_errors();
		GPlatesFileIO::ReadErrorAccumulation::size_type num_initial_errors = read_errors.size();

		GPlatesFileIO::RegularCptReader regular_cpt_reader;
		GPlatesFileIO::ReadErrorAccumulation regular_errors;
		GPlatesGui::RegularCptColourPalette::maybe_null_ptr_type regular_colour_palette_opt =
			regular_cpt_reader.read_file(palette_file_name, regular_errors);

		// There is a slight complication in the detection of whether a
		// CPT file is regular or categorical. For the most part, a line
		// in a categorical CPT file looks nothing like a line in a
		// regular CPT file and will not be successfully parsed; the
		// exception to the rule are the "BFN" lines, the format of
		// which is common to both regular and categorical CPT files.
		// For that reason, we also check if the regular_palette has any
		// ColourSlices.
		//
		// Note: this flow of code is very similar to that in class IntegerCptReader.
		if (regular_colour_palette_opt && regular_colour_palette_opt->size())
		{
			// Add all the errors reported to read_errors.
			read_errors.accumulate(regular_errors);

			GPlatesGui::ColourPalette<double>::non_null_ptr_type colour_palette =
				GPlatesGui::convert_colour_palette<
					GPlatesMaths::Real,
					double
				>(regular_colour_palette_opt.get(), GPlatesGui::RealToBuiltInConverter<double>());

			params->set_colour_palette(palette_file_name,
					GPlatesGui::RasterColourPalette::create<double>(colour_palette));

			d_palette_filename_lineedit->setText(QDir::toNativeSeparators(palette_file_name));
		}
		else
		{
			// Attempt to read the file as a regular CPT file has failed.
			// Now, let's try to parse it as a categorical CPT file.
			GPlatesFileIO::CategoricalCptReader<boost::int32_t>::Type categorical_cpt_reader;
			GPlatesFileIO::ReadErrorAccumulation categorical_errors;
			GPlatesGui::CategoricalCptColourPalette<boost::int32_t>::maybe_null_ptr_type categorical_colour_palette_opt =
				categorical_cpt_reader.read_file(palette_file_name, categorical_errors);

			if (categorical_colour_palette_opt)
			{
				// This time, we return the colour palette even if it just
				// contains "BFN" lines and no ColourEntrys.

				// Add all the errors reported to errors.
				read_errors.accumulate(categorical_errors);

				const GPlatesGui::ColourPalette<boost::int32_t>::non_null_ptr_type colour_palette(
						categorical_colour_palette_opt.get());

				params->set_colour_palette(palette_file_name,
						GPlatesGui::RasterColourPalette::create<boost::int32_t>(colour_palette));

				d_palette_filename_lineedit->setText(QDir::toNativeSeparators(palette_file_name));
			}
		}

		read_errors_dialog.update();
		GPlatesFileIO::ReadErrorAccumulation::size_type num_final_errors = read_errors.size();
		if (num_initial_errors != num_final_errors)
		{
			read_errors_dialog.show();
		}

		d_view_state.get_last_open_directory() = QFileInfo(palette_file_name).path();
	}
}


void
GPlatesQtWidgets::RasterLayerOptionsWidget::handle_use_default_palette_button_clicked()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::RasterVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::RasterVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (params)
		{
			params->use_auto_generated_colour_palette();
		}
	}
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

