/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2021 The University of Sydney, Australia
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

#ifndef GPLATES_API_PYRESOLVETOPOLOGYPARAMETERS_H
#define GPLATES_API_PYRESOLVETOPOLOGYPARAMETERS_H

#include "app-logic/TopologyNetworkParams.h"

#include "utils/ReferenceCount.h"


namespace GPlatesApi
{
	/**
	 * Parameters used when resolving topologies (mostly for deforming network topologies).
	 */
	class ResolveTopologyParameters :
			public GPlatesUtils::ReferenceCount<ResolveTopologyParameters>
	{
	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<ResolveTopologyParameters> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const ResolveTopologyParameters> non_null_ptr_to_const_type;


		static
		non_null_ptr_type
		create()
		{
			return non_null_ptr_type(new ResolveTopologyParameters());
		}

		/**
		 * Return the parameters used to resolve topological networks.
		 */
		const GPlatesAppLogic::TopologyNetworkParams &
		get_topology_network_params() const
		{
			return d_topology_network_params;
		}

	private:

		ResolveTopologyParameters()
		{  }

		GPlatesAppLogic::TopologyNetworkParams d_topology_network_params;
	};
}

#endif // GPLATES_API_PYRESOLVETOPOLOGYPARAMETERS_H
