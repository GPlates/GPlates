/* $Id$ */

/**
 * \file 
 * File specific comments.
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
#include "GpmlPlateId.h"
#include "maths/FiniteRotation.h"
#include "contrib/non_null_intrusive_ptr.h"


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
	 */
	class ReconstructionTree
	{
	public:
		class ReconstructionTreeNode
		{
		public:
			/**
			 * To enable the tree-building algorithm to construct the fullest-possible
			 * rotation-tree from the rotation-graph, both the original *and* reversed
			 * poles are inserted.
			 *
			 * Thus, it will be possible to traverse rotation-graph edges in the
			 * reverse direction (which is what the user will expect sometimes).
			 */
			struct PoleTypes
			{
				enum PoleType {
					ORIGINAL,
					REVERSED
				};
			};

			typedef std::list<ReconstructionTreeNode> node_list_type;

			ReconstructionTreeNode(
					boost::intrusive_ptr<GpmlPlateId> fixed_plate_,
					boost::intrusive_ptr<GpmlPlateId> moving_plate_,
					const GPlatesMaths::FiniteRotation &relative_rotation_,
					PoleTypes::PoleType pole_type_):
				d_fixed_plate(fixed_plate_),
				d_moving_plate(moving_plate_),
				d_relative_rotation(relative_rotation_),
				d_composed_absolute_rotation(relative_rotation_),
				d_pole_type(pole_type_)
			{  }

			const boost::intrusive_ptr<const GpmlPlateId>
			fixed_plate() const
			{
				return d_fixed_plate;
			}

			const boost::intrusive_ptr<const GpmlPlateId>
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
					const GPlatesMaths::FiniteRotation &new_car)
			{
				d_composed_absolute_rotation = new_car;
			}

			PoleTypes::PoleType
			pole_type() const
			{
				return d_pole_type;
			}

			node_list_type &
			tree_children()
			{
				return d_tree_children;
			}

		private:
			boost::intrusive_ptr<GpmlPlateId> d_fixed_plate;
			boost::intrusive_ptr<GpmlPlateId> d_moving_plate;
			GPlatesMaths::FiniteRotation d_relative_rotation;
			GPlatesMaths::FiniteRotation d_composed_absolute_rotation;
			PoleTypes::PoleType d_pole_type;
			node_list_type d_tree_children;
		};

		typedef ReconstructionTreeNode::node_list_type node_list_type;
		typedef node_list_type::iterator list_node_reference_type;
		typedef std::multimap<GpmlPlateId::integer_plate_id_type, list_node_reference_type>
				list_nodes_by_plate_id_map_type;
		typedef list_nodes_by_plate_id_map_type::iterator map_iterator;

		ReconstructionTree()
		{  }

		// Assumes both @a fixed_plate_ and @a moving_plate_ are non-NULL.
		// Total reconstruction poles must be inserted before the "tree" can be built.
		void
		insert_total_reconstruction_pole(
				boost::intrusive_ptr<GpmlPlateId> fixed_plate_,
				boost::intrusive_ptr<GpmlPlateId> moving_plate_,
				const GPlatesMaths::FiniteRotation &pole);

		bool
		tree_is_built() const
		{
			return ( ! d_rootmost_nodes.empty());
		}

		// After all total reconstruction poles have been inserted, you can build the tree
		// by specifying a choice of root plate ID.
		void
		build_tree(
				GpmlPlateId::integer_plate_id_type root_plate_id);

		/**
		 * Access the begin iterator of the collection of rootmost nodes.
		 *
		 * Since the "tree" is built out of total reconstruction poles, it is actually
		 * storing tree-edges rather than tree-nodes.  Hence, the top of the tree will
		 * actually be a collection of poles, each of which will have a fixed plate ID
		 * equal to the "root" plate ID of the "tree".
		 *
		 * ("tree" is in quotes because it needs a better (less overloaded) name.)
		 */
		list_node_reference_type
		rootmost_nodes_begin()
		{
			return d_rootmost_nodes.begin();
		}

		list_node_reference_type
		rootmost_nodes_end()
		{
			return d_rootmost_nodes.end();
		}

		const boost::intrusive_ptr<GPlatesMaths::PointOnSphere>
		reconstruct_point(
				GPlatesContrib::non_null_intrusive_ptr<const GPlatesMaths::PointOnSphere> p,
				GpmlPlateId::integer_plate_id_type plate_id_of_feature,
				GpmlPlateId::integer_plate_id_type root_plate_id);

		const boost::intrusive_ptr<GPlatesMaths::PolylineOnSphere>
		reconstruct_polyline(
				GPlatesContrib::non_null_intrusive_ptr<const GPlatesMaths::PolylineOnSphere> p,
				GpmlPlateId::integer_plate_id_type plate_id_of_feature,
				GpmlPlateId::integer_plate_id_type root_plate_id);

	private:
		/*
		 * This list will be populated by the total reconstruction poles which are
		 * inserted.  When the "tree" is being built, nodes will be extracted from this
		 * list and spliced into the lists in the "tree"; any nodes left over will be
		 * "unused".
		 */
		node_list_type d_unused_nodes;

		/*
		 * This list will remain empty while the reconstruction tree is being populated
		 * (ie, while total reconstruction poles are being inserted); when the "tree" is
		 * being built, this will contain the rootmost nodes of the tree (ie, the nodes of
		 * the tree whose fixed ref-frame is equal to the 'root' of the tree).
		 */
		node_list_type d_rootmost_nodes;

		/*
		 * This map will be used when building the "tree".
		 */
		list_nodes_by_plate_id_map_type d_list_nodes_by_fixed_plate_id;

		/*
		 * This map will be used when the user is querying the "tree".
		 */
		list_nodes_by_plate_id_map_type d_list_nodes_by_moving_plate_id;

		/*
		 * Find all nodes whose fixed plate ID matches 'fixed_plate_id'.
		 *
		 * The purpose of the pair is similar to the purpose of the return-value of the
		 * 'multimap::equal_range' member function, but it is a range of elements which
		 * reference poles whose fixed plate ID is equal to the specified fixed plate ID.
		 */
		std::pair<map_iterator, map_iterator>
		find_nodes_whose_fixed_plate_id_match(
				GpmlPlateId::integer_plate_id_type fixed_plate_id);

		/*
		 * Demolish the tree, to reset all the containers.
		 */
		void
		demolish_tree();

		/*
		 * Find all nodes whose moving plate ID matches 'moving_plate_id'.
		 *
		 * The purpose of the pair is similar to the purpose of the return-value of the
		 * 'multimap::equal_range' member function, but it is a range of elements which
		 * reference poles whose moving plate ID is equal to the specified moving plate ID.
		 */
		std::pair<map_iterator, map_iterator>
		find_nodes_whose_moving_plate_id_match(
				GpmlPlateId::integer_plate_id_type moving_plate_id,
				GpmlPlateId::integer_plate_id_type root_plate_id);

	};

}

#endif  // GPLATES_MODEL_RECONSTRUCTIONTREE_H
