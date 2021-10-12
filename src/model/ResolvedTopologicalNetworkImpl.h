/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#ifndef GPLATES_MODEL_RESOLVEDTOPOLOGICALNETWORKIMPL_H
#define GPLATES_MODEL_RESOLVEDTOPOLOGICALNETWORKIMPL_H

#include <cstddef>
#include <iterator>
#include <vector>
#include <boost/operators.hpp>
#include <boost/optional.hpp>

#include "FeatureHandle.h"

#include "app-logic/CgalUtils.h"
#include "app-logic/PlateVelocityUtils.h"
#include "app-logic/TopologyUtils.h"

#include "maths/PolygonOnSphere.h"

#include "property-values/GeoTimeInstant.h"

#include "utils/ReferenceCount.h"


namespace GPlatesModel
{
	/**
	 * Contains the full topological network unlike @a ResolvedTopologicalNetwork.
	 *
	 * FIXME: Currently a @a ResolvedTopologicalNetwork only represents a single
	 * triangle in the topological network's triangulation because we need to store
	 * a geometry and currently cannot store a geometry that is an arbitrary network
	 * (the only derived types of @a GeometryOnSphere are point, multi-point, polyline
	 * and polygon).
	 *
	 * So many @a ResolvedTopologicalNetwork objects share
	 * a single @a ResolvedTopologicalNetworkImpl object.
	 */
	class ResolvedTopologicalNetworkImpl :
			public GPlatesUtils::ReferenceCount<ResolvedTopologicalNetworkImpl>
	{
	public:
		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<ResolvedTopologicalNetworkImpl,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<ResolvedTopologicalNetworkImpl,
				GPlatesUtils::NullIntrusivePointerHandler> non_null_ptr_type;

		/**
		 * A convenience typedef for the geometry of a node of this RTN.
		 */
		typedef GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type node_geometry_ptr_type;


		/**
		 * Records the reconstructed geometry, and any other relevant information,
		 * of a node that is part of the topology network.
		 * Each node will typically reference a different feature and possibly
		 * different reconstruction plate id.
		 */
		class Node
		{
		public:
			Node(
					const node_geometry_ptr_type &node_geometry,
					const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref) :
				d_node_geometry(node_geometry),
				d_feature_ref(feature_ref)
			{  }

			/**
			 * The vertices of topological section used to reconstruct this node.
			 */
			node_geometry_ptr_type
			get_geometry() const
			{
				return d_node_geometry;
			}

			//! Reference to the feature referenced by the topological section.
			const GPlatesModel::FeatureHandle::const_weak_ref &
			get_feature_ref() const
			{
				return d_feature_ref;
			}

		private:
			//! The node geometry.
			node_geometry_ptr_type d_node_geometry;

			//! Reference to the source feature handle of the topological section.
			GPlatesModel::FeatureHandle::const_weak_ref d_feature_ref;
		};


		//! Typedef for a sequence of @a Node objects.
		typedef std::vector<Node> node_seq_type;


		/**
		 * Forward iterator over export template filename sequence.
		 * Dereferencing iterator returns a 'const Node &'.
		 */
		class NodeConstIterator :
				public std::iterator<std::bidirectional_iterator_tag, const Node>,
				public boost::bidirectional_iteratable<NodeConstIterator, const Node *>
		{
		public:
			//! Create begin iterator.
			static
			NodeConstIterator
			create_begin(
					const node_seq_type &node_seq)
			{
				return NodeConstIterator(node_seq, 0);
			}


			//! Create end iterator.
			static
			NodeConstIterator
			create_end(
					const node_seq_type &node_seq)
			{
				return NodeConstIterator(node_seq, node_seq.size());
			}


			/**
			 * Dereference operator.
			 * Operator->() provided by class boost::bidirectional_iteratable.
			 */
			const Node &
			operator*() const
			{
				return (*d_node_seq)[d_node_index];
			}


			/**
			 * Pre-increment operator.
			 * Post-increment operator provided by base class boost::bidirectional_iteratable.
			 */
			NodeConstIterator &
			operator++()
			{
				++d_node_index;
				return *this;
			}


			/**
			 * Pre-decrement operator.
			 * Post-decrement operator provided by base class boost::bidirectional_iteratable.
			 */
			NodeConstIterator &
			operator--()
			{
				--d_node_index;
				return *this;
			}


			/**
			 * Equality comparison.
			 * Inequality operator provided by base class boost::bidirectional_iteratable.
			 */
			friend
			bool
			operator==(
					const NodeConstIterator &lhs,
					const NodeConstIterator &rhs)
			{
				return lhs.d_node_seq == rhs.d_node_seq &&
					lhs.d_node_index == rhs.d_node_index;
			}

		private:
			const node_seq_type *d_node_seq;
			std::size_t d_node_index;


			NodeConstIterator(
					const node_seq_type &node_seq,
					std::size_t sequence_index) :
				d_node_seq(&node_seq),
				d_node_index(sequence_index)
			{  }
		};


		/**
		 * The type used to const_iterate over the nodes.
		 */
		typedef NodeConstIterator node_const_iterator;

		/**
		 * Create a ResolvedTopologicalNetworkImpl instance with an optional plate ID and an
		 * optional time of formation.
		 */
		template<typename NodeForwardIter>
		static
		const non_null_ptr_type
		create(
				boost::shared_ptr<GPlatesAppLogic::CgalUtils::cgal_delaunay_triangulation_type> cgal_triangulation,
				FeatureHandle &feature_handle,
				FeatureHandle::properties_iterator property_iterator_,
				NodeForwardIter node_sequence_begin,
				NodeForwardIter node_sequence_end,
				boost::optional<integer_plate_id_type> plate_id_ = boost::none,
				boost::optional<GPlatesPropertyValues::GeoTimeInstant> time_of_formation_ = boost::none)
		{
			non_null_ptr_type ptr(
					new ResolvedTopologicalNetworkImpl(cgal_triangulation, feature_handle, property_iterator_,
							node_sequence_begin, node_sequence_end,
							plate_id_, time_of_formation_),
					GPlatesUtils::NullIntrusivePointerHandler());
			return ptr;
		}

		/**
		 * Returns const iterator to beginning of the internal sequence of @a Node objects.
		 */
		node_const_iterator
		nodes_begin() const
		{
			return node_const_iterator::create_begin(d_node_seq);
		}

		/**
		 * Returns const iterator to end of the internal sequence of @a Node objects.
		 */
		node_const_iterator
		nodes_end() const
		{
			return node_const_iterator::create_end(d_node_seq);
		}

		/**
		 * The delaunay triangulation of all the points in the network.
		 */
		const GPlatesAppLogic::CgalUtils::cgal_delaunay_triangulation_type &
		get_cgal_triangulation() const
		{
			return *d_cgal_triangulation;
		}

		/**
		 * Get the velocity data at the points of this network.
		 *
		 * This can be used to interpolate velocities at arbitrary points inside the network.
		 *
		 * Use 'contains_velocities()' on the returned object to see if the velocities have been set.
		 * If velocities are never calculated (ie, not needed) then it's ok to never set the query.
		 */
		const GPlatesAppLogic::PlateVelocityUtils::TopologicalNetworkVelocities &
		get_network_velocities() const
		{
			return d_network_velocities;
		}

		/**
		 * Set the velocity data at the points of this network.
		 *
		 * If velocities are never calculated (ie, not needed) then it's ok to never call this.
		 */
		void
		set_network_velocities(
				const GPlatesAppLogic::PlateVelocityUtils::TopologicalNetworkVelocities &
						topological_network_velocities)
		{
			d_network_velocities = topological_network_velocities;
		}

		const FeatureHandle::weak_ref
		get_feature_ref() const
		{
			return d_feature_ref;
		}

		const FeatureHandle::properties_iterator
		property() const
		{
			return d_property_iterator;
		}


		const boost::optional<integer_plate_id_type> &
		plate_id() const
		{
			return d_plate_id;
		}

		const boost::optional<GPlatesPropertyValues::GeoTimeInstant> &
		time_of_formation() const
		{
			return d_time_of_formation;
		}

	private:
		/**
		 * Reference to the topology network feature.
		 */
		FeatureHandle::weak_ref d_feature_ref;

		/**
		 * This is an iterator to the (topological-geometry-valued) property from which
		 * this RTN was derived.
		 */
		FeatureHandle::properties_iterator d_property_iterator;

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
		boost::optional<integer_plate_id_type> d_plate_id;

		/**
		 * The cached time of formation of the feature, if it exists.
		 *
		 * This is cached so that it can be used to calculate the age of the feature at any
		 * particular reconstruction time.  The age of the feature is used when colouring
		 * feature geometries by age.
		 */
		boost::optional<GPlatesPropertyValues::GeoTimeInstant> d_time_of_formation;

		/**
		 * The sequence of @a Node objects that make up the resolved topology network.
		 */
		node_seq_type d_node_seq;

		/**
		 * The delaunay triangulation of all the points in the network.
		 */
		boost::shared_ptr<GPlatesAppLogic::CgalUtils::cgal_delaunay_triangulation_type> d_cgal_triangulation;

		/**
		 * Stores the velocity data at the points of this network and
		 * can be used to interpolate velocities at arbitrary points within the network.
		 */
		GPlatesAppLogic::PlateVelocityUtils::TopologicalNetworkVelocities
				d_network_velocities;

		/**
		 * Instantiate a network with an optional reconstruction plate ID and
		 * an optional time of formation.
		 *
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 */
		template <typename NodeForwardIter>
		ResolvedTopologicalNetworkImpl(
				boost::shared_ptr<GPlatesAppLogic::CgalUtils::cgal_delaunay_triangulation_type> cgal_triangulation,
				FeatureHandle &feature_handle,
				FeatureHandle::properties_iterator property_iterator_,
				NodeForwardIter node_sequence_begin,
				NodeForwardIter node_sequence_end,
				boost::optional<integer_plate_id_type> plate_id_ = boost::none,
				boost::optional<GPlatesPropertyValues::GeoTimeInstant> time_of_formation_ = boost::none) :
			d_feature_ref(feature_handle.reference()),
			d_property_iterator(property_iterator_),
			d_plate_id(plate_id_),
			d_time_of_formation(time_of_formation_),
			d_node_seq(node_sequence_begin, node_sequence_end),
			d_cgal_triangulation(cgal_triangulation)
		{  }
	};
}

#endif // GPLATES_MODEL_RESOLVEDTOPOLOGICALNETWORKIMPL_H
