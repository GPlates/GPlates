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

#include <vector>
#include <QString>

#include "RasterLayerOptionsWidgetUi.h"

#include "LayerOptionsWidget.h"
#include "OpenFileDialog.h"


namespace GPlatesAppLogic
{
	class ApplicationState;
}

namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesQtWidgets
{
	// Forward declarations.
	class ColourScaleWidget;
	class FriendlyLineEdit;
	class ReadErrorAccumulationDialog;
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

		void
		make_signal_slot_connections();

		void
		set_colour_palette(
				const QString &palette_file_name);


		GPlatesAppLogic::ApplicationState &d_application_state;
		GPlatesPresentation::ViewState &d_view_state;
		ViewportWindow *d_viewport_window;

		FriendlyLineEdit *d_palette_filename_lineedit;
		OpenFileDialog d_open_file_dialog;
		ColourScaleWidget *d_colour_scale_widget;

		/**
		 * The visual layer for which we are currently displaying options.
		 */
		boost::weak_ptr<GPlatesPresentation::VisualLayer> d_current_visual_layer;
	};
}

#endif  // GPLATES_QTWIDGETS_RASTERLAYEROPTIONSWIDGET_H
