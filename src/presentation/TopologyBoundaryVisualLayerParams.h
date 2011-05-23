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

#ifndef GPLATES_PRESENTATION_TOPOLOGYBOUNDARYVISUALLAYERPARAMS_H
#define GPLATES_PRESENTATION_TOPOLOGYBOUNDARYVISUALLAYERPARAMS_H

#include "VisualLayerParams.h"


namespace GPlatesPresentation
{
	class TopologyBoundaryVisualLayerParams :
			public VisualLayerParams
	{
	public:
		typedef GPlatesUtils::non_null_intrusive_ptr<TopologyBoundaryVisualLayerParams> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const TopologyBoundaryVisualLayerParams> non_null_ptr_to_const_type;

		static
		non_null_ptr_type
		create(
			GPlatesAppLogic::LayerTaskParams &layer_task_params)
		{
			return new TopologyBoundaryVisualLayerParams( layer_task_params );
		}

		void
		set_fill_polygons(
				bool fill)
		{
			d_fill_polygons = fill;
			emit_modified();
		}

		bool
		get_fill_polygons() const
		{
			return d_fill_polygons;
		}

		virtual
		void
		accept_visitor(
				ConstVisualLayerParamsVisitor &visitor) const
		{
			visitor.visit_topology_boundary_visual_layer_params(*this);
		}

		virtual
		void
		accept_visitor(
				VisualLayerParamsVisitor &visitor)
		{
			visitor.visit_topology_boundary_visual_layer_params(*this);
		}

	protected:
		explicit
		TopologyBoundaryVisualLayerParams( 
				GPlatesAppLogic::LayerTaskParams &layer_task_params) :
			VisualLayerParams(layer_task_params),
			d_fill_polygons(false)
		{  }

	private:

		bool d_fill_polygons;
	};
}

#endif // GPLATES_PRESENTATION_TOPOLOGYBOUNDARYVISUALLAYERPARAMS_H
