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
#include "QtWidgetUtils.h"
#include "TotalReconstructionPolesDialog.h"
#include "ViewportWindow.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/Layer.h"
#include "app-logic/ReconstructGraph.h"

#include "gui/Dialogs.h"

#include "presentation/ViewState.h"
#include "presentation/VisualLayer.h"
#include "presentation/VisualLayers.h"


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
	d_viewport_window->dialogs().pop_up_total_reconstruction_poles_dialog(d_current_visual_layer);
}


void
GPlatesQtWidgets::ReconstructionLayerOptionsWidget::handle_merge_reconstruction_tree_layers_link_activated()
{
#if 0
	if (!d_merge_reconstruction_tree_layers_dialog)
	{
		d_merge_reconstruction_tree_layers_dialog = new MergeReconstructionTreeLayersDialog(
				d_application_state,
				&d_viewport_window->dialogs().visual_layers_dialog());
	}

	d_merge_reconstruction_tree_layers_dialog->populate(d_current_visual_layer);

	// This dialog is shown modally.
	d_merge_reconstruction_tree_layers_dialog->exec();
#endif

	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_current_visual_layer = d_current_visual_layer.lock())
	{
		GPlatesAppLogic::Layer current_layer = locked_current_visual_layer->get_reconstruct_graph_layer();

		// All 'reconstruction tree' layers have the same main input channel name.
		const GPlatesAppLogic::LayerInputChannelName::Type main_input_channel =
				current_layer.get_main_input_feature_collection_channel();

		// Get the input files currently connected to the current layer.
		const std::vector<GPlatesAppLogic::Layer::InputConnection> current_main_inputs =
				current_layer.get_channel_inputs(main_input_channel);
		std::vector<GPlatesAppLogic::Layer::InputFile> current_main_input_files;
		BOOST_FOREACH(const GPlatesAppLogic::Layer::InputConnection &current_main_input, current_main_inputs)
		{
			boost::optional<GPlatesAppLogic::Layer::InputFile> current_input_file = current_main_input.get_input_file();
			if (current_input_file)
			{
				current_main_input_files.push_back(current_input_file.get());
			}
		}

		GPlatesAppLogic::ReconstructGraph &reconstruct_graph = d_application_state.get_reconstruct_graph();

		std::vector<GPlatesAppLogic::Layer> layers_to_remove;
		std::vector<GPlatesAppLogic::Layer::InputFile> input_files_to_connect;
		bool set_current_layer_as_default = false;

		// Get all the reconstruction layers, and get their input files.
		BOOST_FOREACH(GPlatesAppLogic::Layer layer, reconstruct_graph)
		{
			if (layer == current_layer)
			{
				continue;
			}
			
			if (layer.get_type() != GPlatesAppLogic::LayerTaskType::RECONSTRUCTION)
			{
				continue;
			}

			layers_to_remove.push_back(layer);
			if (layer == reconstruct_graph.get_default_reconstruction_tree_layer())
			{
				set_current_layer_as_default = true;
			}

			// The 'reconstruction tree' layer has input feature collections on its main input channel.
			const std::vector<GPlatesAppLogic::Layer::InputConnection> main_inputs =
					layer.get_channel_inputs(main_input_channel);

			// Loop over all input connections to get the files (feature collections).
			std::vector<GPlatesAppLogic::Layer::InputConnection>::const_iterator main_inputs_iter = main_inputs.begin();
			std::vector<GPlatesAppLogic::Layer::InputConnection>::const_iterator main_inputs_end = main_inputs.end();
			for ( ; main_inputs_iter != main_inputs_end; ++main_inputs_iter)
			{
				const GPlatesAppLogic::Layer::InputConnection &main_input_connection = *main_inputs_iter;

				boost::optional<GPlatesAppLogic::Layer::InputFile> input_file = main_input_connection.get_input_file();

				// If it's not a file (ie, it's a layer) then continue to the next file.
				// This shouldn't happen for 'reconstruction tree' layers though.
				if (!input_file)
				{
					continue;
				}

				// If the input file is already connected to the current layer then ignore it.
				if (std::find(current_main_input_files.begin(), current_main_input_files.end(), input_file.get()) !=
					current_main_input_files.end())
				{
					continue;
				}

				input_files_to_connect.push_back(input_file.get());
			}
		}

		// Connect the input files to the main input channel of the current layer.
		BOOST_FOREACH(const GPlatesAppLogic::Layer::InputFile &input_file, input_files_to_connect)
		{
			current_layer.connect_input_to_file(input_file, main_input_channel);
		}

		// As an optimisation (ie, not required), put all layer removals in a single add layers group.
		GPlatesAppLogic::ReconstructGraph::AddOrRemoveLayersGroup remove_layers_group(reconstruct_graph);
		remove_layers_group.begin_add_or_remove_layers();

		// Delete the other 'reconstruction tree' layers.
		BOOST_FOREACH(const GPlatesAppLogic::Layer &layer, layers_to_remove)
		{
			reconstruct_graph.remove_layer(layer);
		}

		remove_layers_group.end_add_or_remove_layers();

		if (set_current_layer_as_default)
		{
			reconstruct_graph.set_default_reconstruction_tree_layer(current_layer);
		}
	}
}


void
GPlatesQtWidgets::ReconstructionLayerOptionsWidget::handle_keep_as_default_checkbox_clicked(
		bool checked)
{
	d_application_state.set_update_default_reconstruction_tree_layer(!checked);
}

