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

#include <list>
#include <map>
#include "types.h"
#include "maths/FiniteRotation.h"
#include "contrib/non_null_intrusive_ptr.h"
#include "property-values/GpmlPlateId.h"


// Forward declaration for intrusive-pointer.
// (We want to avoid the inclusion of "maths/PointOnSphere.h" and "maths/PolylineOnSphere.h" into
// this header file.)
namespace GPlatesMaths
{
	class PointOnSphere;
	class PolylineOnSphere;

	void
	intrusive_ptr_add_ref(
			const PointOnSphere *p);

	void
	intrusive_ptr_release(
			const PointOnSphere *p);

	void
	intrusive_ptr_add_ref(
			const PolylineOnSphere *p);

	void
	intrusive_ptr_release(
			const PolylineOnSphere *p);
}

namespace GPlatesModel
{
	/**
	 * A reconstruction tree represents the plate-reconstruction hierarchy of total
	 * reconstruction poles at an instant in time.
	 *
	 * Creating a useful tree is a three-step process:
	 * -# Instantiate ReconstructionTree.
	 * -# Populate the ReconstructionTree instance by inserting interpolated poles (@a insert
	 * function).
	 * -# Build the tree-structure (@a build_tree function), specifying the root of the tree.
	 */
	class ReconstructionTree
	{
	public:
		/**
		 * A reconstruction tree edge corresponds to a total reconstruction pole.
		 *
		 * A reconstruction tree edge is a directed edge which links two reconstruction
		 * tree vertices, each of which corresponds to a plate ID (actually: a plate
		 * identified by the plate ID).
		 */
		class ReconstructionTreeEdge
		{
		public:
			/**
			 * To enable the tree-building algorithm to construct the fullest-possible
			 * reconstruction-tree from the reconstruction-graph, edges are inserted
			 * which correspond to both the original *and* reversed poles.
			 *
			 * Thus, it will be possible to traverse reconstruction-graph edges in the
			 * reverse direction (which is what the user will expect sometimes).
			 */
			struct PoleTypes
			{
				enum PoleType {
					ORIGINAL,
					REVERSED
				};
			};

			typedef std::list<ReconstructionTreeEdge> edge_list_type;

			ReconstructionTreeEdge(
					GPlatesPropertyValues::GpmlPlateId::non_null_ptr_type fixed_plate_,
					GPlatesPropertyValues::GpmlPlateId::non_null_ptr_type moving_plate_,
					const GPlatesMaths::FiniteRotation &relative_rotation_,
					PoleTypes::PoleType pole_type_):
				d_fixed_plate(fixed_plate_),
				d_moving_plate(moving_plate_),
				d_relative_rotation(relative_rotation_),
				d_composed_absolute_rotation(relative_rotation_),
				d_pole_type(pole_type_)
			{  }

			const GPlatesPropertyValues::GpmlPlateId::non_null_ptr_to_const_type
			fixed_plate() const
			{
				return d_fixed_plate;
			}

			const GPlatesPropertyValues::GpmlPlateId::non_null_ptr_to_const_type
			moving_plate() const
			{
				return d_moving_plate;
			}

			const GPlatesMaths::FiniteRotation &
			relative_rotation() const
			{
				return d_relative_rotation;
			}

			const GPlatesMaths::FiniteRotation &
			composed_absolute_rotation() const
			{
				return d_composed_absolute_rotation;
			}

			void
			set_composed_absolute_rotation(
					const GPlatesMaths::FiniteRotation &new_rotation)
			{
				d_composed_absolute_rotation = new_rotation;
			}

			PoleTypes::PoleType
			pole_type() const
			{
				return d_pole_type;
			}

			/**
			 * When the tree-structure is being built, the elements of this list are
			 * the edges which are the "children" of this edge instance.
			 *
			 * That is, the edges in this list will be one step further away from the
			 * root of the tree than this edge instance; and the moving plate of this
			 * edge instance will be the fixed plate of each of the elements in this
			 * list.  (Every edge in this list will "hang off" this edge.)
			 */
			edge_list_type &
			tree_children()
			{
				return d_tree_children;
			}

		private:
			GPlatesPropertyValues::GpmlPlateId::non_null_ptr_type d_fixed_plate;
			GPlatesPropertyValues::GpmlPlateId::non_null_ptr_type d_moving_plate;
			GPlatesMaths::FiniteRotation d_relative_rotation;
			GPlatesMaths::FiniteRotation d_composed_absolute_rotation;
			PoleTypes::PoleType d_pole_type;

			/**
			 * When the tree-structure is being built, the elements of this list are
			 * the edges which are the "children" of this edge instance.
			 *
			 * That is, the edges in this list will be one step further away from the
			 * root of the tree than this edge instance; and the moving plate of this
			 * edge instance will be the fixed plate of each of the elements in this
			 * list.  (Every edge in this list will "hang off" this edge.)
			 */
			edge_list_type d_tree_children;
		};

		/**
		 * This type represents a list of edges of a reconstruction tree.
		 */
		typedef ReconstructionTreeEdge::edge_list_type edge_list_type;

		/**
		 * This type represents an iterator into an edge list.
		 *
		 * This type can also be used as a reference to an edge.
		 */
		typedef edge_list_type::iterator edge_list_iterator;

		/**
		 * This type represents a mapping of an integer plate ID type to an
		 * edge_list_iterator.
		 */
		typedef std::multimap<integer_plate_id_type, edge_list_iterator>
				edge_list_iters_by_plate_id_map_type;

