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
#include "LayerInputChannelName.h"
#include "LayerProxy.h"
#include "Reconstruction.h"
#include "ReconstructionTree.h"

#include "model/FeatureCollectionHandle.h"
#include "model/types.h"
#include "model/WeakReferenceCallback.h"

#include "scribe/Transcribe.h"
#include "scribe/TranscribeContext.h"


namespace GPlatesAppLogic
{
	class Layer;
	class LayerTask;
	class LayerTaskParams;
	class ReconstructGraph;

	namespace ReconstructGraphImpl
	{
		class Layer;
		class LayerInputConnection;


		/**
		 * Data objects in the Reconstruct Graph Implementation are a wrapper around the
		 * two kinds of data you find in the graph. It can wrap around an input file,
		 * or represent the output of another layer.
		 */
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
					const LayerProxy::non_null_ptr_type &layer_proxy);

			/**
			 * Returns the input file.
			 *
			 * Returns true only if 'this' was created using a file.
			 */
			boost::optional<FeatureCollectionFileState::file_reference>
			get_input_file() const;

			/**
			 * Returns the layer proxy.
			 *
			 * Returns true only if 'this' was created using a layer proxy.
			 */
			boost::optional<LayerProxy::non_null_ptr_type>
			get_layer_proxy() const;

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
			 * Typedef for the data that differs depending on whether this data object
			 * is an input feature collection or the output of a layer.
			 */
			typedef boost::variant<
					FeatureCollectionFileState::file_reference,
					LayerProxy::non_null_ptr_type>
							data_type;


			data_type d_data;
			connection_seq_type d_output_connections;

			// Only used if this data object is the output of a layer.
			boost::optional< boost::weak_ptr<Layer> > d_outputting_layer;

		private: // Transcribing...

			//! Constructor data members that do not have a default constructor - the rest are transcribed.
			explicit
			Data(
					const data_type &data) :
				d_data(data)
			{  }

			GPlatesScribe::TranscribeResult
			transcribe(
					GPlatesScribe::Scribe &scribe,
					bool transcribed_construct_data);

			static
			GPlatesScribe::TranscribeResult
			transcribe_construct_data(
					GPlatesScribe::Scribe &scribe,
					GPlatesScribe::ConstructObject<Data> &data);

