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

#include "VelocityFieldCalculatorLayerOptionsWidget.h"

#include "TotalReconstructionPolesDialog.h"
#include "ViewportWindow.h"

#include "presentation/VelocityFieldCalculatorVisualLayerParams.h"


GPlatesQtWidgets::VelocityFieldCalculatorLayerOptionsWidget::VelocityFieldCalculatorLayerOptionsWidget(
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

#if 0
	QObject::connect(
			constrained_checkbox,
			SIGNAL(clicked()),
			this,
			SLOT(handle_constrained_clicked()));
#endif

	QObject::connect(
			triangulation_checkbox,
			SIGNAL(clicked()),
			this,
			SLOT(handle_triangulation_clicked()));
}


GPlatesQtWidgets::LayerOptionsWidget *
GPlatesQtWidgets::VelocityFieldCalculatorLayerOptionsWidget::create(
		GPlatesAppLogic::ApplicationState &application_state,
		GPlatesPresentation::ViewState &view_state,
		ViewportWindow *viewport_window,
		QWidget *parent_)
{
	return new VelocityFieldCalculatorLayerOptionsWidget(
			application_state,
			view_state,
			viewport_window,
			parent_);
}


void
GPlatesQtWidgets::VelocityFieldCalculatorLayerOptionsWidget::set_data(
		const boost::weak_ptr<GPlatesPresentation::VisualLayer> &visual_layer)
{
	d_current_visual_layer = visual_layer;

	// Set the state of the checkboxes.
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = d_current_visual_layer.lock())
	{
		GPlatesPresentation::VelocityFieldCalculatorVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::VelocityFieldCalculatorVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (params)
		{
			//constrained_checkbox->setChecked(params->show_constrained_vectors());
			triangulation_checkbox->setChecked(params->show_delaunay_vectors());
		}
	}
}


const QString &
GPlatesQtWidgets::VelocityFieldCalculatorLayerOptionsWidget::get_title()
{
	static const QString TITLE = tr("Velocity & Interoplation options");
	return TITLE;
}



void
GPlatesQtWidgets::VelocityFieldCalculatorLayerOptionsWidget::handle_constrained_clicked()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = d_current_visual_layer.lock())
	{
		GPlatesPresentation::VelocityFieldCalculatorVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::VelocityFieldCalculatorVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (params)
		{
			//params->set_show_constrained_vectors(constrained_checkbox->isChecked());
		}
	}
}

void
GPlatesQtWidgets::VelocityFieldCalculatorLayerOptionsWidget::handle_triangulation_clicked()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = d_current_visual_layer.lock())
	{
		GPlatesPresentation::VelocityFieldCalculatorVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::VelocityFieldCalculatorVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (params)
		{
			params->set_show_delaunay_vectors(triangulation_checkbox->isChecked());
		}
	}
}

