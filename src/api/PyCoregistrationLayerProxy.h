/* $Id: FeatureCollection.h 11961 2011-07-07 03:49:38Z mchin $ */

/**
 * \file 
 * $Revision: 11961 $
 * $Date: 2011-07-07 13:49:38 +1000 (Thu, 07 Jul 2011) $
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

#ifndef GPLATES_API_COREGISTRATIONPROXY_H
#define GPLATES_API_COREGISTRATIONPROXY_H

#include "app-logic/CoRegistrationLayerProxy.h"

#include "global/python.h"

namespace bp=boost::python;

namespace GPlatesApi
{
	/**
	 * Wrapper around CoregistrationLayerProxy.
	 *
	 */
	
	class PyCoregistrationLayerProxy 
	{
	public:
		PyCoregistrationLayerProxy(
				GPlatesAppLogic::CoRegistrationLayerProxy::non_null_ptr_type proxy):
			d_proxy(proxy)
		{ }
	
		
		bp::list
		get_all_seed_features();


		bp::list
		get_associations();


		bp::list
		get_coregistration_data(
				float time);

		
		bp::list
		get_coregistration_data();
	
	private:
		GPlatesAppLogic::CoRegistrationLayerProxy::non_null_ptr_type d_proxy;		
	};

}

#endif  // GPLATES_API_COREGISTRATIONPROXY_H

