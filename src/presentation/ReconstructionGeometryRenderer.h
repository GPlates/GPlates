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

#include <vector>
#include <boost/bind.hpp>
#include <boost/optional.hpp>

#include "VisualLayerParamsVisitor.h"

#include "app-logic/AppLogicFwd.h"
#include "app-logic/ReconstructionGeometryVisitor.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "gui/Colour.h"
#include "gui/ColourPalette.h"
#include "gui/DrawStyleManager.h"
#include "gui/Symbol.h"
#include "gui/RasterColourPalette.h"

#include "maths/CubeQuadTreePartition.h"
#include "maths/CubeQuadTreePartitionUtils.h"
#include "maths/Rotation.h"
#include "view-operations/RenderedGeometry.h"
#include "presentation/VisualLayer.h"

#include "view-operations/RenderedGeometryParameters.h"
#include "view-operations/ScalarField3DRenderParameters.h"


namespace GPlatesViewOperations
{
	class RenderedGeometryLayer;
}

namespace GPlatesPresentation
{
	/**
	 * Visits classes derived from @a ReconstructionGeometry and
	 * renders them by creating @a RenderedGeometry objects.
	 */
	class ReconstructionGeometryRenderer :
			public GPlatesAppLogic::ConstReconstructionGeometryVisitor
	{
	public:
		/**
		 * Various parameters that control rendering.
		 */
		struct RenderParams
		{
			RenderParams(
					float reconstruction_line_width_hint_ =
							GPlatesViewOperations::RenderedLayerParameters::RECONSTRUCTION_LINE_WIDTH_HINT,
					float reconstruction_point_size_hint_ =
							GPlatesViewOperations::RenderedLayerParameters::RECONSTRUCTION_POINT_SIZE_HINT,
					bool fill_polygons_ = false,
					float ratio_zoom_dependent_bin_dimension_to_globe_radius_ = 0,
					// FIXME: Move this hard-coded value somewhere sensible...
					float velocity_ratio_unit_vector_direction_to_globe_radius_ = 0.05f,

					bool show_topological_network_mesh_triangulation_ 			= true,
					bool show_topological_network_constrained_triangulation_ 	= false,
					bool show_topological_network_delaunay_triangulation_ 		= false,
					bool show_topological_network_segment_velocity_				= true,
					bool show_velocity_field_delaunay_vectors_					= true,
					bool show_velocity_field_constrained_vectors_ 				= false
			);

			float reconstruction_line_width_hint;
			float reconstruction_point_size_hint;
			bool fill_polygons;

			/**
			 * Used to control density of points/arrows in rendered geometry layer
			 *
			 * A value of zero means there is no limit on the density (all points/arrows are rendered).
			 */
			float ratio_zoom_dependent_bin_dimension_to_globe_radius;

			float velocity_ratio_unit_vector_direction_to_globe_radius;

			// Scalar field render parameters.
			GPlatesViewOperations::ScalarField3DRenderParameters scalar_field_render_parameters;

			//
			// Raster-specific parameters.
			//
			//! Raster colour palette.
			GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type raster_colour_palette;
			/**
			 * Raster modulate colour (eg, to control raster opacity/intensity).
			 *
			 * TODO: This could be used for general layer transparency.
			 */
			GPlatesGui::Colour raster_modulate_colour;

			// VGP-specific parameters.
			bool vgp_draw_circular_error;

			// Topological network settings:
			bool show_topological_network_mesh_triangulation;
			bool show_topological_network_delaunay_triangulation;
			bool show_topological_network_constrained_triangulation;
			bool show_topological_network_segment_velocity;

			// VelocityFieldCalculator settings
			bool show_velocity_field_delaunay_vectors;
			bool show_velocity_field_constrained_vectors;
		};


		/**
		 * Populates @a RenderParams from @a VisualLayerParams.
		 */
		class RenderParamsPopulator :
				public ConstVisualLayerParamsVisitor
		{
		public:

			const RenderParams &
			get_render_params() const
			{
				return d_render_params;
			}

			virtual
			void
			visit_raster_visual_layer_params(
					const RasterVisualLayerParams &params);

			virtual
			void
			visit_reconstruct_visual_layer_params(
					const ReconstructVisualLayerParams &params);

			virtual
			void
			visit_scalar_field_3d_visual_layer_params(
					const ScalarField3DVisualLayerParams &params);

			void
			visit_topology_boundary_visual_layer_params(
					topology_boundary_visual_layer_params_type &params);

			virtual
			void
			visit_topology_network_visual_layer_params(
					const TopologyNetworkVisualLayerParams &params);

			virtual
			void
			visit_velocity_field_calculator_visual_layer_params(
					const VelocityFieldCalculatorVisualLayerParams &params);

		private:

			RenderParams d_render_params;
		};

		//! Typedef for a spatial partition of rendered geometries.
		typedef GPlatesMaths::CubeQuadTreePartition<GPlatesViewOperations::RenderedGeometry>
				rendered_geometries_spatial_partition_type;


		/**
		 * Created @a RenderedGeometry objects are added to the spatial partition of
		 * rendered geometries @a rendered_geometry_spatial_partition.
		 *
		 * The colour of visited @a ReconstructionGeometry objects is determined at a later
		 * time via class @a ColourProxy unless @a colour is specified in which case all
		 * reconstruction geometries are drawn with that colour.
		 *
		 * @a render_params controls various rendering options.
		 *
		 * @a reconstruction_adjustment is only used to rotate derived @a ReconstructionGeometry
		 * objects that are reconstructed. Ignored by types not explicitly reconstructed.
		 *
		 * It is the caller's responsibility to handle tasks such as clearing rendered geometries
		 * from the layer and activating the layer.
		 */
		ReconstructionGeometryRenderer(
				const RenderParams &render_params = RenderParams(),
				const boost::optional<GPlatesGui::Colour> &colour = boost::none,
				const boost::optional<GPlatesMaths::Rotation> &reconstruction_adjustment = boost::none,
				const boost::optional<GPlatesGui::symbol_map_type> &feature_type_symbol_map = boost::none,
				const GPlatesGui::StyleAdapter* sa = NULL);


		/**
		 * Begins rendering into the specified @a rendered_geometry_layer.
		 *
		 * This must be called before any rendering including visiting any reconstruction geometries.
		 *
		 * Internally creates a rendered geometries spatial partition that all rendered
		 * geometries are added to.
		 *
		 * NOTE: @a begin_render and @a end_render calls *cannot* be nested.
		 *
		 * NOTE: It is the caller's responsility to clear the rendered geometries where they see fit.
		 * Rendered geometries added between a @a begin_render and @a end_render pair will
		 * simply accumulate rendered geometries in addition to whatever is already in the
		 * rendered geometry layer.
		 *
		 * @throws PreconditionViolationError if does not match a @a end_render call.
		 */
		void
		begin_render(
				GPlatesViewOperations::RenderedGeometryLayer &rendered_geometry_layer);


		/**
		 * Renders all created rendered geometries since the last call to @a begin_render
		 * into the rendered geometry layer specified in @a begin_render.
		 *
		 * Rendering, including visiting any reconstruction geometries, should be done
		 * between a @a begin_render / @a end_render pair.
		 *
		 * Internally transfers the rendered geometries spatial partition to the
		 * rendered geometries layer specified in @a begin_render.
		 *
		 * NOTE: Multiple @a begin_render / @a end_render pairs to the *same*
		 * rendered geometry layer will accumulate rendered geometries as expected.
		 *
		 * NOTE: @a begin_render and @a end_render calls *cannot* be nested.
		 *
		 * @throws PreconditionViolationError if does not match a @a begin_render call.
		 */
		void
		end_render();


		/**
		 * Creates rendered geometries from reconstruction geometries (a derived type) and
		 * adds them to an internal spatial partition of rendered geometries.
		 *
		 * NOTE: Only those rendered geometries that represent the *geometry* of the reconstruction
		 * geometry are added to the quad trees of the spatial partition (of the rendered geometry layer).
		 * Other rendered geometries are added to the root of the spatial partition.
		 * The quad trees are where the efficiency of the spatial partition comes into play -
		 * adding to the root is like adding to a linear sequence.
		 * For example, velocity arrows are not added to the quad trees since their
		 * bounded size is not known (from the input spatial partition) - however the points
		 * at the base of the arrows are added to the quad trees.
		 *
		 * @throws PreconditionViolationError if called outside a @a begin_render / @a end_render pair.
		 */
		template <class ReconstructionGeometryDerivedType>
		void
		render(
				const GPlatesMaths::CubeQuadTreePartition<
						GPlatesUtils::non_null_intrusive_ptr<ReconstructionGeometryDerivedType>
								> &reconstruction_geometries_spatial_partition);

		//
		// The following methods are for visiting derived @a ReconstructionGeometry objects.
		//

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
				const GPlatesUtils::non_null_intrusive_ptr<reconstructed_motion_path_type> &rmp);


