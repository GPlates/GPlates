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
 
#ifndef GPLATES_QTWIDGETS_RASTERLAYEROPTIONSWIDGET_H
#define GPLATES_QTWIDGETS_RASTERLAYEROPTIONSWIDGET_H

#include <utility>
#include <vector>
#include <QString>
#include <QToolButton>

#include "RasterLayerOptionsWidgetUi.h"

#include "LayerOptionsWidget.h"
#include "OpenFileDialog.h"

#include "app-logic/Layer.h"

#include "gui/RasterColourPalette.h"


namespace GPlatesAppLogic
{
	class ApplicationState;
	class Layer;
}

namespace GPlatesPresentation
{
	class RasterVisualLayerParams;
	class ViewState;
}

namespace GPlatesQtWidgets
{
	// Forward declarations.
	class ReadErrorAccumulationDialog;
	class RemappedColourPaletteWidget;
	class ViewportWindow;

	/**
	 * RasterLayerOptionsWidget is used to show additional options for raster
	 * layers in the visual layers widget.
	 */
	class RasterLayerOptionsWidget :
			public LayerOptionsWidget,
			protected Ui_RasterLayerOptionsWidget
	{
		Q_OBJECT
		
	public:

		static
		LayerOptionsWidget *
		create(
				GPlatesAppLogic::ApplicationState &application_state,
				GPlatesPresentation::ViewState &view_state,
				ViewportWindow *viewport_window,
				QWidget *parent_);

		virtual
		void
		set_data(
				const boost::weak_ptr<GPlatesPresentation::VisualLayer> &visual_layer);

		virtual
		const QString &
		get_title();

	private Q_SLOTS:

		void
		handle_band_combobox_activated(
				const QString &text);

		void
		handle_select_palette_filename_button_clicked();

		void
		handle_use_default_palette_button_clicked();

		void
		handle_use_age_palette_button_clicked();

		void
		handle_palette_range_check_box_changed(
				int state);

		void
		handle_palette_min_line_editing_finished(
				double value);

		void
		handle_palette_max_line_editing_finished(
				double value);

		void
		handle_palette_range_restore_min_max_button_clicked();

		void
		handle_palette_range_restore_mean_deviation_button_clicked();

		void
		handle_palette_range_restore_mean_deviation_spinbox_changed(
				double value);

		void
		handle_opacity_spinbox_changed(
				double value);

		void
		handle_intensity_spinbox_changed(
				double value);

		void
		handle_surface_relief_scale_spinbox_changed(
				double value);

	private:

		explicit
		RasterLayerOptionsWidget(
				GPlatesAppLogic::ApplicationState &application_state,
				GPlatesPresentation::ViewState &view_state,
				ViewportWindow *viewport_window,
				QWidget *parent_);

		std::pair<double, double>
		get_raster_scalar_min_max(
				GPlatesAppLogic::Layer &layer) const;

		std::pair<double, double>
		get_raster_scalar_mean_std_dev(
				GPlatesAppLogic::Layer &layer) const;

		GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type
		load_colour_palette(
				const QString &palette_file_name,
				std::pair<double, double> &colour_palette_range,
				const GPlatesPresentation::RasterVisualLayerParams &params);


		GPlatesAppLogic::ApplicationState &d_application_state;
		GPlatesPresentation::ViewState &d_view_state;
		ViewportWindow *d_viewport_window;

		OpenFileDialog d_open_file_dialog;
		QToolButton *d_use_age_palette_button;
		RemappedColourPaletteWidget *d_colour_palette_widget;

		/**
		 * The visual layer for which we are currently displaying options.
		 */
		boost::weak_ptr<GPlatesPresentation::VisualLayer> d_current_visual_layer;
	};
}

#endif  // GPLATES_QTWIDGETS_RASTERLAYEROPTIONSWIDGET_H
