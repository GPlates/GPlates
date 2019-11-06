/* $Id$ */

/**
 * \file 
 * Contains the definition of the class ReconstructionTree.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007 The University of Sydney, Australia
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

#ifndef GPLATES_APP_LOGIC_RECONSTRUCTIONTREE_H
#define GPLATES_APP_LOGIC_RECONSTRUCTIONTREE_H

#include <map>
#include <boost/intrusive/slist.hpp>
#include <boost/optional.hpp>
#include <boost/pool/object_pool.hpp>

#include "ReconstructionGraph.h"

#include "maths/FiniteRotation.h"

#include "model/types.h"

#include "property-values/GeoTimeInstant.h"

#include "utils/ReferenceCount.h"


namespace GPlatesAppLogic
{
	/**
	 * A reconstruction tree represents the plate-reconstruction hierarchy of total
	 * reconstruction poles at an instant in time.
	 *
	 * A reconstruction tree is created from a ReconstructionGraph.
	 */
	class ReconstructionTree :
			public GPlatesUtils::ReferenceCount<ReconstructionTree>
	{
	public:

		typedef GPlatesUtils::non_null_intrusive_ptr<ReconstructionTree> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const ReconstructionTree> non_null_ptr_to_const_type;


		// Some setup needed for an intrusive list of edges.
		typedef boost::intrusive::slist_base_hook<
				// Turn off safe linking (the default link mode) because it asserts if we destroy
				// an edge before removing it from a list (eg, a child list of a parent edge).
				// Since we're destroying the entire tree in one go, we don't want to go to the
				// extra (unnecessary) effort of removing edges from lists before destroying them...
				//
				// NOTE: This should be changed back to 'safe_link' if debugging a problem though...
				boost::intrusive::link_mode<boost::intrusive::normal_link> >
						edge_list_base_hook_type;
		class Edge;
		//! Typedef for a list of edges.
		typedef boost::intrusive::slist<Edge, boost::intrusive::base_hook<edge_list_base_hook_type> >
						edge_list_type;


		/**
		 * Represents the relative rotation from a fixed plate to a moving plate.
		 */
		class Edge :
				// Using an intrusive list avoids extra memory allocations and is generally faster...
				public edge_list_base_hook_type
		{
		public:

			GPlatesModel::integer_plate_id_type
			get_fixed_plate() const
			{
				return d_fixed_plate;
			}

			GPlatesModel::integer_plate_id_type
			get_moving_plate() const
			{
				return d_moving_plate;
			}

			/**
			 * Returns true if the direction of this tree edge (from @a get_fixed_plate to @a get_moving_plate)
			 * is the opposite of the associated @a ReconstructionGraph edge.
			 *
			 * In other words, if this tree edge is reversed compared to the total reconstruction pole
			 * in the rotation features or files (used to build the reconstruction graph).
			 */
			bool
			is_reversed() const
			{
				// We're reversed if the fixed/moving plate IDs are swapped.
				return d_moving_plate == d_graph_edge.get_fixed_plate().get_plate_id();
			}

			/**
			 * Return the parent edge, or NULL if there is no parent edge (if this is a root edge).
			 */
			const Edge *
			get_parent_edge() const
			{
				return d_parent_edge;
			}

			/**
			 * Return the "children" of this edge instance in the tree.
			 *
			 * That is, these edges will be one step further away from the root (anchor) of the
			 * tree than 'this' edge; and the moving plate of this edge instance will be the
			 * fixed plate of each child edge (every child edge will "hang off" this edge).
			 */
			const edge_list_type &
			get_child_edges() const
			{
				return d_child_edges;
			}

			/**
			 * Return the relative rotation describing the motion of our moving plate
			 * relative to our fixed plate.
			 */
			const GPlatesMaths::FiniteRotation &
			get_relative_rotation() const
			{
				if (!d_relative_rotation)
				{
					cache_relative_rotation();
				}

				return d_relative_rotation.get();
			}

			/**
			 * Get the composed absolute rotation describing the motion of our moving plate
			 * relative to the anchor plate.
			 */
			const GPlatesMaths::FiniteRotation &
			get_composed_absolute_rotation() const
			{
				if (!d_composed_absolute_rotation)
				{
					cache_composed_absolute_rotation();
				}

				return d_composed_absolute_rotation.get();
			}

		private:

			friend class ReconstructionTree;
			friend class boost::object_pool<Edge>;  // Access to Edge constructor.

			Edge(
					GPlatesModel::integer_plate_id_type fixed_plate,
					GPlatesModel::integer_plate_id_type moving_plate,
					const GPlatesPropertyValues::GeoTimeInstant &reconstruction_time_instant,
					const ReconstructionGraph::Edge &graph_edge) :
				d_fixed_plate(fixed_plate),
				d_moving_plate(moving_plate),
				d_reconstruction_time_instant(reconstruction_time_instant),
				d_graph_edge(graph_edge),
				d_parent_edge(NULL)
			{  }

			/**
			 * Calculates the relative rotation of the associated *graph* edge.
			 *
			 * Note that the *tree* edge may reverse this (if @a is_reversed returns true).
			 */
			GPlatesMaths::FiniteRotation
			calculate_graph_edge_relative_rotation() const;

			void
			cache_relative_rotation() const
			{
				d_relative_rotation = calculate_graph_edge_relative_rotation();

				// Reverse the relative rotation if we are reversed wrt the *graph* edge.
				if (is_reversed())
				{
					d_relative_rotation = GPlatesMaths::get_reverse(d_relative_rotation.get());
				}
			}

			void
			cache_composed_absolute_rotation() const;


			GPlatesModel::integer_plate_id_type d_fixed_plate;
			GPlatesModel::integer_plate_id_type d_moving_plate;

			/**
			 * The reconstruction time of the tree.
			 *
			 * This is needed when calculating the relative rotation.
			 */
			GPlatesPropertyValues::GeoTimeInstant d_reconstruction_time_instant;

			/**
			 * Reference to associated edge in ReconstructionGraph.
			 *
			 * The reconstruction graph edge contains the time sequence of finite rotations
			 * whereas we only contain the finite rotation at the reconstruction time of our tree.
			 *
			 * Note: The graph edge should remain alive as long as we're alive because we're owned
			 * by ReconstructionTree which has a shared reference to ReconstructionGraph which owns
			 * the graph edges. So we don't need to worry about a dangling reference here.
			 */
			const ReconstructionGraph::Edge &d_graph_edge;

			//! Reference to sole parent edge (is NULL for anchor plate edges).
			Edge *d_parent_edge;
			//! List of child edges.
			edge_list_type d_child_edges;

			// We only calculate these when needed...
			mutable boost::optional<GPlatesMaths::FiniteRotation> d_relative_rotation;
			mutable boost::optional<GPlatesMaths::FiniteRotation> d_composed_absolute_rotation;
		};


		//! Typedef for mapping moving plate IDs to @a Edge objects.
		typedef std::map<GPlatesModel::integer_plate_id_type, const Edge *> edge_map_type;


		/**
		 * Create a new ReconstructionTree instance from the ReconstructionGraph instance
		 * @a graph, building a tree-structure which has @a anchor_plate_id as the anchor plate.
		 */
		static
		const non_null_ptr_type
		create(
				ReconstructionGraph::non_null_ptr_to_const_type reconstruction_graph,
				const double &reconstruction_time,
				GPlatesModel::integer_plate_id_type anchor_plate_id);

		/**
		 * Return the @a ReconstructionGraph that this reconstruction tree was created from.
		 *
		 * This enables other reconstruction trees to be created at different reconstruction times.
		 */
		ReconstructionGraph::non_null_ptr_to_const_type
		get_reconstruction_graph() const
		{
			return d_reconstruction_graph;
		}

		/**
		 * Returns the plate id of the anchor plate that all rotations are calculated relative to.
		 */
		GPlatesModel::integer_plate_id_type
		get_anchor_plate_id() const
		{
			return d_anchor_plate_id;
		}

		/**
		 * Return the reconstruction time of this tree.
		 */
		double
		get_reconstruction_time() const
		{
			return d_reconstruction_time_instant.value();
		}
	
		/**
		 * Return all edges.
		 *
		 * Maps moving plate IDs to pointers to const @a Edge objects.
		 */
		const edge_map_type &
		get_all_edges() const
		{
			return d_all_edges;
		}

		/**
		 * Return edges of anchor plate (edges whose fixed plate ID equals the anchor plate ID).
		 *
		 * Since the tree is built out of the edges (total reconstruction poles),
		 * tree-traversal begins by iterating through a collection of edges, each of which
		 * has a fixed plate ID which is equal to the "anchor" plate ID of the tree.
		 */
		const edge_list_type &
		get_anchor_plate_edges() const
		{
			return d_anchor_plate_edges;
		}


		/**
		 * Return the @a Edge associated with the specified moving plate ID
		 * (or none if this tree does not contain the moving plate ID).
		 */
		boost::optional<const Edge &>
		get_edge(
				GPlatesModel::integer_plate_id_type moving_plate_id) const
		{
			edge_map_type::const_iterator edge_iter = d_all_edges.find(moving_plate_id);
			if (edge_iter == d_all_edges.end())
			{
				return boost::none;
			}

			return *edge_iter->second;
		}

		/**
		 * Get the composed absolute rotation which describes the motion of @a
		 * moving_plate_id relative to the anchor plate ID.
		 *
		 * If the motion of @a moving_plate_id is not described by this tree, the identity
		 * rotation will be returned.
		 */
		GPlatesMaths::FiniteRotation
		get_composed_absolute_rotation(
				GPlatesModel::integer_plate_id_type moving_plate_id) const
		{
			boost::optional<const Edge &> edge = get_edge(moving_plate_id);
			if (!edge)
			{
				return GPlatesMaths::FiniteRotation::create_identity_rotation();
			}

			return edge->get_composed_absolute_rotation();
		}

		/**
		 * Same as @a get_composed_absolute_rotation except returns boost::none if
		 * the motion of @a moving_plate_id is not described by this tree.
		 */
		boost::optional<GPlatesMaths::FiniteRotation>
		get_composed_absolute_rotation_or_none(
				GPlatesModel::integer_plate_id_type moving_plate_id) const
		{
			boost::optional<const Edge &> edge = get_edge(moving_plate_id);
			if (!edge)
			{
				return boost::none;
			}

			return edge->get_composed_absolute_rotation();
		}

	private:

		/**
		 * We maintain a shared reference to the graph since we reference its graph nodes and edges
		 * (because we build the absolute rotations at each plate ID as needed, as an optimisation).
		 */
		ReconstructionGraph::non_null_ptr_to_const_type d_reconstruction_graph;

		/**
		 * This is the reconstruction time of the total reconstruction poles in this tree.
		 */
		GPlatesPropertyValues::GeoTimeInstant d_reconstruction_time_instant;

		/**
		 * The anchor (root-most) plate of this reconstruction tree.
		 */
		GPlatesModel::integer_plate_id_type d_anchor_plate_id;

		/**
		 * Storage for the edges.
		 */
		boost::object_pool<Edge> d_edge_pool;

		/**
		* Edges whose fixed plate ID equals the anchor plate ID.
		*/
		edge_list_type d_anchor_plate_edges;

		/**
		 * This is a mapping of moving plate IDs to edges.
		 */
		edge_map_type d_all_edges;


		/**
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 */
		ReconstructionTree(
				ReconstructionGraph::non_null_ptr_to_const_type reconstruction_graph,
				const GPlatesPropertyValues::GeoTimeInstant &reconstruction_time_instant,
				GPlatesModel::integer_plate_id_type anchor_plate_id) :
			d_reconstruction_graph(reconstruction_graph),
			d_reconstruction_time_instant(reconstruction_time_instant),
			d_anchor_plate_id(anchor_plate_id)
		{  }

		/**
		 * Create zero, one or more sub-trees emanating from a plate.
		 *
		 * Each outgoing edge (and at most one incoming edge) can generate its own sub-tree.
		 */
		void
		create_sub_trees_from_graph_plate(
				const ReconstructionGraph::Plate &graph_plate,
				Edge *parent_tree_edge,
				edge_list_type &tree_edges);

		/**
		 * Create a sub-tree by following the specified graph edge in the forward direction
		 * (or reverse direction if @a reverse_tree_edge is true).
		 *
		 * But only create a tree edge if the graph edge contains the reconstruction time and
		 * if a new tree edge does not create a cycle in the reconstruction tree.
		 *
		 * Returns true if an edge was created.
		 */
		bool
		create_sub_tree_from_graph_edge(
				const ReconstructionGraph::Edge &graph_edge,
				Edge *parent_tree_edge,
				edge_list_type &tree_edges,
				bool reverse_tree_edge);
	};
}

#endif  // GPLATES_APP_LOGIC_RECONSTRUCTIONTREE_H
