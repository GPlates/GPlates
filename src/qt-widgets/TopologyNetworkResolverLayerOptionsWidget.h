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
		handle_strain_rate_smoothing_button(
				bool checked);

		void
		handle_fill_triangulation_clicked();

		void
		handle_fill_rigid_blocks_clicked();

		void
		handle_segment_velocity_clicked();

		void
		handle_colour_mode_button(
				bool checked);

		void
		handle_min_abs_dilatation_spinbox_changed(
				double min_abs_dilatation);

		void
		handle_max_abs_dilatation_spinbox_changed(
				double max_abs_dilatation);

		void
		handle_select_dilatation_palette_filename_button_clicked();

		void
		handle_use_default_dilatation_palette_button_clicked();

		void
		handle_min_abs_second_invariant_spinbox_changed(
				double min_abs_second_invariant);

		void
		handle_max_abs_second_invariant_spinbox_changed(
				double max_abs_second_invariant);

		void
		handle_select_second_invariant_palette_filename_button_clicked();

		void
		handle_use_default_second_invariant_palette_button_clicked();

		void
		handle_fill_opacity_spinbox_changed(
				double value);

		void
		handle_fill_intensity_spinbox_changed(
				double value);

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
		DrawStyleDialog *d_draw_style_dialog_ptr;

		OpenFileDialog d_open_file_dialog;

		FriendlyLineEdit *d_dilatation_palette_filename_lineedit;
		ColourScaleWidget *d_dilatation_colour_scale_widget;

		FriendlyLineEdit *d_second_invariant_palette_filename_lineedit;
		ColourScaleWidget *d_second_invariant_colour_scale_widget;

		/**
		 * The visual layer for which we are currently displaying options.
		 */
		boost::weak_ptr<GPlatesPresentation::VisualLayer> d_current_visual_layer;


		//! Used to scale min/max dilatation values into their spinboxes.
		static const double DILATATION_SCALE;
		//! Used to scale min/max second invariant values into their spinboxes.
		static const double SECOND_INVARIANT_SCALE;
	};
}

#endif  // GPLATES_QTWIDGETS_TOPOLOGYNETWORKRESOLVERLAYEROPTIONSWIDGET_H
