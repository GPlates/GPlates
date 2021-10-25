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
 
#ifndef GPLATES_QTWIDGETS_TOPOLOGYNETWORKRESOLVERLAYEROPTIONSWIDGET_H
#define GPLATES_QTWIDGETS_TOPOLOGYNETWORKRESOLVERLAYEROPTIONSWIDGET_H

#include "TopologyNetworkResolverLayerOptionsWidgetUi.h"

#include "LayerOptionsWidget.h"
#include "DrawStyleDialog.h"
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
	// Forward declaration.
	class ViewportWindow;
	class ColourScaleWidget;
	class FriendlyLineEdit;
	class ReadErrorAccumulationDialog;

	/**
	 * TopologyNetworkResolverLayerOptionsWidget is used to show additional options for
	 * topology network layers in the visual layers widget.
	 */
	class TopologyNetworkResolverLayerOptionsWidget :
			public LayerOptionsWidget,
			protected Ui_TopologyNetworkResolverLayerOptionsWidget
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
		handle_triangulation_clicked();

		void
		handle_constrained_clicked();

		void
		handle_mesh_clicked();

		void
		handle_total_triangulation_clicked();

		void
		handle_color_index_combobox_activated();

		void
		handle_fill_clicked();

		void
		handle_segment_velocity_clicked();

		void
		handle_update_button_clicked();

		void
		handle_select_palette_filename_button_clicked();

		void
		handle_use_default_palette_button_clicked();

		void
		open_draw_style_setting_dlg();

	private:

		TopologyNetworkResolverLayerOptionsWidget(
				GPlatesAppLogic::ApplicationState &application_state,
				GPlatesPresentation::ViewState &view_state,
				ViewportWindow *viewport_window,
				QWidget *parent_);

		GPlatesAppLogic::ApplicationState &d_application_state;
		GPlatesPresentation::ViewState &d_view_state;
		ViewportWindow *d_viewport_window;
		DrawStyleDialog		   *d_draw_style_dialog_ptr;

		FriendlyLineEdit *d_palette_filename_lineedit;
		OpenFileDialog d_open_file_dialog;

		ColourScaleWidget *d_colour_scale_widget;

		/**
		 * The visual layer for which we are currently displaying options.
		 */
		boost::weak_ptr<GPlatesPresentation::VisualLayer> d_current_visual_layer;
	};
}

#endif  // GPLATES_QTWIDGETS_TOPOLOGYNETWORKRESOLVERLAYEROPTIONSWIDGET_H
