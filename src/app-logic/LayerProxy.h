/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
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

#ifndef GPLATES_APP_LOGIC_LAYERPROXY_H
#define GPLATES_APP_LOGIC_LAYERPROXY_H
	
#include "LayerProxyVisitor.h"

#include "utils/ReferenceCount.h"


namespace GPlatesAppLogic
{
	/**
	 * A handle to a layer proxy.
	 *
	 * This is useful when you need a pointer to a layer proxy for object identification but
	 * you don't want clients to be able to use the layer proxy interface - for example,
	 * because the layer is inactive.
	 */
	class LayerProxyHandle :
			public GPlatesUtils::ReferenceCount<LayerProxyHandle>
	{
	public:
		// Convenience typedefs for a shared pointer to a @a LayerProxyHandle.
		typedef GPlatesUtils::non_null_intrusive_ptr<LayerProxyHandle> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const LayerProxyHandle> non_null_ptr_to_const_type;

		virtual
		~LayerProxyHandle()
		{  }
	};


	/**
	 * Base class for layer proxies.
	 *
	 * Each application logic layer has a single layer proxy object at its output that clients,
	 * including other layers, can use to query that layer (eg, ask layer to do some processing).
	 *
	 * Layer proxy derived classes should cache any processing they do in case another client
	 * asks it to do the same processing.
	 *
	 * This is because layers now use a pull model where previously a push model was used.
	 * Previously all layers were executed and each layer generated results that were stored
	 * in their layer outputs (such as generating ReconstructionGeometry's).
	 * With the layer proxy concept, layers only generate output when asked to do so.
	 *
	 * This is both more efficient and allows each proxy to provide as specialised and rich
	 * an interface as is appropriate for that layer.
	 *
	 * The hierarchy of layer proxy objects is visitable with the @a LayerProxyVisitor hierarchy.
	 * See also @a LayerProxyUtils for convenient ways to access derived layer proxy classes.
	 */
	class LayerProxy :
			public LayerProxyHandle
	{
	public:
		// Convenience typedefs for a shared pointer to a @a LayerProxy.
		typedef GPlatesUtils::non_null_intrusive_ptr<LayerProxy> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const LayerProxy> non_null_ptr_to_const_type;


		virtual
		~LayerProxy()
		{  }


		/**
		 * Accept a ConstLayerProxyVisitor instance.
		 */
		virtual
		void
		accept_visitor(
				ConstLayerProxyVisitor &visitor) const = 0;

		/**
		 * Accept a LayerProxyVisitor instance.
		 */
		virtual
		void
		accept_visitor(
				LayerProxyVisitor &visitor) = 0;

	};
}

#endif // GPLATES_APP_LOGIC_LAYERPROXY_H
