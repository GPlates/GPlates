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

#ifndef GPLATES_MODEL_RECONSTRUCTIONTREEEDGE_H
#define GPLATES_MODEL_RECONSTRUCTIONTREEEDGE_H

#include <vector>
#include <iosfwd>

#include "types.h"
#include "maths/FiniteRotation.h"
#include "utils/non_null_intrusive_ptr.h"


namespace GPlatesModel
{
	/**
	 * A reconstruction tree edge corresponds to a total reconstruction pole in a
	 * reconstruction tree.
	 *
	 * A reconstruction tree edge is a directed edge which links two reconstruction
	 * tree vertices, each of which corresponds to a plate ID (actually: a plate
	 * identified by the plate ID).
	 */
	class ReconstructionTreeEdge
	{
	public:
		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<ReconstructionTreeEdge>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<ReconstructionTreeEdge> non_null_ptr_type;

		/**
		 * The type used to store the reference-count of an instance of this class.
		 */
		typedef long ref_count_type;

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

		/**
		 * The type used for collections of edges.
		 */
		typedef std::vector<ReconstructionTreeEdge::non_null_ptr_type> edge_collection_type;

		/**
		 * Create a new ReconstructionTreeEdge instance.
		 */
		static
		const non_null_ptr_type
		create(
				GPlatesModel::integer_plate_id_type fixed_plate_,
				GPlatesModel::integer_plate_id_type moving_plate_,
				const GPlatesMaths::FiniteRotation &relative_rotation_,
				PoleTypes::PoleType pole_type_)
		{
			non_null_ptr_type ptr(
					*(new ReconstructionTreeEdge(fixed_plate_, moving_plate_,
							relative_rotation_, pole_type_)));
			return ptr;
		}

		/**
		 * Create a duplicate of this ReconstructionTreeEdge instance.
		 */
		const non_null_ptr_type
		clone() const
		{
			non_null_ptr_type dup(*(new ReconstructionTreeEdge(*this)));
			return dup;
		}

		GPlatesModel::integer_plate_id_type
		fixed_plate() const
		{
			return d_fixed_plate;
		}

		GPlatesModel::integer_plate_id_type
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
		 * When the tree-structure has been built, the elements of this set are the edges
		 * which are the "children" of this edge instance in the tree.
		 *
		 * That is, the edges in this set will be one step further away from the root of
		 * the tree than this edge instance; and the moving plate of this edge instance
		 * will be the fixed plate of each of the elements in this set.  (Every edge in
		 * this list will "hang off" this edge.)
		 */
		edge_collection_type &
		children_in_built_tree()
		{
			return d_children_in_built_tree;
		}

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

		/**
		 * The plate ID of the fixed plate of the total reconstruction pole.
		 */
		GPlatesModel::integer_plate_id_type d_fixed_plate;

		/**
		 * The plate ID of the fixed plate of the total reconstruction pole.
		 */
		GPlatesModel::integer_plate_id_type d_moving_plate;

		/**
		 * The relative finite rotation of the moving plate relative to the fixed plate.
		 */
		GPlatesMaths::FiniteRotation d_relative_rotation;

		/**
		 * The combined absolute finite rotation of the moving plate relative to the root
		 * plate of the built reconstruction tree.
		 *
		 * Note that the value of this member is not meaningful unless this edge is
		 * contained within a built ReconstructionTree instance.
		 */
		GPlatesMaths::FiniteRotation d_composed_absolute_rotation;

		/**
		 * Whether the pole (finite rotation) of this edge is the same as the original
		 * pole, or the reverse.
		 */
		PoleTypes::PoleType d_pole_type;

		/**
		 * When the tree-structure has been built, the elements of this set are the edges
		 * which are the "children" of this edge instance in the tree.
		 *
		 * That is, the edges in this set will be one step further away from the root of
		 * the tree than this edge instance; and the moving plate of this edge instance
		 * will be the fixed plate of each of the elements in this set.  (Every edge in
		 * this list will "hang off" this edge.)
		 */
		edge_collection_type d_children_in_built_tree;

		/**
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 */
		ReconstructionTreeEdge(
				GPlatesModel::integer_plate_id_type fixed_plate_,
				GPlatesModel::integer_plate_id_type moving_plate_,
				const GPlatesMaths::FiniteRotation &relative_rotation_,
				PoleTypes::PoleType pole_type_):
			d_ref_count(0),
			d_fixed_plate(fixed_plate_),
			d_moving_plate(moving_plate_),
			d_relative_rotation(relative_rotation_),
			d_composed_absolute_rotation(relative_rotation_),
			d_pole_type(pole_type_)
		{  }

		/**
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 *
		 * This ctor should only be invoked by the 'clone' member function, which will
		 * create a duplicate instance and return a new non_null_intrusive_ptr reference to
		 * the new duplicate.  Since initially the only reference to the new duplicate will
		 * be the one returned by the 'clone' function, *before* the new intrusive-pointer
		 * is created, the ref-count of the new ReconstructionTreeEdge instance should be
		 * zero.
		 *
		 * Note that this ctor should act exactly the same as the default (auto-generated)
		 * copy-ctor, except that it should initialise the ref-count to zero.
		 */
		ReconstructionTreeEdge(
				const ReconstructionTreeEdge &other):
			d_ref_count(0),
			d_fixed_plate(other.d_fixed_plate),
			d_moving_plate(other.d_moving_plate),
			d_relative_rotation(other.d_relative_rotation),
			d_composed_absolute_rotation(other.d_composed_absolute_rotation),
			d_pole_type(other.d_pole_type)
		{  }

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment.
		ReconstructionTreeEdge &
		operator=(
				const ReconstructionTreeEdge &);
	};


	/**
	 * Write the ReconstructedTreeEdge @a edge to output stream @a os in a format suitable for
	 * debugging purposes.
	 */
	void
	output_for_debugging(
			std::ostream &os,
			const ReconstructionTreeEdge &edge);


	inline
	void
	intrusive_ptr_add_ref(
			const ReconstructionTreeEdge *p)
	{
		p->increment_ref_count();
	}


	inline
	void
	intrusive_ptr_release(
			const ReconstructionTreeEdge *p)
	{
		if (p->decrement_ref_count() == 0) {
			delete p;
		}
	}

}

#endif  // GPLATES_MODEL_RECONSTRUCTIONTREEEDGE_H
