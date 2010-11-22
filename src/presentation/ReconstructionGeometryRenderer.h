/* $Id$ */


/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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
 
#ifndef GPLATES_PRESENTATION_RECONSTRUCTION_GEOMETRY_RENDERER_H
#define GPLATES_PRESENTATION_RECONSTRUCTION_GEOMETRY_RENDERER_H

#include <boost/optional.hpp>

#include "app-logic/ReconstructionGeometryVisitor.h"
#include "gui/Colour.h"
#include "maths/Rotation.h"
#include "view-operations/RenderedGeometryParameters.h"


namespace GPlatesViewOperations
{
	class RenderedGeometryLayer;
}

namespace GPlatesPresentation
{
	using namespace GPlatesViewOperations::RenderedLayerParameters;

	/**
	 * Visits classes derived from @a ReconstructionGeometry and
	 * renders them by creating @a RenderedGeometry objects.
	 */
	class ReconstructionGeometryRenderer :
			public GPlatesAppLogic::ConstReconstructionGeometryVisitor
	{
	public:
		/**
		 * Various parameters that control rendering style.
		 */
		struct StyleParams
		{
			StyleParams(
					float reconstruction_line_width_hint_ = RECONSTRUCTION_LINE_WIDTH_HINT,
					float reconstruction_point_size_hint_ = RECONSTRUCTION_POINT_SIZE_HINT,
					// FIXME: Move this hard-coded value somewhere sensible...
					float velocity_ratio_unit_vector_direction_to_globe_radius_ = 0.05f);

			float reconstruction_line_width_hint;
			float reconstruction_point_size_hint;
			float velocity_ratio_unit_vector_direction_to_globe_radius;
		};


		/**
		 * Created @a RenderedGeometry objects are added to layer @a rendered_geometry_layer.
		 *
		 * The colour of visited @a ReconstructionGeometry objects is determined at a later
		 * time via class @a ColourProxy unless @a colour is specified in which case all
		 * reconstruction geometries are drawn with that colour.
		 *
		 * @a style_params controls various rendering options.
		 *
		 * @a reconstruction_adjustment is only used to rotate derived @a ReconstructionGeometry
		 * objects that are reconstructed. Ignored by types not explicitly reconstructed.
		 *
		 * It is the caller's responsibility to handle tasks such as clearing rendered geometries
		 * from the layer and activating the layer.
		 */
		ReconstructionGeometryRenderer(
				GPlatesViewOperations::RenderedGeometryLayer &rendered_geometry_layer,
				const StyleParams &style_params = StyleParams(),
				const boost::optional<GPlatesGui::Colour> &colour = boost::none,
				const boost::optional<GPlatesMaths::Rotation> &reconstruction_adjustment = boost::none);


		virtual
		void
		visit(
				const GPlatesUtils::non_null_intrusive_ptr<multi_point_vector_field_type> &mpvf);

		virtual
		void
		visit(
				const GPlatesUtils::non_null_intrusive_ptr<reconstructed_feature_geometry_type> &rfg);

		virtual
		void
		visit(
				const GPlatesUtils::non_null_intrusive_ptr<reconstructed_flowline_type> &rf);

		virtual
		void
		visit(
				const GPlatesUtils::non_null_intrusive_ptr<reconstructed_motion_track_type> &rmt);

		virtual
		void
		visit(
				const GPlatesUtils::non_null_intrusive_ptr<reconstructed_virtual_geomagnetic_pole_type> &rvgp);

		virtual
		void
		visit(
				const GPlatesUtils::non_null_intrusive_ptr<resolved_raster_type> &rr);

		virtual
		void
		visit(
				const GPlatesUtils::non_null_intrusive_ptr<age_grid_raster_type> &agr);

		virtual
		void
		visit(
				const GPlatesUtils::non_null_intrusive_ptr<resolved_topological_boundary_type> &rtb);

		virtual
		void
		visit(
				const GPlatesUtils::non_null_intrusive_ptr<resolved_topological_network_type> &rtn);

		virtual
		void
		visit(
				const GPlatesUtils::non_null_intrusive_ptr<co_registration_data_type> &crr);

	private:
		GPlatesViewOperations::RenderedGeometryLayer &d_rendered_geometry_layer;
		StyleParams d_style_params;
		boost::optional<GPlatesGui::Colour> d_colour;
		boost::optional<GPlatesMaths::Rotation> d_reconstruction_adjustment;
	};
}

#endif // GPLATES_PRESENTATION_RECONSTRUCTION_GEOMETRY_RENDERER_H
