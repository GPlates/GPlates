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

#include "TopologyBoundaryResolverLayerOptionsWidget.h"

#include "ViewportWindow.h"

#include "presentation/TopologyBoundaryVisualLayerParams.h"
#include "presentation/VisualLayer.h"


GPlatesQtWidgets::TopologyBoundaryResolverLayerOptionsWidget::TopologyBoundaryResolverLayerOptionsWidget(
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
			fill_polygons,
			SIGNAL(clicked()),
			this,
			SLOT(handle_fill_polygons_clicked()));
}


GPlatesQtWidgets::LayerOptionsWidget *
GPlatesQtWidgets::TopologyBoundaryResolverLayerOptionsWidget::create(
		GPlatesAppLogic::ApplicationState &application_state,
		GPlatesPresentation::ViewState &view_state,
		ViewportWindow *viewport_window,
		QWidget *parent_)
{
	return new TopologyBoundaryResolverLayerOptionsWidget(
			application_state,
			view_state,
			viewport_window,
			parent_);
}


void
GPlatesQtWidgets::TopologyBoundaryResolverLayerOptionsWidget::set_data(
		const boost::weak_ptr<GPlatesPresentation::VisualLayer> &visual_layer)
{
	d_current_visual_layer = visual_layer;

	// Set the state of the checkboxes.
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = d_current_visual_layer.lock())
	{
		GPlatesPresentation::TopologyBoundaryVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::TopologyBoundaryVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());

		if (params)
		{
			fill_polygons->setChecked(params->get_fill_polygons());
		}
	}
}


const QString &
GPlatesQtWidgets::TopologyBoundaryResolverLayerOptionsWidget::get_title()
{
	static const QString TITLE = tr("Topology options");
	return TITLE;
}


void
GPlatesQtWidgets::TopologyBoundaryResolverLayerOptionsWidget::handle_fill_polygons_clicked()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = d_current_visual_layer.lock())
	{
		GPlatesPresentation::TopologyBoundaryVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::TopologyBoundaryVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (params)
		{
			params->set_fill_polygons(fill_polygons->isChecked());
		}
	}
}
