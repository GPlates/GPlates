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
 
#ifndef GPLATES_QTWIDGETS_RECONSTRUCTIONLAYEROPTIONSWIDGET_H
#define GPLATES_QTWIDGETS_RECONSTRUCTIONLAYEROPTIONSWIDGET_H

#include "ui_ReconstructionLayerOptionsWidgetUi.h"

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
	class MergeReconstructionLayersDialog;
	class ViewportWindow;

	/**
	 * ReconstructionLayerOptionsWidget is used to show additional options for
	 * reconstruction layers in the visual layers widget.
	 */
	class ReconstructionLayerOptionsWidget :
			public LayerOptionsWidget,
			protected Ui_ReconstructionLayerOptionsWidget
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
		handle_view_total_reconstruction_poles_link_activated();

		void
		handle_merge_reconstruction_tree_layers_link_activated();

		void
		handle_extend_total_reconstruction_poles_to_distant_past_clicked();

		void
		handle_keep_as_default_checkbox_clicked(
				bool checked);

	private:

		explicit
		ReconstructionLayerOptionsWidget(
				GPlatesAppLogic::ApplicationState &application_state,
				GPlatesPresentation::ViewState &view_state,
				ViewportWindow *viewport_window,
				QWidget *parent_);


		GPlatesAppLogic::ApplicationState &d_application_state;
		GPlatesPresentation::ViewState &d_view_state;
		ViewportWindow *d_viewport_window;
		MergeReconstructionLayersDialog *d_merge_reconstruction_layers_dialog;

		/**
		 * The visual layer for which we are currently displaying options.
		 */
		boost::weak_ptr<GPlatesPresentation::VisualLayer> d_current_visual_layer;

		GPlatesQtWidgets::InformationDialog *d_help_extend_total_reconstruction_pole_to_distant_past_dialog;
	};
}

#endif  // GPLATES_QTWIDGETS_RECONSTRUCTIONLAYEROPTIONSWIDGET_H
