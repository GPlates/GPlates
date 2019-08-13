/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2014 The University of Sydney, Australia
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

#ifndef GPLATES_APP_LOGIC_RESOLVEDTOPOLOGICALLINE_H
#define GPLATES_APP_LOGIC_RESOLVEDTOPOLOGICALLINE_H

#include <vector>
#include <boost/optional.hpp>

#include "ReconstructionGeometryVisitor.h"
#include "ResolvedTopologicalGeometry.h"
#include "ResolvedTopologicalGeometrySubSegment.h"
#include "ResolvedVertexSourceInfo.h"

#include "maths/PolylineOnSphere.h"

#include "model/WeakObserverVisitor.h"


namespace GPlatesAppLogic
{
	/**
	 * A resolved topological *polyline*.
	 */
	class ResolvedTopologicalLine :
			public ResolvedTopologicalGeometry
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a ResolvedTopologicalLine.
		typedef GPlatesUtils::non_null_intrusive_ptr<ResolvedTopologicalLine> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a non-const @a ResolvedTopologicalLine.
		typedef GPlatesUtils::non_null_intrusive_ptr<const ResolvedTopologicalLine> non_null_ptr_to_const_type;

		//! A convenience typedef for boost::intrusive_ptr<ResolvedTopologicalLine>.
		typedef boost::intrusive_ptr<ResolvedTopologicalLine> maybe_null_ptr_type;

		//! A convenience typedef for boost::intrusive_ptr<const ResolvedTopologicalLine>.
		typedef boost::intrusive_ptr<const ResolvedTopologicalLine> maybe_null_ptr_to_const_type;

		//! A convenience typedef for a resolved topological polyline geometry.
		typedef GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type resolved_topology_line_ptr_type;


		/**
		 * Create a resolved topological *line* with an optional plate ID and an optional time of formation.
		 *
		 * For instance, a @a ResolvedTopologicalLine might be created without a plate ID if no
		 * plate ID is found amongst the properties of the feature whose topological line was resolved.
		 */
		template<typename SubSegmentForwardIter>
		static
		const non_null_ptr_type
		create(
				const ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree,
				const ReconstructionTreeCreator &reconstruction_tree_creator,
				const resolved_topology_line_ptr_type &resolved_topology_line_ptr,
				GPlatesModel::FeatureHandle &feature_handle,
				GPlatesModel::FeatureHandle::iterator property_iterator_,
				SubSegmentForwardIter sub_segment_sequence_begin,
				SubSegmentForwardIter sub_segment_sequence_end,
				boost::optional<GPlatesModel::integer_plate_id_type> plate_id_ = boost::none,
				boost::optional<GPlatesPropertyValues::GeoTimeInstant> time_of_formation_ = boost::none,
				boost::optional<ReconstructHandle::type> reconstruct_handle_ = boost::none)
		{
			return non_null_ptr_type(
					new ResolvedTopologicalLine(
							reconstruction_tree,
							reconstruction_tree_creator,
							resolved_topology_line_ptr,
							feature_handle,
							property_iterator_,
							sub_segment_sequence_begin,
							sub_segment_sequence_end,
							plate_id_,
							time_of_formation_,
							reconstruct_handle_));
		}

		virtual
		~ResolvedTopologicalLine()
		{  }

		/**
		 * Get a non-null pointer to a const @a ResolvedTopologicalLine which points to this instance.
		 *
		 * Since the @a ResolvedTopologicalLine constructors are private, it should never be the
		 * case that a @a ResolvedTopologicalLine instance has been constructed on the stack.
		 */
		const non_null_ptr_to_const_type
		get_non_null_pointer_to_const() const
		{
			return GPlatesUtils::get_non_null_pointer(this);
		}

		/**
		 * Get a non-null pointer to a @a ResolvedTopologicalLine which points to this instance.
		 *
		 * Since the @a ResolvedTopologicalLine constructors are private, it should never be the
		 * case that a @a ResolvedTopologicalLine instance has been constructed on the stack.
		 */
		const non_null_ptr_type
		get_non_null_pointer()
		{
			return GPlatesUtils::get_non_null_pointer(this);
		}

		/**
		 * Access the resolved topology polyline as a @a GeometryOnSphere.
		 */
		virtual
		const resolved_topology_geometry_ptr_type
		resolved_topology_geometry() const
		{
			return d_resolved_topology_line_ptr;
		}

