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

#include "TopologyNetworkResolverLayerOptionsWidget.h"

#include "TotalReconstructionPolesDialog.h"
#include "ViewportWindow.h"

#include "presentation/TopologyNetworkVisualLayerParams.h"


GPlatesQtWidgets::TopologyNetworkResolverLayerOptionsWidget::TopologyNetworkResolverLayerOptionsWidget(
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
			mesh_checkbox,
			SIGNAL(clicked()),
			this,
			SLOT(handle_mesh_clicked()));

	QObject::connect(
			constrained_checkbox,
			SIGNAL(clicked()),
			this,
			SLOT(handle_constrained_clicked()));

	QObject::connect(
			triangulation_checkbox,
			SIGNAL(clicked()),
			this,
			SLOT(handle_triangulation_clicked()));

	QObject::connect(
			segment_velocity_checkbox,
			SIGNAL(clicked()),
			this,
			SLOT(handle_segment_velocity_clicked()));
}


GPlatesQtWidgets::LayerOptionsWidget *
GPlatesQtWidgets::TopologyNetworkResolverLayerOptionsWidget::create(
		GPlatesAppLogic::ApplicationState &application_state,
		GPlatesPresentation::ViewState &view_state,
		ViewportWindow *viewport_window,
		QWidget *parent_)
{
	return new TopologyNetworkResolverLayerOptionsWidget(
			application_state,
			view_state,
			viewport_window,
			parent_);
}


void
GPlatesQtWidgets::TopologyNetworkResolverLayerOptionsWidget::set_data(
		const boost::weak_ptr<GPlatesPresentation::VisualLayer> &visual_layer)
{
	d_current_visual_layer = visual_layer;

	// Set the state of the checkboxes.
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = d_current_visual_layer.lock())
	{
		GPlatesPresentation::TopologyNetworkVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::TopologyNetworkVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());

		if (params)
		{
			mesh_checkbox->setChecked(params->show_mesh_triangulation());
			constrained_checkbox->setChecked(params->show_constrained_triangulation());
			triangulation_checkbox->setChecked(params->show_delaunay_triangulation());
			segment_velocity_checkbox->setChecked(params->show_segment_velocity());
		}
	}
}


const QString &
GPlatesQtWidgets::TopologyNetworkResolverLayerOptionsWidget::get_title()
{
	static const QString TITLE = tr("Network & Triangulation options");
	return TITLE;
}


void
GPlatesQtWidgets::TopologyNetworkResolverLayerOptionsWidget::handle_mesh_clicked()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = d_current_visual_layer.lock())
	{
		GPlatesPresentation::TopologyNetworkVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::TopologyNetworkVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (params)
		{
			params->set_show_mesh_triangulation(mesh_checkbox->isChecked());
		}
	}
}

void
GPlatesQtWidgets::TopologyNetworkResolverLayerOptionsWidget::handle_constrained_clicked()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = d_current_visual_layer.lock())
	{
		GPlatesPresentation::TopologyNetworkVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::TopologyNetworkVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (params)
		{
			params->set_show_constrained_triangulation(constrained_checkbox->isChecked());
		}
	}
}

void
GPlatesQtWidgets::TopologyNetworkResolverLayerOptionsWidget::handle_triangulation_clicked()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = d_current_visual_layer.lock())
	{
		GPlatesPresentation::TopologyNetworkVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::TopologyNetworkVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (params)
		{
			params->set_show_delaunay_triangulation(triangulation_checkbox->isChecked());
		}
	}
}

void
GPlatesQtWidgets::TopologyNetworkResolverLayerOptionsWidget::handle_segment_velocity_clicked()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = d_current_visual_layer.lock())
	{
		GPlatesPresentation::TopologyNetworkVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::TopologyNetworkVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (params)
		{
			params->set_show_segment_velocity(segment_velocity_checkbox->isChecked());
		}
	}
}

