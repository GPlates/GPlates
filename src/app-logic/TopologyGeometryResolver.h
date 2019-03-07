
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

#ifndef GPLATES_APP_LOGIC_TOPOLOGY_GEOMETRY_RESOLVER_H
#define GPLATES_APP_LOGIC_TOPOLOGY_GEOMETRY_RESOLVER_H

#include <cstddef> // For std::size_t
#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>

#include "ReconstructedFeatureGeometry.h"
#include "ReconstructHandle.h"
#include "ReconstructionFeatureProperties.h"
#include "ReconstructionTree.h"
#include "ResolvedTopologicalBoundary.h"
#include "ResolvedTopologicalLine.h"
#include "TopologyIntersections.h"

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
	 * Finds all topological geometry features such as topological closed plate boundaries or
	 * topological lines, in the features visited, that exist at a particular reconstruction time
	 * and creates @a ResolvedTopologicalBoundary and/or @a ResolvedTopologicalLine objects.
	 */
	class TopologyGeometryResolver : 
			public GPlatesModel::FeatureVisitor,
			private boost::noncopyable
	{
	public:

		/**
		 * The resolved topological *lines* are appended to @a resolved_topological_lines.
		 *
		 * @param reconstruct_handle is placed in all resolved topological geometries as a reconstruction identifier.
		 * @param topological_sections_reconstruct_handles is a list of reconstruct handles that identifies
		 *        the subset, of all reconstruction geometries observing the topological section features,
		 *        that should be searched when resolving the topological geometries.
		 *        This is useful to avoid outdated reconstruction geometries still in existence (and other scenarios).
		 */
		TopologyGeometryResolver(
				std::vector<ResolvedTopologicalLine::non_null_ptr_type> &resolved_topological_lines,
				ReconstructHandle::type reconstruct_handle,
				const ReconstructionTreeCreator &reconstruction_tree_creator,
				const double &reconstruction_time,
				boost::optional<const std::vector<ReconstructHandle::type> &> topological_sections_reconstruct_handles);

		/**
		 * The resolved topological *boundaries* are appended to @a resolved_topological_boundaries.
		 *
		 * @param reconstruct_handle is placed in all resolved topological geometries as a reconstruction identifier.
		 * @param topological_sections_reconstruct_handles is a list of reconstruct handles that identifies
		 *        the subset, of all reconstruction geometries observing the topological section features,
		 *        that should be searched when resolving the topological geometries.
		 *        This is useful to avoid outdated reconstruction geometries still in existence (and other scenarios).
		 */
		TopologyGeometryResolver(
				std::vector<ResolvedTopologicalBoundary::non_null_ptr_type> &resolved_topological_boundaries,
				ReconstructHandle::type reconstruct_handle,
				const ReconstructionTreeCreator &reconstruction_tree_creator,
				const double &reconstruction_time,
				boost::optional<const std::vector<ReconstructHandle::type> &> topological_sections_reconstruct_handles);

		/**
		 * The resolved topological *lines* are appended to @a resolved_topological_lines and
		 * the resolved topological *boundaries* are appended to @a resolved_topological_boundaries.
		 *
		 * @param reconstruct_handle is placed in all resolved topological geometries as a reconstruction identifier.
		 * @param topological_sections_reconstruct_handles is a list of reconstruct handles that identifies
		 *        the subset, of all reconstruction geometries observing the topological section features,
		 *        that should be searched when resolving the topological geometries.
		 *        This is useful to avoid outdated reconstruction geometries still in existence (and other scenarios).
		 */
		TopologyGeometryResolver(
				std::vector<ResolvedTopologicalLine::non_null_ptr_type> &resolved_topological_lines,
				std::vector<ResolvedTopologicalBoundary::non_null_ptr_type> &resolved_topological_boundaries,
				ReconstructHandle::type reconstruct_handle,
				const ReconstructionTreeCreator &reconstruction_tree_creator,
				const double &reconstruction_time,
				boost::optional<const std::vector<ReconstructHandle::type> &> topological_sections_reconstruct_handles);

		virtual
		~TopologyGeometryResolver() 
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

		void
		visit_gpml_topological_line(
				GPlatesPropertyValues::GpmlTopologicalLine &gpml_topological_line);

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
		class ResolvedGeometry
		{
		public:
			//! Reset in preparation for a new sequence of topological sections.
			void
			reset()
			{
				d_sections.clear();
			}

			//! Keeps track of topological section information when visiting topological sections.
			class Section
			{
			public:
				Section(
						const GPlatesModel::FeatureId &source_feature_id,
						const ReconstructionGeometry::non_null_ptr_type &source_rg,
						const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &section_geometry,
						bool reverse_hint) :
					d_source_feature_id(source_feature_id),
					d_source_rg(source_rg),
					d_intersection_results(TopologicalIntersections::create(source_rg, section_geometry, reverse_hint))
				{  }

				//! The feature id of the feature referenced by this topological section.
				GPlatesModel::FeatureId d_source_feature_id;

				//! The source reconstruction geometry.
				ReconstructionGeometry::non_null_ptr_type d_source_rg;

				/**
				 * Keeps track of temporary results from intersections of this section with its neighbours.
				 */
				TopologicalIntersections::shared_ptr_type d_intersection_results;
			};

			//! Typedef for a sequence of sections.
			typedef std::vector<Section> section_seq_type;

			//! Sequence of sections of the currently visited topological geometry.
			section_seq_type d_sections;
		};


		//! The type of topological geometry to resolve.
		enum ResolveGeometryType
		{
			RESOLVE_BOUNDARY,
			RESOLVE_LINE,

			NUM_RESOLVE_GEOMETRY_TYPES // This must be last.
		};


		/**
		 * The resolved topological *lines* we're generating (if requested).
		 */
		boost::optional<std::vector<ResolvedTopologicalLine::non_null_ptr_type> &> d_resolved_topological_lines;

		/**
		 * The resolved topological *boundaries* we're generating (if requested).
		 */
		boost::optional<std::vector<ResolvedTopologicalBoundary::non_null_ptr_type> &> d_resolved_topological_boundaries;

		/**
		 * The reconstruction identifier placed in all resolved topological geometries.
		 */
		ReconstructHandle::type d_reconstruct_handle;

		/**
		 * The reconstruction tree creator associated with the resolved topological geometries.
		 */
		const ReconstructionTreeCreator d_reconstruction_tree_creator;

		/**
		 * The reconstruction tree associated with the resolved topological geometries being generated.
		 */
		ReconstructionTree::non_null_ptr_to_const_type d_reconstruction_tree;

		/**
		 * A list of reconstruct handles that identifies the subset, of all reconstruction geometries observing the
		 * topological section features, that should be searched when resolving the topological geometry.
		 *
		 * This is useful to avoid outdated reconstruction geometries still in existence (and other scenarios).
		 */
		boost::optional<std::vector<ReconstructHandle::type> > d_topological_sections_reconstruct_handles;

		//! The current feature being visited.
		GPlatesModel::FeatureHandle::weak_ref d_currently_visited_feature;

		//! The current resolved geometry property type being visited.
		boost::optional<ResolveGeometryType> d_current_resolved_geometry_type;

		//! Gathers some useful reconstruction parameters.
		ReconstructionFeatureProperties d_reconstruction_params;

		//! Used to help build the resolved geometry of the current topological geometry.
		ResolvedGeometry d_resolved_geometry;


		/**
		 * Create a *polygon* @a ResolvedTopologicalBoundary from information gathered from the most
		 * recently visited topological polygon (stored in @a d_resolved_geometry).
		 */
		void
		create_resolved_topological_boundary();

		/**
		 * Create a *polyline* @a ResolvedTopologicalLine from information gathered from the most
		 * recently visited topological line (stored in @a d_resolved_geometry).
		 */
		void
		create_resolved_topological_line();

		template <typename TopologicalSectionsIterator>
		void
		record_topological_sections(
				const TopologicalSectionsIterator &sections_begin,
				const TopologicalSectionsIterator &sections_end);

		boost::optional<ResolvedGeometry::Section>
		record_topological_section_reconstructed_geometry(
				const GPlatesModel::FeatureId &source_feature_id,
				const GPlatesPropertyValues::GpmlPropertyDelegate &geometry_delegate,
				bool reverse_hint);

		void
		process_resolved_boundary_topological_section_intersections();

		void
		process_resolved_boundary_topological_section_intersection(
				const std::size_t current_section_index,
				const bool two_sections = false);

		void
		process_resolved_line_topological_section_intersections();

		void
		process_resolved_line_topological_section_intersection(
				const std::size_t current_section_index);

		void
		debug_output_topological_section_feature_id(
				const GPlatesModel::FeatureId &section_feature_id);
	};
}

#endif  // GPLATES_APP_LOGIC_TOPOLOGY_GEOMETRY_RESOLVER_H
