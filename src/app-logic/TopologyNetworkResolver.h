
/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date: 2008-08-15 02:13:48 -0700 (Fri, 15 Aug 2008) $
 * 
 * Copyright (C) 2008, 2010 The University of Sydney, Australia
 * Copyright (C) 2008, 2009, 2013 California Institute of Technology 
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

#ifndef GPLATES_APP_LOGIC_TOPOLOGY_NETWORK_RESOLVER_H
#define GPLATES_APP_LOGIC_TOPOLOGY_NETWORK_RESOLVER_H

#include <cstddef> // For std::size_t
#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <boost/random.hpp>
#include <boost/generator_iterator.hpp>

#include "AppLogicFwd.h"
#include "ReconstructHandle.h"
#include "ReconstructionFeatureProperties.h"
#include "ReconstructionGeometry.h"
#include "ResolvedTriangulationNetwork.h"
#include "TopologyIntersections.h"

#include "maths/GeometryOnSphere.h"

#include "model/types.h"
#include "model/FeatureId.h"
#include "model/FeatureHandle.h"
#include "model/FeatureVisitor.h"
#include "model/FeatureCollectionHandle.h"
#include "model/Model.h"

#include "property-values/GpmlTopologicalNetwork.h"
#include "property-values/GpmlTopologicalSection.h"


namespace GPlatesPropertyValues
{
	class GpmlPropertyDelegate;
	class GpmlTimeWindow;
}

