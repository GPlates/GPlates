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
 
#ifndef GPLATES_APP_LOGIC_LAYER_H
#define GPLATES_APP_LOGIC_LAYER_H

#include <memory>
#include <utility>
#include <vector>
#include <boost/mpl/assert.hpp>
#include <boost/mpl/contains.hpp>
#include <boost/operators.hpp>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/tuple/tuple.hpp>
#include <QString>

#include "FeatureCollectionFileState.h"
#include "LayerTaskDataType.h"
#include "ReconstructGraphImpl.h"

#include "global/GPlatesException.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "model/FeatureCollectionHandle.h"


namespace GPlatesAppLogic
{
	class LayerTask;

	/**
	 * Wrapper around a layer of @a ReconstructGraph that can be used to query the layer.
	 */
	class Layer :
			public boost::equivalent<Layer>
	{
	public:
		/**
		 * The data type that is input to a layer.
		 *
		 * The three possible types are:
		 * 1) feature collection - typically used as the first level of input
		 *    to layers in the graph,
		 * 2) reconstruction geometries - typically output by layers and can be used
		 *    as inputs to other connected layers,
		 * 3) reconstruction tree - typically output by a layer that converts rotation
		 *    features (total reconstruction sequences) to a rotation tree that is used
		 *    as input to other layers for reconstruction purposes.
		 */
		enum LayerInputDataType
		{
			INPUT_FEATURE_COLLECTION_DATA,
			INPUT_RECONSTRUCTED_GEOMETRY_COLLECTION_DATA,
			INPUT_RECONSTRUCTION_TREE_DATA
		};

		/**
		 * The data type that is output from a layer.
		 *
		 * The two possible types are:
		 * 1) reconstruction geometries,
		 * 2) reconstruction tree - typically output by a layer that converts rotation
		 *    features (total reconstruction sequences) to a rotation tree.
		 */
		enum LayerOutputDataType
		{
			OUTPUT_RECONSTRUCTED_GEOMETRY_COLLECTION_DATA,
			OUTPUT_RECONSTRUCTION_TREE_DATA
		};

		/**
		 * Represents the number of data inputs allowed by a specific input channel of a layer.
		 *
		 * A layer can have one of more input channels representing different classifications
		 * of input data and each channel can have one or more data objects.
		 * The latter is what's determined here.
		 *
		 * For example the @a RECONSTRUCT_LAYER layer has a "rotation tree" input channel and a
		 * "reconstructable features" input channel.
		 * In the "reconstructable features" there can be multiple feature collections but
		 * in the rotation tree channel there can only be one reconstruction tree.
		 */
		enum ChannelDataArity
		{
			ONE_DATA_IN_CHANNEL,
			MULTIPLE_DATAS_IN_CHANNEL
		};

		/**
		 * Typedef for information describing the data type and arity allowed for
		 * a single input channel.
		 */
		typedef boost::tuple<
				QString, // input channel name
				LayerInputDataType,
				ChannelDataArity> input_channel_definition_type;


		/**
		 * Wrapper around an input file to a layer.
		 */
		class InputFile :
				public boost::equivalent<InputFile>
		{
		public:
			//! Constructor.
			explicit
			InputFile(
					const boost::weak_ptr<ReconstructGraphImpl::Data> &input_file_impl =
							boost::weak_ptr<ReconstructGraphImpl::Data>()) :
				d_impl(input_file_impl)
			{  }

			/**
			 * Returns true if this input file is still loaded.
			 */
			bool
			is_valid() const
			{
				return !d_impl.expired();
			}

			/**
			 * Activates (or deactivates) this loaded input file.
			 *
			 * Any layers connected to us will only receive the feature collection data
			 * in us if @a activate is true.
			 *
			 * FIXME: This is really a temporary method until file activation in
			 * ManageFeatureCollectionsDialog is removed and layer activation is provided
			 * in the layers GUI. We could keep both but it might be confusing for the user.
			 */
			void
			activate(
					bool active = true);

			/**
			 * Returns the loaded file that 'this' wraps.
			 *
			 * @throws PreconditionViolationError if @a is_valid is false.
			 */
			FeatureCollectionFileState::file_reference
			get_file() const;

			/**
			 * Returns the file information of the loaded file that 'this' wraps.
			 *
			 * @throws PreconditionViolationError if @a is_valid is false.
			 */
			const GPlatesFileIO::FileInfo &
			get_file_info() const
			{
				return get_file().get_file().get_file_info();
			}

			/**
			 * Returns the feature collection in the loaded file that 'this' wraps.
			 *
			 * @throws PreconditionViolationError if @a is_valid is false.
			 */
			GPlatesModel::FeatureCollectionHandle::weak_ref
			get_feature_collection() const
			{
				return get_file().get_file().get_feature_collection();
			}

			/**
			 * 'operator==()' provided by boost::equivalent.
			 */
			bool
			operator<(
					const InputFile &rhs) const
			{
				return d_impl < rhs.d_impl;
			}

			//! Used by implementation.
			const boost::weak_ptr<ReconstructGraphImpl::Data> &
			get_impl() const
			{
				return d_impl;
			}

		private:
			boost::weak_ptr<ReconstructGraphImpl::Data> d_impl;
		};


		/**
		 * Wrapper around an input connection of a layer.
		 */
		class InputConnection :
				public boost::equivalent<InputConnection>
		{
		public:
			//! Constructor.
			explicit
			InputConnection(
					const boost::weak_ptr<ReconstructGraphImpl::LayerInputConnection> &input_connection_impl =
							boost::weak_ptr<ReconstructGraphImpl::LayerInputConnection>()) :
				d_impl(input_connection_impl)
			{  }

			/**
			 * Returns true if this input connection is still valid and has not been destroyed.
			 */
			bool
			is_valid() const
			{
				return !d_impl.expired();
			}

			/**
			 * Explicitly disconnects an input data source from a layer.
			 *
			 * There are two situations where this can occur:
			 * 1) A feature collection that is used as input to a layer,
			 * 2) A layer output that is used as input to a layer.
			 *
			 * Note: You don't need to call this when destroying a layer as that will happen
			 * automatically and the memory used by the connection itself will be released.
			 *
			 * This will emit the @a ReconstructGraph signal
			 * @a layer_about_to_remove_input_connection.
			 * NOTE: This signal only gets emitted if a connection is explicitly disconnected
			 * (by calling @a disconnect). If this input connection is automatically destroyed
			 * because its parent layer is removed then no signal is emitted.
			 *
			 * This method is useful if the user explicitly changes the input sources
			 * of a layer (via the GUI) - by disconnecting an input and making a new connection.
			 *
			 * NOTE: this will make 'this' invalid (see @a is_valid) upon returning.
			 *
			 * @throws PreconditionViolationError if @a is_valid is false.
			 */
			void
			disconnect();

			/**
			 * Returns the input channel that 'this' connection belongs to.
			 *
			 * @throws PreconditionViolationError if @a is_valid is false.
			 */
			const QString &
			get_input_channel_name() const;

			/**
			 * Returns the parent layer of 'this' connection - the layer that this connection
			 * is inputting into.
			 *
			 * @throws PreconditionViolationError if @a is_valid is false.
			 */
			Layer
			get_layer() const;

			/**
			 * Returns the loaded file connected to 'this' input.
			 *
			 * This is useful when displaying layer input connections to the user via the GUI.
			 *
			 * Returns false if the data connected to 'this' input is the output
			 * of another layer.
			 * In this case @a get_input_layer should return true.
			 *
			 * @throws PreconditionViolationError if @a is_valid is false.
			 */
			boost::optional<InputFile>
			get_input_file() const;

			/**
			 * Returns the layer whose output is connected to 'this' input.
			 *
			 * This is useful when displaying layer input connections to the user via the GUI.
			 *
			 * Returns false if the data connected to 'this' input is a feature collection.
			 * In this case @a get_input_file should return true.
			 *
			 * @throws PreconditionViolationError if @a is_valid is false.
			 */
			boost::optional<Layer>
			get_input_layer() const;

			/**
			 * 'operator==()' provided by boost::equivalent.
			 */
			bool
			operator<(
					const InputConnection &rhs) const
			{
				return d_impl < rhs.d_impl;
			}

		private:
			boost::weak_ptr<ReconstructGraphImpl::LayerInputConnection> d_impl;
		};


		/**
		 * Exception thrown when a cycle is detected in the reconstruct graph.
		 * Currently only @a connect_input_to_layer_output can throw this exception.
		 */
		class CycleDetectedInReconstructGraph :
				public GPlatesGlobal::Exception
		{
		public:
			explicit
			CycleDetectedInReconstructGraph(
					const GPlatesUtils::CallStack::Trace &exception_source) :
				GPlatesGlobal::Exception(exception_source)
			{  }

		protected:
			virtual
			const char *
			exception_name() const
			{
				return "CycleDetectedInReconstructGraph";
			}
		};

		
		//! Constructor.
		explicit
		Layer(
				const boost::weak_ptr<ReconstructGraphImpl::Layer> &layer_impl =
						boost::weak_ptr<ReconstructGraphImpl::Layer>());


		/**
		 * Returns true if this layer is still valid and has not been destroyed.
		 */
		bool
		is_valid() const
		{
			return !d_impl.expired();
		}


		/**
		 * Returns true if this layer is currently active.
		 *
		 * When this layer is first created it is active.
		 */
		bool
		is_active() const;


		/**
		 * Activates (or deactivates) this layer.
		 *
		 * Output data, for this layer, will only be generated (the next time
		 * the @a ReconstructGraph is executed) if @a activate is true.
		 *
		 * Any layers connected to us will only receive our output data if @a activate is true.
		 *
		 * Emits the @a ReconstructGraph signal @a layer_activation_changed if the active
		 * state of this layer is changed by this method.
		 *
		 * @throws PreconditionViolationError if @a is_valid is false.
		 */
		void
		activate(
				bool active = true);


		/**
		 * Returns the name and description of this layer.
		 *
		 * This is useful for display to the user so they know what this layer does.
		 *
		 * @throws PreconditionViolationError if @a is_valid is false.
		 */
		std::pair<QString, QString>
		get_name_and_description() const;


		/**
		 * Returns a description of each input channel of this layer.
		 *
		 * The description includes the channel name, channel data type and
		 * number of data instances allowed per channel (one or multiple).
		 *
		 * This can be used to determine which other layers provide the necessary data type
		 * as outputs and hence which other layers can be connected this layer.
		 * This information can be used to query the user (via the GUI) which
		 * layers to connect @a layer_id to.
		 * The same applies to feature collections although usually a layer will accept
		 * a feature collection as input on only one of its channels and this usually
		 * gives layers their one-to-one correspondence with loaded feature collections.
		 *
		 * @throws PreconditionViolationError if @a is_valid is false.
		 */
		std::vector<input_channel_definition_type>
		get_input_channel_definitions() const;


		/**
		 * Returns the main input feature collection channel used by this layer.
		 *
		 * This is the channel containing the feature collection(s) used to determine the
		 * layer tasks that are applicable to this layer.
		 *
		 * This can be used by the GUI to list available layer tasks to the user.
		 *
		 * NOTE: The data type of this input channel, as returned by @a get_input_channel_definitions,
		 * must be INPUT_FEATURE_COLLECTION_DATA.
		 *
		 * @throws PreconditionViolationError if @a is_valid is false.
		 */
		QString
		get_main_input_feature_collection_channel() const;


		/**
		 * Returns the data type output by this layer.
		 *
		 * @throws PreconditionViolationError if @a is_valid is false.
		 */
		LayerOutputDataType
		get_output_definition() const;


		/**
		 * Changes the layer task for this layer.
		 *
		 * Use @a LayerTaskRegistry to get a list of layer tasks that can be used with
		 * this layer. This can be done by passing the input feature collection(s) of this
		 * layer's main input channel (the channel returned by
		 * @a get_main_input_feature_collection_channel) to
		 * LayerTaskRegistry::get_layer_task_types_that_can_process_feature_collections().
		 *
		 * NOTE: A new layer task has different input channel definitions so any current
		 * input channel connections (except the main input feature collection channel)
		 * probably don't make sense anymore, so should probably disconnect them.
		 *
		 * @throws PreconditionViolationError if @a is_valid is false.
		 */
		void
		set_layer_task(
				const boost::shared_ptr<LayerTask> &layer_task);


		/**
		 * Connects a feature collection, from a loaded file, as input on the
		 * @a layer_input_data_channel input channel.
		 *
		 * The returned @a InputConnection is a weak reference - it can be ignored (in other
		 * words it does not need to be stored somewhere to keep the connection alive).
		 *
		 * The returned connection will automatically be destroyed if @a input_file
		 * is subsequently unloaded (in which case the returned @a InputConnection will
		 * become invalid).
		 *
		 * NOTE: A connection to an input file can always be made without introducing a cycle.
		 *
		 * Emits the @a ReconstructGraph signal @a layer_added_input_connection.
		 *
		 * @throws PreconditionViolationError if @a is_valid is false or
		 * if @a input_file is not valid.
		 */
		InputConnection
		connect_input_to_file(
				const InputFile &input_file,
				const QString &input_data_channel);


		/**
		 * Connects the output of the @a layer_outputting_data layer as input to the
		 * this layer on the @a input_data_channel input channel.
		 *
		 * The returned @a InputConnection is a weak reference - it can be ignored (in other
		 * words it does not need to be stored somewhere to keep the connection alive).
		 *
		 * The returned connection will automatically be destroyed if @a layer_outputting_data
		 * is subsequently destroyed (in which case the returned @a InputConnection will
		 * become invalid).
		 *
		 * Emits the @a ReconstructGraph signal @a layer_added_input_connection.
		 *
		 * @throws @a CycleDetectedInReconstructGraph if the resulting connection would
		 * create a cycle in the graph - in this case the connection is not made and the
		 * state of the graph is unchanged.
		 * Clients can try/catch this exception and inform user that connection cannot be made.
		 *
		 * @throws @a PreconditionViolationError if either this layer or @a layer_outputting_data
		 * has @a is_valid returning false.
		 */
		InputConnection
		connect_input_to_layer_output(
				const Layer &layer_outputting_data,
				const QString &input_data_channel);


		/**
		 * Disconnects all input data sources on input channel @a input_data_channel
		 * from this layer.
		 *
		 * This method simply calls InputConnection::disconnect on all connection objects
		 * returned by @a get_channel_inputs.
		 *
		 * See the documentation on InputConnection::disconnect for signals emitted.
		 *
		 * @throws PreconditionViolationError if @a is_valid is false.
		 */
		void
		disconnect_channel_inputs(
				const QString &input_data_channel);


		/**
		 * Returns the input connections on input channel @a input_data_channel.
		 *
		 * This is useful for displaying the connections to the user via the GUI.
		 *
		 * @throws PreconditionViolationError if @a is_valid is false.
		 */
		std::vector<InputConnection>
		get_channel_inputs(
				const QString &input_data_channel) const;


		/**
		 * Returns all input connections.
		 *
		 * @throws PreconditionViolationError if @a is_valid is false.
		 */
		std::vector<InputConnection>
		get_all_inputs() const;


		/**
		 * Extracts the output data generated by this layer.
		 *
		 * This is the output generated during the most recent call
		 * to ReconstructGraph::execute_layer_tasks().
		 *
		 * Returns false if the output data is not of type LayerOutputDataType.
		 *
		 * The template parameter LayerOutputDataType is expected to be one of the types
		 * in the @a layer_task_data_type variant.
		 *
		 * Examples of the template parameter LayerOutputDataType are:
		 * - ReconstructionGeometryCollection::non_null_ptr_to_const_type
		 * - ReconstructionTree::non_null_ptr_to_const_type
		 *
		 * Example usage:
		 *   layer.get_output_data<ReconstructionTree::non_null_ptr_to_const_type>();
		 *
		 * @throws PreconditionViolationError if @a is_valid is false.
		 */
		template <class LayerOutputDataType>
		boost::optional<LayerOutputDataType>
		get_output_data() const
		{
			// Compile time error to make sure that the template parameter 'LayerOutputDataType'
			// is one of the bounded types in the 'layer_task_data_type' variant.
#ifdef WIN32 // Old-style cast error in gcc...
			BOOST_MPL_ASSERT((boost::mpl::contains<layer_task_data_types, LayerOutputDataType>));
#endif

			// Throw our own exception to track location of throw.
			GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				is_valid(),
				GPLATES_ASSERTION_SOURCE);

			boost::shared_ptr<ReconstructGraphImpl::Layer> layer_impl(d_impl);

			// Get the layer output data.
			const boost::optional<layer_task_data_type> &layer_task_output_data =
					layer_impl->get_output_data()->get_data();
			if (!layer_task_output_data)
			{
				return boost::none;
			}

			// Attempt to cast the variant to the type expected by the caller.
			return get_layer_task_data<LayerOutputDataType>(layer_task_output_data.get());
		}


		/**
		 * 'operator==()' provided by boost::equivalent.
		 */
		bool
		operator<(
				const Layer &rhs) const
		{
			return d_impl < rhs.d_impl;
		}


		//! Used by implementation.
		const boost::weak_ptr<ReconstructGraphImpl::Layer> &
		get_impl() const
		{
			return d_impl;
		}

	private:
		boost::weak_ptr<ReconstructGraphImpl::Layer> d_impl;
	};
}

#endif // GPLATES_APP_LOGIC_LAYER_H
