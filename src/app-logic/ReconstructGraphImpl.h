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
 
#ifndef GPLATES_APP_LOGIC_RECONSTRUCTGRAPHIMPL_H
#define GPLATES_APP_LOGIC_RECONSTRUCTGRAPHIMPL_H

#include <list>
#include <map>
#include <string>
#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/variant.hpp>
#include <boost/weak_ptr.hpp>

#include "FeatureCollectionFileState.h"
#include "LayerTaskDataType.h"
#include "Reconstruction.h"
#include "ReconstructionTree.h"

#include "model/types.h"


namespace GPlatesAppLogic
{
	class LayerTask;
	class ReconstructGraph;

	namespace ReconstructGraphImpl
	{
		class Layer;
		class LayerInputConnection;


		class Data :
				public boost::noncopyable
		{
		public:
			typedef std::list<LayerInputConnection *> connection_seq_type;

			/**
			 * Constructor used when connecting a layer to an input feature collection.
			 * In this case 'this' object is *not* connected to an outputting layer.
			 */
			explicit
			Data(
					const FeatureCollectionFileState::file_reference &file);

			/**
			 * Constructor used when connecting a layer to an output of another layer.
			 * In this case 'this' object *should* be connected to an outputting layer
			 * using @a set_outputting_layer.
			 */
			explicit
			Data(
					const boost::optional<layer_task_data_type> &data = boost::none);

			/**
			 * Activates (or deactivates) this data (must be a file).
			 *
			 * @throws PreconditionViolationError if 'this' was *not* created using a file.
			 *
			 * FIXME: This is really a temporary method until file activation in
			 * ManageFeatureCollectionsDialog is removed and layer activation is provided
			 * in the layers GUI. We could keep both but it might be confusing for the user.
			 */
			void
			activate_input_file(
					bool activate = true);

			/**
			 * Returns the input file.
			 *
			 * Returns true only if 'this' was created using a file.
			 */
			boost::optional<FeatureCollectionFileState::file_reference>
			get_input_file() const;

			/**
			 * Returns the layer outputting us.
			 *
			 * Returns true only if @a set_outputting_layer was successfully called
			 * (which in turn also means 'this' was not created using a file).
			 *
			 * This is used to indirectly get a reference to the layer connected to an
			 * input connection.
			 */
			boost::optional< boost::weak_ptr<Layer> >
			get_outputting_layer() const;

			/**
			 * Sets the layer that outputs data to 'this'.
			 * NOTE: This does not apply to input feature collections which are not
			 * the output of a layer.
			 *
			 * @throws PreconditionViolationError if 'this' was created using a file or
			 * if @a outputting_layer is not a valid reference.
			 */
			void
			set_outputting_layer(
					const boost::weak_ptr<Layer> &outputting_layer);

			/**
			 * Returns the data contained within.
			 *
			 * False is returned under two situations:
			 * - the data was set to false using @a set_data (or the constructor), or
			 * - this data is currently deactivated (only applies if this data is a file).
			 */
			boost::optional<layer_task_data_type>
			get_data() const;

			/**
			 * Sets the data - only makes sense if 'this' is the output of a layer.
			 *
			 * @throws PreconditionViolationError if @a set_outputting_layer was never called.
			 */
			void
			set_data(
					const boost::optional<layer_task_data_type> &data);

			const connection_seq_type &
			get_output_connections() const
			{
				return d_output_connections;
			}

			void
			add_output_connection(
					LayerInputConnection *layer_input_connection);

			void
			remove_output_connection(
					LayerInputConnection *layer_input_connection);

			/**
			 * Gets all output connections to disconnect themselves from their parent layers,
			 * which will destroy them, which will remove them from our output connections list.
			 *
			 * This should be called either:
			 * - by the layer that has 'this' as its output data when that layer is destroyed, or
			 * - by an input feature collection when its containing file is unloaded.
			 */
			void
			disconnect_output_connections();

		private:
			/**
			 * Typedef for some state that differs depending on whether this data object
			 * is an input feature collection or the output of a layer.
			 */
			typedef boost::variant<
					FeatureCollectionFileState::file_reference,
					boost::weak_ptr<Layer> >
							state_type;

			boost::optional<layer_task_data_type> d_data;
			connection_seq_type d_output_connections;
			state_type d_state;

			/**
			 * Active status of this data as an input file.
			 *
			 * FIXME: Remove this when @a activate_input_file is removed.
			 */
			bool d_active;
		};


