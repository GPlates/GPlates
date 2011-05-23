/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009, 2010 The University of Sydney, Australia
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

#include "AppLogicFwd.h"
#include "CgalUtils.h"
#include "PlateVelocityUtils.h"
#include "ReconstructionGeometry.h"

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

		//! A convenience typedef for boost::intrusive_ptr<ResolvedTopologicalNetwork>.
		typedef boost::intrusive_ptr<ResolvedTopologicalNetwork> maybe_null_ptr_type;

		//! A convenience typedef for boost::intrusive_ptr<const ResolvedTopologicalNetwork>.
		typedef boost::intrusive_ptr<const ResolvedTopologicalNetwork> maybe_null_ptr_to_const_type;

		//! A convenience typedef for the WeakObserver base class of this class.
		typedef GPlatesModel::WeakObserver<GPlatesModel::FeatureHandle> WeakObserverType;

		//! A convenience typedef for the geometry of this @a ResolvedTopologicalNetwork.
		typedef GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type resolved_topology_geometry_ptr_type;

		//! A convenience typedef for the geometry of a node of this RTN.
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


#if 0
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
#endif


		/**
		 * The type used to const_iterate over the nodes.
		 */
#if 0
		typedef NodeConstIterator node_const_iterator;
#endif
		typedef node_seq_type::const_iterator node_const_iterator;


		virtual
		~ResolvedTopologicalNetwork()
		{  }


		/**
		 * Create a ResolvedTopologicalNetwork instance.
		 */
		template<typename NodeForwardIter>
		static
		const non_null_ptr_type
		create(
				const reconstruction_tree_non_null_ptr_to_const_type &reconstruction_tree,
				boost::shared_ptr<CgalUtils::cgal_delaunay_triangulation_2_type> delaunay_triangulation_2,
				boost::shared_ptr<CgalUtils::cgal_constrained_delaunay_triangulation_2_type> constrained_delaunay_triangulation_2,
				GPlatesModel::FeatureHandle &feature_handle,
				GPlatesModel::FeatureHandle::iterator property_iterator_,
				NodeForwardIter node_sequence_begin,
				NodeForwardIter node_sequence_end,
				boost::optional<GPlatesModel::integer_plate_id_type> plate_id_ = boost::none,
				boost::optional<GPlatesPropertyValues::GeoTimeInstant> time_of_formation_ = boost::none)
		{
			return non_null_ptr_type(
					new ResolvedTopologicalNetwork(
							reconstruction_tree,
							delaunay_triangulation_2,
							constrained_delaunay_triangulation_2,
							feature_handle,
							property_iterator_,
							node_sequence_begin,
							node_sequence_end,
							plate_id_,
							time_of_formation_),
					GPlatesUtils::NullIntrusivePointerHandler());
		}

		/**
		 * Returns const iterator to beginning of the internal sequence of @a Node objects.
		 */
		node_const_iterator
		nodes_begin() const
		{
#if 0
			return node_const_iterator::create_begin(d_node_seq);
#endif
			return d_node_seq.begin();
		}

		/**
		 * Returns const iterator to end of the internal sequence of @a Node objects.
		 */
		node_const_iterator
		nodes_end() const
		{
#if 0
			return node_const_iterator::create_end(d_node_seq);
#endif
			return d_node_seq.end();
		}

		/**
		 * The delaunay triangulation of all the points in the network.
		 */
		const CgalUtils::cgal_delaunay_triangulation_2_type &
		get_delaunay_triangulation_2() const
		{
			return *d_delaunay_triangulation_2;
		}

		const CgalUtils::cgal_constrained_delaunay_triangulation_2_type &
		get_constrained_delaunay_triangulation_2() const
		{
			return *d_constrained_delaunay_triangulation_2;
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
						topological_network_velocities) const
		{
			d_network_velocities = topological_network_velocities;
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
		 * Access the resolved topology polygon geometry.
		 *
		 * This returns the same geometry as the base class @a geometry method does but
		 * returns it as a @a resolved_topology_geometry_ptr_type instead
		 * of a @a geometry_ptr_type.
		 */
		const std::vector<resolved_topology_geometry_ptr_type>
		resolved_topology_geometries_from_triangulation_2() const;

		const std::vector<resolved_topology_geometry_ptr_type>
		resolved_topology_geometries_from_constrained() const;

		const std::vector<resolved_topology_geometry_ptr_type>
		resolved_topology_geometries_from_mesh() const;

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
		 * The sequence of @a Node objects that make up the resolved topology network.
		 */
		node_seq_type d_node_seq;

		/**
		 * The delaunay triangulation of all the points in the network.
		 */
		boost::shared_ptr<CgalUtils::cgal_delaunay_triangulation_2_type> d_delaunay_triangulation_2;

		boost::shared_ptr<CgalUtils::cgal_constrained_delaunay_triangulation_2_type> d_constrained_delaunay_triangulation_2;

		/**
		 * Stores the velocity data at the points of this network and
		 * can be used to interpolate velocities at arbitrary points within the network.
		 */
		mutable GPlatesAppLogic::PlateVelocityUtils::TopologicalNetworkVelocities d_network_velocities;

		/**
		 * Instantiate a network with an optional reconstruction plate ID and
		 * an optional time of formation.
		 *
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 */
		template <typename NodeForwardIter>
		ResolvedTopologicalNetwork(
				const reconstruction_tree_non_null_ptr_to_const_type &reconstruction_tree_,
				boost::shared_ptr<CgalUtils::cgal_delaunay_triangulation_2_type> delaunay_triangulation_2,
				boost::shared_ptr<CgalUtils::cgal_constrained_delaunay_triangulation_2_type> constrained_delaunay_triangulation_2,
				GPlatesModel::FeatureHandle &feature_handle,
				GPlatesModel::FeatureHandle::iterator property_iterator_,
				NodeForwardIter node_sequence_begin,
				NodeForwardIter node_sequence_end,
				boost::optional<GPlatesModel::integer_plate_id_type> plate_id_ = boost::none,
				boost::optional<GPlatesPropertyValues::GeoTimeInstant> time_of_formation_ = boost::none) :
			ReconstructionGeometry(reconstruction_tree_),
			WeakObserverType(feature_handle),
			d_property_iterator(property_iterator_),
			d_plate_id(plate_id_),
			d_time_of_formation(time_of_formation_),
			d_node_seq(node_sequence_begin, node_sequence_end),
			d_delaunay_triangulation_2(delaunay_triangulation_2),
			d_constrained_delaunay_triangulation_2(constrained_delaunay_triangulation_2)
		{  }
	};
}

#endif // GPLATES_APP_LOGIC_RESOLVEDTOPOLOGICALNETWORK_H
