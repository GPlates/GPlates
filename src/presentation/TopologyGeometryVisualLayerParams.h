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

#ifndef GPLATES_PRESENTATION_TOPOLOGYGEOMETRYVISUALLAYERPARAMS_H
#define GPLATES_PRESENTATION_TOPOLOGYGEOMETRYVISUALLAYERPARAMS_H

#include "VisualLayerParams.h"

#include "gui/Colour.h"
#include "gui/DrawStyleManager.h"


namespace GPlatesPresentation
{
	class TopologyGeometryVisualLayerParams :
			public VisualLayerParams
	{
	public:
		typedef GPlatesUtils::non_null_intrusive_ptr<TopologyGeometryVisualLayerParams> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const TopologyGeometryVisualLayerParams> non_null_ptr_to_const_type;

		static
		non_null_ptr_type
		create(
				GPlatesAppLogic::LayerParams::non_null_ptr_type layer_params)
		{
			return new TopologyGeometryVisualLayerParams( layer_params );
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

		/**
		 * Sets the opacity of filled primitives.
		 */
		void
		set_fill_opacity(
				const double &opacity)
		{
			d_fill_opacity = opacity;
			emit_modified();
		}

		/**
		 * Gets the opacity of filled primitives.
		 */
		double
		get_fill_opacity() const
		{
			return d_fill_opacity;
		}

		/**
		 * Sets the intensity of filled primitives.
		 */
		void
		set_fill_intensity(
				const double &intensity)
		{
			d_fill_intensity = intensity;
			emit_modified();
		}

		/**
		 * Gets the intensity of filled primitives.
		 */
		double
		get_fill_intensity() const
		{
			return d_fill_intensity;
		}

		/**
		 * Returns the filled primitives modulate colour.
		 *
		 * This is a combination of the opacity and intensity as (I, I, I, O) where
		 * 'I' is intensity and 'O' is opacity.
		 */
		GPlatesGui::Colour
		get_fill_modulate_colour() const
		{
			return GPlatesGui::Colour(d_fill_intensity, d_fill_intensity, d_fill_intensity, d_fill_opacity);
		}

		virtual
		void
		accept_visitor(
				ConstVisualLayerParamsVisitor &visitor) const
		{
			visitor.visit_topology_geometry_visual_layer_params(*this);
		}

		virtual
		void
		accept_visitor(
				VisualLayerParamsVisitor &visitor)
		{
			visitor.visit_topology_geometry_visual_layer_params(*this);
		}

	protected:
		explicit
		TopologyGeometryVisualLayerParams( 
				GPlatesAppLogic::LayerParams::non_null_ptr_type layer_params) :
			VisualLayerParams(
					layer_params,
					GPlatesGui::DrawStyleManager::instance()->default_style()),
			d_fill_polygons(false),
			d_fill_opacity(1.0),
			d_fill_intensity(1.0)
		{  }

	private:

		bool d_fill_polygons;

		//! The opacity of filled primitives in the range [0,1].
		double d_fill_opacity;
		//! The intensity of filled primitives in the range [0,1].
		double d_fill_intensity;
	};
}

#endif // GPLATES_PRESENTATION_TOPOLOGYGEOMETRYVISUALLAYERPARAMS_H