		class LayerInputConnection :
				public boost::noncopyable
		{
		public:
			LayerInputConnection(
					const boost::shared_ptr<Data> &input_data,
					const boost::weak_ptr<Layer> &layer_receiving_input,
					const QString &layer_input_channel_name);

			~LayerInputConnection();

			boost::shared_ptr<Data>
			get_input_data() const
			{
				return d_input_data;
			}

			const boost::weak_ptr<Layer> &
			get_layer_receiving_input() const
			{
				return d_layer_receiving_input;
			}

			const QString &
			get_input_channel_name() const
			{
				return d_layer_input_channel_name;
			}

			/**
			 * NOTE: this will effectively destroy 'this' since our parent layer has the only
			 * owning reference to 'this'.
			 */
			void
			disconnect_from_parent_layer();
			
		private:
			boost::shared_ptr<Data> d_input_data;
			boost::weak_ptr<Layer> d_layer_receiving_input;
			QString d_layer_input_channel_name;
		};


		class LayerInputConnections :
				public boost::noncopyable
		{
		public:
			typedef std::vector< boost::shared_ptr<LayerInputConnection> > connection_seq_type;

			typedef std::multimap<
					QString,
					boost::shared_ptr<LayerInputConnection> > input_connection_map_type;

			//! NOTE: should only be called by class LayerInputConnection.
			void
			add_input_connection(
					const QString &input_channel_name,
					const boost::shared_ptr<LayerInputConnection> &input_connection);

			//! NOTE: should only be called by class LayerInputConnection.
			void
			remove_input_connection(
					const QString &input_channel_name,
					LayerInputConnection *input_connection);

			/**
			 * Returns all input connections as a sequence of @a LayerInputConnection pointers.
			 */
			connection_seq_type
			get_input_connections() const;

			/**
			 * Returns all input connections associated with the channel @a input_channel_name
			 * as a sequence of @a LayerInputConnection pointers.
			 */
			connection_seq_type
			get_input_connections(
					const QString &input_channel_name) const;

			/**
			 * Returns all input connections as a sequence of @a LayerInputConnection pointers.
			 */
			const input_connection_map_type &
			get_input_connection_map() const
			{
				return d_connections;
			}

		private:
			input_connection_map_type d_connections;
		};


		class Layer :
				public boost::noncopyable
		{
		public:
			Layer(
					const boost::shared_ptr<LayerTask> &layer_task,
					ReconstructGraph &reconstruct_graph);

			~Layer();

			/**
			 * If this layer has been deactivated then the output data generated by this
			 * call will be empty (ie, boost::none).
			 */
			void
			execute(
					Reconstruction &reconstruction,
					GPlatesModel::integer_plate_id_type anchored_plate_id);

			/**
			 * Activates (or deactivates) this layer.
			 *
			 * Output data, for this layer, will only be generated (the next time
			 * the @a ReconstructGraph is executed) if @a activate is true.
			 *
			 * Any layers connected to us will only receive our output data if @a activate is true.
			 */
			void
			activate(
					bool active = true);

			/**
			 * Returns true if 'this' layer is currently active
			 */
			bool
			is_active() const
			{
				return d_active;
			}

			void
			set_layer_task(
					const boost::shared_ptr<LayerTask> &layer_task)
			{
				d_layer_task = layer_task;
			}

			const LayerTask *
			get_layer_task() const
			{
				return d_layer_task.get();
			}

			const LayerInputConnections &
			get_input_connections() const
			{
				return d_input_data;
			}

			LayerInputConnections &
			get_input_connections()
			{
				return d_input_data;
			}

			const boost::shared_ptr<Data> &
			get_output_data() const
			{
				return d_output_data;
			}

			ReconstructGraph &
			get_reconstruct_graph() const
			{
				return *d_reconstruct_graph;
			}

		private:
			ReconstructGraph *d_reconstruct_graph;
			boost::shared_ptr<LayerTask> d_layer_task;

			LayerInputConnections d_input_data;
			boost::shared_ptr<Data> d_output_data;
			bool d_active;
		};

		/**
		 * Returns true if a cycle would occur starting at @a originating_layer and also
		 * ending at @a originating_layer if @a originating_layer had its input connected
		 * to the output of @a input_layer.
		 *
		 * This assumes the current graph has no cycles in it.
		 *
		 * This takes into account only explicit connections in the graph.
		 *
		 * NOTE: This does not take into account implicit connections to the
		 * default reconstruction tree because it's not necessary (since reconstruction tree
		 * layers cannot take input from other layer outputs and hence cannot introduce a cycle).
		 * NOTE: This does not take into account the implicit dependencies that features in
		 * topological layers have on features in reconstruct layers (since we're only really
		 * checking cycles to avoid infinite recursion when executing layers in the graph) and
		 * these feature reference dependencies will not produce cycles in the layers.
		 */
		bool
		detect_cycle_in_graph(
				const Layer *originating_layer,
				const Layer *input_layer);
	}
}


#endif // GPLATES_APP_LOGIC_RECONSTRUCTGRAPHIMPL_H
