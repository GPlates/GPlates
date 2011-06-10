
/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date: 2008-08-15 02:13:48 -0700 (Fri, 15 Aug 2008) $
 * 
 * Copyright (C) 2008, 2010 The University of Sydney, Australia
 * Copyright (C) 2008, 2009 California Institute of Technology 
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

#include "AppLogicFwd.h"
#include "ReconstructionFeatureProperties.h"
#include "ResolvedTopologicalBoundary.h"
#include "TopologyBoundaryIntersections.h"

#include "maths/GeometryOnSphere.h"

#include "model/types.h"
#include "model/FeatureId.h"
#include "model/FeatureHandle.h"
#include "model/FeatureVisitor.h"
#include "model/FeatureCollectionHandle.h"
#include "model/Model.h"

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
		 * @param reconstruction_tree is associated with the output resolved topological networks.
		 * @param reconstructed_topological_sections are the reconstructed feature geometries
		 *        of the topological sections used to form the networks.
		 *        We don't reference them directly but by forcing clients to produce them we
		 *        ensure they exist while we search for them indirectly via feature observers.
		 *        If clients didn't produce them, or forgot to, then we'd find no RFGs during the global search.
		 * @param restrict_topological_sections_to_same_reconstruction_tree is used to restrict the
		 *        reconstructed topological boundary sections, specified with
		 *        @a reconstructed_topological_sections, to those that were reconstructed
		 *        using @a reconstruction_tree (ie, the same reconstruction tree associated with
		 *        the resolved topological networks being generated).
		 */
		TopologyNetworkResolver(
				std::vector<resolved_topological_boundary_non_null_ptr_type> &resolved_topological_boundaries,
				std::vector<resolved_topological_network_non_null_ptr_type> &resolved_topological_networks,
				const reconstruction_tree_non_null_ptr_to_const_type &reconstruction_tree,
				const std::vector<reconstructed_feature_geometry_non_null_ptr_type> &reconstructed_topological_sections,
				bool restrict_topological_sections_to_same_reconstruction_tree = true);

		virtual
		~TopologyNetworkResolver() 
		{  }

		virtual
		bool
		initialise_pre_feature_properties(
				GPlatesModel::FeatureHandle &feature_handle);

		virtual
		void
		finalise_post_feature_properties(
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
		visit_gpml_topological_interior(
			 	GPlatesPropertyValues::GpmlTopologicalInterior &gpml_toplogical_interior);

		virtual
		void
		visit_gpml_topological_polygon(
			 	GPlatesPropertyValues::GpmlTopologicalPolygon &gpml_toplogical_polygon);

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
				d_sections.clear();
			}

			//////////////////////////////////////
			// NOTE: from TopologyBoundaryResolver
			//! Keeps track of @a GpmlTopologicalIntersection information.
			class Intersection
			{
			public:
				Intersection(
						const GPlatesMaths::PointOnSphere &reference_point) :
					d_reference_point(reference_point)
				{  }

				//! The reference (clicked) point for the intersection.
				GPlatesMaths::PointOnSphere d_reference_point;

				//! The reconstructed reference point - optional because valid not known at construction.
				boost::optional<GPlatesMaths::PointOnSphere> d_reconstructed_reference_point;
			};
			//////////////////////////////////////

			//! Keeps track of topological section information when visiting topological sections.
			class Section
			{
			public:
				Section(
						const GPlatesModel::FeatureId &source_feature_id,
						const reconstructed_feature_geometry_non_null_ptr_type &source_rfg);

				//! The feature id of the feature referenced by this topological section.
				GPlatesModel::FeatureId d_source_feature_id;

				//! The source @a ReconstructedFeatureGeometry.
				reconstructed_feature_geometry_non_null_ptr_type d_source_rfg;

				//! The optional start intersection - only topological line sections can have this.
				boost::optional<Intersection> d_start_intersection;

				//! The optional end intersection - only topological line sections can have this.
				boost::optional<Intersection> d_end_intersection;

				/**
				 * Should the subsegment geometry be reversed when creating polygon boundary.
				 */
				bool d_use_reverse;

				//! flag to keep track of polygon_centroid points to determine if poly should be meshed
				bool d_is_seed_point;

				//! flags to keep track of geom types going into a network
				bool d_is_point;
				bool d_is_line;
				bool d_is_polygon;

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
				TopologicalBoundaryIntersections d_intersection_results;

				//! The segment geometry.
				boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> d_geometry;
			};

			//! Typedef for a sequence of sections.
			typedef std::vector<Section> section_seq_type;

			//! Sequence of sections of the currently visited topological polygon.
			section_seq_type d_sections;

			//! Sequence of sections of the currently visited topological interior.
			section_seq_type d_sections_interior;
		};


		/**
		 * The resolved topological boundaries we're generating.
		 */
		std::vector<ResolvedTopologicalBoundary::non_null_ptr_type> &d_resolved_topological_boundaries;

		/**
		 * The resolved topological networks we're generating.
		 */
		std::vector<resolved_topological_network_non_null_ptr_type> &d_resolved_topological_networks;

		/**
		 * The reconstruction tree associated with the resolved topological boundaries begin generated.
		 */
		reconstruction_tree_non_null_ptr_to_const_type d_reconstruction_tree;

		/**
		 * The reconstructed topogical sections we're using to assemble our networks.
		 *
		 * NOTE: We don't reference them directly but by forcing clients to produce them we
		 * ensure they exist while we search for them indirectly via feature observers.
		 * If the client didn't produce them, or forgot to, then we'd find no RFGs during the global search.
		 */
		const std::vector<reconstructed_feature_geometry_non_null_ptr_type> &d_reconstructed_topological_sections;

		/**
		 * Boolean to determine whether to restrict the reconstructed topological sections
		 * to those that were reconstructed using the same reconstruction tree associated with
		 * the resolved topological networks being generated.
		 */
		bool d_restrict_topological_sections_to_same_reconstruction_tree;

		//! The current feature being visited.
		GPlatesModel::FeatureHandle::weak_ref d_currently_visited_feature;

		//! Gathers some useful reconstruction parameters.
		ReconstructionFeatureProperties d_reconstruction_params;

		//! Used to help build the resolved network of the current topological polygon.
		ResolvedNetwork d_resolved_network;

		//! The number of topologies visited.
		int d_num_topologies;

		//! controls on the triangulation
		double d_shape_factor;
		//! controls on the triangulation
		double d_max_edge;

		//! Visitor mode: controls where and how to visit TopologicalPoint and Line
		bool d_is_visit_interior;

		//! The feature property iterator of the GpmlTopologicalPolygon property.
		boost::optional<feature_iterator_type> d_topological_polygon_feature_iterator;

		/**
		 * Create a @a ResolvedTopologicalBoundary from information gathered
		 * from the most recently visited topological polygon
		 * (stored in @a d_resolved_network) and add it to the @a Reconstruction.
		 */
		void
		create_resolved_topology_boundary(
				GPlatesModel::FeatureHandle &feature_handle);

		/**
		 * Create a @a ResolvedTopologicalNetwork from information gathered
		 * from the most recently visited topological polygon
		 * (stored in @a d_resolved_network) and add it to the @a Reconstruction.
		 */
		void
		create_resolved_topology_network(
				GPlatesModel::FeatureHandle &feature_handle);

		//! 
		void
		record_topological_sections(
				std::vector<GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type> &
						sections);

