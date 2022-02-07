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

#ifndef GPLATES_APP_LOGIC_RESOLVEDTOPOLOGICALBOUNDARY_H
#define GPLATES_APP_LOGIC_RESOLVEDTOPOLOGICALBOUNDARY_H

#include <vector>

#include "ReconstructionGeometryVisitor.h"
#include "ResolvedTopologicalGeometry.h"
#include "ResolvedTopologicalGeometrySubSegment.h"

#include "maths/PolygonOnSphere.h"

#include "model/WeakObserverVisitor.h"


namespace GPlatesAppLogic
{
	/**
	 * A resolved topological *polygon*.
	 */
	class ResolvedTopologicalBoundary :
			public ResolvedTopologicalGeometry
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a ResolvedTopologicalBoundary.
		typedef GPlatesUtils::non_null_intrusive_ptr<ResolvedTopologicalBoundary> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a non-const @a ResolvedTopologicalBoundary.
		typedef GPlatesUtils::non_null_intrusive_ptr<const ResolvedTopologicalBoundary> non_null_ptr_to_const_type;

		//! A convenience typedef for boost::intrusive_ptr<ResolvedTopologicalBoundary>.
		typedef boost::intrusive_ptr<ResolvedTopologicalBoundary> maybe_null_ptr_type;

		//! A convenience typedef for boost::intrusive_ptr<const ResolvedTopologicalBoundary>.
		typedef boost::intrusive_ptr<const ResolvedTopologicalBoundary> maybe_null_ptr_to_const_type;

		//! A convenience typedef for a resolved topological polygon geometry.
		typedef GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type resolved_topology_boundary_ptr_type;


		/**
		 * Create a resolved topological *boundary* with an optional plate ID and an optional time of formation.
		 *
		 * For instance, a @a ResolvedTopologicalBoundary might be created without a plate ID if no
		 * plate ID is found amongst the properties of the feature whose topological boundary was resolved.
		 */
		template<typename SubSegmentForwardIter>
		static
		const non_null_ptr_type
		create(
				const ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree,
				const ReconstructionTreeCreator &reconstruction_tree_creator,
				const resolved_topology_boundary_ptr_type &resolved_topology_boundary_ptr,
				GPlatesModel::FeatureHandle &feature_handle,
				GPlatesModel::FeatureHandle::iterator property_iterator_,
				SubSegmentForwardIter sub_segment_sequence_begin,
				SubSegmentForwardIter sub_segment_sequence_end,
				boost::optional<GPlatesModel::integer_plate_id_type> plate_id_ = boost::none,
				boost::optional<GPlatesPropertyValues::GeoTimeInstant> time_of_formation_ = boost::none,
				boost::optional<ReconstructHandle::type> reconstruct_handle_ = boost::none)
		{
			return non_null_ptr_type(
					new ResolvedTopologicalBoundary(
							reconstruction_tree,
							reconstruction_tree_creator,
							resolved_topology_boundary_ptr,
							feature_handle,
							property_iterator_,
							sub_segment_sequence_begin,
							sub_segment_sequence_end,
							plate_id_,
							time_of_formation_,
							reconstruct_handle_));
		}

		virtual
		~ResolvedTopologicalBoundary()
		{  }

		/**
		 * Get a non-null pointer to a const @a ResolvedTopologicalBoundary which points to this instance.
		 *
		 * Since the @a ResolvedTopologicalBoundary constructors are private, it should never be the
		 * case that a @a ResolvedTopologicalBoundary instance has been constructed on the stack.
		 */
		const non_null_ptr_to_const_type
		get_non_null_pointer_to_const() const
		{
			return GPlatesUtils::get_non_null_pointer(this);
		}

		/**
		 * Get a non-null pointer to a @a ResolvedTopologicalBoundary which points to this instance.
		 *
		 * Since the @a ResolvedTopologicalBoundary constructors are private, it should never be the
		 * case that a @a ResolvedTopologicalBoundary instance has been constructed on the stack.
		 */
		const non_null_ptr_type
		get_non_null_pointer()
		{
			return GPlatesUtils::get_non_null_pointer(this);
		}

		/**
		 * Access the resolved topology polygon as a @a GeometryOnSphere.
		 */
		virtual
		const resolved_topology_geometry_ptr_type
		resolved_topology_geometry() const
		{
			return d_resolved_topology_boundary_ptr;
		}

		/**
		 * Returns the resolved topology polygon as a @a PolygonOnSphere.
		 */
		resolved_topology_boundary_ptr_type
		resolved_topology_boundary() const
		{
			return d_resolved_topology_boundary_ptr;
		}


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
			visitor.visit_resolved_topological_boundary(*this);
		}

	private:

		/**
		 * The resolved topology polygon.
		 */
		resolved_topology_boundary_ptr_type d_resolved_topology_boundary_ptr;

		/**
		 * The sequence of @a SubSegment objects that form the resolved topology boundary.
		 *
		 * This contains the subset of vertices of each reconstructed topological section
		 * used to generate the resolved topology boundary.
		 */
		sub_segment_seq_type d_sub_segment_seq;


		/**
		 * Instantiate a resolved topological boundary with an optional reconstruction
		 * plate ID and an optional time of formation.
		 *
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 */
		template<typename SubSegmentForwardIter>
		ResolvedTopologicalBoundary(
				const ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree_,
				const ReconstructionTreeCreator &reconstruction_tree_creator,
				resolved_topology_boundary_ptr_type resolved_topology_boundary_ptr,
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
			d_resolved_topology_boundary_ptr(resolved_topology_boundary_ptr),
			d_sub_segment_seq(sub_segment_sequence_begin, sub_segment_sequence_end)
		{  }
	};
}

#endif  // GPLATES_APP_LOGIC_RESOLVEDTOPOLOGICALBOUNDARY_H
