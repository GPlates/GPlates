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

#include <functional>
#include <map>
#include <set>
#include <utility>
#include <vector>
#include <boost/bind/bind.hpp>
#include <boost/function.hpp>
#include <boost/optional.hpp>

#include "TopologyNetworkVisualLayerParams.h"
#include "VisualLayer.h"
#include "VisualLayerParamsVisitor.h"

#include "app-logic/ReconstructionGeometryVisitor.h"
#include "app-logic/ResolvedTopologicalNetwork.h"
#include "app-logic/ResolvedTriangulationDelaunay2.h"
#include "app-logic/ResolvedTriangulationNetwork.h"
#include "app-logic/ResolvedTriangulationUtils.h"
#include "app-logic/TopologyUtils.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "gui/Colour.h"
#include "gui/ColourProxy.h"
#include "gui/ColourPalette.h"
#include "gui/DrawStyleManager.h"
#include "gui/RasterColourPalette.h"
#include "gui/Symbol.h"

#include "maths/CubeQuadTreePartition.h"
#include "maths/CubeQuadTreePartitionUtils.h"
#include "maths/PointOnSphere.h"
#include "maths/Real.h"
#include "maths/Rotation.h"

#include "model/FeatureHandle.h"
#include "model/FeatureId.h"

#include "view-operations/RenderedColouredTriangleSurfaceMesh.h"
#include "view-operations/RenderedGeometry.h"
#include "view-operations/ScalarField3DRenderParameters.h"


namespace GPlatesGui
{
	class RenderSettings;
}

namespace GPlatesViewOperations
{
	class RenderedGeometryLayer;
	class RenderedGeometryParameters;
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
			explicit
			RenderParams(
					const GPlatesViewOperations::RenderedGeometryParameters &rendered_geometry_parameters_,
					bool fill_polygons_ = false,
					bool fill_polylines_ = false);

			float reconstruction_line_width_hint;
			float reconstruction_point_size_hint;
			float reconstruction_topology_size_multiplier;
			bool fill_polygons;
			bool fill_polylines;

			/**
			 * Modulate colour (eg, to control opacity/intensity).
			 *
			 * This is currently used for rasters, filled polygons/polylines and deforming network triangulations.
			 */
			GPlatesGui::Colour fill_modulate_colour;

			/**
			 * Used to control density of points/arrows in rendered geometry layer
			 *
			 * A value of zero means there is no limit on the density (all points/arrows are rendered).
			 */
			float ratio_zoom_dependent_bin_dimension_to_globe_radius;

			// Arrow body and head scaling.
			float ratio_arrow_unit_vector_direction_to_globe_radius;
			float ratio_arrowhead_size_to_globe_radius;

			//
			// Scalar field render parameters.
			//

			GPlatesViewOperations::ScalarField3DRenderParameters scalar_field_render_parameters;

			//
			// Scalar-coverage-specific parameters.
			//

			//! Scalar colour palette.
			GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type scalar_coverage_colour_palette;

			//
			// Raster-specific parameters.
			//

			//! Raster colour palette.
			GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type raster_colour_palette;

			/**
			 * Scales the heights used to calculate normals in a normal map raster.
			 *
			 * Note that the normal map raster can be from another layer, but this scale factor
			 * is specific to the current layer.
			 */
			float normal_map_height_field_scale_factor;

			//
			// VGP-specific parameters.
			//
			// Function returns bool to show VGP given the current time and VGP's optional age.
			boost::function<bool (double/*time*/, boost::optional<double>/*age*/)> show_vgp;
			bool vgp_draw_circular_error;

			//
			// Topology_reconstructed feature geometry settings.
			//
			bool show_topology_reconstructed_feature_geometries;
			bool show_strain_accumulation;
			double strain_accumulation_scale;