			// Only the scribe system should be able to transcribe.
			friend class GPlatesScribe::Access;

		};


		class LayerInputConnection :
				public boost::noncopyable
		{
		public:
			/**
			 * @throws PreconditionViolationError if @a input_data is NULL.
			 *
			 * @a is_input_layer_active is only used if the input is a layer (ie, if the
			 * input data is the output of another layer).
			 */
			LayerInputConnection(
					const boost::shared_ptr<Data> &input_data,
					const boost::weak_ptr<Layer> &layer_receiving_input,
					LayerInputChannelName::Type layer_input_channel_name,
					bool is_input_layer_active = true);

			~LayerInputConnection();

			const boost::shared_ptr<Data> &
			get_input_data() const
			{
				return d_input_data;
			}

			const boost::weak_ptr<Layer> &
			get_layer_receiving_input() const
			{
				return d_layer_receiving_input;
			}

			LayerInputChannelName::Type
			get_input_channel_name() const
			{
				return d_layer_input_channel_name;
			}

			/**
			 * NOTE: this will effectively destroy 'this' since our parent layer has the only
			 * owning reference to 'this'.
			 *
			 * A situation where it does not get disconnected is when 'this' layer connection
			 * connects a layer's input to the output of that same layer *and* that layer is
			 * currently in the process of being destroyed (ie, in layer's destructor).
			 * In this case the connection will get destroyed soon anyway when the layer's
			 * destruction process completes.
			 */
			void
			disconnect_from_parent_layer();

			/**
			 * Called when the input layer has been activated/deactivated (if the input is a layer).
			 *
			 * This only applies if the input is a layer (ie, if the input data is the output
			 * of another layer).
			 *
			 * This message will get delivered to the layer task of the layer receiving input
			 * so that it knows whether to access the input layer or not. If the input layer is
			 * inactive then the layer receiving input should not access it.
			 */
			void
			input_layer_activated(
					bool active);
			
		private:

			/**
			 * Receives notifications when input file, if connected to one, is modified.
			 */
			class FeatureCollectionModified :
					public GPlatesModel::WeakReferenceCallback<const GPlatesModel::FeatureCollectionHandle>
			{
			public:
				explicit
				FeatureCollectionModified(
						LayerInputConnection *layer_input_connection) :
					d_layer_input_connection(layer_input_connection)
				{
				}

				void
				publisher_modified(
						const modified_event_type &event)
				{
					d_layer_input_connection->modified_input_feature_collection();
				}

			private:
				LayerInputConnection *d_layer_input_connection;

			private: // Transcribing...

				GPlatesScribe::TranscribeResult
				transcribe(
						GPlatesScribe::Scribe &scribe,
						bool transcribed_construct_data);

				static
				GPlatesScribe::TranscribeResult
				transcribe_construct_data(
						GPlatesScribe::Scribe &scribe,
						GPlatesScribe::ConstructObject<FeatureCollectionModified> &callback);

				// Only the scribe system should be able to transcribe.
				friend class GPlatesScribe::Access;
			};


			void
			modified_input_feature_collection();


			boost::shared_ptr<Data> d_input_data;
			boost::weak_ptr<Layer> d_layer_receiving_input;
			LayerInputChannelName::Type d_layer_input_channel_name;
			bool d_is_input_layer_active;

			/**
			 * Keep a reference to the input feature collection just for our callback - if the
			 * input is not a file then this data member is ignored.
			 *
			 * Only we have access to this weak ref and we make sure the client doesn't have
			 * access to it. This is because any copies of this weak reference also get
			 * copies of the callback thus allowing it to get called more than once per modification.
			 */
			GPlatesModel::FeatureCollectionHandle::const_weak_ref d_callback_input_feature_collection;

		private: // Transcribing...

			LayerInputConnection() :
				d_is_input_layer_active(false)
			{  }

			//! Transcribe to/from serialization archives.
			GPlatesScribe::TranscribeResult
			transcribe(
					GPlatesScribe::Scribe &scribe,
					bool transcribed_construct_data);

			// Only the scribe system should be able to transcribe.
			friend class GPlatesScribe::Access;
		};


		class LayerInputConnections :
				public boost::noncopyable
		{
		public:
			typedef std::vector< boost::shared_ptr<LayerInputConnection> > connection_seq_type;

			typedef std::multimap<
					LayerInputChannelName::Type,
					boost::shared_ptr<LayerInputConnection> > input_connection_map_type;

			//! NOTE: should only be called by class LayerInputConnection.
			void
			add_input_connection(
					LayerInputChannelName::Type input_channel_name,
					const boost::shared_ptr<LayerInputConnection> &input_connection);

			//! NOTE: should only be called by class LayerInputConnection.
			void
			remove_input_connection(
					LayerInputChannelName::Type input_channel_name,
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
					LayerInputChannelName::Type input_channel_name) const;

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

		private: // Transcribing...

			//! Transcribe to/from serialization archives.
			GPlatesScribe::TranscribeResult
			transcribe(
					GPlatesScribe::Scribe &scribe,
					bool transcribed_construct_data);

			// Only the scribe system should be able to transcribe.
			friend class GPlatesScribe::Access;
		};


		class Layer :
				public boost::noncopyable
		{
		public:
			/**
			 * @throws PreconditionViolationError if @a layer_task is NULL.
			 */
			Layer(
					const boost::shared_ptr<LayerTask> &layer_task,
					ReconstructGraph &reconstruct_graph,
					bool auto_created = false);

			~Layer();

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

			//! Returns true if this layer was auto-created (when a file was loaded).
			bool
			get_auto_created() const
			{
				return d_auto_created;
			}

			void
			set_auto_created(
					bool auto_created = true)
			{
				d_auto_created = auto_created;
			}

			void
			set_layer_task(
					const boost::shared_ptr<LayerTask> &layer_task)
			{
				d_layer_task = layer_task;
			}

			const LayerTask &
			get_layer_task() const
			{
				return *d_layer_task;
			}

			LayerTask &
			get_layer_task()
			{
				return *d_layer_task;
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

			LayerTaskParams &
			get_layer_task_params();

		private:

			ReconstructGraph *d_reconstruct_graph;
			boost::shared_ptr<LayerTask> d_layer_task;

			LayerInputConnections d_input_data;
			boost::shared_ptr<Data> d_output_data;
			bool d_active;
			bool d_auto_created;

		private: // Transcribing...

			//! Constructor data members that do not have a default constructor - the rest are transcribed.
			explicit
			Layer(
					ReconstructGraph *reconstruct_graph) :
				d_reconstruct_graph(reconstruct_graph),
				d_active(false),
				d_auto_created(false)
			{  }

			GPlatesScribe::TranscribeResult
			transcribe(
					GPlatesScribe::Scribe &scribe,
					bool transcribed_construct_data);

			static
			GPlatesScribe::TranscribeResult
			transcribe_construct_data(
					GPlatesScribe::Scribe &scribe,
					GPlatesScribe::ConstructObject<Layer> &layer);

			// Only the scribe system should be able to transcribe.
			friend class GPlatesScribe::Access;
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
		 *
		 * UPDATE: From a purely graph point-of-view cycles are actually allowed.
		 * For example, a raster can use an age-grid during reconstruction but also the age-grid
		 * can use the raster as a normal map for its surface lighting. This is a cycle but it's
		 * OK because there's a disconnect between a layer's input and output. In this example
		 * there's a disconnect in the raster layer between the age-grid input and the normal map
		 * output - they are unrelated and don't depend on each other. So in this example while there
		 * is a cycle in the connection graph there is no actual cycle in the dependencies.
		 * TODO: For now we'll just disable cycle checking - if it's reintroduced it'll need to
		 * be smarter and get help from the layer proxies to determine dependency cycles.
		 */
		bool
		detect_cycle_in_graph(
				const Layer *originating_layer,
				const Layer *input_layer);
	}
}

namespace GPlatesScribe
{
	//
	// ReconstructGraphImpl::Layer ...
	//

	template <>
	class TranscribeContext<GPlatesAppLogic::ReconstructGraphImpl::Layer>
	{
	public:
		explicit
		TranscribeContext(
				GPlatesAppLogic::ReconstructGraph &reconstruct_graph_) :
			reconstruct_graph(&reconstruct_graph_)
		{  }

		GPlatesAppLogic::ReconstructGraph *reconstruct_graph;
	};
}

#endif // GPLATES_APP_LOGIC_RECONSTRUCTGRAPHIMPL_H
