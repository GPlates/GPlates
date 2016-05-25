/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2015 The University of Sydney, Australia
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
#include <QDir>
#include <QFileInfo>

#include "ReconstructScalarCoverageLayerOptionsWidget.h"

#include "QtWidgetUtils.h"
#include "ReadErrorAccumulationDialog.h"
#include "RemappedColourPaletteWidget.h"
#include "ViewportWindow.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/ReconstructScalarCoverageLayerTask.h"
#include "app-logic/ReconstructScalarCoverageParams.h"

#include "file-io/ReadErrorAccumulation.h"

#include "global/GPlatesAssert.h"

#include "gui/ColourPaletteUtils.h"
#include "gui/CptColourPalette.h"
#include "gui/Dialogs.h"

#include "presentation/ReconstructScalarCoverageVisualLayerParams.h"
#include "presentation/VisualLayer.h"

#include "property-values/ValueObjectType.h"


GPlatesQtWidgets::LayerOptionsWidget *
GPlatesQtWidgets::ReconstructScalarCoverageLayerOptionsWidget::create(
		GPlatesAppLogic::ApplicationState &application_state,
		GPlatesPresentation::ViewState &view_state,
		ViewportWindow *viewport_window,
		QWidget *parent_)
{
	return new ReconstructScalarCoverageLayerOptionsWidget(
			application_state,
			view_state,
			viewport_window,
			parent_);
}


GPlatesQtWidgets::ReconstructScalarCoverageLayerOptionsWidget::ReconstructScalarCoverageLayerOptionsWidget(
		GPlatesAppLogic::ApplicationState &application_state,
		GPlatesPresentation::ViewState &view_state,
		ViewportWindow *viewport_window,
		QWidget *parent_) :
	LayerOptionsWidget(parent_),
	d_application_state(application_state),
	d_view_state(view_state),
	d_viewport_window(viewport_window),
	d_open_file_dialog(
			this,
			tr("Open CPT File"),
			tr("Regular CPT file (*.cpt);;All files (*)"),
			view_state),
	d_colour_palette_widget(new RemappedColourPaletteWidget(view_state, viewport_window, this))
{
	setupUi(this);


	scalar_type_combobox->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			scalar_type_combobox, SIGNAL(activated(const QString &)),
			this, SLOT(handle_scalar_type_combobox_activated(const QString &)));

	//
	// Colour palette.
	//

	QtWidgetUtils::add_widget_to_placeholder(
			d_colour_palette_widget,
			palette_placeholder_widget);
	d_colour_palette_widget->setCursor(QCursor(Qt::ArrowCursor));

	QObject::connect(
			d_colour_palette_widget, SIGNAL(select_palette_filename_button_clicked()),
			this, SLOT(handle_select_palette_filename_button_clicked()));
	QObject::connect(
			d_colour_palette_widget, SIGNAL(use_default_palette_button_clicked()),
			this, SLOT(handle_use_default_palette_button_clicked()));

	QObject::connect(
			d_colour_palette_widget, SIGNAL(range_check_box_changed(int)),
			this, SLOT(handle_palette_range_check_box_changed(int)));

	QObject::connect(
			d_colour_palette_widget, SIGNAL(min_line_editing_finished(double)),
			this, SLOT(handle_palette_min_line_editing_finished(double)));
	QObject::connect(
			d_colour_palette_widget, SIGNAL(max_line_editing_finished(double)),
			this, SLOT(handle_palette_max_line_editing_finished(double)));

	QObject::connect(
			d_colour_palette_widget, SIGNAL(range_restore_min_max_button_clicked()),
			this, SLOT(handle_palette_range_restore_min_max_button_clicked()));
	QObject::connect(
			d_colour_palette_widget, SIGNAL(range_restore_mean_deviation_button_clicked()),
			this, SLOT(handle_palette_range_restore_mean_deviation_button_clicked()));
	QObject::connect(
			d_colour_palette_widget, SIGNAL(range_restore_mean_deviation_spinbox_changed(double)),
			this, SLOT(handle_palette_range_restore_mean_deviation_spinbox_changed(double)));
}