namespace GPlatesAppLogic
{
	/**
	 * Finds all topological network features (in the features visited)
	 * that exist at a particular reconstruction time and creates
	 * @a ResolvedTopologicalNetwork objects for each one.
	 */
	class TopologyNetworkResolver: 
		public GPlatesModel::FeatureVisitor,
		private boost::noncopyable
	{

	public:
		/**
		 * The resolved networks are appended to @a resolved_topological_networks.
		 *
		 * @param reconstruct_handle is placed in all resolved topological networks as a reconstruction identifier.
		 * @param reconstruction_tree is associated with the output resolved topological networks.
		 * @param topological_geometry_reconstruct_handles is a list of reconstruct handles that
		 *        identifies the subset, of all RFGs observing the topological boundary section and/or
		 *        interior features, and all resolved topological lines (@a ResolvedTopologicalGeometry
		 *        containing polylines) observing the topological boundary section features,
		 *        that should be searched when resolving the topological networks.
		 *        This is useful to avoid outdated RFGs and RTGS still in existence (among other scenarios).
		 */
		TopologyNetworkResolver(
				std::vector<resolved_topological_network_non_null_ptr_type> &resolved_topological_networks,
				const double &reconstruction_time,
				ReconstructHandle::type reconstruct_handle,
				boost::optional<const std::vector<ReconstructHandle::type> &> topological_geometry_reconstruct_handles);

		virtual
		~TopologyNetworkResolver() 
		{  }

		virtual
		bool
		initialise_pre_feature_properties(
				GPlatesModel::FeatureHandle &feature_handle);

		virtual
		void
		visit_gpml_constant_value(
				GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value);

		virtual
		void
		visit_gpml_piecewise_aggregation(
				GPlatesPropertyValues::GpmlPiecewiseAggregation &gpml_piecewise_aggregation);

		void
		visit_gpml_time_window(
				GPlatesPropertyValues::GpmlTimeWindow &gpml_time_window);

		virtual
		void
		visit_gpml_topological_network(
			 	GPlatesPropertyValues::GpmlTopologicalNetwork &gpml_toplogical_network);

		virtual
		void
		visit_gpml_topological_line_section(
				GPlatesPropertyValues::GpmlTopologicalLineSection &gpml_toplogical_line_section);

		virtual
		void
		visit_gpml_topological_point(
				GPlatesPropertyValues::GpmlTopologicalPoint &gpml_toplogical_point);

	private:
		/**
		 * Stores/builds information from iterating over @a GpmlTopologicalSection objects.
		 */
		class ResolvedNetwork
		{
		public:
			//! Reset in preparation for a new sequence of topological sections.
			void
			reset()
			{
				d_boundary_sections.clear();
				d_interior_geometries.clear();
				d_seed_geometries.clear();
			}

			//! Keeps track of topological boundary section information when visiting topological sections.
			class BoundarySection
			{
			public:
				BoundarySection(
						const GPlatesModel::FeatureId &source_feature_id,
						const ReconstructionGeometry::non_null_ptr_type &source_rg,
						const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &section_geometry);

				//! The feature id of the feature referenced by this topological section.
				GPlatesModel::FeatureId d_source_feature_id;

				//! The source reconstruction geometry.
				ReconstructionGeometry::non_null_ptr_type d_source_rg;

				/**
				 * The original source geometry-on-sphere.
				 *
				 * NOTE: This is *not* the intersection-clipped *subsegment* geometry.
				 */
				GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type d_source_geometry;

				/**
				 * Should the subsegment geometry be reversed when creating polygon boundary.
				 */
				bool d_use_reverse;

				/**
				 * The final possibly clipped boundary segment geometry.
				 *
				 * This is empty until it this section been tested against both its
				 * neighbours and the appropriate possibly clipped subsegment is chosen
				 * to be part of the plate polygon boundary.
				 */
				boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type>
						d_final_boundary_segment_unreversed_geom;

				/**
				 * Keeps track of temporary results from intersections of this section
				 * with its neighbours.
				 */
				TopologicalIntersections d_intersection_results;
			};

			//! Keeps track of interior geometry information when visiting topological interiors.
			class InteriorGeometry
			{
			public:
				InteriorGeometry(
						const GPlatesModel::FeatureId &source_feature_id,
						const ReconstructionGeometry::non_null_ptr_type &source_rg,
						const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geometry);

				//! The feature id of the feature referenced by this topological interior.
				GPlatesModel::FeatureId d_source_feature_id;

				//! The source reconstruction geometry.
				ReconstructionGeometry::non_null_ptr_type d_source_rg;

				//! The geometry-on-sphere.
				GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type d_geometry;
			};

			//! Keeps track of the seed points (used to avoid CGAL meshing inside interior polygons).
			class SeedGeometry
			{
			public:
				explicit
				SeedGeometry(
						const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geometry);

				/**
				 * The seed geometry-on-sphere.
				 *
				 * Should be a point but allow any geometry type just in case.
				 * All points in this geometry will be given to CGAL as seed points.
				 */
				GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type d_geometry;
			};


			//! Typedef for a sequence of boundary sections.
			typedef std::vector<BoundarySection> boundary_section_seq_type;

			//! Typedef for a sequence of interior geometries.
			typedef std::vector<InteriorGeometry> interior_geometry_seq_type;

			//! Typedef for a sequence of seed geometries.
			typedef std::vector<SeedGeometry> seed_geometry_seq_type;


			//! Sequence of boundary sections.
			boundary_section_seq_type d_boundary_sections;

			//! Sequence of interior geometries.
			interior_geometry_seq_type d_interior_geometries;

			//! Sequence of seed geometries.
			seed_geometry_seq_type d_seed_geometries;
		};


		/**
		 * The resolved topological networks we're generating.
		 */
		std::vector<resolved_topological_network_non_null_ptr_type> &d_resolved_topological_networks;

		/**
		 * The time at which topologies are being resolved.
		 */
		double d_reconstruction_time;

		/**
		 * The reconstruction identifier placed in all resolved topological networks.
		 */
		ReconstructHandle::type d_reconstruct_handle;

		/**
		 * The reconstructed topological boundary sections and/or interior geometries we're using to
		 * assemble our network.
		 *
		 * NOTE: We don't reference them directly but by forcing clients to produce them we
		 * ensure they exist while we search for them indirectly via feature observers.
		 * If the client didn't produce them, or forgot to, then we'd find no RFGs during the global search.
		 */
		boost::optional<std::vector<ReconstructHandle::type> > d_topological_geometry_reconstruct_handles;

		//! The current feature being visited.
		GPlatesModel::FeatureHandle::weak_ref d_currently_visited_feature;

		//! Gathers some useful reconstruction parameters.
		ReconstructionFeatureProperties d_reconstruction_params;

		//! Used to help build the resolved network of the current topological polygon.
		ResolvedNetwork d_resolved_network;

		//! controls on the triangulation
		double d_shape_factor;
		//! controls on the triangulation
		double d_max_edge;


		boost::optional<ReconstructionGeometry::non_null_ptr_type>
		find_topological_reconstruction_geometry(
				const GPlatesPropertyValues::GpmlPropertyDelegate &geometry_delegate);

		boost::optional<ResolvedNetwork::SeedGeometry>
		find_seed_geometry(
				const ReconstructionGeometry::non_null_ptr_type &reconstruction_geometry);

		void
		record_topological_interior_geometries(
				GPlatesPropertyValues::GpmlTopologicalNetwork &gpml_topological_network);

		void
		record_topological_interior_geometry(
				const GPlatesPropertyValues::GpmlTopologicalNetwork::Interior &gpml_topological_interior);

		boost::optional<ResolvedNetwork::InteriorGeometry>
		record_topological_interior_reconstructed_geometry(
				const GPlatesModel::FeatureId &interior_source_feature_id,
				const ReconstructionGeometry::non_null_ptr_type &interior_source_rg);

		void
		record_topological_boundary_sections(
				GPlatesPropertyValues::GpmlTopologicalNetwork &gpml_topological_network);

		boost::optional<ResolvedNetwork::BoundarySection>
		record_topological_boundary_section_reconstructed_geometry(
				const GPlatesModel::FeatureId &boundary_section_source_feature_id,
				const ReconstructionGeometry::non_null_ptr_type &boundary_section_source_rg);


		void
		process_topological_boundary_section_intersections();

		void
		process_topological_section_intersection_boundary(
				const std::size_t current_section_index,
				const bool two_sections = false);


		void
		assign_boundary_segments();

		void
		assign_boundary_segment_boundary(
				const std::size_t section_index);


		/**
		 * Create a @a ResolvedTopologicalNetwork from information gathered
		 * from the most recently visited topological polygon
		 * (stored in @a d_resolved_network) and add it to the @a Reconstruction.
		 */
		void
		create_resolved_topology_network();

		/**
		 * Add boundary points for delaunay triangulation from a reconstructed feature geometry.
		 */
		void
		add_boundary_delaunay_points_from_reconstructed_feature_geometry(
				const reconstructed_feature_geometry_non_null_ptr_type &boundary_section_rfg,
				const GPlatesMaths::GeometryOnSphere &boundary_section_geometry,
				std::vector<ResolvedTriangulation::Network::DelaunayPoint> &all_delaunay_points);

		/**
		 * Add boundary points for delaunay triangulation from a resolved topological *line*.
		 */
		void
		add_boundary_delaunay_points_from_resolved_topological_line(
				const resolved_topological_geometry_non_null_ptr_type &boundary_section_rtg,
				const GPlatesMaths::GeometryOnSphere &boundary_section_geometry,
				std::vector<ResolvedTriangulation::Network::DelaunayPoint> &all_delaunay_points);

		/**
		 * Add a boundary section's points to the final list of boundary points.
		 */
		void
		add_boundary_points(
				const ResolvedNetwork::BoundarySection &boundary_section,
				std::vector<GPlatesMaths::PointOnSphere> &boundary_points);

		/**
		 * Add interior points for delaunay triangulation from a reconstructed feature geometry.
		 */
		void
		add_interior_delaunay_points_from_reconstructed_feature_geometry(
				const reconstructed_feature_geometry_non_null_ptr_type &interior_rfg,
				const GPlatesMaths::GeometryOnSphere &interior_geometry,
				std::vector<ResolvedTriangulation::Network::DelaunayPoint> &all_delaunay_points,
				std::vector<reconstructed_feature_geometry_non_null_ptr_type> &all_interior_polygons);

		/**
		 * Add interior points for delaunay triangulation from a resolved topological *line*.
		 */
		void
		add_interior_delaunay_points_from_resolved_topological_line(
				const resolved_topological_geometry_non_null_ptr_type &interior_rtg,
				std::vector<ResolvedTriangulation::Network::DelaunayPoint> &all_delaunay_points);

		/**
		 * Add interior points for constrained delaunay triangulation.
		 */
		void
		add_interior_constrained_delaunay_points(
				const ResolvedNetwork::InteriorGeometry &interior_geometry,
				std::vector<ResolvedTriangulation::Network::ConstrainedDelaunayGeometry> &all_constrained_delaunay_geometries,
				std::vector<GPlatesMaths::PointOnSphere> &all_scattered_points);


		void
		debug_output_topological_source_feature(
				const GPlatesModel::FeatureId &source_feature_id);
	};
}

#endif  // GPLATES_APP_LOGIC_TOPOLOGY_NETWORK_RESOLVER_H