		/**
		 * Returns the resolved topology polyline as a @a PolylineOnSphere.
		 */
		resolved_topology_line_ptr_type
		resolved_topology_line() const
		{
			return d_resolved_topology_line_ptr;
		}


		/**
		 * Returns the per-vertex source reconstructed feature geometries.
		 *
		 * Each vertex returned by @a resolved_topology_line references a source reconstructed feature geometry.
		 * This method returns the same number of vertex sources as vertices returned by @a resolved_topology_line.
		 */
		const resolved_vertex_source_info_seq_type &
		get_vertex_source_infos() const;


		/**
		 * Returns the internal sequence of @a SubSegment objects.
		 */
		const sub_segment_seq_type &
		get_sub_segment_sequence() const
		{
			return d_sub_segment_seq;
		}


		/**
		 * Accept a ConstReconstructionGeometryVisitor instance.
		 */
		virtual
		void
		accept_visitor(
				ConstReconstructionGeometryVisitor &visitor) const
		{
			visitor.visit(this->get_non_null_pointer_to_const());
		}


		/**
		 * Accept a ReconstructionGeometryVisitor instance.
		 */
		virtual
		void
		accept_visitor(
				ReconstructionGeometryVisitor &visitor)
		{
			visitor.visit(this->get_non_null_pointer());
		}

		/**
		 * Accept a WeakObserverVisitor instance.
		 */
		virtual
		void
		accept_weak_observer_visitor(
				GPlatesModel::WeakObserverVisitor<GPlatesModel::FeatureHandle> &visitor)
		{
			visitor.visit_resolved_topological_line(*this);
		}


		/**
		 * Whether rubber band points of this resolved topological line's sub-segments contributed to its line geometry.
		 *
		 * They're not really needed since they don't change the shape of the line geometry (because they're halfway between
		 * adjacent sub-segments), but they are needed for the individual sub-segments that make up the line geometry
		 * (in order to delineate the individual sub-segments).
		 */
		static const bool INCLUDE_SUB_SEGMENT_RUBBER_BAND_POINTS_IN_RESOLVED_LINE = false;

	private:

		/**
		 * The resolved topology polyline.
		 */
		resolved_topology_line_ptr_type d_resolved_topology_line_ptr;

		/**
		 * The sequence of @a SubSegment objects that form the resolved topology line.
		 *
		 * This contains the subset of vertices of each reconstructed topological section
		 * used to generate the resolved topology line.
		 */
		sub_segment_seq_type d_sub_segment_seq;


		/**
		 * Each point in the resolved topological line can potentially reference a different
		 * source reconstructed feature geometry.
		 *
		 * As an optimisation, this is only created when first requested.
		 */
		mutable boost::optional<resolved_vertex_source_info_seq_type> d_vertex_source_infos;


		/**
		 * Instantiate a resolved topological line with an optional reconstruction
		 * plate ID and an optional time of formation.
		 *
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 */
		template<typename SubSegmentForwardIter>
		ResolvedTopologicalLine(
				const ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree_,
				const ReconstructionTreeCreator &reconstruction_tree_creator,
				resolved_topology_line_ptr_type resolved_topology_line_ptr,
				GPlatesModel::FeatureHandle &feature_handle,
				GPlatesModel::FeatureHandle::iterator property_iterator_,
				SubSegmentForwardIter sub_segment_sequence_begin,
				SubSegmentForwardIter sub_segment_sequence_end,
				boost::optional<GPlatesModel::integer_plate_id_type> plate_id_,
				boost::optional<GPlatesPropertyValues::GeoTimeInstant> time_of_formation_,
				boost::optional<ReconstructHandle::type> reconstruct_handle_):
			ResolvedTopologicalGeometry(
					reconstruction_tree_,
					reconstruction_tree_creator,
					feature_handle,
					property_iterator_,
					plate_id_,
					time_of_formation_,
					reconstruct_handle_),
			d_resolved_topology_line_ptr(resolved_topology_line_ptr),
			d_sub_segment_seq(sub_segment_sequence_begin, sub_segment_sequence_end)
		{  }

		void
		calc_vertex_source_infos() const;
	};
}

#endif  // GPLATES_APP_LOGIC_RESOLVEDTOPOLOGICALLINE_H