void
GPlatesQtWidgets::ReconstructScalarCoverageLayerOptionsWidget::set_data(
		const boost::weak_ptr<GPlatesPresentation::VisualLayer> &visual_layer)
{
	d_current_visual_layer = visual_layer;

	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = d_current_visual_layer.lock())
	{
		GPlatesAppLogic::Layer layer = locked_visual_layer->get_reconstruct_graph_layer();
		GPlatesAppLogic::ReconstructScalarCoverageLayerTask::Params *layer_task_params =
			dynamic_cast<GPlatesAppLogic::ReconstructScalarCoverageLayerTask::Params *>(
					&layer.get_layer_task_params());
		if (layer_task_params)
		{
			// Populate the scalar type combobox with the list of scalar types, and ensure that
			// the correct one is selected.
			int scalar_type_index = -1;
			const GPlatesPropertyValues::ValueObjectType &selected_scalar_type =
					layer_task_params->get_scalar_type();

			scalar_type_combobox->clear();
			const std::vector<GPlatesPropertyValues::ValueObjectType> &scalar_types =
				layer_task_params->get_scalar_types();
			for (int i = 0; i != static_cast<int>(scalar_types.size()); ++i)
			{
				const GPlatesPropertyValues::ValueObjectType &curr_scalar_type = scalar_types[i];
				if (curr_scalar_type == selected_scalar_type)
				{
					scalar_type_index = i;
				}
				scalar_type_combobox->addItem(convert_qualified_xml_name_to_qstring(curr_scalar_type));
			}

			scalar_type_combobox->setCurrentIndex(scalar_type_index);
		}

		GPlatesPresentation::ReconstructScalarCoverageVisualLayerParams *visual_layer_params =
			dynamic_cast<GPlatesPresentation::ReconstructScalarCoverageVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (visual_layer_params)
		{
			// Set the colour palette.
			d_colour_palette_widget->set_parameters(
					visual_layer_params->get_current_colour_palette_parameters());
		}
	}
}


const QString &
GPlatesQtWidgets::ReconstructScalarCoverageLayerOptionsWidget::get_title()
{
	static const QString TITLE = tr("Scalar Coverage options");
	return TITLE;
}


void
GPlatesQtWidgets::ReconstructScalarCoverageLayerOptionsWidget::handle_scalar_type_combobox_activated(
		const QString &text)
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		// Set the scalar type in the app-logic layer params.
		GPlatesAppLogic::Layer layer = locked_visual_layer->get_reconstruct_graph_layer();
		GPlatesAppLogic::ReconstructScalarCoverageLayerTask::Params *layer_task_params =
			dynamic_cast<GPlatesAppLogic::ReconstructScalarCoverageLayerTask::Params *>(
					&layer.get_layer_task_params());
		if (layer_task_params)
		{
			boost::optional<GPlatesPropertyValues::ValueObjectType> scalar_type =
					GPlatesModel::convert_qstring_to_qualified_xml_name<
							GPlatesPropertyValues::ValueObjectType>(text);
			if (scalar_type)
			{
				layer_task_params->set_scalar_type(scalar_type.get());
			}
		}
	}
}


void
GPlatesQtWidgets::ReconstructScalarCoverageLayerOptionsWidget::handle_select_palette_filename_button_clicked()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::ReconstructScalarCoverageVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::ReconstructScalarCoverageVisualLayerParams *>(
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

		d_view_state.get_last_open_directory() = QFileInfo(palette_file_name).path();

		std::pair<double, double> colour_palette_range;
		GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type colour_palette =
				load_colour_palette(palette_file_name, colour_palette_range);
		if (GPlatesGui::RasterColourPaletteType::get_type(*colour_palette) ==
			GPlatesGui::RasterColourPaletteType::INVALID)
		{
			return;
		}

		// Update the colour palette in the layer params.
		GPlatesPresentation::RemappedColourPaletteParameters colour_palette_parameters =
				params->get_current_colour_palette_parameters();
		colour_palette_parameters.set_colour_palette(
				palette_file_name,
				colour_palette,
				colour_palette_range);
		params->set_current_colour_palette_parameters(colour_palette_parameters);
	}
}


void
GPlatesQtWidgets::ReconstructScalarCoverageLayerOptionsWidget::handle_use_default_palette_button_clicked()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::ReconstructScalarCoverageVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::ReconstructScalarCoverageVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (params)
		{
			GPlatesPresentation::RemappedColourPaletteParameters colour_palette_parameters =
					params->get_current_colour_palette_parameters();
			colour_palette_parameters.use_default_colour_palette();
			params->set_current_colour_palette_parameters(colour_palette_parameters);
		}
	}
}


