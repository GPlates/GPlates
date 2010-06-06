/* $Id$ */

/**
 * \file 
 * Contains the definition of the class ReconstructionGraph.
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

#ifndef GPLATES_APP_LOGIC_RECONSTRUCTIONGRAPH_H
#define GPLATES_APP_LOGIC_RECONSTRUCTIONGRAPH_H

#include <map>  // std::multimap
#include <vector>
#include <algorithm>  // std::swap
#include <utility>  // std::pair

#include "ReconstructionTreeEdge.h"

#include "model/FeatureCollectionHandle.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/NullIntrusivePointerHandler.h"


namespace GPlatesAppLogic
{
	// Forward declaration for intrusive-pointer.  ReconstructionTree needs to see the whole
	// definition of ReconstructionGraph (since a ReconstructionGraph will be contained within
	// a ReconstructionTree) so this header cannot #include the header for ReconstructionTree.
	class ReconstructionTree;


	/**
	 * A reconstruction graph is a collection of reconstruction tree edges.
	 *
	 * By specifying a root plate ID, you can build a reconstruction graph into a
	 * ReconstructionTree.
	 *
	 * Building a reconstruction tree is a three-step process:
	 * -# Instantiate ReconstructionGraph.
	 * -# Populate the ReconstructionGraph instance by inserting interpolated poles (function
	 * @a insert_total_reconstruction_pole).
	 * -# Build the reconstruction tree (function @a build_tree), specifying the root of the
	 * tree.
	 */
	class ReconstructionGraph
	{
	public:
		/**
		 * This type is used to reference an edge in this graph.
		 */
		typedef ReconstructionTreeEdge::non_null_ptr_type edge_ref_type;

		/**
		 * This type is used to map plate IDs to edge-refs.
		 */
		typedef std::multimap<GPlatesModel::integer_plate_id_type, edge_ref_type>
				edge_refs_by_plate_id_map_type;

		/**
		 * This type is used to describe the number of edges in the graph.
		 */
		typedef edge_refs_by_plate_id_map_type::size_type size_type;

		/**
		 * This type is the iterator for a map of plate IDs to edge-refs.
		 */
		typedef edge_refs_by_plate_id_map_type::iterator edge_refs_by_plate_id_map_iterator;

		/**
		 * This type is the const-iterator for a map of plate IDs to edge-refs.
		 */
		typedef edge_refs_by_plate_id_map_type::const_iterator edge_refs_by_plate_id_map_const_iterator;

		/**
		 * The purpose of this type is the same as the purpose of the return-type of the
		 * function 'multimap::equal_range' -- it contains the "begin" and "end" iterators
		 * of an STL container range.
		 */
		typedef std::pair<edge_refs_by_plate_id_map_iterator, edge_refs_by_plate_id_map_iterator>
				edge_refs_by_plate_id_map_range_type;

		/**
		 * The purpose of this type is the same as the purpose of the return-type of the
		 * function 'multimap::equal_range' -- it contains the "begin" and "end" iterators
		 * of an STL container range.
		 *
		 * This contains const-iterators.
		 */
		typedef std::pair<edge_refs_by_plate_id_map_const_iterator, edge_refs_by_plate_id_map_const_iterator>
				edge_refs_by_plate_id_map_const_range_type;

		/**
		 * Create a reconstruction graph for the specified @a reconstruction_time_.
		 *
		 * This reconstruction graph will be empty -- that is, will not contain any total
		 * reconstruction poles.
		 *
		 * @a reconstruction_features are the features that will be used to fill this graph.
		 * This can be used to find which ReconstructionTrees were generated using the same
		 * set of reconstruction features and can also be used to find the set of reconstruction
		 * features used to reconstruct specific geometries so that those reconstruction features
		 * can be modified (such as inserting new rotation poles).
		 */
		explicit
		ReconstructionGraph(
				const double &reconstruction_time_,
				const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &reconstruction_features_):
			d_reconstruction_time(reconstruction_time_),
			d_reconstruction_features(reconstruction_features_)
		{  }

		/**
		 * Return the reconstruction time of this graph.
		 *
		 * This function does not throw.
		 */
		const double &
		reconstruction_time() const
		{
			return d_reconstruction_time;
		}

		/**
		 * Return the number of edges contained in this graph.
		 *
		 * This function does not throw.
		 */
		size_type
		num_edges_contained() const
		{
			return d_edges_by_fixed_plate_id.size();
		}

		/**
		 * Access the features containing the reconstruction features used to
		 * create the total reconstruction pole inserted into this graph via
		 * @a insert_total_reconstruction_pole.
		 */
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &
		get_reconstruction_features() const
		{
			return d_reconstruction_features;
		}

		/**
		 * Insert a total reconstruction pole for this reconstruction time.
		 *
		 * This will result in the creation of reconstruction tree edges, which will be
		 * added to the graph.
		 *
		 * For maximum computational efficiency, you should insert all relevant total
		 * reconstruction poles before building the tree.
		 *
		 * This function is strongly exception-safe and exception-neutral.
		 */
		void
		insert_total_reconstruction_pole(
				GPlatesModel::integer_plate_id_type fixed_plate_id_,
				GPlatesModel::integer_plate_id_type moving_plate_id_,
				const GPlatesMaths::FiniteRotation &pole);

		/**
		 * Build the graph into a ReconstructionTree, specifying the @a root_plate_id of
		 * the tree.
		 *
		 * Note that invoking this function will cause all total reconstruction poles in
		 * this graph to be transferred to the new ReconstructionTree instance, leaving
		 * this graph empty (as if it had just been created).
		 *
		 * This function is strongly exception-safe and exception-neutral.
		 */
		GPlatesUtils::non_null_intrusive_ptr<ReconstructionTree>
		build_tree(
				GPlatesModel::integer_plate_id_type root_plate_id);

		/**
		 * Find all edges in this graph whose fixed plate ID matches @a plate_id.
		 *
		 * The return-value is a typedef for a 'std::pair'.  Its purpose is the same as the
		 * purpose of the return-type of the function 'multimap::equal_range' -- it
		 * contains the "begin" and "end" iterators of an STL container range.
		 *
		 * This function does not throw.
		 */
		edge_refs_by_plate_id_map_const_range_type
		find_edges_whose_fixed_plate_id_match(
				GPlatesModel::integer_plate_id_type plate_id) const;

		/**
		 * Find all edges in this graph whose fixed plate ID matches @a plate_id.
		 *
		 * The return-value is a typedef for a 'std::pair'.  Its purpose is the same as the
		 * purpose of the return-type of the function 'multimap::equal_range' -- it
		 * contains the "begin" and "end" iterators of an STL container range.
		 *
		 * This function does not throw.
		 */
		edge_refs_by_plate_id_map_range_type
		find_edges_whose_fixed_plate_id_match(
				GPlatesModel::integer_plate_id_type plate_id);

		/**
		 * Swap the contents of this graph with @a other.
		 *
		 * This function does not throw.
		 */
		void
		swap(
				ReconstructionGraph &other)
		{
			d_edges_by_fixed_plate_id.swap(other.d_edges_by_fixed_plate_id);
			std::swap(d_reconstruction_time, other.d_reconstruction_time);
			std::swap(d_reconstruction_features, other.d_reconstruction_features);
		}

		/**
		 * Copy-assign the value of @a other to this.
		 *
		 * This function is strongly exception-safe and exception-neutral.
		 *
		 * This copy-assignment operator should act exactly the same as the default
		 * (auto-generated) copy-assignment operator (except that we can guarantee that
		 * this operator will be strongly exception-safe).
		 */
		ReconstructionGraph &
		operator=(
				const ReconstructionGraph &other)
		{
			// Use the copy+swap idiom to enable strong exception safety.
			ReconstructionGraph dup(other);
			this->swap(dup);
			return *this;
		}

	private:
		/**
		 * This is a mapping of fixed plate IDs to edge-refs.
		 *
		 * It is populated when total reconstruction poles are inserted.
		 *
		 * It is used when building the reconstruction tree:  Starting with the root plate
		 * ID, a tree is built recursively by finding edges whose fixed plate ID matches
		 * the root plate ID; examining the moving plate IDs of each of those edges; and
		 * for each of these moving plate IDs, finding edges whose fixed plate ID matches
		 * the current (moving) plate ID.
		 */
		edge_refs_by_plate_id_map_type d_edges_by_fixed_plate_id;

		/**
		 * This is the reconstruction time of the total reconstruction poles in this graph.
		 */
		double d_reconstruction_time;

		/**
		 * The features used to generate the total reconstruction poles inserted into this graph.
		 */
		std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> d_reconstruction_features;
	};
}


namespace std
{
	/**
	 * This is a template specialisation of the standard function @a swap.
	 *
	 * See Josuttis, section 4.4.2, "Swapping Two Values" for more information.
	 */
	template<>
	inline
	void
	swap<GPlatesAppLogic::ReconstructionGraph>(
			GPlatesAppLogic::ReconstructionGraph &rg1,
			GPlatesAppLogic::ReconstructionGraph &rg2)
	{
		rg1.swap(rg2);
	}
}

#endif  // GPLATES_APP_LOGIC_RECONSTRUCTIONGRAPH_H
