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

#include "ReconstructionLayerOptionsWidget.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/ReconstructGraph.h"

#include "LinkWidget.h"
#include "QtWidgetUtils.h"
#include "TotalReconstructionPolesDialog.h"
#include "ViewportWindow.h"


GPlatesQtWidgets::ReconstructionLayerOptionsWidget::ReconstructionLayerOptionsWidget(
		GPlatesAppLogic::ApplicationState &application_state,
		GPlatesPresentation::ViewState &view_state,
		ViewportWindow *viewport_window,
		QWidget *parent_) :
	LayerOptionsWidget(parent_),
	d_application_state(application_state),
	d_view_state(view_state),
	d_viewport_window(viewport_window)
{
	setupUi(this);
	keep_as_default_checkbox->setCursor(QCursor(Qt::ArrowCursor));

	LinkWidget *view_total_reconstruction_poles_link = new LinkWidget(
			tr("View total reconstruction poles"), this);
	QtWidgetUtils::add_widget_to_placeholder(
			view_total_reconstruction_poles_link,
			view_total_reconstruction_poles_placeholder_widget);

	QObject::connect(
			view_total_reconstruction_poles_link,
			SIGNAL(link_activated()),
			this,
			SLOT(handle_view_total_reconstruction_poles_link_activated()));
	QObject::connect(
			keep_as_default_checkbox,
			SIGNAL(clicked(bool)),
			this,
			SLOT(handle_keep_as_default_checkbox_clicked(bool)));
}


GPlatesQtWidgets::LayerOptionsWidget *
GPlatesQtWidgets::ReconstructionLayerOptionsWidget::create(
		GPlatesAppLogic::ApplicationState &application_state,
		GPlatesPresentation::ViewState &view_state,
		ViewportWindow *viewport_window,
		QWidget *parent_)
{
	return new ReconstructionLayerOptionsWidget(
			application_state,
			view_state,
			viewport_window,
			parent_);
}


void
GPlatesQtWidgets::ReconstructionLayerOptionsWidget::set_data(
		const boost::weak_ptr<GPlatesPresentation::VisualLayer> &visual_layer)
{
	d_current_visual_layer = visual_layer;

	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			visual_layer.lock())
	{
		GPlatesAppLogic::Layer layer = locked_visual_layer->get_reconstruct_graph_layer();
		bool is_default = (layer == d_application_state.get_reconstruct_graph().get_default_reconstruction_tree_layer());
		keep_as_default_checkbox->setEnabled(is_default);
		keep_as_default_checkbox->setChecked(is_default &&
				!d_application_state.is_updating_default_reconstruction_tree_layer());
	}
}


const QString &
GPlatesQtWidgets::ReconstructionLayerOptionsWidget::get_title()
{
	static const QString TITLE = "Reconstruction tree options";
	return TITLE;
}


void
GPlatesQtWidgets::ReconstructionLayerOptionsWidget::handle_view_total_reconstruction_poles_link_activated()
{
	d_viewport_window->pop_up_total_reconstruction_poles_dialog(d_current_visual_layer);
}


void
GPlatesQtWidgets::ReconstructionLayerOptionsWidget::handle_keep_as_default_checkbox_clicked(
		bool checked)
{
	d_application_state.set_update_default_reconstruction_tree_layer(!checked);
}