			//
			// Topological network settings.
			//
			bool fill_topological_network_rigid_blocks;
			bool show_topological_network_segment_velocity;
			TopologyNetworkVisualLayerParams::TriangulationColourMode topological_network_triangulation_colour_mode;
			TopologyNetworkVisualLayerParams::TriangulationDrawMode topological_network_triangulation_draw_mode;
			//! Delaunay triangulation colour palette.
			boost::optional<GPlatesGui::ColourPalette<double>::non_null_ptr_type> topological_network_triangulation_colour_palette;
		};


		/**
		 * Populates @a RenderParams from @a VisualLayerParams.
		 */
		class RenderParamsPopulator :
				public ConstVisualLayerParamsVisitor
		{
		public:

			explicit
			RenderParamsPopulator(
					const GPlatesViewOperations::RenderedGeometryParameters &rendered_geometry_parameters) :
				d_render_params(rendered_geometry_parameters)
			{  }

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
			visit_reconstruct_scalar_coverage_visual_layer_params(
					const ReconstructScalarCoverageVisualLayerParams &params);

			virtual
			void
			visit_reconstruct_visual_layer_params(
					const ReconstructVisualLayerParams &params);

			virtual
			void
			visit_scalar_field_3d_visual_layer_params(
					const ScalarField3DVisualLayerParams &params);

			void
			visit_topology_geometry_visual_layer_params(
					const TopologyGeometryVisualLayerParams &params);

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
		 * @a render_settings are show/hide settings that control geometry visibility.
		 *
		 * @a topological_sections are all topological sections referenced by loaded topologies.
		 * Together with @a render_settings this determines whether to show/hide topological sections.
		 *
		 * @a all_resolved_topological_shared_sub_segments are resolved topological shared sub-segments
		 * between ALL resolved topological boundaries and networks for the current reconstruction being rendered.
		 *
		 * @a reconstruction_adjustment is only used to rotate derived @a ReconstructionGeometry
		 * objects that are reconstructed. Ignored by types not explicitly reconstructed.
		 *
		 * It is the caller's responsibility to handle tasks such as clearing rendered geometries
		 * from the layer and activating the layer.
		 */
		ReconstructionGeometryRenderer(
				const RenderParams &render_params,
				const GPlatesGui::RenderSettings &render_settings,
				const std::set<GPlatesModel::FeatureId> &topological_sections,
				const GPlatesAppLogic::TopologyUtils::resolved_topological_boundaries_networks_to_shared_sub_segments_map_type &all_resolved_topological_shared_sub_segments,
				const boost::optional<GPlatesGui::Colour> &colour = boost::none,
				const boost::optional<GPlatesMaths::Rotation> &reconstruction_adjustment = boost::none,
				boost::optional<const GPlatesGui::symbol_map_type &> feature_type_symbol_map = boost::none,
				boost::optional<const GPlatesGui::StyleAdapter &> style_adaptor = boost::none);


		/**
		 * Begins rendering into the specified @a rendered_geometry_layer.
		 *
		 * This must be called before any calls to @a render.
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
		 * All calls to @a render should be done between a @a begin_render / @a end_render pair.
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
		 * Creates rendered geometry(s) from the specified reconstruction geometry (a derived type).
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
				const GPlatesUtils::non_null_intrusive_ptr<ReconstructionGeometryDerivedType> &reconstruction_geometry,
				boost::optional<const GPlatesMaths::CubeQuadTreeLocation &> spatial_partition_location = boost::none);

	private:

		// Bring base class visit methods into scope of current class.
		using GPlatesAppLogic::ConstReconstructionGeometryVisitor::visit;

		//
		// The following methods are for visiting derived @a ReconstructionGeometry objects.
		//

		virtual
		void
		visit(
				const GPlatesUtils::non_null_intrusive_ptr<co_registration_data_type> &crr);

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
				const GPlatesUtils::non_null_intrusive_ptr<reconstructed_scalar_coverage_type> &rsc);

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
				const GPlatesUtils::non_null_intrusive_ptr<topology_reconstructed_feature_geometry_type> &trfg);

	private:

		/**
		 * The default depth of the rendered geometries spatial partition (the quad trees in each cube face).
		 */
		static const unsigned int DEFAULT_SPATIAL_PARTITION_DEPTH = 7;


		RenderParams d_render_params;
		const GPlatesGui::RenderSettings &d_render_settings;
		const std::set<GPlatesModel::FeatureId> &d_topological_sections;
		const GPlatesAppLogic::TopologyUtils::resolved_topological_boundaries_networks_to_shared_sub_segments_map_type &d_all_resolved_topological_shared_sub_segments;
		boost::optional<GPlatesGui::Colour> d_colour;
		boost::optional<GPlatesMaths::Rotation> d_reconstruction_adjustment;
		boost::optional<const GPlatesGui::symbol_map_type &> d_feature_type_symbol_map;
		boost::optional<const GPlatesGui::StyleAdapter &> d_style_adapter;

		/**
		 * The rendered geometry layer for all rendering between @a begin_render and @a end_render.
		 */
		boost::optional<GPlatesViewOperations::RenderedGeometryLayer &> d_rendered_geometry_layer;

		/**
		 * Location in the rendered geometries spatial partition to add rendered geometries to.
		 *
		 * It is only valid during @a render.
		 */
		boost::optional<const rendered_geometries_spatial_partition_type::location_type &> d_rendered_geometries_spatial_partition_location;

		/**
		 * Mapping from shared boundary sub-segments to their sharing resolved topological boundaries and networks
		 * (rendered in the current rendered geometry layer).
		 *
		 * This is used to render all shared sub-segments at the end of a rendered geometry layer.
		 */
		std::map<
				GPlatesAppLogic::ResolvedTopologicalSharedSubSegment::non_null_ptr_type,
				std::vector<GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type>> d_resolved_topological_shared_sub_segments_map;


		/**
		 * Adds a rendered geometry that does not correspond to a @a GeometryOnSphere from
		 * a reconstruction geometry.
		 */
		void
		render(
				const GPlatesViewOperations::RenderedGeometry &rendered_geometry)
		{
			// Ignore the location in the rendered geometry layer's spatial partition and
			// just add to the root of the spatial partition.
			d_rendered_geometry_layer->add_rendered_geometry(rendered_geometry);
		}


		/**
		 * Adds the specified rendered geometry that corresponds to the @a GeometryOnSphere
		 * in the @a ReconstructionGeometry being visited - for example, for RFGs this is
		 * the geometry returned by 'ReconstructedFeatureGeometry::reconstructed_geometry()'.
		 *
		 * Internally if a destination location in the rendered geometries spatial partition has
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
			// If a spatial partition location has been specified then use it.
			if (d_rendered_geometries_spatial_partition_location)
			{
				d_rendered_geometry_layer->add_rendered_geometry(
						rendered_geometry,
						d_rendered_geometries_spatial_partition_location.get());
			}
			else
			{
				d_rendered_geometry_layer->add_rendered_geometry(rendered_geometry);
			}
		}


		typedef std::pair<
				GPlatesMaths::PointOnSphere, 
				GPlatesAppLogic::ResolvedTriangulation::Delaunay_2::Face_handle>
						smoothed_vertex_type;

		/**
		 * Enables @a smoothed_vertex_type to be used as a key in a 'std::map'.
		 */
		class SmoothedVertexMapPredicate
		{
		public:
			bool
			operator()(
					const smoothed_vertex_type &lhs,
					const smoothed_vertex_type &rhs) const
			{
				return d_point_on_sphere_predicate(lhs.first, rhs.first);
			}

		private:
			GPlatesMaths::PointOnSphereMapPredicate d_point_on_sphere_predicate;
		};

		typedef GPlatesAppLogic::ResolvedTriangulation::VertexIndices<
				smoothed_vertex_type,
				SmoothedVertexMapPredicate>
						smoothed_vertex_indices_type;

		typedef GPlatesAppLogic::ResolvedTriangulation::VertexIndices<
				GPlatesAppLogic::ResolvedTriangulation::Delaunay_2::Vertex_handle>
						unsmoothed_vertex_indices_type;


		void
		render_topological_network_delaunay_face_smoothed_strain_rate(
				const GPlatesMaths::PointOnSphere &point1,
				const GPlatesMaths::PointOnSphere &point2,
				const GPlatesMaths::PointOnSphere &point3,
				smoothed_vertex_indices_type &vertex_indices,
				GPlatesViewOperations::RenderedColouredTriangleSurfaceMesh::triangle_seq_type &rendered_triangle_mesh_triangles,
				const GPlatesAppLogic::ResolvedTriangulation::Network &resolved_triangulation_network,
				GPlatesAppLogic::ResolvedTriangulation::Delaunay_2::Face_handle delaunay_face,
				const double &subdivide_face_threshold);

		void
		render_topological_network_delaunay_faces_smoothed_strain_rates(
				const GPlatesAppLogic::ResolvedTopologicalNetwork::non_null_ptr_to_const_type &rtn,
				const double &subdivide_face_threshold);

		void
		render_topological_network_delaunay_faces_unsmoothed_strain_rates(
				const GPlatesAppLogic::ResolvedTopologicalNetwork::non_null_ptr_to_const_type &rtn);

		void
		render_topological_network_delaunay_edges_smoothed_strain_rates(
				const GPlatesAppLogic::ResolvedTopologicalNetwork::non_null_ptr_to_const_type &rtn,
				const double &subdivide_edge_threshold_angle);

		void
		render_topological_network_delaunay_edges_unsmoothed_strain_rates(
				const GPlatesAppLogic::ResolvedTopologicalNetwork::non_null_ptr_to_const_type &rtn);

		void
		render_topological_network_delaunay_edges_using_draw_style(
				const GPlatesAppLogic::ResolvedTopologicalNetwork::non_null_ptr_to_const_type &rtn);

		void
		render_topological_network_fill_using_draw_style(
				const GPlatesAppLogic::ResolvedTopologicalNetwork::non_null_ptr_to_const_type &rtn);

		void
		render_topological_network_rigid_blocks(
				const GPlatesAppLogic::ResolvedTopologicalNetwork::non_null_ptr_to_const_type &rtn);

		/**
		 * Get the reconstruction geometries that are resolved topological networks and
		 * draw the velocities at the network points if there are any.
		 */
		void
		render_topological_network_velocities(
				const GPlatesAppLogic::ResolvedTopologicalNetwork::non_null_ptr_to_const_type &topological_network);


		/**
		 * Add the shared sub-segments of a resolved topological boundary or network to be rendered later
		 * (at the end of the current rendered geometry layer).
		 */
		void
		add_topological_shared_sub_segments(
				const GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type &resolved_topology,
				const GPlatesModel::FeatureHandle::iterator &resolved_topology_feature_property);

		/**
		 * Render shared sub-segments of resolved topological boundaries and networks
		 * (rendered in the current rendered geometry layer).
		 */
		void
		render_topological_shared_sub_segments();

		enum class SubductionPolarity { LEFT, RIGHT };

		/**
		 * Returns the subduction polarity if the specified reconstruction geometry represents a subduction zone.
		 */
		boost::optional<SubductionPolarity>
		get_subduction_polarity(
				const GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type &resolved_topological_section) const;
	};


	//
	// Implementation
	//


	template <class ReconstructionGeometryDerivedType>
	void
	ReconstructionGeometryRenderer::render(
			const GPlatesUtils::non_null_intrusive_ptr<ReconstructionGeometryDerivedType> &reconstruction_geometry,
			boost::optional<const GPlatesMaths::CubeQuadTreeLocation &> spatial_partition_location)
	{
		// Must be between 'begin_render' and 'end_render'.
		GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
				d_rendered_geometry_layer,
				GPLATES_ASSERTION_SOURCE);

		d_rendered_geometries_spatial_partition_location = spatial_partition_location;

		// NOTE: We need full visitor dispatch because some derived types derive from
		// other derived types (eg, flowlines derive from RFG).
		reconstruction_geometry->accept_visitor(*this);

		d_rendered_geometries_spatial_partition_location = boost::none;
	}
}

#endif // GPLATES_PRESENTATION_RECONSTRUCTION_GEOMETRY_RENDERER_H
