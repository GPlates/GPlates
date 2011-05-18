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
 
#ifndef GPLATES_APP_LOGIC_RECONSTRUCTGRAPH_H
#define GPLATES_APP_LOGIC_RECONSTRUCTGRAPH_H

#include <iterator>
#include <list>
#include <map>
#include <set>
#include <vector>
#include <boost/mpl/assert.hpp>
#include <boost/mpl/contains.hpp>
#include <boost/noncopyable.hpp>
#include <boost/operators.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/utility/enable_if.hpp>
#include <boost/foreach.hpp>
#include <boost/function.hpp>
#include <boost/iterator/transform_iterator.hpp>
#include <boost/lambda/construct.hpp>
#include <QObject>

#include "FeatureCollectionFileState.h"
#include "Layer.h"
#include "Reconstruction.h"
#include "ReconstructionLayerProxy.h"

#include "model/FeatureCollectionHandle.h"


namespace GPlatesAppLogic
{
	class ApplicationState;
	class LayerTask;

	namespace ReconstructGraphImpl
	{
		class Layer;
		class LayerInputConnection;
		class Data;
	}


	/**
	 * Manages layer creation and connection to other layers or input feature collections
	 * and generates a @a Reconstruction that is the accumulated result of all layer outputs
	 * for a specific reconstruction time.
	 */
	class ReconstructGraph :
			public QObject,
			private boost::noncopyable
	{
		Q_OBJECT

	private:
		//! Typedef for a shared pointer to a layer added to the graph.
		typedef boost::shared_ptr<ReconstructGraphImpl::Layer> layer_ptr_type;

		//! Typedef for a sequence of layers.
		typedef std::list<layer_ptr_type> layer_ptr_seq_type;

		//! Typedef for a function that creates a weak reference to a layer.
		typedef boost::function< Layer (const boost::shared_ptr<ReconstructGraphImpl::Layer>) >
			make_layer_fn_type;

	public:
		/**
		 * Typedef for a const iterator over the layers in the graph.
		 */
		typedef boost::transform_iterator<make_layer_fn_type,
				layer_ptr_seq_type::const_iterator> const_iterator;

		/**
		 * Typedef for an iterator over the layers in the graph.
		 */
		typedef boost::transform_iterator<make_layer_fn_type,
				layer_ptr_seq_type::iterator> iterator;


		/**
		 * Constructor.
		 */
		ReconstructGraph(
				ApplicationState &application_state);


		/**
		 * Adds a new layer to the graph.
		 *
		 * The layer will still need to be connected to a feature collection or
		 * the output of another layer.
		 *
		 * Emits the signal @a layer_added - clients of that signal should note that
		 * the layer does not yet have any input connections - whoever called @a add_layer
		 * could still be in the process of making input connections.
		 *
		 * NOTE: If it's a reconstruction tree layer then it does *not* set the default
		 * reconstruction tree layer (previously it did but this has been deprecated).
		 *
		 * You can create @a layer_task using @a LayerTaskRegistry.
		 */
		Layer
		add_layer(
				const boost::shared_ptr<LayerTask> &layer_task);


		/**
		 * Removes a layer from the graph and sets the default reconstruction tree layer
		 * to none (if removing the default reconstruction tree layer).
		 *
		 * Any layers whose input is connected to the output of @a layer
		 * will be disconnected from it.
		 * Any feature collections (or layers whose output is) connected to the input of
		 * @a layer will be disconnected from it.
		 *
		 * This call will also be necessary before any memory can be released (even if
		 * @a layer was never connected to the graph).
		 *
		 * Emits the signal @a layer_about_to_be_removed before the layer is removed.
		 *
		 * Also emits the signal @a layer_activation_changed (before the signal
		 * @a layer_about_to_be_removed) if @a layer is currently active.
		 *
		 * Can also emit the signal @a default_reconstruction_tree_layer_changed if
		 * @a layer is currently the default reconstruction tree layer.
		 *
		 * @throws @a PreconditionViolationError if layer already removed from graph.
		 */
		void
		remove_layer(
				Layer layer);


		/**
		 * Gets the input file handle for @a input_file.
		 *
		 * The returned file is a weak reference and does not need to be stored anywhere.
		 * It's typically passed straight to Layer::connect_to_input_file.
		 *
		 * If this file gets unloaded by the user then the returned weak reference will
		 * become invalid and all layers connecting to this input file will lose those
		 * connections automatically. This is the primary reason for having this method.
		 * If any layer has no more input connections on its main channel input then that
		 * layer will be removed automatically and destroyed.
		 *
		 * @throws PreconditionViolationError if @a input_file is not a currently loaded file.
		 */
		Layer::InputFile
		get_input_file(
				const FeatureCollectionFileState::file_reference input_file);


		/**
		 * Sets the current default reconstruction tree layer.
		 *
		 * Any layers that require a reconstruction tree input but are not explicitly
		 * connected to one will use the default reconstruction tree layer instead.
		 *
		 * Layers that are explicitly connected to a reconstruction tree layer will
		 * ignore the default reconstruction tree layer.
		 *
		 * Disabling or destroying the default reconstruction tree layer will result in
		 * layers that implicitly connect to it to use identity rotations for all plates.
		 *
		 * If @a default_reconstruction_tree_layer is invalid then there will be no
		 * default reconstruction tree layer - this is the way to specify no default layer.
		 *
		 * Emits the @a default_reconstruction_tree_layer_changed signal if the default
		 * reconstruction tree layer changed.
		 *
		 * @a throws PreconditionViolationError if @a default_reconstruction_tree_layer is valid,
		 * but the layer it refers to is not a reconstruction tree layer.
		 */
		void
		set_default_reconstruction_tree_layer(
				const Layer &default_reconstruction_tree_layer = Layer());


		/**
		 * Returns the current default reconstruction tree layer.
		 *
		 * NOTE: Check the returned weak reference is valid before using since
		 * there may be no default layer.
		 */
		Layer
		get_default_reconstruction_tree_layer() const;


		/**
		 * Returns the "begin" const_iterator to iterate over the
		 * sequence of @a Layer objects in this graph.
		 */
		const_iterator
		begin() const
		{
			return boost::make_transform_iterator(
					d_layers.begin(),
					make_layer_fn_type(boost::lambda::constructor<Layer>()));
		}


		/**
		 * Returns the "end" const_iterator to iterate over the
		 * sequence of @a Layer objects in this graph.
		 */
		const_iterator
		end() const
		{
			return boost::make_transform_iterator(
					d_layers.end(),
					make_layer_fn_type(boost::lambda::constructor<Layer>()));
		}


		/**
		 * Returns the "begin" iterator to iterate over the
		 * sequence of @a Layer objects in this graph.
		 */
		iterator
		begin()
		{
			return boost::make_transform_iterator(
					d_layers.begin(),
					make_layer_fn_type(boost::lambda::constructor<Layer>()));
		}


		/**
		 * Returns the "end" const_iterator to iterate over the
		 * sequence of @a Layer objects in this graph.
		 */
		iterator
		end()
		{
			return boost::make_transform_iterator(
					d_layers.end(),
					make_layer_fn_type(boost::lambda::constructor<Layer>()));
		}


		///////////////////////////////////////////////////////////////
		// The following public methods are used by ApplicationState //
		///////////////////////////////////////////////////////////////


		/**
		 * Updates the layer tasks in the current reconstruction graph.
		 *
		 * Returns a list of the layer proxies at the output of each *active* layer and
		 * the default reconstruction tree if there is one.
		 */
		Reconstruction::non_null_ptr_to_const_type
		update_layer_tasks(
				const double &reconstruction_time);


		// NOTE: all signals/slots should use namespace scope for all arguments
		//       otherwise differences between signals and slots will cause Qt
		//       to not be able to connect them at runtime.

		// Used to be a slot but is now called directly by @a ApplicationState.
		void
		handle_file_state_files_added(
				GPlatesAppLogic::FeatureCollectionFileState &file_state,
				const std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference> &new_files);

		// Used to be a slot but is now called directly by @a ApplicationState.
		void
		handle_file_state_file_about_to_be_removed(
				GPlatesAppLogic::FeatureCollectionFileState &file_state,
				GPlatesAppLogic::FeatureCollectionFileState::file_reference file);

	signals:
		// NOTE: all signals/slots should use namespace scope for all arguments
		//       otherwise differences between signals and slots will cause Qt
		//       to not be able to connect them at runtime.

		/**
		 * Emitted when a new layer has been added by @a add_layer.
		 */
		void
		layer_added(
				GPlatesAppLogic::ReconstructGraph &reconstruct_graph,
				GPlatesAppLogic::Layer layer);

		/**
		 * Emitted when an existing layer is about to be removed inside @a remove_layer.
		 */
		void
		layer_about_to_be_removed(
				GPlatesAppLogic::ReconstructGraph &reconstruct_graph,
				GPlatesAppLogic::Layer layer);

		/**
		 * Emitted after an existing layer has been removed inside @a remove_layer.
		 * To work out which layer was removed, also listen to @a layer_about_to_be_removed.
		 */
		void
		layer_removed(
				GPlatesAppLogic::ReconstructGraph &reconstruct_graph);

		/**
		 * Emitted when layer @a layer has been activated or deactivated.
		 */
		void
		layer_activation_changed(
				GPlatesAppLogic::ReconstructGraph &reconstruct_graph,
				GPlatesAppLogic::Layer layer,
				bool activation);

		/**
		 * Emitted when layer @a layer has added a new input connection.
		 */
		void
		layer_added_input_connection(
				GPlatesAppLogic::ReconstructGraph &reconstruct_graph,
				GPlatesAppLogic::Layer layer,
				GPlatesAppLogic::Layer::InputConnection input_connection);

		/**
		 * Emitted when layer @a layer is about to remove an existing input connection.
		 *
		 * NOTE: This signal only gets emitted if a connection is explicitly disconnected
		 * (by calling Layer::InputConnection::disconnect()). When an input connection is
		 * automatically destroyed because its parent layer is removed then no signal is emitted.
		 */
		void
		layer_about_to_remove_input_connection(
				GPlatesAppLogic::ReconstructGraph &reconstruct_graph,
				GPlatesAppLogic::Layer layer,
				GPlatesAppLogic::Layer::InputConnection input_connection);

		/**
		 * Emitted when layer @a layer has finished removing an existing input connection.
		 *
		 * NOTE: This signal only gets emitted if a connection is explicitly disconnected
		 * (by calling Layer::InputConnection::disconnect()). When an input connection is
		 * automatically destroyed because its parent layer is removed then no signal is emitted.
		 */
		void
		layer_removed_input_connection(
				GPlatesAppLogic::ReconstructGraph &reconstruct_graph,
				GPlatesAppLogic::Layer layer);

		/**
		 * Emitted when the default reconstruction tree layer is changed.
		 *
		 * An invalid layer means no layer (in other words there was or will be
		 * no default reconstruction tree layer).
		 */
		void
		default_reconstruction_tree_layer_changed(
				GPlatesAppLogic::ReconstructGraph &reconstruct_graph,
				GPlatesAppLogic::Layer prev_default_reconstruction_tree_layer,
				GPlatesAppLogic::Layer new_default_reconstruction_tree_layer);

	private:
		//! Emits the @a layer_activation_changed signal.
		void
		emit_layer_activation_changed(
				const Layer &layer,
				bool activation);

		//! Emits the @a layer_added_input_connection signal.
		void
		emit_layer_added_input_connection(
				GPlatesAppLogic::Layer layer,
				GPlatesAppLogic::Layer::InputConnection input_connection);

		//! Emits the @a layer_about_to_remove_input_connection signal.
		void
		emit_layer_about_to_remove_input_connection(
				GPlatesAppLogic::Layer layer,
				GPlatesAppLogic::Layer::InputConnection input_connection);

		//! Emits the @a layer_removed_input_connection signal.
		void
		emit_layer_removed_input_connection(
				GPlatesAppLogic::Layer layer);

		friend class Layer;

	private:
		//! Typedef for a shared pointer to an input file.
		typedef boost::shared_ptr<ReconstructGraphImpl::Data> input_file_ptr_type;

		//! Typedef for a sequence of input files with file references as map keys.
		typedef std::map<FeatureCollectionFileState::file_reference, input_file_ptr_type>
				input_file_ptr_map_type;


		ApplicationState &d_application_state;

		input_file_ptr_map_type d_input_files;
		layer_ptr_seq_type d_layers;

		/**
		 * The default reconstruction tree layer.
		 * Can be invalid if no default layer set or if the default layer was destroyed.
		 */
		Layer d_default_reconstruction_tree_layer;

		/**
		 * Used if there are no reconstruction tree layers currently loaded.
		 *
		 * Generates identity rotations.
		 */
		ReconstructionLayerProxy::non_null_ptr_type d_identity_rotation_reconstruction_layer_proxy;


		/**
		 * Determines an order of layers that will not violate the dependency graph -
		 * in other words does not execute a layer before its input layers have been executed.
		 */
		std::vector<layer_ptr_type>
		get_layer_dependency_order(
				const layer_ptr_type &default_reconstruction_tree_layer) const;

		/**
		 * Find dependency ancestors of @a layer and appends them to @a ancestor_layers.
		 */
		void
		find_dependency_ancestors_of_layer(
				const layer_ptr_type &layer,
				std::set<layer_ptr_type> &ancestor_layers) const;

		/**
		 * Determines a dependency ordering of the dependency graph rooted at @a layer and
		 * outputs the ordered results to @a dependency_ordered_layers.
		 *
		 * @a all_layers_visited is used to avoid visiting the same layer more than once.
		 */
		void
		find_layer_dependency_order(
				const layer_ptr_type &layer,
				std::vector<layer_ptr_type> &dependency_ordered_layers,
				std::set<layer_ptr_type> &all_layers_visited) const;
	};
}

#endif // GPLATES_APP_LOGIC_RECONSTRUCTGRAPH_H
