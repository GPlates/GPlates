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

#include <boost/cast.hpp>
#include <boost/shared_ptr.hpp>
#include <QHeaderView>
#include <QTableWidget>

#include "MergeReconstructionLayersDialog.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/Layer.h"
#include "app-logic/ReconstructGraph.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "presentation/ViewState.h"
#include "presentation/VisualLayer.h"
#include "presentation/VisualLayers.h"


GPlatesQtWidgets::MergeReconstructionLayersDialog::MergeReconstructionLayersDialog(
		GPlatesAppLogic::ApplicationState &application_state,
		GPlatesPresentation::ViewState &view_state,
		QWidget *parent_) :
	QDialog(parent_),
	d_application_state(application_state),
	d_view_state(view_state)
{
	setupUi(this);

	setup_connections();

	// Try to adjust column widths.
	QHeaderView *header = reconstruction_tree_layers_to_merge_table_widget->horizontalHeader();
	header->setResizeMode(LAYER_NAME_COLUMN, QHeaderView::Stretch);
	header->setResizeMode(ENABLE_LAYER_COLUMN, QHeaderView::ResizeToContents);
}


bool
GPlatesQtWidgets::MergeReconstructionLayersDialog::populate(
		const boost::weak_ptr<GPlatesPresentation::VisualLayer> &visual_layer)
{
	// Store pointer so we can write the settings back later.
	d_current_visual_layer = visual_layer;

	// Clear the previous list of reconstruction layers (if any).
	clear_layers();

	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_current_visual_layer = d_current_visual_layer.lock())
	{
		GPlatesAppLogic::Layer current_layer = locked_current_visual_layer->get_reconstruct_graph_layer();

		// Populate a list of potential reconstruction layers to merge (the user will choose which ones).
		GPlatesAppLogic::ReconstructGraph &reconstruct_graph = d_application_state.get_reconstruct_graph();
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

			// Get the visual layer associated with the layer.
			boost::weak_ptr<GPlatesPresentation::VisualLayer> visual_layer =
					d_view_state.get_visual_layers().get_visual_layer(layer);
			boost::shared_ptr<const GPlatesPresentation::VisualLayer> locked_visual_layer = visual_layer.lock();
			if (!locked_visual_layer)
			{
				continue;
			}

			//
			// Add the layer to the list.
			//

			// The rows in the QTableWidget and our internal layer sequence should be in sync.
			const int row = reconstruction_tree_layers_to_merge_table_widget->rowCount();
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					row == boost::numeric_cast<int>(d_layer_state_seq.size()),
					GPLATES_ASSERTION_SOURCE);

			// Add a row.
			reconstruction_tree_layers_to_merge_table_widget->insertRow(row);
			d_layer_state_seq.push_back(LayerState(layer));
			const LayerState &row_layer_state = d_layer_state_seq.back();

			// Add layer name item.
			QTableWidgetItem *layer_name_item = new QTableWidgetItem(locked_visual_layer->get_name());
			layer_name_item->setFlags(Qt::ItemIsEnabled);
			reconstruction_tree_layers_to_merge_table_widget->setItem(row, LAYER_NAME_COLUMN, layer_name_item);

			// Add checkbox item to enable/disable the layer.
			QTableWidgetItem* layer_enabled_item = new QTableWidgetItem();
			layer_enabled_item->setToolTip(tr("Select to enable layer for merging"));
			layer_enabled_item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
			layer_enabled_item->setCheckState(row_layer_state.enabled ? Qt::Checked : Qt::Unchecked);
			reconstruction_tree_layers_to_merge_table_widget->setItem(row, ENABLE_LAYER_COLUMN, layer_enabled_item);
		}

		// Set up the current layer name (in case user wants to change it due to merging layers).
		layer_name_line_edit->setText(locked_current_visual_layer->get_name());
	}

	return false;
}


void
GPlatesQtWidgets::MergeReconstructionLayersDialog::setup_connections()
{
	// Listen for changes to the checkbox that enables/disables layers.
	QObject::connect(
			reconstruction_tree_layers_to_merge_table_widget, SIGNAL(cellChanged(int, int)),
			this, SLOT(react_cell_changed_layers(int, int)));
	QObject::connect(
			button_clear_all_layers, SIGNAL(clicked()),
			this, SLOT(react_clear_all_layers()));
	QObject::connect(
			button_select_all_layers, SIGNAL(clicked()),
			this, SLOT(react_select_all_layers()));

	QObject::connect(
			main_buttonbox,
			SIGNAL(accepted()),
			this,
			SLOT(handle_apply()));
	QObject::connect(
			main_buttonbox,
			SIGNAL(rejected()),
			this,
			SLOT(handle_reject()));
}