		/**
		 * This type represents an iterator into an edge_list_iters_by_plate_id_map_type.
		 */
		typedef edge_list_iters_by_plate_id_map_type::iterator edge_ref_map_iterator;

		/**
		 * This type is intended to represent a sub-range of an
		 * edge_list_iters_by_plate_id_map_type.
		 *
		 * It is intended that the first member of the pair is the "begin" iterator of an
		 * STL container range, and the second member is the "end" iterator.
		 */
		typedef std::pair<edge_ref_map_iterator, edge_ref_map_iterator> edge_ref_map_range_type;

		ReconstructionTree()
		{  }

		// Total reconstruction poles must be inserted before the "tree" can be built.
		void
		insert_total_reconstruction_pole(
				GPlatesPropertyValues::GpmlPlateId::non_null_ptr_type fixed_plate_,
				GPlatesPropertyValues::GpmlPlateId::non_null_ptr_type moving_plate_,
				const GPlatesMaths::FiniteRotation &pole);

		bool
		tree_is_built() const
		{
			return ( ! d_rootmost_edges.empty());
		}

		// After all total reconstruction poles have been inserted, you can build the tree
		// by specifying a choice of root plate ID.
		void
		build_tree(
				integer_plate_id_type root_plate_id);

		/**
		 * Access the begin iterator of the collection of rootmost edges.
		 *
		 * Since the tree-structure is built out of the edges (total reconstruction poles),
		 * tree-traversal begins by iterating through a collection of edges, each of which
		 * has a fixed plate ID which is equal to the "root" plate ID of the tree.
		 */
		edge_list_iterator
		rootmost_edges_begin()
		{
			return d_rootmost_edges.begin();
		}

		/**
		 * Access the end iterator of the collection of rootmost edges.
		 *
		 * Since the tree-structure is built out of the edges (total reconstruction poles),
		 * tree-traversal begins by iterating through a collection of edges, each of which
		 * has a fixed plate ID which is equal to the "root" plate ID of the tree.
		 */
		edge_list_iterator
		rootmost_edges_end()
		{
			return d_rootmost_edges.end();
		}

		const boost::intrusive_ptr<GPlatesMaths::PointOnSphere>
		reconstruct_point(
				GPlatesContrib::non_null_intrusive_ptr<const GPlatesMaths::PointOnSphere> p,
				integer_plate_id_type plate_id_of_feature,
				integer_plate_id_type root_plate_id);

		const boost::intrusive_ptr<GPlatesMaths::PolylineOnSphere>
		reconstruct_polyline(
				GPlatesContrib::non_null_intrusive_ptr<const GPlatesMaths::PolylineOnSphere> p,
				integer_plate_id_type plate_id_of_feature,
				integer_plate_id_type root_plate_id);

	private:
		/**
		 * This list will be populated by the edges (total reconstruction poles) which are
		 * inserted by the clients of this class.
		 *
		 * When the tree-structure is being built, edges will be extracted from this list
		 * and spliced into the lists in edges in the tree; any edges left in this list
		 * will be unused in the tree-structure (hence the name of this data member).
		 */
		edge_list_type d_unused_edges;

		/**
		 * This list will remain empty while the reconstruction tree is being populated
		 * (ie, while edges (total reconstruction poles) are being inserted by the clients
		 * of this class); when the tree-structure is being built, this will contain the
		 * rootmost edges of the tree (ie, the edges of the tree whose fixed plate is equal
		 * to the "root" of the tree).
		 */
		edge_list_type d_rootmost_edges;

		/**
		 * This map will be used when building the tree-structure.
		 *
		 * FIXME:  What @em exactly is it used for?
		 */
		edge_list_iters_by_plate_id_map_type d_edge_refs_by_fixed_plate_id;

		/**
		 * This map will be used when the user is querying the reconstruction tree.
		 *
		 * FIXME:  What @em exactly is it used for?
		 */
		edge_list_iters_by_plate_id_map_type d_edge_refs_by_moving_plate_id;

		/**
		 * Find all edges whose fixed plate ID matches 'fixed_plate_id'.
		 *
		 * The function of this return-value (a 'std::pair') is similar to the function of
		 * the return-value of the 'multimap::equal_range' function (that is, it returns
		 * the "begin" and "end" iterators of an STL container range):  This return-value
		 * describes the range of elements in the map, each of which references an edge
		 * whose fixed plate ID is equal to the specified fixed plate ID.
		 */
		edge_ref_map_range_type
		find_edges_whose_fixed_plate_id_match(
				integer_plate_id_type fixed_plate_id);

		/**
		 * Demolish the tree-structure, to reset all the containers.
		 */
		void
		demolish_tree();

		/**
		 * Find all edges whose moving plate ID matches 'moving_plate_id'.
		 *
		 * The function of this return-value (a 'std::pair') is similar to the function of
		 * the return-value of the 'multimap::equal_range' function (that is, it returns
		 * the "begin" and "end" iterators of an STL container range):  This return-value
		 * describes the range of elements in the map, each of which references an edge
		 * whose moving plate ID is equal to the specified moving plate ID.
		 */
		edge_ref_map_range_type
		find_edges_whose_moving_plate_id_match(
				integer_plate_id_type moving_plate_id,
				integer_plate_id_type root_plate_id);

	};

}

#endif  // GPLATES_MODEL_RECONSTRUCTIONTREE_H
