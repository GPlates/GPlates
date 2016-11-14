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

#include "DrawStyleDialog.h"
#include "LinkWidget.h"
#include "QtWidgetUtils.h"
#include "SetVGPVisibilityDialog.h"
#include "SetTopologyReconstructionParametersDialog.h"
#include "ViewportWindow.h"
#include "VisualLayersDialog.h"

#include "app-logic/ReconstructLayerParams.h"
#include "app-logic/ReconstructParams.h"

#include "gui/Dialogs.h"

#include "presentation/ReconstructVisualLayerParams.h"
#include "presentation/VisualLayer.h"

#include "utils/ComponentManager.h"


GPlatesQtWidgets::ReconstructLayerOptionsWidget::ReconstructLayerOptionsWidget(
		GPlatesAppLogic::ApplicationState &application_state,
		GPlatesPresentation::ViewState &view_state,
		ViewportWindow *viewport_window,
		QWidget *parent_) :
	LayerOptionsWidget(parent_),
	d_application_state(application_state),
	d_viewport_window(viewport_window),
	d_set_vgp_visibility_dialog(NULL),
	d_set_topology_reconstruction_parameters_dialog(NULL),
	d_draw_style_dialog_ptr(&viewport_window->dialogs().draw_style_dialog())
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

	LinkWidget *set_topology_reconstruction_parameters_link = new LinkWidget(
			tr("Set parameters..."), this);
	QtWidgetUtils::add_widget_to_placeholder(
			set_topology_reconstruction_parameters_link,
			set_deformation_placeholder_widget);
	QObject::connect(
			set_topology_reconstruction_parameters_link,
			SIGNAL(link_activated()),
			this,
			SLOT(open_topology_reconstruction_parameters_dialog()));
	
	LinkWidget *draw_style_link = new LinkWidget(
			tr("Set Draw style..."), this);
	QtWidgetUtils::add_widget_to_placeholder(
			draw_style_link,
			draw_style_placeholder_widget);
	QObject::connect(
			draw_style_link,
			SIGNAL(link_activated()),
			this,
			SLOT(open_draw_style_setting_dlg()));

	dont_use_topologies_radio_button->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			dont_use_topologies_radio_button, SIGNAL(toggled(bool)),
			this, SLOT(handle_use_topologies_button(bool)));
	use_topologies_radio_button->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			dont_use_topologies_radio_button, SIGNAL(toggled(bool)),
			this, SLOT(handle_use_topologies_button(bool)));

	prompt_set_topology_reconstruction_parameters_check_box->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			prompt_set_topology_reconstruction_parameters_check_box, SIGNAL(clicked()),
			this, SLOT(handle_prompt_set_topology_reconstruction_parameters_clicked()));

	fill_polygons->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			fill_polygons,
			SIGNAL(clicked()),
			this,
			SLOT(handle_fill_polygons_clicked()));

	fill_polylines->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			fill_polylines,
			SIGNAL(clicked()),
			this,
			SLOT(handle_fill_polylines_clicked()));

	fill_opacity_spinbox->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			fill_opacity_spinbox,
			SIGNAL(valueChanged(double)),
			this,
			SLOT(handle_fill_opacity_spinbox_changed(double)));

	fill_intensity_spinbox->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			fill_intensity_spinbox,
			SIGNAL(valueChanged(double)),
			this,
			SLOT(handle_fill_intensity_spinbox_changed(double)));

	if(!GPlatesUtils::ComponentManager::instance().is_enabled(GPlatesUtils::ComponentManager::Component::python()))
	{
		draw_style_link->setVisible(false);
	}

}


GPlatesQtWidgets::ReconstructLayerOptionsWidget::~ReconstructLayerOptionsWidget()
{
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
		GPlatesAppLogic::Layer layer = locked_visual_layer->get_reconstruct_graph_layer();
		GPlatesAppLogic::ReconstructLayerParams *layer_params =
			dynamic_cast<GPlatesAppLogic::ReconstructLayerParams *>(
					layer.get_layer_params().get());
		if (layer_params)
		{
			prompt_set_topology_reconstruction_parameters_check_box->setChecked(
					layer_params->get_prompt_to_change_topology_reconstruction_parameters());

			const GPlatesAppLogic::ReconstructParams &reconstruct_params = layer_params->get_reconstruct_params();

			// Use Topologies Mode.
			//
			// Changing the current mode will emit signals which can lead to an infinitely recursive decent.
			// To avoid this we temporarily disconnect their signals.
			QObject::disconnect(
					dont_use_topologies_radio_button, SIGNAL(toggled(bool)),
					this, SLOT(handle_use_topologies_button(bool)));
			QObject::disconnect(
					dont_use_topologies_radio_button, SIGNAL(toggled(bool)),
					this, SLOT(handle_use_topologies_button(bool)));
			if (reconstruct_params.get_reconstruct_using_topologies())
			{
				use_topologies_radio_button->setChecked(true);

				prompt_set_topology_reconstruction_parameters_check_box->hide();
				set_deformation_placeholder_widget->show();
			}
			else
			{
				dont_use_topologies_radio_button->setChecked(true);

				prompt_set_topology_reconstruction_parameters_check_box->show();
				set_deformation_placeholder_widget->hide();
			}
			QObject::connect(
					dont_use_topologies_radio_button, SIGNAL(toggled(bool)),
					this, SLOT(handle_use_topologies_button(bool)));
			QObject::connect(
					dont_use_topologies_radio_button, SIGNAL(toggled(bool)),
					this, SLOT(handle_use_topologies_button(bool)));
		}

		GPlatesPresentation::ReconstructVisualLayerParams *visual_layer_params =
			dynamic_cast<GPlatesPresentation::ReconstructVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());

		if (visual_layer_params)
		{
			fill_polygons->setChecked(visual_layer_params->get_fill_polygons());
			fill_polylines->setChecked(visual_layer_params->get_fill_polylines());

			// Setting the values in the spin boxes will emit signals if the value changes
			// which can lead to an infinitely recursive decent.
			// To avoid this we temporarily disconnect their signals.

			QObject::disconnect(
					fill_opacity_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_fill_opacity_spinbox_changed(double)));
			fill_opacity_spinbox->setValue(visual_layer_params->get_fill_opacity());
			QObject::connect(
					fill_opacity_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_fill_opacity_spinbox_changed(double)));

			QObject::disconnect(
					fill_intensity_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_fill_intensity_spinbox_changed(double)));
			fill_intensity_spinbox->setValue(visual_layer_params->get_fill_intensity());
			QObject::connect(
					fill_intensity_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_fill_intensity_spinbox_changed(double)));
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
				&d_viewport_window->dialogs().visual_layers_dialog());
	}

	d_set_vgp_visibility_dialog->populate(d_current_visual_layer);

	// This dialog is shown modally.
	d_set_vgp_visibility_dialog->exec();
}


