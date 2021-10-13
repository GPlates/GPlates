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

#include "ReconstructionLayerOptionsWidget.h"

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

	QObject::connect(
			view_total_reconstruction_poles_button,
			SIGNAL(clicked()),
			this,
			SLOT(handle_button_clicked()));
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
}


const QString &
GPlatesQtWidgets::ReconstructionLayerOptionsWidget::get_title()
{
	static const QString TITLE = "Reconstruction tree options";
	return TITLE;
}


void
GPlatesQtWidgets::ReconstructionLayerOptionsWidget::handle_button_clicked()
{
	d_viewport_window->pop_up_total_reconstruction_poles_dialog(d_current_visual_layer);
}

