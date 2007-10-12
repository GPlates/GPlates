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

#ifndef GPLATES_MODEL_RECONSTRUCTIONTREE_H
#define GPLATES_MODEL_RECONSTRUCTIONTREE_H

#include <map>  // std::multimap

#include "ReconstructionGraph.h"
#include "maths/GeometryForwardDeclarations.h"
#include "utils/non_null_intrusive_ptr.h"


namespace GPlatesModel
{
	/**
	 * A reconstruction tree represents the plate-reconstruction hierarchy of total
	 * reconstruction poles at an instant in time.
	 *
	 * A reconstruction tree is created from a ReconstructionGraph.
	 */
	class ReconstructionTree
	{
	public:
		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<ReconstructionTree>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<ReconstructionTree> non_null_ptr_type;

		/**
		 * The type used to store the reference-count of an instance of this class.
		 */
		typedef long ref_count_type;

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
		 * Create a new ReconstructionTree instance from the ReconstructionGraph instance
		 * @a graph, building a tree-structure which has @a root_plate_id as the root.
		 *
		 * Note that invoking this function will cause all total reconstruction poles in
		 * the ReconstructionGraph instance to be transferred to this instance, leaving the
		 * ReconstructionGraph instance empty (as if it had just been created).
		 */
		static
		const non_null_ptr_type
		create(
				ReconstructionGraph &graph,
				integer_plate_id_type root_plate_id_);

		/**
		 * Create a duplicate of this ReconstructionTree instance.
		 *
		 * Note that this will perform a "shallow copy".
		 */
		const non_null_ptr_type
		clone() const
		{
			non_null_ptr_type dup(*(new ReconstructionTree(*this)));
			return dup;
		}

		/**
		 * Access the begin iterator of the collection of rootmost edges.
		 *
		 * Since the tree is built out of the edges (total reconstruction poles),
		 * tree-traversal begins by iterating through a collection of edges, each of which
		 * has a fixed plate ID which is equal to the "root" plate ID of the tree.
		 */
		edge_collection_type::iterator
		rootmost_edges_begin()
		{
			return d_rootmost_edges.begin();
		}

		/**
		 * Access the end iterator of the collection of rootmost edges.
		 *
		 * Since the tree is built out of the edges (total reconstruction poles),
		 * tree-traversal begins by iterating through a collection of edges, each of which
		 * has a fixed plate ID which is equal to the "root" plate ID of the tree.
		 */
		edge_collection_type::iterator
		rootmost_edges_end()
		{
			return d_rootmost_edges.end();
		}

		edge_refs_by_plate_id_map_const_range_type
		find_edges_whose_moving_plate_id_match(
				integer_plate_id_type plate_id) const;

		edge_refs_by_plate_id_map_range_type
		find_edges_whose_moving_plate_id_match(
				integer_plate_id_type plate_id);

		const boost::intrusive_ptr<const GPlatesMaths::PointOnSphere>
		reconstruct_point(
				GPlatesUtils::non_null_intrusive_ptr<const GPlatesMaths::PointOnSphere> p,
				integer_plate_id_type plate_id_of_feature) const;

		const boost::intrusive_ptr<const GPlatesMaths::PolylineOnSphere>
		reconstruct_polyline(
				GPlatesUtils::non_null_intrusive_ptr<const GPlatesMaths::PolylineOnSphere> p,
				integer_plate_id_type plate_id_of_feature) const;

		/**
		 * Get the composed absolute rotation which describes the motion of @a
		 * moving_plate_id relative to the root plate ID
		 *
		 * If the motion of @a moving_plate_id is not described by this tree, the identity
		 * rotation will be returned.
		 */
		const GPlatesMaths::FiniteRotation
		get_composed_absolute_rotation(
				integer_plate_id_type moving_plate_id) const;

		/**
		 * Increment the reference-count of this instance.
		 *
		 * Client code should not use this function!
		 *
		 * This function is used by boost::intrusive_ptr and
		 * GPlatesUtils::non_null_intrusive_ptr.
		 */
		void
		increment_ref_count() const
		{
			++d_ref_count;
		}

		/**
		 * Decrement the reference-count of this instance, and return the new
		 * reference-count.
		 *
		 * Client code should not use this function!
		 *
		 * This function is used by boost::intrusive_ptr and
		 * GPlatesUtils::non_null_intrusive_ptr.
		 */
		ref_count_type
		decrement_ref_count() const
		{
			return --d_ref_count;
		}

	private:
		/**
		 * The reference-count of this instance by intrusive-pointers.
		 */
		mutable ref_count_type d_ref_count;

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

		integer_plate_id_type d_root_plate_id;



		/**
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 */
		explicit
		ReconstructionTree(
				integer_plate_id_type root_plate_id_,
				const double &reconstruction_time_):
			d_ref_count(0),
			d_graph(reconstruction_time_),
			d_root_plate_id(root_plate_id_)
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
			d_ref_count(0),
			d_graph(other.d_graph),
			d_edges_by_moving_plate_id(other.d_edges_by_moving_plate_id),
			d_rootmost_edges(other.d_rootmost_edges),
			d_root_plate_id(other.d_root_plate_id)
		{  }

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment.
		ReconstructionTree &
		operator=(
				const ReconstructionTree &);
	};


	inline
	void
	intrusive_ptr_add_ref(
			const ReconstructionTree *p)
	{
		p->increment_ref_count();
	}


	inline
	void
	intrusive_ptr_release(
			const ReconstructionTree *p)
	{
		if (p->decrement_ref_count() == 0) {
			delete p;
		}
	}

}

#endif  // GPLATES_MODEL_RECONSTRUCTIONTREE_H
