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

#ifndef GPLATES_PRESENTATION_LAYEROUTPUTRENDERER_H
#define GPLATES_PRESENTATION_LAYEROUTPUTRENDERER_H

#include "app-logic/LayerProxyVisitor.h"


namespace GPlatesViewOperations
{
	class RenderedGeometryLayer;
}

namespace GPlatesPresentation
{
	class ReconstructionGeometryRenderer;

	/**
	 * Visits the output of layers (the layer proxy objects) and renders their outputs
	 * to a @a RenderedGeometryLayer using a @a ReconstructionGeometryRenderer object.
	 *
	 * This class differs from @a ReconstructionGeometryRenderer in that this class
	 * deals with the specific interfaces of the layer outputs (layer proxies) and
	 * then delegates the @a ReconstructionGeometry rendering to @a ReconstructionGeometryRenderer.
	 */
	class LayerOutputRenderer :
			public GPlatesAppLogic::LayerProxyVisitor
	{
	public:
		LayerOutputRenderer(
				ReconstructionGeometryRenderer &reconstruction_geometry_renderer,
				GPlatesViewOperations::RenderedGeometryLayer &rendered_geometry_layer);


		virtual
		void
		visit(
				const GPlatesUtils::non_null_intrusive_ptr<co_registration_layer_proxy_type> &layer_proxy);

		virtual
		void
		visit(
				const GPlatesUtils::non_null_intrusive_ptr<raster_layer_proxy_type> &layer_proxy);

		virtual
		void
		visit(
				const GPlatesUtils::non_null_intrusive_ptr<reconstruct_layer_proxy_type> &layer_proxy);

		virtual
		void
		visit(
				const GPlatesUtils::non_null_intrusive_ptr<reconstruction_layer_proxy_type> &layer_proxy);

		virtual
		void
		visit(
				const GPlatesUtils::non_null_intrusive_ptr<scalar_field_3d_layer_proxy_type> &layer_proxy);

		virtual
		void
		visit(
				const GPlatesUtils::non_null_intrusive_ptr<topology_boundary_resolver_layer_proxy_type> &layer_proxy);

		virtual
		void
		visit(
				const GPlatesUtils::non_null_intrusive_ptr<topology_network_resolver_layer_proxy_type> &layer_proxy);

		virtual
		void
		visit(
				const GPlatesUtils::non_null_intrusive_ptr<velocity_field_calculator_layer_proxy_type> &layer_proxy);

	private:
		ReconstructionGeometryRenderer &d_reconstruction_geometry_renderer;
		GPlatesViewOperations::RenderedGeometryLayer &d_rendered_geometry_layer;
	};
}

#endif // GPLATES_PRESENTATION_LAYEROUTPUTRENDERER_H