void
GPlatesQtWidgets::ReconstructScalarCoverageLayerOptionsWidget::handle_palette_range_check_box_changed(
		int state)
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::ReconstructScalarCoverageVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::ReconstructScalarCoverageVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (params)
		{
			GPlatesPresentation::RemappedColourPaletteParameters colour_palette_parameters =
					params->get_current_colour_palette_parameters();
			// Map or unmap the colour palette range.
			if (state == Qt::Checked)
			{
				colour_palette_parameters.map_palette_range(
						colour_palette_parameters.get_mapped_palette_range().first,
						colour_palette_parameters.get_mapped_palette_range().second);
			}
			else
			{
				colour_palette_parameters.unmap_palette_range();
			}
			params->set_current_colour_palette_parameters(colour_palette_parameters);
		}
	}
}


void
GPlatesQtWidgets::ReconstructScalarCoverageLayerOptionsWidget::handle_palette_min_line_editing_finished(
		double min_value)
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::ReconstructScalarCoverageVisualLayerParams *visual_layer_params =
			dynamic_cast<GPlatesPresentation::ReconstructScalarCoverageVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (visual_layer_params)
		{
			GPlatesAppLogic::Layer layer = locked_visual_layer->get_reconstruct_graph_layer();
			const std::pair<double, double> scalar_min_max = get_scalar_min_max(layer);
			const double min_range_delta = 0.0001 * (scalar_min_max.second - scalar_min_max.first);

			GPlatesPresentation::RemappedColourPaletteParameters colour_palette_parameters =
					visual_layer_params->get_current_colour_palette_parameters();

			// Ensure min not greater than max.
			if (min_value > colour_palette_parameters.get_palette_range().second/*max*/ - min_range_delta)
			{
				min_value = colour_palette_parameters.get_palette_range().second/*max*/ - min_range_delta;
			}

			colour_palette_parameters.map_palette_range(
					min_value/*min*/,
					colour_palette_parameters.get_palette_range().second/*max*/);

			visual_layer_params->set_current_colour_palette_parameters(colour_palette_parameters);
		}
	}
}


void
GPlatesQtWidgets::ReconstructScalarCoverageLayerOptionsWidget::handle_palette_max_line_editing_finished(
		double max_value)
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::ReconstructScalarCoverageVisualLayerParams *visual_layer_params =
			dynamic_cast<GPlatesPresentation::ReconstructScalarCoverageVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (visual_layer_params)
		{
			GPlatesAppLogic::Layer layer = locked_visual_layer->get_reconstruct_graph_layer();
			const std::pair<double, double> scalar_min_max = get_scalar_min_max(layer);
			const double min_range_delta = 0.0001 * (scalar_min_max.second - scalar_min_max.first);

			GPlatesPresentation::RemappedColourPaletteParameters colour_palette_parameters =
					visual_layer_params->get_current_colour_palette_parameters();

			// Ensure max not less than min.
			if (max_value < colour_palette_parameters.get_palette_range().first/*min*/ + min_range_delta)
			{
				max_value = colour_palette_parameters.get_palette_range().first/*min*/ + min_range_delta;
			}

			colour_palette_parameters.map_palette_range(
					colour_palette_parameters.get_palette_range().first/*min*/,
					max_value/*max*/);

			visual_layer_params->set_current_colour_palette_parameters(colour_palette_parameters);
		}
	}
}


void
GPlatesQtWidgets::ReconstructScalarCoverageLayerOptionsWidget::handle_palette_range_restore_min_max_button_clicked()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::ReconstructScalarCoverageVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::ReconstructScalarCoverageVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (params)
		{
			GPlatesAppLogic::Layer layer = locked_visual_layer->get_reconstruct_graph_layer();
			const std::pair<double, double> scalar_min_max = get_scalar_min_max(layer);

			GPlatesPresentation::RemappedColourPaletteParameters colour_palette_parameters =
					params->get_current_colour_palette_parameters();

			colour_palette_parameters.map_palette_range(
					scalar_min_max.first,
					scalar_min_max.second);

			params->set_current_colour_palette_parameters(colour_palette_parameters);
		}
	}
}


void
GPlatesQtWidgets::ReconstructScalarCoverageLayerOptionsWidget::handle_palette_range_restore_mean_deviation_button_clicked()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::ReconstructScalarCoverageVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::ReconstructScalarCoverageVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (params)
		{
			GPlatesAppLogic::Layer layer = locked_visual_layer->get_reconstruct_graph_layer();
			const std::pair<double, double> scalar_mean_std_dev = get_scalar_mean_std_dev(layer);

			GPlatesPresentation::RemappedColourPaletteParameters colour_palette_parameters =
					params->get_current_colour_palette_parameters();

			double range_min = scalar_mean_std_dev.first -
					colour_palette_parameters.get_deviation_from_mean() * scalar_mean_std_dev.second;
			double range_max = scalar_mean_std_dev.first +
					colour_palette_parameters.get_deviation_from_mean() * scalar_mean_std_dev.second;

			colour_palette_parameters.map_palette_range(range_min, range_max);

			params->set_current_colour_palette_parameters(colour_palette_parameters);
		}
	}
}


