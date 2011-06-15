
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

#ifndef GPLATES_APP_LOGIC_TOPOLOGY_BOUNDARY_RESOLVER_H
#define GPLATES_APP_LOGIC_TOPOLOGY_BOUNDARY_RESOLVER_H

#include <cstddef> // For std::size_t
#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>

#include "ReconstructedFeatureGeometry.h"
#include "ReconstructionFeatureProperties.h"
#include "ReconstructionTree.h"
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
	 * Finds all topological closed plate boundary features (in the features visited)
	 * that exist at a particular reconstruction time and creates
	 * @a ResolvedTopologicalBoundary objects for each one.
	 */
	class TopologyBoundaryResolver: 
		public GPlatesModel::FeatureVisitor,
		private boost::noncopyable
	{

	public:
		/**
		 * The resolved dynamic polygons are appended to @a resolved_topological_boundaries.
		 *
		 * @param reconstruction_tree is associated with the output resolved topological boundaries.
		 * @param reconstructed_topological_boundary_sections are the reconstructed feature geometries
		 *        of the topological boundary sections used to form the close plate polygons.
		 *        We don't reference them directly but by forcing clients to produce them we
		 *        ensure they exist while we search for them indirectly via feature observers.
		 *        If clients didn't produce them, or forgot to, then we'd find no RFGs during the global search.
		 * @param restrict_boundary_sections_to_same_reconstruction_tree is used to restrict the
		 *        reconstructed topological boundary sections, specified with
		 *        @a reconstructed_topological_boundary_sections, to those that were reconstructed
		 *        using @a reconstruction_tree (ie, the same reconstruction tree associated with
		 *        the resolved topological boundaries being generated).
		 */
		TopologyBoundaryResolver(
				std::vector<ResolvedTopologicalBoundary::non_null_ptr_type> &resolved_topological_boundaries,
				const ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree,
				const std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> &reconstructed_topological_boundary_sections,
				bool restrict_boundary_sections_to_same_reconstruction_tree = true);

		virtual
		~TopologyBoundaryResolver() 
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
		class ResolvedBoundary
		{
		public:
			//! Reset in preparation for a new sequence of topological sections.
			void
			reset()
			{
				d_sections.clear();
			}

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

			//! Keeps track of topological section information when visiting topological sections.
			class Section
			{
			public:
				Section(
						const GPlatesModel::FeatureId &source_feature_id,
						const ReconstructedFeatureGeometry::non_null_ptr_type &source_rfg) :
					d_source_feature_id(source_feature_id),
					d_source_rfg(source_rfg),
					d_use_reverse(false),
					d_intersection_results(source_rfg->reconstructed_geometry())
				{  }

				//! The feature id of the feature referenced by this topological section.
				GPlatesModel::FeatureId d_source_feature_id;

				//! The source @a ReconstructedFeatureGeometry.
				ReconstructedFeatureGeometry::non_null_ptr_type d_source_rfg;

				//! The optional start intersection - only topological line sections can have this.
				boost::optional<Intersection> d_start_intersection;

				//! The optional end intersection - only topological line sections can have this.
				boost::optional<Intersection> d_end_intersection;

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
				TopologicalBoundaryIntersections d_intersection_results;
			};

			//! Typedef for a sequence of sections.
			typedef std::vector<Section> section_seq_type;

			//! Sequence of sections of the currently visited topological polygon.
			section_seq_type d_sections;
		};

		/**
		 * The resolved topological boundaries we're generating.
		 */
		std::vector<ResolvedTopologicalBoundary::non_null_ptr_type> &d_resolved_topological_boundaries;

		/**
		 * The reconstruction tree associated with the resolved topological boundaries begin generated.
		 */
		ReconstructionTree::non_null_ptr_to_const_type d_reconstruction_tree;

		/**
		 * The reconstructed topogical boundary sections we're using to assemble our dynamic polygons.
		 *
		 * NOTE: We don't reference them directly but by forcing clients to produce them we
		 * ensure they exist while we search for them indirectly via feature observers.
		 * If the client didn't produce them, or forgot to, then we'd find no RFGs during the global search.
		 */
		const std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> &d_reconstructed_topological_boundary_sections;

		/**
		 * Boolean to determine whether to restrict the reconstructed topological boundary sections
		 * to those that were reconstructed using the same reconstruction tree associated with
		 * the resolved topological boundaries being generated.
		 */
		bool d_restrict_boundary_sections_to_same_reconstruction_tree;

		//! The current feature being visited.
		GPlatesModel::FeatureHandle::weak_ref d_currently_visited_feature;

		//! Gathers some useful reconstruction parameters.
		ReconstructionFeatureProperties d_reconstruction_params;

		//! Used to help build the resolved polygon of the current topological polygon.
		ResolvedBoundary d_resolved_boundary;

		//! The number of topologies visited.
		int d_num_topologies;


		/**
		 * Create a @a ResolvedTopologicalBoundary from information gathered
		 * from the most recently visited topological polygon
		 * (stored in @a d_resolved_boundary) and add it to the @a Reconstruction.
		 */
		void
		create_resolved_topology_boundary();

		void
		record_topological_sections(
				std::vector<GPlatesPropertyValues::GpmlTopologicalSection::non_null_ptr_type> &
						sections);

		boost::optional<ResolvedBoundary::Section>
		record_topological_section_reconstructed_geometry(
				const GPlatesModel::FeatureId &source_feature_id,
				const GPlatesPropertyValues::GpmlPropertyDelegate &geometry_delegate);

		void
		validate_topological_section_intersections();

		void
		validate_topological_section_intersection(
				const std::size_t current_section_index);

		void
		process_topological_section_intersections();

		void
		process_topological_section_intersection(
				const std::size_t current_section_index,
				const bool two_sections = false);

		void
		assign_boundary_segments();

		void
		assign_boundary_segment(
				const std::size_t section_index);

		void
		debug_output_topological_section_feature_id(
				const GPlatesModel::FeatureId &section_feature_id);
	};
}

#endif  // GPLATES_APP_LOGIC_TOPOLOGY_BOUNDARY_RESOLVER_H
