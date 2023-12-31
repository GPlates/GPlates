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

#include <algorithm>

#include "ReconstructionLayerOptionsWidget.h"

#include "LinkWidget.h"
#include "MergeReconstructionLayersDialog.h"
#include "QtWidgetUtils.h"
#include "ViewportWindow.h"
#include "VisualLayersDialog.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/Layer.h"
#include "app-logic/ReconstructionLayerParams.h"
#include "app-logic/ReconstructionParams.h"

#include "gui/Dialogs.h"

#include "presentation/ViewState.h"
#include "presentation/VisualLayer.h"


namespace
{
	const QString HELP_EXTEND_TOTAL_RECONSTRUCTION_POLE_TO_DISTANT_PAST_DIALOG_TITLE =
			QObject::tr("Extending total rotation poles to distant past");
	const QString HELP_EXTEND_TOTAL_RECONSTRUCTION_POLE_TO_DISTANT_PAST_DIALOG_TEXT = QObject::tr(
			"<html><body>\n"
			"<p> When this is enabled each moving plate rotation sequence is extended infinitely far into "
			"the distant past such that reconstructed geometries no longer snap back to their present day positions "
			"when the reconstruction time is older than the oldest time instants specified in the rotation file.</p>"
			"<p> To accomplish this, for each moving plate in the rotation file(s), the total rotation pole "
			"at the oldest time of the oldest fixed plate sequence is extended infinitely far into the distant past. "
			"For example, moving plate 9 might move relative to plate 7 (from 0 - 200Ma) and relative to plate 8 "
			"(from 200 - 400Ma), and so the pole at 400Ma (belonging to the older sequence 8->9) is extended "
			"such that the total rotation of plate 9 relative to plate 8 for any time older than 400Ma is equal to "
			"that 400Ma pole.</p>"
			"</body></html>\n");
}


GPlatesQtWidgets::ReconstructionLayerOptionsWidget::ReconstructionLayerOptionsWidget(
		GPlatesAppLogic::ApplicationState &application_state,
		GPlatesPresentation::ViewState &view_state,
		ViewportWindow *viewport_window,
		QWidget *parent_) :
	LayerOptionsWidget(parent_),
	d_application_state(application_state),
	d_view_state(view_state),
	d_viewport_window(viewport_window),
	d_merge_reconstruction_layers_dialog(NULL),
	d_help_extend_total_reconstruction_pole_to_distant_past_dialog(
			new InformationDialog(
					HELP_EXTEND_TOTAL_RECONSTRUCTION_POLE_TO_DISTANT_PAST_DIALOG_TEXT,
					HELP_EXTEND_TOTAL_RECONSTRUCTION_POLE_TO_DISTANT_PAST_DIALOG_TITLE,
					viewport_window))
{
	setupUi(this);
	keep_as_default_checkbox->setCursor(QCursor(Qt::ArrowCursor));

	LinkWidget *view_total_reconstruction_poles_link = new LinkWidget(
			tr("View total reconstruction poles..."), this);
	QtWidgetUtils::add_widget_to_placeholder(
			view_total_reconstruction_poles_link,
			view_total_reconstruction_poles_placeholder_widget);

	LinkWidget *merge_reconstruction_tree_layers_link = new LinkWidget(
			tr("Merge reconstruction tree layers..."), this);
	QtWidgetUtils::add_widget_to_placeholder(
			merge_reconstruction_tree_layers_link,
			merge_reconstruction_tree_layers_placeholder_widget);

	QObject::connect(
			view_total_reconstruction_poles_link,
			SIGNAL(link_activated()),
			this,
			SLOT(handle_view_total_reconstruction_poles_link_activated()));
	QObject::connect(
			merge_reconstruction_tree_layers_link,
			SIGNAL(link_activated()),
			this,
			SLOT(handle_merge_reconstruction_tree_layers_link_activated()));
	QObject::connect(
			keep_as_default_checkbox,
			SIGNAL(clicked(bool)),
			this,
			SLOT(handle_keep_as_default_checkbox_clicked(bool)));

	extend_total_reconstruction_poles_to_distant_past_check_box->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			extend_total_reconstruction_poles_to_distant_past_check_box, SIGNAL(clicked()),
			this, SLOT(handle_extend_total_reconstruction_poles_to_distant_past_clicked()));

	push_button_help_extend_total_reconstruction_poles_to_distant_past->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			push_button_help_extend_total_reconstruction_poles_to_distant_past, SIGNAL(clicked()),
			d_help_extend_total_reconstruction_pole_to_distant_past_dialog, SLOT(show()));
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

	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = visual_layer.lock())
	{
		GPlatesAppLogic::Layer layer = locked_visual_layer->get_reconstruct_graph_layer();

		bool is_default = (layer == d_application_state.get_reconstruct_graph().get_default_reconstruction_tree_layer());
		keep_as_default_checkbox->setEnabled(is_default);
		keep_as_default_checkbox->setChecked(is_default &&
				!d_application_state.is_updating_default_reconstruction_tree_layer());

		GPlatesAppLogic::ReconstructionLayerParams *layer_params =
			dynamic_cast<GPlatesAppLogic::ReconstructionLayerParams *>(
					layer.get_layer_params().get());
		if (layer_params)
		{
			const GPlatesAppLogic::ReconstructionParams &reconstruction_params = layer_params->get_reconstruction_params();

			extend_total_reconstruction_poles_to_distant_past_check_box->setChecked(
					reconstruction_params.get_extend_total_reconstruction_poles_to_distant_past());
		}
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
	d_viewport_window->dialogs().pop_up_total_reconstruction_poles_dialog(d_current_visual_layer);
}


void
GPlatesQtWidgets::ReconstructionLayerOptionsWidget::handle_merge_reconstruction_tree_layers_link_activated()
{
	if (!d_merge_reconstruction_layers_dialog)
	{
		d_merge_reconstruction_layers_dialog = new MergeReconstructionLayersDialog(
				d_application_state,
				d_view_state,
				&d_viewport_window->dialogs().visual_layers_dialog());
	}

	d_merge_reconstruction_layers_dialog->populate(d_current_visual_layer);

	// This dialog is shown modally.
	d_merge_reconstruction_layers_dialog->exec();
}


void
GPlatesQtWidgets::ReconstructionLayerOptionsWidget::handle_extend_total_reconstruction_poles_to_distant_past_clicked()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = d_current_visual_layer.lock())
	{
		GPlatesAppLogic::Layer layer = locked_visual_layer->get_reconstruct_graph_layer();
		GPlatesAppLogic::ReconstructionLayerParams *layer_params =
			dynamic_cast<GPlatesAppLogic::ReconstructionLayerParams *>(
					layer.get_layer_params().get());
		if (layer_params)
		{
			GPlatesAppLogic::ReconstructionParams reconstruction_params = layer_params->get_reconstruction_params();
			reconstruction_params.set_extend_total_reconstruction_poles_to_distant_past(
					extend_total_reconstruction_poles_to_distant_past_check_box->isChecked());
			layer_params->set_reconstruction_params(reconstruction_params);
		}
	}
}


void
GPlatesQtWidgets::ReconstructionLayerOptionsWidget::handle_keep_as_default_checkbox_clicked(
		bool checked)
{
	d_application_state.set_update_default_reconstruction_tree_layer(!checked);
}

