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
#include <boost/shared_ptr.hpp>

#include "ReconstructGraph.h"
#include "LayerTask.h"


namespace GPlatesAppLogic
{
	namespace ReconstructGraphImpl
	{
		class LayerInputConnection;


		class Data :
				public boost::noncopyable
		{
		public:
			typedef std::list<LayerInputConnection *> connection_seq_type;

			explicit
			Data(
					const layer_data_type &data = layer_data_type());

			const layer_data_type &
			get_data() const
			{
				return d_data;
			}

			layer_data_type &
			get_data()
			{
				return d_data;
			}

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

		private:
			layer_data_type d_data;
			connection_seq_type d_output_connections;
		};


		class LayerInputConnection :
				public boost::noncopyable
		{
		public:
			LayerInputConnection(
					Data *input_data,
					Layer *layer,
					const ReconstructGraph::layer_input_channel_name_type &layer_input_channel_name);

			~LayerInputConnection();

			Data *
			get_input_data() const
			{
				return d_input_data;
			}

			Layer *
			get_layer() const
			{
				return d_layer;
			}
			
		private:
			Data *d_input_data;
			Layer *d_layer;
			ReconstructGraph::layer_input_channel_name_type d_layer_input_channel_name;
		};


		class LayerInputConnections :
				public boost::noncopyable
		{
		public:
			typedef std::vector<LayerInputConnection *> connection_seq_type;

			typedef std::multimap<
					ReconstructGraph::layer_input_channel_name_type,
					LayerInputConnection *> input_connection_map_type;

			void
			add_input_connection(
					const ReconstructGraph::layer_input_channel_name_type &input_channel_name,
					LayerInputConnection *input_connection);

			void
			remove_input_connection(
					const ReconstructGraph::layer_input_channel_name_type &input_channel_name,
					LayerInputConnection *input_connection);

			/**
			 * Returns all input connections as a sequence of @a LayerInputConnection pointers.
			 */
			connection_seq_type
			get_input_connections() const;

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
			explicit
			Layer(
					const boost::shared_ptr<LayerTask> &layer_task);

			void
			execute(
					const double &reconstruction_time);

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

			Data *
			get_output_data()
			{
				return &d_output_data;
			}

		private:
			boost::shared_ptr<LayerTask> d_layer_task;

			LayerInputConnections d_input_data;
			Data d_output_data;
		};
	}
}


#endif // GPLATES_APP_LOGIC_RECONSTRUCTGRAPHIMPL_H
