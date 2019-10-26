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

#include <map>  // std::multimap

#include "ReconstructionGraph.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/NullIntrusivePointerHandler.h"
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
		/**
		 * A convenience typedef for a shared pointer to non-const @a ReconstructionTree.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<ReconstructionTree> non_null_ptr_type;

		/**
		 * A convenience typedef for a shared pointer to const @a ReconstructionTree.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const ReconstructionTree> non_null_ptr_to_const_type;

		/**
		 * This type is used to reference an edge in this graph.
		 */
		typedef ReconstructionGraph::edge_ref_type edge_ref_type;

		/**
		 * This type is used to map plate IDs to edge-refs.
		 */
		typedef ReconstructionGraph::edge_refs_by_plate_id_map_type
				edge_refs_by_plate_id_map_type;

		/**
		 * This type is used to describe the number of edges in the graph.
		 */
		typedef ReconstructionGraph::size_type size_type;

		/**
		 * This type is the iterator for a map of plate IDs to edge-refs.
		 */
		typedef ReconstructionGraph::edge_refs_by_plate_id_map_iterator
				edge_refs_by_plate_id_map_iterator;

		/**
		 * This type is the const-iterator for a map of plate IDs to edge-refs.
		 */
		typedef ReconstructionGraph::edge_refs_by_plate_id_map_const_iterator
				edge_refs_by_plate_id_map_const_iterator;

		/**
		 * The purpose of this type is the same as the purpose of the return-type of the
		 * function 'multimap::equal_range' -- it contains the "begin" and "end" iterators
		 * of an STL container range.
		 */
		typedef ReconstructionGraph::edge_refs_by_plate_id_map_range_type
				edge_refs_by_plate_id_map_range_type;

		/**
		 * The purpose of this type is the same as the purpose of the return-type of the
		 * function 'multimap::equal_range' -- it contains the "begin" and "end" iterators
		 * of an STL container range.
		 *
		 * This contains const-iterators.
		 */
		typedef ReconstructionGraph::edge_refs_by_plate_id_map_const_range_type
				edge_refs_by_plate_id_map_const_range_type;

		/**
		 * The type used for collections of edges.
		 */
		typedef ReconstructionTreeEdge::edge_collection_type edge_collection_type;

		/**
		 * This is the enumeration of the circumstances which surround a reconstruction.
		 */
		enum ReconstructionCircumstance
		{
			ExactlyOnePlateIdMatchFound,
			NoPlateIdMatchesFound,
			MultiplePlateIdMatchesFound
		};

		/**
		 * Create a new ReconstructionTree instance from the ReconstructionGraph instance
		 * @a graph, building a tree-structure which has @a anchor_plate_id_ as the anchor plate.
		 *
		 * Note that invoking this function will cause all total reconstruction poles in
		 * the ReconstructionGraph instance to be transferred to this instance, leaving the
		 * ReconstructionGraph instance empty (as if it had just been created).
		 */
		static
		const non_null_ptr_type
		create(
				ReconstructionGraph &graph,
				GPlatesModel::integer_plate_id_type anchor_plate_id_,
				const double &reconstruction_time_);

		/**
		 * Create a duplicate of this ReconstructionTree instance.
		 *
		 * Note that this will perform a "shallow copy".
		 */
		const non_null_ptr_type
		clone() const
		{
			non_null_ptr_type dup(new ReconstructionTree(*this),
					GPlatesUtils::NullIntrusivePointerHandler());
			return dup;
		}
	
		edge_refs_by_plate_id_map_const_iterator
		edge_map_begin() const
		{
			return d_edges_by_moving_plate_id.begin();
		}

		edge_refs_by_plate_id_map_const_iterator
		edge_map_end() const
		{
			return d_edges_by_moving_plate_id.end();
		}

		/**
		 * Access the begin iterator of the collection of rootmost edges.
		 *
		 * Since the tree is built out of the edges (total reconstruction poles),
		 * tree-traversal begins by iterating through a collection of edges, each of which
		 * has a fixed plate ID which is equal to the "anchor" plate ID of the tree.
		 */
		edge_collection_type::const_iterator
		rootmost_edges_begin() const
		{
			return d_rootmost_edges.begin();
		}

		/**
		 * Access the end iterator of the collection of rootmost edges.
		 *
		 * Since the tree is built out of the edges (total reconstruction poles),
		 * tree-traversal begins by iterating through a collection of edges, each of which
		 * has a fixed plate ID which is equal to the "anchor" plate ID of the tree.
		 */
		edge_collection_type::const_iterator
		rootmost_edges_end() const
		{
			return d_rootmost_edges.end();
		}

		edge_refs_by_plate_id_map_const_range_type
		find_edges_whose_moving_plate_id_match(
				GPlatesModel::integer_plate_id_type plate_id) const;

		edge_refs_by_plate_id_map_range_type
		find_edges_whose_moving_plate_id_match(
				GPlatesModel::integer_plate_id_type plate_id);

		/**
		 * Get the composed absolute rotation which describes the motion of @a
		 * moving_plate_id relative to the anchor plate ID
		 *
		 * If the motion of @a moving_plate_id is not described by this tree, the identity
		 * rotation will be returned.
		 */
		const std::pair<GPlatesMaths::FiniteRotation,
				ReconstructionCircumstance>
		get_composed_absolute_rotation(
				GPlatesModel::integer_plate_id_type moving_plate_id) const;

		/**
		 * Returns the plate id of the anchor plate that all rotations are calculated
		 * relative to.
		 */
		GPlatesModel::integer_plate_id_type
		get_anchor_plate_id() const
		{
			return d_anchor_plate_id;
		}

		/**
		 * Return the reconstruction time of this tree.
		 */
		const double &
		get_reconstruction_time() const
		{
			return d_reconstruction_time;
		}

	private:

		ReconstructionGraph d_graph;

		/**
		 * This is a mapping of moving plate IDs to edge-refs.
		 *
		 * It is populated when the tree is created.
		 *
		 * It is used to reconstruct geometries and query the composed absolute rotations.
		 */
		edge_refs_by_plate_id_map_type d_edges_by_moving_plate_id;

		edge_collection_type d_rootmost_edges;

		GPlatesModel::integer_plate_id_type d_anchor_plate_id;

		/**
		 * This is the reconstruction time of the total reconstruction poles in this tree.
		 */
		double d_reconstruction_time;



		/**
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 */
		explicit
		ReconstructionTree(
				GPlatesModel::integer_plate_id_type anchor_plate_id_,
				const double &reconstruction_time_) :
			d_anchor_plate_id(anchor_plate_id_),
			d_reconstruction_time(reconstruction_time_)
		{  }

		/**
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 *
		 * This ctor should only be invoked by the 'clone' member function, which will
		 * create a duplicate instance and return a new non_null_intrusive_ptr reference to
		 * the new duplicate.  Since initially the only reference to the new duplicate will
		 * be the one returned by the 'clone' function, *before* the new intrusive-pointer
		 * is created, the ref-count of the new ReconstructionTree instance should be zero.
		 *
		 * Note that this ctor should act exactly the same as the default (auto-generated)
		 * copy-ctor, except that it should initialise the ref-count to zero.
		 */
		ReconstructionTree(
				const ReconstructionTree &other):
			GPlatesUtils::ReferenceCount<ReconstructionTree>(),
			d_graph(other.d_graph),
			d_edges_by_moving_plate_id(other.d_edges_by_moving_plate_id),
			d_rootmost_edges(other.d_rootmost_edges),
			d_anchor_plate_id(other.d_anchor_plate_id)
		{  }

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment.
		ReconstructionTree &
		operator=(
				const ReconstructionTree &);
	};
}

#endif  // GPLATES_APP_LOGIC_RECONSTRUCTIONTREE_H
