
/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009, 2010 The University of Sydney, Australia
 * Copyright (C) 2012, 2013 California Institute of Technology
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

#ifndef GPLATES_APP_LOGIC_RESOLVEDTOPOLOGICALNETWORK_H
#define GPLATES_APP_LOGIC_RESOLVEDTOPOLOGICALNETWORK_H

#include <cstddef>
#include <iterator>
#include <vector>
#include <boost/operators.hpp>
#include <boost/optional.hpp>

#include "ReconstructedFeatureGeometry.h"
#include "ReconstructionGeometry.h"
#include "ResolvedTopologicalGeometrySubSegment.h"
#include "ResolvedTriangulationNetwork.h"
#include "ResolvedVertexSourceInfo.h"

#include "maths/PolygonOnSphere.h"

#include "model/FeatureHandle.h"
#include "model/types.h"
#include "model/WeakObserver.h"

#include "property-values/GeoTimeInstant.h"


namespace GPlatesAppLogic
{
	class ResolvedTopologicalNetwork:
			public ReconstructionGeometry,
			public GPlatesModel::WeakObserver<GPlatesModel::FeatureHandle>
	{
	public:
		//! A convenience typedef for a non-null intrusive ptr to @a ResolvedTopologicalNetwork.
		typedef GPlatesUtils::non_null_intrusive_ptr<ResolvedTopologicalNetwork> non_null_ptr_type;

		//! A convenience typedef for a non-null intrusive ptr to @a ResolvedTopologicalNetwork.
		typedef GPlatesUtils::non_null_intrusive_ptr<const ResolvedTopologicalNetwork> non_null_ptr_to_const_type;

		//! A convenience typedef for the WeakObserver base class of this class.
		typedef GPlatesModel::WeakObserver<GPlatesModel::FeatureHandle> WeakObserverType;

		//! A convenience typedef for the polygon boundary of this @a ResolvedTopologicalNetwork.
		typedef GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type boundary_polygon_ptr_type;

		//! Typedef for a sequence of @a ResolvedTopologicalGeometrySubSegment objects.
		typedef sub_segment_seq_type boundary_sub_segment_seq_type;

		/**
		 * The type used to const_iterate over the interior rigid blocks.
		 */
		typedef ResolvedTriangulation::Network::rigid_block_seq_type::const_iterator rigid_block_const_iterator;


		virtual
		~ResolvedTopologicalNetwork()
		{  }


		/**
		 * Create a ResolvedTopologicalNetwork instance.
		 */
		template <typename BoundarySubSegmentForwardIter>
		static
		const non_null_ptr_type
		create(
				const double &reconstruction_time_,
				const ResolvedTriangulation::Network::non_null_ptr_type &triangulation_network,
				GPlatesModel::FeatureHandle &feature_handle,
				GPlatesModel::FeatureHandle::iterator property_iterator_,
				BoundarySubSegmentForwardIter boundary_sub_segment_sequence_begin,
				BoundarySubSegmentForwardIter boundary_sub_segment_sequence_end,
				boost::optional<GPlatesModel::integer_plate_id_type> plate_id_ = boost::none,
				boost::optional<GPlatesPropertyValues::GeoTimeInstant> time_of_formation_ = boost::none,
				boost::optional<ReconstructHandle::type> reconstruct_handle_ = boost::none)
		{
			return non_null_ptr_type(
					new ResolvedTopologicalNetwork(
							reconstruction_time_,
							triangulation_network,
							feature_handle,
							property_iterator_,
							boundary_sub_segment_sequence_begin,
							boundary_sub_segment_sequence_end,
							plate_id_,
							time_of_formation_,
							reconstruct_handle_));
		}


		/**
		 * Returns the *boundary* subsegments.
		 */
		const boundary_sub_segment_seq_type &
		get_boundary_sub_segment_sequence() const
		{
			return d_boundary_sub_segment_seq;
		}

		/**
		 * Access the boundary polygon of this resolved topology network.
		 */
		const boundary_polygon_ptr_type
		boundary_polygon() const
		{
			return get_triangulation_network().get_boundary_polygon();
		}

		/**
		 * Access the boundary polygon (including rigid block holes) of this resolved topology network.
		 *
		 * The outlines of interior rigid block holes (if any) in the network form interiors of the returned polygon.
		 */
		const boundary_polygon_ptr_type
		boundary_polygon_with_rigid_block_holes() const
		{
			return get_triangulation_network().get_boundary_polygon_with_rigid_block_holes();
		}



		/**
		 * Returns the boundary per-vertex source reconstructed feature geometries.
		 *
		 * Each vertex returned by @a boundary_polygon references a source reconstructed feature geometry.
		 * This method returns the same number of vertex sources as vertices returned by @a boundary_polygon.
		 */
		const resolved_vertex_source_info_seq_type &
		get_boundary_vertex_source_infos() const;


		/**
		 * The triangulation network.
		 */
		const ResolvedTriangulation::Network &
		get_triangulation_network() const
		{
			return *d_triangulation_network;
		}

		/**
		 * Get a non-null pointer to a const ResolvedTopologicalNetwork which points to this
		 * instance.
		 *
		 * Since the ResolvedTopologicalNetwork constructors are private, it should never
		 * be the case that a ResolvedTopologicalNetwork instance has been constructed on
		 * the stack.
		 */
		const non_null_ptr_to_const_type
		get_non_null_pointer_to_const() const
		{
			return GPlatesUtils::get_non_null_pointer(this);
		}

		/**
		 * Get a non-null pointer to a ResolvedTopologicalNetwork which points to this
		 * instance.
		 *
		 * Since the ResolvedTopologicalNetwork constructors are private, it should never
		 * be the case that a ResolvedTopologicalNetwork instance has been constructed on
		 * the stack.
		 */
		const non_null_ptr_type
		get_non_null_pointer()
		{
			return GPlatesUtils::get_non_null_pointer(this);
		}

		/**
		 * Return whether this RTN references @a that_feature_handle.
		 *
		 * This function will not throw.
		 */
		bool
		references(
				const GPlatesModel::FeatureHandle &that_feature_handle) const
		{
			return (feature_handle_ptr() == &that_feature_handle);
		}

		/**
		 * Return the pointer to the FeatureHandle.
		 *
		 * The pointer returned will be NULL if this instance does not reference a
		 * FeatureHandle; non-NULL otherwise.
		 *
		 * This function will not throw.
		 */
		GPlatesModel::FeatureHandle *
		feature_handle_ptr() const
		{
			return WeakObserverType::publisher_ptr();
		}

		/**
		 * Return whether this pointer is valid to be dereferenced (to obtain a
		 * FeatureHandle).
		 *
		 * This function will not throw.
		 */
		bool
		is_valid() const
		{
			return (feature_handle_ptr() != NULL);
		}

		/**
		 * Return a weak-ref to the feature whose resolved topological geometry this RTN
		 * contains, or an invalid weak-ref, if this pointer is not valid to be
		 * dereferenced.
		 */
		const GPlatesModel::FeatureHandle::weak_ref
		get_feature_ref() const;

		/**
		 * Access the topological polygon feature property used to generate
		 * the resolved topological geometry.
		 */
		const GPlatesModel::FeatureHandle::iterator
		property() const
		{
			return d_property_iterator;
		}

		/**
		 * Access the cached plate ID, if it exists.
		 *
		 * Note that it's possible for a ResolvedTopologicalNetwork to be created without
		 * a plate ID -- for example, if no plate ID is found amongst the properties of the
		 * feature whose topological geometry was resolved.
		 */
		const boost::optional<GPlatesModel::integer_plate_id_type> &
		plate_id() const
		{
			return d_plate_id;
		}

		/**
		 * Return the cached time of formation of the feature.
		 */
		const boost::optional<GPlatesPropertyValues::GeoTimeInstant> &
		time_of_formation() const
		{
			return d_time_of_formation;
		}


		/**
		 * Accept a ConstReconstructionGeometryVisitor instance.
		 */
		virtual
		void
		accept_visitor(
				ConstReconstructionGeometryVisitor &visitor) const;

		/**
		 * Accept a ReconstructionGeometryVisitor instance.
		 */
		virtual
		void
		accept_visitor(
				ReconstructionGeometryVisitor &visitor);

		/**
		 * Accept a WeakObserverVisitor instance.
		 */
		virtual
		void
		accept_weak_observer_visitor(
				GPlatesModel::WeakObserverVisitor<GPlatesModel::FeatureHandle> &visitor);


		/**
		* Whether rubber band points of this resolved topological network's boundary sub-segments contributed to its boundary geometry.
		*
		* They're not really needed since they don't change the shape of the boundary geometry (because they're halfway between
		* adjacent sub-segments), but they are needed for the individual sub-segments that make up the boundary geometry
		* (in order to delineate the individual sub-segments).
		*
		* Note that boundary sub-segments can be resolved topological *lines* (as well as reconstructed feature geometries).
		*/
		static const bool INCLUDE_SUB_SEGMENT_RUBBER_BAND_POINTS_IN_RESOLVED_NETWORK_BOUNDARY = false;

	private:
		/**
		 * This is an iterator to the (topological-geometry-valued) property from which
		 * this RTN was derived.
		 */
		GPlatesModel::FeatureHandle::iterator d_property_iterator;

		/**
		 * The cached plate ID, if it exists.
		 *
		 * Note that it's possible for a ResolvedTopologicalNetwork to be created without
		 * a plate ID -- for example, if no plate ID is found amongst the properties of the
		 * feature whose topological geometry was resolved.
		 *
		 * The plate ID is used when colouring feature geometries by plate ID.  It's also
		 * of interest to a user who has clicked on the feature geometry.
		 */
		boost::optional<GPlatesModel::integer_plate_id_type> d_plate_id;

		/**
		 * The cached time of formation of the feature, if it exists.
		 *
		 * This is cached so that it can be used to calculate the age of the feature at any
		 * particular reconstruction time.  The age of the feature is used when colouring
		 * feature geometries by age.
		 */
		boost::optional<GPlatesPropertyValues::GeoTimeInstant> d_time_of_formation;

		/**
		 * The sequence of @a SubSegment objects that form the resolved topology geometry *boundary*.
		 *
		 * This contains the subset of vertices of each reconstructed topological section
		 * used to generate the resolved topology geometry.
		 */
		boundary_sub_segment_seq_type d_boundary_sub_segment_seq;

		/**
		 * The triangulation network.
		 */
		ResolvedTriangulation::Network::non_null_ptr_type d_triangulation_network;


		/**
		 * Each point in the boundary of the resolved topological network can potentially reference
		 * a different source reconstructed feature geometry.
		 *
		 * As an optimisation, this is only created when first requested.
		 */
		mutable boost::optional<resolved_vertex_source_info_seq_type> d_boundary_vertex_source_infos;


		/**
		 * Instantiate a network with an optional reconstruction plate ID and
		 * an optional time of formation.
		 *
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 */
		template <typename BoundarySubSegmentForwardIter>
		ResolvedTopologicalNetwork(
				const double &reconstruction_time_,
				const ResolvedTriangulation::Network::non_null_ptr_type &triangulation_network,
				GPlatesModel::FeatureHandle &feature_handle,
				GPlatesModel::FeatureHandle::iterator property_iterator_,
				BoundarySubSegmentForwardIter boundary_sub_segment_sequence_begin,
				BoundarySubSegmentForwardIter boundary_sub_segment_sequence_end,
				boost::optional<GPlatesModel::integer_plate_id_type> plate_id_,
				boost::optional<GPlatesPropertyValues::GeoTimeInstant> time_of_formation_,
				boost::optional<ReconstructHandle::type> reconstruct_handle_) :
			ReconstructionGeometry(reconstruction_time_, reconstruct_handle_),
			WeakObserverType(feature_handle),
			d_property_iterator(property_iterator_),
			d_plate_id(plate_id_),
			d_time_of_formation(time_of_formation_),
			d_boundary_sub_segment_seq(boundary_sub_segment_sequence_begin, boundary_sub_segment_sequence_end),
			d_triangulation_network(triangulation_network)
		{  }

		void
		calc_boundary_vertex_source_infos() const;
	};
}

#endif // GPLATES_APP_LOGIC_RESOLVEDTOPOLOGICALNETWORK_H
