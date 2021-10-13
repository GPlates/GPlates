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

#include "ReconstructLayerOptionsWidget.h"

#include "LinkWidget.h"
#include "QtWidgetUtils.h"
#include "SetVGPVisibilityDialog.h"
#include "ViewportWindow.h"
#include "VisualLayersDialog.h"

#include "presentation/ReconstructVisualLayerParams.h"
#include "presentation/VisualLayer.h"


GPlatesQtWidgets::ReconstructLayerOptionsWidget::ReconstructLayerOptionsWidget(
		GPlatesAppLogic::ApplicationState &application_state,
		GPlatesPresentation::ViewState &view_state,
		ViewportWindow *viewport_window,
		QWidget *parent_) :
	LayerOptionsWidget(parent_),
	d_application_state(application_state),
	d_viewport_window(viewport_window),
	d_set_vgp_visibility_dialog(NULL)
{
	setupUi(this);

	LinkWidget *set_vgp_visibility_link = new LinkWidget(
			tr("Set VGP visibility..."), this);
	QtWidgetUtils::add_widget_to_placeholder(
			set_vgp_visibility_link,
			set_vgp_visibility_placeholder_widget);
	QObject::connect(
			set_vgp_visibility_link,
			SIGNAL(link_activated()),
			this,
			SLOT(open_vgp_visibility_dialog()));
	QObject::connect(
			fill_polygons,
			SIGNAL(clicked()),
			this,
			SLOT(handle_fill_polygons_clicked()));

}


GPlatesQtWidgets::LayerOptionsWidget *
GPlatesQtWidgets::ReconstructLayerOptionsWidget::create(
		GPlatesAppLogic::ApplicationState &application_state,
		GPlatesPresentation::ViewState &view_state,
		ViewportWindow *viewport_window,
		QWidget *parent_)
{
	return new ReconstructLayerOptionsWidget(
			application_state,
			view_state,
			viewport_window,
			parent_);
}


void
GPlatesQtWidgets::ReconstructLayerOptionsWidget::set_data(
		const boost::weak_ptr<GPlatesPresentation::VisualLayer> &visual_layer)
{
	d_current_visual_layer = visual_layer;

	// Set the state of the filled polygon checkbox.
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = d_current_visual_layer.lock())
	{
		GPlatesPresentation::ReconstructVisualLayerParams *visual_layer_params =
			dynamic_cast<GPlatesPresentation::ReconstructVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());

		if (visual_layer_params)
		{
			fill_polygons->setChecked(visual_layer_params->get_fill_polygons());
		}
	}
}


const QString &
GPlatesQtWidgets::ReconstructLayerOptionsWidget::get_title()
{
	static const QString TITLE = "Reconstruction options";
	return TITLE;
}


void
GPlatesQtWidgets::ReconstructLayerOptionsWidget::open_vgp_visibility_dialog()
{
	if (!d_set_vgp_visibility_dialog)
	{
		d_set_vgp_visibility_dialog = new SetVGPVisibilityDialog(
				d_application_state,
				&d_viewport_window->visual_layers_dialog());
	}

	d_set_vgp_visibility_dialog->populate(d_current_visual_layer);

	// This dialog is shown modally.
	d_set_vgp_visibility_dialog->exec();
}


void
GPlatesQtWidgets::ReconstructLayerOptionsWidget::handle_fill_polygons_clicked()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = d_current_visual_layer.lock())
	{
		GPlatesPresentation::ReconstructVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::ReconstructVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (params)
		{
			params->set_fill_polygons(fill_polygons->isChecked());
		}
	}
}
