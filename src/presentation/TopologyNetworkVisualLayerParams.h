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
 
#ifndef GPLATES_PRESENTATION_TOPOLOGYNETWORKVISUALLAYERPARAMS_H
#define GPLATES_PRESENTATION_TOPOLOGYNETWORKVISUALLAYERPARAMS_H

#include "VisualLayerParams.h"

#include "gui/DrawStyleManager.h"


namespace GPlatesPresentation
{
	class TopologyNetworkVisualLayerParams :
			public VisualLayerParams
	{
	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<TopologyNetworkVisualLayerParams> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const TopologyNetworkVisualLayerParams> non_null_ptr_to_const_type;

		static
		non_null_ptr_type
		create(
			GPlatesAppLogic::LayerTaskParams &layer_task_params)
		{
			return new TopologyNetworkVisualLayerParams( layer_task_params );
		}

		bool
		show_delaunay_triangulation() const
		{
			return d_show_delaunay_triangulation;
		}

		void
		set_show_delaunay_triangulation(
				bool b)
		{
			d_show_delaunay_triangulation = b;
			emit_modified();
		}

		bool
		show_constrained_triangulation() const
		{
			return d_show_constrained_triangulation;
		}

		void
		set_show_constrained_triangulation(
				bool b)
		{
			d_show_constrained_triangulation = b;
			emit_modified();
		}

		bool
		show_mesh_triangulation() const
		{
			return d_show_mesh_triangulation;
		}

		void
		set_show_mesh_triangulation(
				bool b)
		{
			d_show_mesh_triangulation = b;
			emit_modified();
		}

		bool
		show_segment_velocity() const
		{
			return d_show_segment_velocity;
		}

		void
		set_show_segment_velocity(
				bool b)
		{
			d_show_segment_velocity = b;
			emit_modified();
		}

		virtual
		void
		accept_visitor(
				ConstVisualLayerParamsVisitor &visitor) const
		{
			visitor.visit_topology_network_visual_layer_params(*this);
		}

		virtual
		void
		accept_visitor(
				VisualLayerParamsVisitor &visitor)
		{
			visitor.visit_topology_network_visual_layer_params(*this);
		}

	protected:

		explicit
		TopologyNetworkVisualLayerParams( 
				GPlatesAppLogic::LayerTaskParams &layer_task_params) :
			VisualLayerParams(
					layer_task_params,
					GPlatesGui::DrawStyleManager::instance()->default_style()),
			d_show_delaunay_triangulation(false),
			d_show_constrained_triangulation(false),
			d_show_mesh_triangulation(true),
			d_show_segment_velocity(false)
		{  }

	private:

		bool d_show_delaunay_triangulation;
		bool d_show_constrained_triangulation;
		bool d_show_mesh_triangulation;
		bool d_show_segment_velocity;
	};
}

#endif // GPLATES_PRESENTATION_TOPOLOGYNETWORKVISUALLAYERPARAMS_H