void
GPlatesQtWidgets::ReconstructScalarCoverageLayerOptionsWidget::handle_palette_range_restore_mean_deviation_spinbox_changed(
		double deviation_from_mean)
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::ReconstructScalarCoverageVisualLayerParams *visual_layer_params =
			dynamic_cast<GPlatesPresentation::ReconstructScalarCoverageVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (visual_layer_params)
		{
			GPlatesPresentation::RemappedColourPaletteParameters colour_palette_parameters =
					visual_layer_params->get_current_colour_palette_parameters();

			colour_palette_parameters.set_deviation_from_mean(deviation_from_mean);

			visual_layer_params->set_current_colour_palette_parameters(colour_palette_parameters);
		}
	}
}


std::pair<double, double>
GPlatesQtWidgets::ReconstructScalarCoverageLayerOptionsWidget::get_scalar_min_max(
		GPlatesAppLogic::Layer &layer) const
{
	const GPlatesAppLogic::ReconstructScalarCoverageLayerTask::Params *layer_task_params =
		dynamic_cast<const GPlatesAppLogic::ReconstructScalarCoverageLayerTask::Params *>(
				&layer.get_layer_task_params());

	// Default values result in clearing the colour scale widget.
	double scalar_min = 0;
	double scalar_max = 0;
	if (layer_task_params)
	{
		boost::optional<GPlatesPropertyValues::ScalarCoverageStatistics> statistics =
				layer_task_params->get_scalar_statistics(layer_task_params->get_scalar_type());
		if (statistics)
		{
			scalar_min = statistics->minimum;
			scalar_max = statistics->maximum;
		}
	}

	return std::make_pair(scalar_min, scalar_max);
}


std::pair<double, double>
GPlatesQtWidgets::ReconstructScalarCoverageLayerOptionsWidget::get_scalar_mean_std_dev(
		GPlatesAppLogic::Layer &layer) const
{
	const GPlatesAppLogic::ReconstructScalarCoverageLayerTask::Params *layer_task_params =
		dynamic_cast<const GPlatesAppLogic::ReconstructScalarCoverageLayerTask::Params *>(
				&layer.get_layer_task_params());

	// Default values result in clearing the colour scale widget.
	double scalar_mean = 0;
	double scalar_std_dev = 0;
	if (layer_task_params)
	{
		boost::optional<GPlatesPropertyValues::ScalarCoverageStatistics> statistics =
				layer_task_params->get_scalar_statistics(layer_task_params->get_scalar_type());
		if (statistics)
		{
			scalar_mean = statistics->mean;
			scalar_std_dev = statistics->standard_deviation;
		}
	}

	return std::make_pair(scalar_mean, scalar_std_dev);
}


GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type
GPlatesQtWidgets::ReconstructScalarCoverageLayerOptionsWidget::load_colour_palette(
		const QString &palette_file_name,
		std::pair<double, double> &colour_palette_range)
{
	GPlatesFileIO::ReadErrorAccumulation cpt_read_errors;
	GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type raster_colour_palette =
			GPlatesGui::ColourPaletteUtils::read_cpt_raster_colour_palette(
					palette_file_name,
					// We only allow real-valued colour palettes since our data is real-valued...
					false/*allow_integer_colour_palette*/,
					cpt_read_errors);

	// Copy the input value range to the caller.
	boost::optional< std::pair<double, double> > range =
			GPlatesGui::ColourPaletteUtils::get_range(*raster_colour_palette);
	if (range)
	{
		colour_palette_range = std::make_pair(range->first, range->second);
	}
	else
	{
		// Null range for empty colour palette.
		colour_palette_range = std::pair<double, double>(0,0);
	}

	// Show any read errors.
	if (cpt_read_errors.size() > 0)
	{
		ReadErrorAccumulationDialog &read_errors_dialog =
				d_viewport_window->dialogs().read_error_accumulation_dialog();

		read_errors_dialog.read_errors().accumulate(cpt_read_errors);
		read_errors_dialog.update();
		read_errors_dialog.show();
	}

	return raster_colour_palette;
}