void
GPlatesQtWidgets::MergeReconstructionLayersDialog::react_clear_all_layers()
{
	for (int row = 0; row < reconstruction_tree_layers_to_merge_table_widget->rowCount(); ++row)
	{
		reconstruction_tree_layers_to_merge_table_widget->item(row, ENABLE_LAYER_COLUMN)->setCheckState(Qt::Unchecked);
	}
}


void
GPlatesQtWidgets::MergeReconstructionLayersDialog::react_select_all_layers()
{
	for (int row = 0; row < reconstruction_tree_layers_to_merge_table_widget->rowCount(); ++row)
	{
		reconstruction_tree_layers_to_merge_table_widget->item(row, ENABLE_LAYER_COLUMN)->setCheckState(Qt::Checked);
	}
}


void
GPlatesQtWidgets::MergeReconstructionLayersDialog::react_cell_changed_layers(
		int row,
		int column)
{
	if (row < 0 ||
			boost::numeric_cast<layer_state_seq_type::size_type>(row) >
					d_layer_state_seq.size())
	{
		return;
	}

	// It should be the enable layer checkbox column as that's the only cell that's editable.
	if (column != ENABLE_LAYER_COLUMN)
	{
		return;
	}

	// Set the enable flag in our internal layer sequence.
	d_layer_state_seq[row].enabled =
			reconstruction_tree_layers_to_merge_table_widget->item(row, column)->checkState() == Qt::Checked;
}


void
GPlatesQtWidgets::MergeReconstructionLayersDialog::handle_apply()
{
	const std::vector<GPlatesAppLogic::Layer> layers_to_merge = get_selected_layers();

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

		std::vector<GPlatesAppLogic::Layer::InputFile> input_files_to_connect;
		bool set_current_layer_as_default = false;

		// Get all the input files of the layers to merge.
		BOOST_FOREACH(GPlatesAppLogic::Layer layer, layers_to_merge)
		{
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

		// Delete the other 'reconstruction tree' layers that the user requested to merge.
		BOOST_FOREACH(const GPlatesAppLogic::Layer &layer, layers_to_merge)
		{
			reconstruct_graph.remove_layer(layer);
		}

		remove_layers_group.end_add_or_remove_layers();

		if (set_current_layer_as_default)
		{
			reconstruct_graph.set_default_reconstruction_tree_layer(current_layer);
		}

		// Change the current layer name if it is different (if user changed it).
		QString current_layer_name = locked_current_visual_layer->get_name();
		if (layer_name_line_edit->text() != current_layer_name)
		{
			current_layer_name = layer_name_line_edit->text();
			locked_current_visual_layer->set_custom_name(current_layer_name);
		}
	}

	clear_layers();

	accept();
}


void
GPlatesQtWidgets::MergeReconstructionLayersDialog::handle_reject()
{
	clear_layers();

	reject();
}


void
GPlatesQtWidgets::MergeReconstructionLayersDialog::clear_layers()
{
	d_layer_state_seq.clear();
	reconstruction_tree_layers_to_merge_table_widget->clearContents(); // Do not clear the header items as well.
	reconstruction_tree_layers_to_merge_table_widget->setRowCount(0);  // Do remove the newly blanked rows.
}


std::vector<GPlatesAppLogic::Layer>
GPlatesQtWidgets::MergeReconstructionLayersDialog::get_selected_layers() const
{
	std::vector<GPlatesAppLogic::Layer> selected_layers;

	// Iterate through the layers accepted by the user.
	layer_state_seq_type::const_iterator layer_state_iter = d_layer_state_seq.begin();
	layer_state_seq_type::const_iterator layer_state_end = d_layer_state_seq.end();
	for ( ; layer_state_iter != layer_state_end; ++layer_state_iter)
	{
		const LayerState &layer_state = *layer_state_iter;

		if (layer_state.enabled)
		{
			selected_layers.push_back(layer_state.layer);
		}
	}

	return selected_layers;
}
