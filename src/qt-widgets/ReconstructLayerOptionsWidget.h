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
 
#ifndef GPLATES_QTWIDGETS_RECONSTRUCTLAYEROPTIONSWIDGET_H
#define GPLATES_QTWIDGETS_RECONSTRUCTLAYEROPTIONSWIDGET_H

#include "ReconstructLayerOptionsWidgetUi.h"

#include "InformationDialog.h"
#include "LayerOptionsWidget.h"


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
	class SetVGPVisibilityDialog;
	class SetTopologyReconstructionParametersDialog;
	class DrawStyleDialog;
	class ViewportWindow;

	/**
	 * ReconstructLayerOptionsWidget is used to show additional options for
	 * reconstructed geometry layers in the visual layers widget.
	 */
	class ReconstructLayerOptionsWidget :
			public LayerOptionsWidget,
			protected Ui_ReconstructLayerOptionsWidget
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

		~ReconstructLayerOptionsWidget();

		virtual
		void
		set_data(
				const boost::weak_ptr<GPlatesPresentation::VisualLayer> &visual_layer);

		virtual
		const QString &
		get_title();

	private Q_SLOTS:

		void
		open_vgp_visibility_dialog();

		void
		open_topology_reconstruction_parameters_dialog();

		void
		open_draw_style_setting_dlg();

		void
		handle_use_topologies_button(
				bool checked);

		void
		handle_prompt_set_topology_reconstruction_parameters_clicked();

		void
		handle_fill_polygons_clicked();

		void
		handle_fill_polylines_clicked();

		void
		handle_fill_opacity_spinbox_changed(
				double value);

		void
		handle_fill_intensity_spinbox_changed(
				double value);

	private:

		ReconstructLayerOptionsWidget(
				GPlatesAppLogic::ApplicationState &application_state,
				GPlatesPresentation::ViewState &view_state,
				ViewportWindow *viewport_window,
				QWidget *parent_);

		GPlatesAppLogic::ApplicationState &d_application_state;
		ViewportWindow *d_viewport_window;
		SetVGPVisibilityDialog *d_set_vgp_visibility_dialog;
		SetTopologyReconstructionParametersDialog *d_set_topology_reconstruction_parameters_dialog;
		DrawStyleDialog *d_draw_style_dialog_ptr;

		/**
		 * The visual layer for which we are currently displaying options.
		 */
		boost::weak_ptr<GPlatesPresentation::VisualLayer> d_current_visual_layer;

		GPlatesQtWidgets::InformationDialog *d_help_reconstruct_using_topologies_dialog;
	};
}

#endif  // GPLATES_QTWIDGETS_RECONSTRUCTLAYEROPTIONSWIDGET_H
