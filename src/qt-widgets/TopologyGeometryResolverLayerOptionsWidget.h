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

#ifndef GPLATES_QT_WIDGETS_TOPOLOGYGEOMETRYRESOLVERLAYEROPTIONSWIDGET_H
#define GPLATES_QT_WIDGETS_TOPOLOGYGEOMETRYRESOLVERLAYEROPTIONSWIDGET_H

#include "ui_TopologyGeometryResolverLayerOptionsWidgetUi.h"

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
	class ViewportWindow;
	class DrawStyleDialog;

	/**
	 * TopologyGeometryResolverLayerOptionsWidget is used to show additional options for
	 * topology geometry layers in the visual layers widget.
	 */
	class TopologyGeometryResolverLayerOptionsWidget :
			public LayerOptionsWidget,
			protected Ui_TopologyGeometryResolverLayerOptionsWidget
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
		open_draw_style_setting_dlg();

		void
		handle_fill_polygons_clicked();

		void
		handle_fill_opacity_spinbox_changed(
				double value);

		void
		handle_fill_intensity_spinbox_changed(
				double value);

	private:

		TopologyGeometryResolverLayerOptionsWidget(
				GPlatesAppLogic::ApplicationState &application_state,
				GPlatesPresentation::ViewState &view_state,
				ViewportWindow *viewport_window,
				QWidget *parent_);

		GPlatesAppLogic::ApplicationState &d_application_state;
		GPlatesPresentation::ViewState &d_view_state;
		ViewportWindow *d_viewport_window;
		DrawStyleDialog		   *d_draw_style_dialog_ptr;
		/**
		 * The visual layer for which we are currently displaying options.
		 */
		boost::weak_ptr<GPlatesPresentation::VisualLayer> d_current_visual_layer;
	};
}

#endif // GPLATES_QT_WIDGETS_TOPOLOGYGEOMETRYRESOLVERLAYEROPTIONSWIDGET_H
