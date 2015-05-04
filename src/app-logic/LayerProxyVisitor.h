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

#ifndef GPLATES_APP_LOGIC_LAYERPROXYVISITOR_H
#define GPLATES_APP_LOGIC_LAYERPROXYVISITOR_H

#include "utils/CopyConst.h"
#include "utils/non_null_intrusive_ptr.h"


namespace GPlatesAppLogic
{
	class LayerProxy;

	// Forward declaration of template visitor class.
	template <class LayerProxyType>
	class LayerProxyVisitorBase;


	/**
	 * Typedef for visitor over non-const @a LayerProxy objects.
	 */
	typedef LayerProxyVisitorBase<LayerProxy> LayerProxyVisitor;

	/**
	 * Typedef for visitor over const @a LayerProxy objects.
	 */
	typedef LayerProxyVisitorBase<const LayerProxy> ConstLayerProxyVisitor;


	// Forward declarations of LayerProxy derived types.
	class CoRegistrationLayerProxy;
	class RasterLayerProxy;
	class ReconstructLayerProxy;
	class ReconstructScalarCoverageLayerProxy;
	class ReconstructionLayerProxy;
	class ScalarField3DLayerProxy;
	class TopologyGeometryResolverLayerProxy;
	class TopologyNetworkResolverLayerProxy;
	class VelocityFieldCalculatorLayerProxy;


	/**
	 * This class defines an abstract interface for a Visitor to visit layer proxy objects.
	 *
	 * See the Visitor pattern (p.331) in Gamma95 for more information on the design and
	 * operation of this class.  This class corresponds to the abstract Visitor class in the
	 * pattern structure.
	 *
	 * @par Notes on the class implementation:
	 *  - All the virtual member "visit" functions have (empty) definitions for convenience, so
	 *    that derivations of this abstract base need only override the "visit" functions which
	 *    interest them.
	 */
	template <class LayerProxyType>
	class LayerProxyVisitorBase
	{
	public:
		//! Typedef for @a CoRegistrationLayerProxy of appropriate const-ness.
		typedef typename GPlatesUtils::CopyConst<
				LayerProxyType, CoRegistrationLayerProxy>::type
						co_registration_layer_proxy_type;

		//! Typedef for @a RasterLayerProxy of appropriate const-ness.
		typedef typename GPlatesUtils::CopyConst<
				LayerProxyType, RasterLayerProxy>::type
						raster_layer_proxy_type;

		//! Typedef for @a ReconstructLayerProxy of appropriate const-ness.
		typedef typename GPlatesUtils::CopyConst<
				LayerProxyType, ReconstructLayerProxy>::type
						reconstruct_layer_proxy_type;

		//! Typedef for @a ReconstructScalarCoverageLayerProxy of appropriate const-ness.
		typedef typename GPlatesUtils::CopyConst<
				LayerProxyType, ReconstructScalarCoverageLayerProxy>::type
						reconstruct_scalar_coverage_layer_proxy_type;

		//! Typedef for @a ReconstructionLayerProxy of appropriate const-ness.
		typedef typename GPlatesUtils::CopyConst<
				LayerProxyType, ReconstructionLayerProxy>::type
						reconstruction_layer_proxy_type;

		//! Typedef for @a ScalarField3DLayerProxy of appropriate const-ness.
		typedef typename GPlatesUtils::CopyConst<
				LayerProxyType, ScalarField3DLayerProxy>::type
						scalar_field_3d_layer_proxy_type;

		//! Typedef for @a TopologyGeometryResolverLayerProxy of appropriate const-ness.
		typedef typename GPlatesUtils::CopyConst<
				LayerProxyType, TopologyGeometryResolverLayerProxy>::type
						topology_geometry_resolver_layer_proxy_type;

		//! Typedef for @a TopologyNetworkResolverLayerProxy of appropriate const-ness.
		typedef typename GPlatesUtils::CopyConst<
				LayerProxyType, TopologyNetworkResolverLayerProxy>::type
						topology_network_resolver_layer_proxy_type;

		//! Typedef for @a VelocityFieldCalculatorLayerProxy of appropriate const-ness.
		typedef typename GPlatesUtils::CopyConst<
				LayerProxyType, VelocityFieldCalculatorLayerProxy>::type
						velocity_field_calculator_layer_proxy_type;


		// We'll make this function pure virtual so that the class is abstract.  The class
		// *should* be abstract, but wouldn't be unless we did this, since all the virtual
		// member functions have (empty) definitions.
		virtual
		~LayerProxyVisitorBase() = 0;


		/**
		 * Override this function in your own derived class.
		 */
		virtual
		void
		visit(
				const GPlatesUtils::non_null_intrusive_ptr<co_registration_layer_proxy_type> &layer_proxy)
		{  }


		/**
		 * Override this function in your own derived class.
		 */
		virtual
		void
		visit(
				const GPlatesUtils::non_null_intrusive_ptr<raster_layer_proxy_type> &layer_proxy)
		{  }

		/**
		 * Override this function in your own derived class.
		 */
		virtual
		void
		visit(
				const GPlatesUtils::non_null_intrusive_ptr<reconstruct_layer_proxy_type> &layer_proxy)
		{  }


		/**
		 * Override this function in your own derived class.
		 */
		virtual
		void
		visit(
				const GPlatesUtils::non_null_intrusive_ptr<reconstruct_scalar_coverage_layer_proxy_type> &layer_proxy)
		{  }

		/**
		 * Override this function in your own derived class.
		 */
		virtual
		void
		visit(
				const GPlatesUtils::non_null_intrusive_ptr<reconstruction_layer_proxy_type> &layer_proxy)
		{  }


		/**
		 * Override this function in your own derived class.
		 */
		virtual
		void
		visit(
				const GPlatesUtils::non_null_intrusive_ptr<scalar_field_3d_layer_proxy_type> &layer_proxy)
		{  }


		/**
		 * Override this function in your own derived class.
		 */
		virtual
		void
		visit(
				const GPlatesUtils::non_null_intrusive_ptr<topology_geometry_resolver_layer_proxy_type> &layer_proxy)
		{  }


		/**
		 * Override this function in your own derived class.
		 */
		virtual
		void
		visit(
				const GPlatesUtils::non_null_intrusive_ptr<topology_network_resolver_layer_proxy_type> &layer_proxy)
		{  }


		/**
		 * Override this function in your own derived class.
		 */
		virtual
		void
		visit(
				const GPlatesUtils::non_null_intrusive_ptr<velocity_field_calculator_layer_proxy_type> &layer_proxy)
		{  }
	};


	template <class LayerProxyType>
	inline
	LayerProxyVisitorBase<LayerProxyType>::~LayerProxyVisitorBase()
	{  }

}

#endif // GPLATES_APP_LOGIC_LAYERPROXYVISITOR_H
