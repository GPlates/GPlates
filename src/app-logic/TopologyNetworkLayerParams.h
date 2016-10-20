/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2016 The University of Sydney, Australia
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

#ifndef GPLATES_APP_LOGIC_TOPOLOGYNETWORKLAYERPARAMS_H
#define GPLATES_APP_LOGIC_TOPOLOGYNETWORKLAYERPARAMS_H

#include <QObject>

#include "LayerParams.h"
#include "TopologyNetworkParams.h"


namespace GPlatesAppLogic
{
	/**
	 * App-logic parameters for a topological network layer.
	 */
	class TopologyNetworkLayerParams :
			public LayerParams
	{
		Q_OBJECT

	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<TopologyNetworkLayerParams> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const TopologyNetworkLayerParams> non_null_ptr_to_const_type;

		static
		non_null_ptr_type
		create()
		{
			return non_null_ptr_type(new TopologyNetworkLayerParams());
		}


		/**
		 * Returns the 'const' topology network parameters.
		 */
		const TopologyNetworkParams &
		get_topology_network_params() const
		{
			return d_topology_network_params;
		}

		/**
		 * Sets the topology network parameters.
		 *
		 * Emits signals 'modified_topology_network_params' and 'modified' if a change detected.
		 */
		void
		set_topology_network_params(
				const TopologyNetworkParams &topology_network_params)
		{
			if (d_topology_network_params == topology_network_params)
			{
				return;
			}

			d_topology_network_params = topology_network_params;

			Q_EMIT modified_topology_network_params(*this);
			emit_modified();
		}


		/**
		 * Override of virtual method in LayerParams base.
		 */
		virtual
		void
		accept_visitor(
				ConstLayerParamsVisitor &visitor) const
		{
			visitor.visit_topology_network_layer_params(*this);
		}

		/**
		 * Override of virtual method in LayerParams base.
		 */
		virtual
		void
		accept_visitor(
				LayerParamsVisitor &visitor)
		{
			visitor.visit_topology_network_layer_params(*this);
		}

	Q_SIGNALS:

		/**
		 * Emitted when @a set_topology_network_params has been called (if a change detected).
		 */
		void
		modified_topology_network_params(
				GPlatesAppLogic::TopologyNetworkLayerParams &layer_params);

	private:

		TopologyNetworkParams d_topology_network_params;


		TopologyNetworkLayerParams()
		{  }
	};
}

#endif // GPLATES_APP_LOGIC_TOPOLOGYNETWORKLAYERPARAMS_H
