/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2010 Geological Survey of Norway
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

#include <boost/shared_ptr.hpp>

#include "MergeReconstructionLayersDialog.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/Layer.h"
#include "app-logic/ReconstructGraph.h"

#include "presentation/ViewState.h"
#include "presentation/VisualLayer.h"
#include "presentation/VisualLayers.h"


GPlatesQtWidgets::MergeReconstructionLayersDialog::MergeReconstructionLayersDialog(
		GPlatesAppLogic::ApplicationState &application_state,
		QWidget *parent_) :
	QDialog(parent_),
	d_application_state(application_state)
{
	setupUi(this);

	setup_connections();
}


bool
GPlatesQtWidgets::MergeReconstructionLayersDialog::populate(
		const boost::weak_ptr<GPlatesPresentation::VisualLayer> &visual_layer)
{
	// Store pointer so we can write the settings back later.
	d_current_visual_layer = visual_layer;

	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = visual_layer.lock())
	{
		return true;
	}

	return false;
}


void
GPlatesQtWidgets::MergeReconstructionLayersDialog::setup_connections()
{
	QObject::connect(
			main_buttonbox,
			SIGNAL(accepted()),
			this,
			SLOT(handle_apply()));
	QObject::connect(
			main_buttonbox,
			SIGNAL(rejected()),
			this,
			SLOT(reject()));
}


void
GPlatesQtWidgets::MergeReconstructionLayersDialog::handle_apply()
{
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

			// If we end up removing a layer that is the default reconstruction tree layer then
			// we should set the current layer to be the new default.
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

	accept();
}