        virtual
        void
        visit(
				const GPlatesUtils::non_null_intrusive_ptr<reconstructed_small_circle_type> &rsc);

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
				const GPlatesUtils::non_null_intrusive_ptr<resolved_scalar_field_3d_type> &rsf);

		virtual
		void
		visit(
				const GPlatesUtils::non_null_intrusive_ptr<resolved_topological_geometry_type> &rtg);

		virtual
		void
		visit(
				const GPlatesUtils::non_null_intrusive_ptr<resolved_topological_network_type> &rtn);

		virtual
		void
		visit(
				const GPlatesUtils::non_null_intrusive_ptr<co_registration_data_type> &crr);

	private:
		/**
		 * The default depth of the rendered geometries spatial partition (the quad trees in each cube face).
		 *
		 * NOTE: This is actually unnecessary because we always build the spatial partition by
		 * mirroring a reconstruction geometries spatial partition (or we add to the root of
		 * the spatial partition).
		 * However we'll keep a default reasonable depth just in case this changes - for now
		 * it won't have any effect on the speed, memory usage or functioning of the rendered
		 * geometries spatial partition.
		 */
		static const unsigned int DEFAULT_SPATIAL_PARTITION_DEPTH = 7;


		RenderParams d_render_params;
		boost::optional<GPlatesGui::Colour> d_colour;
		boost::optional<GPlatesMaths::Rotation> d_reconstruction_adjustment;
		boost::optional<GPlatesGui::symbol_map_type> d_feature_type_symbol_map;
		const GPlatesGui::StyleAdapter* d_style_adapter;

		/**
		 * The rendered geometry layer for all rendering between @a begin_render and @a end_render.
		 */
		boost::optional<GPlatesViewOperations::RenderedGeometryLayer &> d_rendered_geometry_layer;

		/**
		 * All rendered geometries are added to this spatial partition.
		 *
		 * Those rendered geometries that have no spatial information are simply added to the root
		 * of the spatial partition (which effectively treats them like a linear sequence).
		 *
		 * This spatial partition is only initialised inside a @a begin_render / @a end_render pair.
		 */
		boost::optional<rendered_geometries_spatial_partition_type::non_null_ptr_type>
				d_rendered_geometries_spatial_partition;

		/**
		 * Optional destination in the rendered geometries spatial partition to
		 * add rendered geometries to.
		 *
		 * If it's boost::none then rendered geometries are added to the root of the spatial partition.
		 * This is the default when visiting reconstruction geometries that are not in
		 * a reconstruction geometries spatial partition.
		 */
		boost::optional<rendered_geometries_spatial_partition_type::node_reference_type>
				d_rendered_geometries_spatial_partition_node;


		/**
		 * Converts a @a ReconstructionGeometry object, from a spatial partition, into
		 * rendered geometries and adds them to the root of the rendered geometries spatial partition.
		 *
		 * This is the equivalent of just visiting a reconstruction geometry directly
		 * (ie, where the reconstruction geometry is not in a spatial partition).
		 */
		template <class ReconstructionGeometryDerivedType>
		void
		render_to_spatial_partition_root(
				const GPlatesUtils::non_null_intrusive_ptr<ReconstructionGeometryDerivedType> &reconstruction_geometry)
		{
			// NOTE: We need full visitor dispatch because some derived types derive from
			// other derived types (eg, flowlines derive from RFG).
			reconstruction_geometry->accept_visitor(*this);
		}


		/**
		 * Convert a @a ReconstructionGeometry object, from a spatial partition, into
		 * rendered geometries and adds them to the rendered geometries spatial partition.
		 */
		template <class ReconstructionGeometryDerivedType>
		void
		render_to_spatial_partition_quad_tree_node(
				const GPlatesUtils::non_null_intrusive_ptr<ReconstructionGeometryDerivedType> &reconstruction_geometry,
				rendered_geometries_spatial_partition_type::node_reference_type
						rendered_geometries_spatial_partition_node)
		{
			// Specify the destination in the rendered geometries spatial partition before visiting.
			d_rendered_geometries_spatial_partition_node = rendered_geometries_spatial_partition_node;

			// NOTE: We need full visitor dispatch because some derived types derive from
			// other derived types (eg, flowlines derive from RFG).
			reconstruction_geometry->accept_visitor(*this);

			d_rendered_geometries_spatial_partition_node = boost::none;
		}


		/**
		 * Adds a rendered geometry that does not correspond to a @a GeometryOnSphere from
		 * a reconstruction geometry.
		 */
		void
		render(
				const GPlatesViewOperations::RenderedGeometry &rendered_geometry)
		{
			// Ignore the destination node in the spatial partition and
			// just add to the root of the spatial partition.
			d_rendered_geometries_spatial_partition.get()->add_unpartitioned(rendered_geometry);
		}


		/**
		 * Adds the specified rendered geometry that corresponds to the @a GeometryOnSphere
		 * in the @a ReconstructionGeometry being visited - for example, for RFGs this is
		 * the geometry returned by 'ReconstructedFeatureGeometry::reconstructed_geometry()'.
		 *
		 * Internally if a destination node in the rendered geometries spatial partition has
		 * been set up then the rendered geometry is added to that, otherwise it is added
		 * to the root of the spatial partition.
		 *
		 * NOTE: Only call this for rendered geometries that represent the @a GeometryOnSphere
		 * inside a *reconstruction* geometry, otherwise use @a render.
		 */
		void
		render_reconstruction_geometry_on_sphere(
				const GPlatesViewOperations::RenderedGeometry &rendered_geometry)
		{
			// If there's a destination node in the spatial partition then add to that.
			if (d_rendered_geometries_spatial_partition_node)
			{
				d_rendered_geometries_spatial_partition.get()->add(
						rendered_geometry,
						d_rendered_geometries_spatial_partition_node.get());
			}
			else // otherwise just add to the root of the spatial partition...
			{
				d_rendered_geometries_spatial_partition.get()->add_unpartitioned(rendered_geometry);
			}
		}


		/**
		 * Get the reconstruction geometries that are resolved topological networks and
		 * draw the velocities at the network points if there are any.
		 */
		void
		render_topological_network_velocities(
				const GPlatesAppLogic::resolved_topological_network_non_null_ptr_to_const_type &topological_network,
				const ReconstructionGeometryRenderer::RenderParams &render_params,
				const boost::optional<GPlatesGui::Colour> &colour);
	};


	//
	// Implementation
	//


	template <class ReconstructionGeometryDerivedType>
	void
	ReconstructionGeometryRenderer::render(
			const GPlatesMaths::CubeQuadTreePartition<
					GPlatesUtils::non_null_intrusive_ptr<ReconstructionGeometryDerivedType>
							> &reconstruction_geometries_spatial_partition)
	{
		// Must be between 'begin_render' and 'end_render'.
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				d_rendered_geometries_spatial_partition,
				GPLATES_ASSERTION_SOURCE);

		// For each reconstruction geometry in the spatial partition generate a rendered geometry
		// in the rendered geometries spatial partition using methods
		// 'render_to_spatial_partition_root' and 'render_to_spatial_partition_quad_tree_node'
		// to do the transformations.
		GPlatesMaths::CubeQuadTreePartitionUtils::mirror(
				*d_rendered_geometries_spatial_partition.get(),
				reconstruction_geometries_spatial_partition,
				boost::bind(
						&ReconstructionGeometryRenderer::render_to_spatial_partition_root<ReconstructionGeometryDerivedType>,
						this,
						_2),
				boost::bind(
						&ReconstructionGeometryRenderer::render_to_spatial_partition_quad_tree_node<ReconstructionGeometryDerivedType>,
						this,
						_3, _2));
	}
}

#endif // GPLATES_PRESENTATION_RECONSTRUCTION_GEOMETRY_RENDERER_H