#if 0
		void
		record_topological_section_reconstructed_geometry(
				ResolvedNetwork::Section &section,
				const GPlatesPropertyValues::GpmlPropertyDelegate &geometry_delegate);
#endif

		boost::optional<ResolvedNetwork::Section>
		record_topological_section_reconstructed_geometry(
				const GPlatesModel::FeatureId &source_feature_id,
				const GPlatesPropertyValues::GpmlPropertyDelegate &geometry_delegate);


		// validate sections
		void
		validate_topological_section_intersections_boundary();

		void
		validate_topological_section_intersection_boundary(
				const std::size_t current_section_index);

		void
		validate_topological_section_intersections_interior();

		void
		validate_topological_section_intersection_interior(
				const std::size_t current_section_index);


		// process section intersections 
		void
		process_topological_section_intersections_boundary();

		void
		process_topological_section_intersection_boundary(
				const std::size_t current_section_index,
				const bool two_sections = false);

		void
		process_topological_section_intersections_interior();

		void
		process_topological_section_intersection_interior(
				const std::size_t current_section_index,
				const bool two_sections = false);


		// assign segments geom
		void
		assign_boundary_segments_boundary();

		void
		assign_boundary_segment_boundary(
				const std::size_t section_index);

		void
		assign_boundary_segments_interior();

		void
		assign_boundary_segment_interior(
				const std::size_t section_index);


		// debugging 
		void
		debug_output_topological_section(
				const ResolvedNetwork::Section &section);
	};
}

#endif  // GPLATES_APP_LOGIC_TOPOLOGY_NETWORK_RESOLVER_H
