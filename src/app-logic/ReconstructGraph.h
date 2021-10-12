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
#include <QObject>

#include "FeatureCollectionFileState.h"
#include "Layer.h"
#include "LayerTaskDataType.h"
#include "Reconstruction.h"

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

	public:
		/**
		 * Forward iterator over all layers in the graph.
		 */
		class LayerConstIterator :
				public std::iterator<std::forward_iterator_tag, const Layer>,
				public boost::equality_comparable<LayerConstIterator>,
				public boost::incrementable<LayerConstIterator>
		{
		public:

			//! Create a "begin" iterator.
			static
			LayerConstIterator
			create_begin(
					const ReconstructGraph &reconstruct_graph)
			{
				return LayerConstIterator(reconstruct_graph.d_layers.begin());
			}

			//! Create an "end" iterator.
			static
			LayerConstIterator
			create_end(
					const ReconstructGraph &reconstruct_graph)
			{
				return LayerConstIterator(reconstruct_graph.d_layers.end());
			}

			/**
			 * Dereference operator.
			 */
			const Layer &
			operator*() const
			{
				// Convert to a weak reference if current weak reference not valid.
				d_current_layer_weak_ref = Layer(*d_layer_seq_iterator);
				return d_current_layer_weak_ref;
			}


			/**
			 * Arrow operator.
			 */
			const Layer *
			operator->() const
			{
				// Convert to a weak reference if current weak reference not valid.
				d_current_layer_weak_ref = Layer(*d_layer_seq_iterator);
				return &d_current_layer_weak_ref;
			}


			/**
			 * Pre-increment operator.
			 * Post-increment operator provided by base class boost::incrementable.
			 */
			LayerConstIterator &
			operator++()
			{
				++d_layer_seq_iterator;

				// Invalidate current weak reference.
				d_current_layer_weak_ref = Layer();

				return *this;
			}


			/**
			 * Equality comparison for @a LayerConstIterator.
			 * Inequality operator provided by base class boost::equality_comparable.
			 */
			friend
			bool
			operator==(
					const LayerConstIterator &lhs,
					const LayerConstIterator &rhs)
			{
				return lhs.d_layer_seq_iterator == rhs.d_layer_seq_iterator;
			}

		private:

			LayerConstIterator(
					layer_ptr_seq_type::const_iterator layer_seq_iterator) :
				d_layer_seq_iterator(layer_seq_iterator)
			{  }

			layer_ptr_seq_type::const_iterator d_layer_seq_iterator;

			mutable Layer d_current_layer_weak_ref;
		};

		/**
		 * Typedef for an iterator over the layers in the graph.
		 */
		typedef LayerConstIterator const_iterator;


		/**
		 * Constructor.
		 */
		ReconstructGraph(
				ApplicationState &application_state);


		/**
		 * Adds a new layer to the graph and sets it as the default reconstruction tree layer
		 * (if @a layer_task generates a reconstruction tree as output).
		 *
		 * The layer will still need to be connected to a feature collection or
		 * the output of another layer.
		 *
		 * Emits the signal @a layer_added - clients of that signal should note that
		 * the layer does not yet have any input connections - whoever called @a add_layer
		 * could still be in the process of making input connections.
		 *
		 * Can also emit the signal @a default_reconstruction_tree_layer_changed if
		 * the layer created is a reconstruction tree layer.
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
		 * default reconstruction tree layer - this is they way to specify no default layer.
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
			return const_iterator::create_begin(*this);
		}


		/**
		 * Returns the "end" const_iterator to iterate over the
		 * sequence of @a Layer objects in this graph.
		 */
		const_iterator
		end() const
		{
			return const_iterator::create_end(*this);
		}


		///////////////////////////////////////////////////////////////
		// The following public methods are used by ApplicationState //
		///////////////////////////////////////////////////////////////


		/**
		 * Executes the layer tasks in the current reconstruction graph.
		 *
		 * Layer tasks requiring input from other layer tasks are executed in the correct order.
		 *
		 * Also accumulates the output results of each layer into the returned @a Reconstruction.
		 */
		Reconstruction::non_null_ptr_to_const_type
		execute_layer_tasks(
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
		 * Determines an order of layers that will not violate the dependency graph -
		 * in other words does not execute a layer before its input layers have been executed.
		 */
		std::vector<layer_ptr_type>
		get_layer_dependency_order(
				const layer_ptr_type &default_reconstruction_tree_layer) const;

		/**
		 * Partitions all layers into:
		 * - topological layers and their dependency ancestors, and
		 * - the rest.
		 */
		void
		partition_topological_layers_and_their_dependency_ancestors(
				std::set<layer_ptr_type> &topological_layers_and_ancestors,
				std::set<layer_ptr_type> &other_layers) const;

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


namespace boost
{
	// We need to tell BOOST_FOREACH and Boost.Range to always use 'const_iterator',
	// because ReconstructGraph does not have an inner type called 'iterator'.
	// NOTE: This needs to be placed above the definition of find_layer, which uses
	// BOOST_FOREACH, otherwise g++ 4.2 will complain that the template
	// specialisation appears after the template has been instantiated.

	template<>
	struct range_iterator<GPlatesAppLogic::ReconstructGraph>
	{
		typedef GPlatesAppLogic::ReconstructGraph::const_iterator type;
	};

	template<>
	struct range_const_iterator<GPlatesAppLogic::ReconstructGraph>
	{
		typedef GPlatesAppLogic::ReconstructGraph::const_iterator type;
	};
}


namespace GPlatesAppLogic
{
	/**
	 * Finds the layer in @a reconstruct_graph that generated
	 * the output @a layer_output_data_to_match.
	 *
	 * Examples of the template parameter LayerOutputDataType are:
	 * - ReconstructionGeometryCollection::non_null_ptr_to_const_type
	 * - ReconstructionTree::non_null_ptr_to_const_type
	 * ...they are expected to be types in the @a layer_task_data_type variant.
	 */
	template <class LayerOutputDataType>
	boost::optional<Layer>
	find_layer(
			const ReconstructGraph &reconstruct_graph,
			const LayerOutputDataType &layer_output_data_to_match,
			// This function can only be called if the template parameter
			// LayerOutputDataType is one of the bounded types in layer_task_data_types.
			typename boost::enable_if<
				boost::mpl::contains<layer_task_data_types, LayerOutputDataType>
			>::type *dummy = NULL)
	{
		// Iterate over the layers in the graph.
		BOOST_FOREACH(const Layer &layer, reconstruct_graph)
		{
			// Get the most recent output data generated by the current layer.
			boost::optional<LayerOutputDataType> layer_output_data =
					layer.get_output_data<LayerOutputDataType>();

			// If the data type output by the current layer is what we're looking for...
			if (layer_output_data)
			{
				// If the data itself matches then we've found the layer that generated the data.
				if (layer_output_data.get() == layer_output_data_to_match)
				{
					return layer;
				}
			}
		}

		return boost::none;
	}
}


#endif // GPLATES_APP_LOGIC_RECONSTRUCTGRAPH_H
