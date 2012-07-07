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
 
#ifndef GPLATES_PRESENTATION_RECONSTRUCTVISUALLAYERPARAMS_H
#define GPLATES_PRESENTATION_RECONSTRUCTVISUALLAYERPARAMS_H

#include <boost/shared_ptr.hpp>

#include "VisualLayerParams.h"


namespace GPlatesPresentation
{
	class ReconstructVisualLayerParams :
			public VisualLayerParams
	{
	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<ReconstructVisualLayerParams> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const ReconstructVisualLayerParams> non_null_ptr_to_const_type;

		static
		non_null_ptr_type
		create(
				GPlatesAppLogic::LayerTaskParams &layer_task_params)
		{
			return new ReconstructVisualLayerParams(layer_task_params);
		}

		bool
		get_vgp_draw_circular_error() const;

		void
		set_vgp_draw_circular_error(
				bool draw);

		void
		set_fill_polygons(
				bool fill);

		bool
		get_fill_polygons() const;

		/**
		 * Override of virtual method in VirtualLayerParams base.
		 */
		virtual
		void
		accept_visitor(
				ConstVisualLayerParamsVisitor &visitor) const
		{
			visitor.visit_reconstruct_visual_layer_params(*this);
		}

		/**
		 * Override of virtual method in VirtualLayerParams base.
		 */
		virtual
		void
		accept_visitor(
				VisualLayerParamsVisitor &visitor)
		{
			visitor.visit_reconstruct_visual_layer_params(*this);
		}

	protected:

		explicit
		ReconstructVisualLayerParams(
				GPlatesAppLogic::LayerTaskParams &layer_task_params);

	private:

		bool d_vgp_draw_circular_error;
		bool d_fill_polygons;
	};
}

#endif // GPLATES_PRESENTATION_RASTERVISUALLAYERPARAMS_H