void
GPlatesQtWidgets::ReconstructLayerOptionsWidget::open_topology_reconstruction_parameters_dialog()
{
	if (!d_set_topology_reconstruction_parameters_dialog)
	{
		d_set_topology_reconstruction_parameters_dialog = new SetTopologyReconstructionParametersDialog(
				d_application_state,
				&d_viewport_window->dialogs().visual_layers_dialog());
	}

	d_set_topology_reconstruction_parameters_dialog->populate(d_current_visual_layer);

	// This dialog is shown modally.
	d_set_topology_reconstruction_parameters_dialog->exec();
}


void
GPlatesQtWidgets::ReconstructLayerOptionsWidget::open_draw_style_setting_dlg()
{
	//qDebug() << "popup draw style dialog...";
	QtWidgetUtils::pop_up_dialog(d_draw_style_dialog_ptr);
	d_draw_style_dialog_ptr->reset(d_current_visual_layer);
}


void
GPlatesQtWidgets::ReconstructLayerOptionsWidget::handle_use_topologies_button(
		bool checked)
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = d_current_visual_layer.lock())
	{
		GPlatesAppLogic::Layer layer = locked_visual_layer->get_reconstruct_graph_layer();
		GPlatesAppLogic::ReconstructLayerParams *layer_params =
			dynamic_cast<GPlatesAppLogic::ReconstructLayerParams *>(
					layer.get_layer_params().get());
		if (layer_params)
		{
			if (dont_use_topologies_radio_button->isChecked())
			{
				GPlatesAppLogic::ReconstructParams reconstruct_params = layer_params->get_reconstruct_params();
				reconstruct_params.set_reconstruct_using_topologies(false);
				layer_params->set_reconstruct_params(reconstruct_params);

				prompt_set_topology_reconstruction_parameters_check_box->show();
				set_deformation_placeholder_widget->hide();
			}

			if (use_topologies_radio_button->isChecked())
			{
				if (layer_params->get_prompt_to_change_topology_reconstruction_parameters())
				{
					// Ask the user to modify the reconstruct params *before* we switch to using topologies
					// so that we don't get hit by a long topology reconstruction initialisation twice
					// (once when switching it on and again when user changes parameters).
					open_topology_reconstruction_parameters_dialog();
				}

				// Switch to using topologies.
				GPlatesAppLogic::ReconstructParams reconstruct_params = layer_params->get_reconstruct_params();
				reconstruct_params.set_reconstruct_using_topologies(true);
				layer_params->set_reconstruct_params(reconstruct_params);

				prompt_set_topology_reconstruction_parameters_check_box->hide();
				set_deformation_placeholder_widget->show();
			}
		}
	}
}


void
GPlatesQtWidgets::ReconstructLayerOptionsWidget::handle_prompt_set_topology_reconstruction_parameters_clicked()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = d_current_visual_layer.lock())
	{
		GPlatesAppLogic::Layer layer = locked_visual_layer->get_reconstruct_graph_layer();
		GPlatesAppLogic::ReconstructLayerParams *layer_params =
			dynamic_cast<GPlatesAppLogic::ReconstructLayerParams *>(
					layer.get_layer_params().get());
		if (layer_params)
		{
			layer_params->set_prompt_to_change_topology_reconstruction_parameters(
					prompt_set_topology_reconstruction_parameters_check_box->isChecked());
		}
	}
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


void
GPlatesQtWidgets::ReconstructLayerOptionsWidget::handle_fill_polylines_clicked()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = d_current_visual_layer.lock())
	{
		GPlatesPresentation::ReconstructVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::ReconstructVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (params)
		{
			params->set_fill_polylines(fill_polylines->isChecked());
		}
	}
}


void
GPlatesQtWidgets::ReconstructLayerOptionsWidget::handle_fill_opacity_spinbox_changed(
		double value)
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::ReconstructVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::ReconstructVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (params)
		{
			params->set_fill_opacity(value);
		}
	}
}


void
GPlatesQtWidgets::ReconstructLayerOptionsWidget::handle_fill_intensity_spinbox_changed(
		double value)
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::ReconstructVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::ReconstructVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (params)
		{
			params->set_fill_intensity(value);
		}
	}
}
