/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
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

#include <cstddef>
#include <iterator>
#include <vector>
#include <boost/cast.hpp>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>

#include "PyReconstructionTree.h"

#include "PyGPlatesModule.h"
#include "PythonConverterUtils.h"

#include "global/GPlatesAssert.h"
#include "global/python.h"
// This is not included by <boost/python.hpp>.
// Also we must include this after <boost/python.hpp> which means after "global/python.h".
#include <boost/python/stl_iterator.hpp>

#include "app-logic/ReconstructionGraph.h"
#include "app-logic/ReconstructionTree.h"
#include "app-logic/ReconstructionTreeCreator.h"
#include "app-logic/ReconstructionTreeEdge.h"

#include "maths/MathsUtils.h"

#include "model/FeatureCollectionHandle.h"
#include "model/types.h"


#if !defined(GPLATES_NO_PYTHON)

namespace bp = boost::python;


namespace GPlatesApi
{
	/**
	 * Wrapper class for edges in the reconstruction tree.
	 *
	 * The purpose of this wrapper is to keep the reconstruction tree, associated with an edge,
	 * alive so that traversal to other edges does not dereference non-existent edges.
	 * The reconstruction tree is what keeps all its edges alive.
	 */
	class ReconstructionTreeEdge
	{
	public:

		ReconstructionTreeEdge(
				GPlatesAppLogic::ReconstructionTree::non_null_ptr_type reconstruction_tree,
				GPlatesAppLogic::ReconstructionTreeEdge::non_null_ptr_type edge) :
			d_reconstruction_tree(reconstruction_tree),
			d_edge(edge)
		{  }

		GPlatesModel::integer_plate_id_type
		get_fixed_plate_id() const
		{
			return d_edge->fixed_plate();
		}

		GPlatesModel::integer_plate_id_type
		get_moving_plate_id() const
		{
			return d_edge->moving_plate();
		}

		const GPlatesMaths::FiniteRotation &
		get_relative_total_rotation() const
		{
			return d_edge->relative_rotation();
		}

		const GPlatesMaths::FiniteRotation &
		get_equivalent_total_rotation() const
		{
			return d_edge->composed_absolute_rotation();
		}

		// Helper methods...

		GPlatesAppLogic::ReconstructionTree::non_null_ptr_type
		get_reconstruction_tree() const
		{
			return d_reconstruction_tree;
		}

		GPlatesAppLogic::ReconstructionTreeEdge::non_null_ptr_type
		get_edge() const
		{
			return d_edge;
		}

	private:
		// Keep the reconstruction tree alive while we're referencing it so not left with dangling pointer.
		GPlatesAppLogic::ReconstructionTree::non_null_ptr_type d_reconstruction_tree;

		// The edge (also owned by the reconstruction tree).
		GPlatesAppLogic::ReconstructionTreeEdge::non_null_ptr_type d_edge;
	};


	/**
	 * Wrapper class for functions accessing a sequence of reconstruction tree edges.
	 *
	 * An iterator can be obtained from this view and this view supports indexing.
	 *
	 * Another purpose of this wrapper is to keep the reconstruction tree, associated with the sequence
	 * of edges, alive so that traversal to other edges does not dereference non-existent edges.
	 * The reconstruction tree is what keeps all its edges alive.
	 */
	template <
			class EdgeIteratorType,
			// Nontype template parameter is a function pointer that takes an iterator and returns an edge...
			GPlatesAppLogic::ReconstructionTreeEdge::non_null_ptr_type
					(*edge_from_iterator_unary_func)(EdgeIteratorType)>
	class ReconstructionTreeEdgeSequenceView
	{
	public:

		ReconstructionTreeEdgeSequenceView(
				GPlatesAppLogic::ReconstructionTree::non_null_ptr_type reconstruction_tree,
				EdgeIteratorType edges_begin,
				EdgeIteratorType edges_end) :
			d_reconstruction_tree(reconstruction_tree),
			d_edges_begin(edges_begin),
			d_edges_end(edges_end),
			d_num_edges(std::distance(edges_begin, edges_end))
		{  }


		/**
		 * We don't use bp::iterator because we need to convert
		 * 'GPlatesAppLogic::ReconstructionTreeEdge::non_null_ptr_type' to its equivalent wrapper
		 * 'GPlatesApi::ReconstructionTreeEdge'.
		 */
		class Iterator
		{
		public:

			Iterator(
					GPlatesAppLogic::ReconstructionTree::non_null_ptr_type reconstruction_tree,
					EdgeIteratorType edges_begin,
					EdgeIteratorType edges_end) :
				d_reconstruction_tree(reconstruction_tree),
				d_edges_iter(edges_begin),
				d_edges_end(edges_end)
			{  }

			Iterator &
			self()
			{
				return *this;
			}

			ReconstructionTreeEdge
			next()
			{
				if (d_edges_iter == d_edges_end)
				{
					PyErr_SetString(PyExc_StopIteration, "No more data.");
					boost::python::throw_error_already_set();
				}

				GPlatesAppLogic::ReconstructionTreeEdge::non_null_ptr_type edge =
						edge_from_iterator_unary_func(d_edges_iter);
				++d_edges_iter;
				return ReconstructionTreeEdge(d_reconstruction_tree, edge);
			}

		private:
			// Keep the reconstruction tree alive while we're referencing it so not left with dangling pointer.
			GPlatesAppLogic::ReconstructionTree::non_null_ptr_type d_reconstruction_tree;

			// The range of edges (owned by the reconstruction tree).
			EdgeIteratorType d_edges_iter;
			EdgeIteratorType d_edges_end;
		};


		Iterator
		get_iter() const
		{
			return Iterator(d_reconstruction_tree, d_edges_begin, d_edges_end);
		}

		std::size_t
		get_len() const
		{
			return d_num_edges;
		}

		//
		// Support for "__get_item__".
		//
		boost::python::object
		get_item(
				boost::python::object i) const
		{
			// Note: not supporting slices - only integer indices.

			// See if the index is an integer.
			bp::extract<long> extract_index(i);
			if (extract_index.check())
			{
				long index = extract_index();
				if (index < 0)
				{
					index += d_num_edges;
				}

				if (index >= boost::numeric_cast<long>(d_num_edges) ||
					index < 0)
				{
					PyErr_SetString(PyExc_IndexError, "Index out of range");
					bp::throw_error_already_set();
				}

				EdgeIteratorType iter = d_edges_begin;
				std::advance(iter, index);
				GPlatesAppLogic::ReconstructionTreeEdge::non_null_ptr_type edge =
						edge_from_iterator_unary_func(iter);

				return bp::object(ReconstructionTreeEdge(d_reconstruction_tree, edge));
			}

			PyErr_SetString(PyExc_TypeError, "Invalid index type");
			bp::throw_error_already_set();

			return bp::object();
		}

	private:
		// Keep the reconstruction tree alive while we're referencing it so not left with dangling pointer.
		GPlatesAppLogic::ReconstructionTree::non_null_ptr_type d_reconstruction_tree;

		// The range of edges (owned by the reconstruction tree).
		EdgeIteratorType d_edges_begin;
		EdgeIteratorType d_edges_end;

		std::size_t d_num_edges;
	};


	template <typename IteratorType>
	GPlatesAppLogic::ReconstructionTreeEdge::non_null_ptr_type
	edge_from_vector_iterator(
			IteratorType iter)
	{
		return *iter;
	}

	//! Typedef for reconstruction tree edge *vector* sequence view.
	typedef ReconstructionTreeEdgeSequenceView<
			GPlatesAppLogic::ReconstructionTreeEdge::edge_collection_type::const_iterator,
			&edge_from_vector_iterator<GPlatesAppLogic::ReconstructionTreeEdge::edge_collection_type::const_iterator> >
					reconstruction_tree_edge_vector_view_type;


	template <typename IteratorType>
	GPlatesAppLogic::ReconstructionTreeEdge::non_null_ptr_type
	edge_from_map_iterator(
			IteratorType iter)
	{
		return iter->second;
	}

	//! Typedef for reconstruction tree edge *map* sequence view.
	typedef ReconstructionTreeEdgeSequenceView<
			GPlatesAppLogic::ReconstructionTree::edge_refs_by_plate_id_map_const_iterator,
			&edge_from_map_iterator<GPlatesAppLogic::ReconstructionTree::edge_refs_by_plate_id_map_const_iterator> >
					reconstruction_tree_edge_map_view_type;


	// This function moved out of class ReconstructionTreeEdge due to cyclic dependency.
	boost::optional<ReconstructionTreeEdge>
	reconstruction_tree_edge_get_parent_edge(
			const ReconstructionTreeEdge &reconstruction_tree_edge)
	{
		GPlatesAppLogic::ReconstructionTreeEdge *parent_edge =
				reconstruction_tree_edge.get_edge()->parent_edge();
		if (parent_edge == NULL)
		{
			return boost::none;
		}

		return ReconstructionTreeEdge(reconstruction_tree_edge.get_reconstruction_tree(), parent_edge);
	}

	// This function moved out of class ReconstructionTreeEdge due to cyclic dependency.
	reconstruction_tree_edge_vector_view_type
	reconstruction_tree_edge_get_child_edges(
			const ReconstructionTreeEdge &reconstruction_tree_edge)
	{
		return reconstruction_tree_edge_vector_view_type(
				reconstruction_tree_edge.get_reconstruction_tree(),
				reconstruction_tree_edge.get_edge()->children_in_built_tree().begin(),
				reconstruction_tree_edge.get_edge()->children_in_built_tree().end());
	}


	const GPlatesAppLogic::ReconstructionTree::non_null_ptr_type
	reconstruction_tree_create(
			bp::object feature_collections, // Any python iterable (eg, list, tuple).
			const double &reconstruction_time,
			GPlatesModel::integer_plate_id_type anchor_plate_id = 0)
	{
		// Begin/end iterators over the python feature collections iterable.
		bp::stl_input_iterator<GPlatesModel::FeatureCollectionHandle::non_null_ptr_type>
				feature_collections_iter(feature_collections),
				feature_collections_end;

		// Convert the feature collections to weak refs.
		std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> feature_collection_refs;
		for ( ; feature_collections_iter != feature_collections_end; ++feature_collections_iter)
		{
			GPlatesModel::FeatureCollectionHandle::non_null_ptr_type feature_collection =
					*feature_collections_iter;

			feature_collection_refs.push_back(feature_collection->reference());
		}

		// NOTE: Since the feature collections are stored in the reconstruction tree as weak refs
		// it's possible that the caller will not keep the feature collections alive (eg, they get
		// garbage collected if caller no longer has references to them) in which case the weak
		// references stored in the reconstruction tree will become invalid. However currently this
		// is not a problem because we don't use them or expose them to the python user in any way.
		//
		// In fact the whole idea of storing weak refs in the reconstruction tree is debatable.

		return GPlatesAppLogic::create_reconstruction_tree(
				reconstruction_time,
				anchor_plate_id,
				feature_collection_refs);
	}

	boost::optional<ReconstructionTreeEdge>
	reconstruction_tree_get_edge(
			GPlatesAppLogic::ReconstructionTree::non_null_ptr_type reconstruction_tree,
			GPlatesModel::integer_plate_id_type moving_plate_id)
	{
		GPlatesAppLogic::ReconstructionTree::edge_refs_by_plate_id_map_range_type range =
				reconstruction_tree->find_edges_whose_moving_plate_id_match(moving_plate_id);
		if (range.first == range.second)
		{
			// No matches.
			return boost::none;
		}

		// Return first match if more than one match.
		GPlatesAppLogic::ReconstructionTreeEdge::non_null_ptr_type edge = range.first->second;
		return ReconstructionTreeEdge(reconstruction_tree, edge);
	}

	reconstruction_tree_edge_map_view_type
	reconstruction_tree_get_edges(
			GPlatesAppLogic::ReconstructionTree::non_null_ptr_type reconstruction_tree)
	{
		return reconstruction_tree_edge_map_view_type(
				reconstruction_tree,
				reconstruction_tree->edge_map_begin(),
				reconstruction_tree->edge_map_end());
	}

	reconstruction_tree_edge_vector_view_type
	reconstruction_tree_get_anchor_plate_edges(
			GPlatesAppLogic::ReconstructionTree::non_null_ptr_type reconstruction_tree)
	{
		return reconstruction_tree_edge_vector_view_type(
				reconstruction_tree,
				reconstruction_tree->rootmost_edges_begin(),
				reconstruction_tree->rootmost_edges_end());
	}

	boost::optional<GPlatesMaths::FiniteRotation>
	get_equivalent_total_rotation(
			const GPlatesAppLogic::ReconstructionTree &reconstruction_tree,
			GPlatesModel::integer_plate_id_type moving_plate_id,
			bool use_identity_for_missing_plate_ids)
	{
		const std::pair<
				GPlatesMaths::FiniteRotation,
				GPlatesAppLogic::ReconstructionTree::ReconstructionCircumstance> result =
						reconstruction_tree.get_composed_absolute_rotation(moving_plate_id);
		if (result.second == GPlatesAppLogic::ReconstructionTree::NoPlateIdMatchesFound)
		{
			if (use_identity_for_missing_plate_ids)
			{
				// It's already an identity rotation.
				return result.first;
			}

			return boost::none;
		}

		// Currently ignoring difference between 'one' and 'one or more' moving plate id matches.
		return result.first;
	}

	boost::optional<GPlatesMaths::FiniteRotation>
	get_relative_total_rotation(
			const GPlatesAppLogic::ReconstructionTree &reconstruction_tree,
			GPlatesModel::integer_plate_id_type moving_plate_id,
			GPlatesModel::integer_plate_id_type fixed_plate_id,
			bool use_identity_for_missing_plate_ids)
	{
		boost::optional<GPlatesMaths::FiniteRotation> equivalent_plate_rotation =
				get_equivalent_total_rotation(
						reconstruction_tree,
						moving_plate_id,
						use_identity_for_missing_plate_ids);
		if (!equivalent_plate_rotation)
		{
			return boost::none;
		}

		boost::optional<GPlatesMaths::FiniteRotation> equivalent_relative_plate_rotation =
				get_equivalent_total_rotation(
						reconstruction_tree,
						fixed_plate_id,
						use_identity_for_missing_plate_ids);
		if (!equivalent_relative_plate_rotation)
		{
			return boost::none;
		}

		// Rotation from anchor plate 'Anchor' to plate 'To' (via plate 'From'):
		//
		// R(Anchor->To) = R(Anchor->From) * R(From->To)
		// ...or by pre-multiplying both sides by R(From->Anchor) this becomes...
		// R(From->To) = R(From->Anchor) * R(Anchor->To)
		// R(From->To) = inverse[R(Anchor->From)] * R(Anchor->To)
		//
		// See comments in implementation of 'GPlatesAppLogic::ReconstructUtils::get_stage_pole()'
		// for a more in-depth coverage of the above.
		//
		return compose(
				get_reverse(equivalent_relative_plate_rotation.get()),
				equivalent_plate_rotation.get());
	}

	boost::optional<GPlatesMaths::FiniteRotation>
	get_equivalent_stage_rotation(
			const GPlatesAppLogic::ReconstructionTree &from_reconstruction_tree,
			const GPlatesAppLogic::ReconstructionTree &to_reconstruction_tree,
			GPlatesModel::integer_plate_id_type plate_id,
			bool use_identity_for_missing_plate_ids)
	{
		// The anchor plate ids of both trees must match.
		GPlatesGlobal::Assert<DifferentAnchoredPlatesInReconstructionTreesException>(
				from_reconstruction_tree.get_anchor_plate_id() == to_reconstruction_tree.get_anchor_plate_id(),
				GPLATES_ASSERTION_SOURCE);

		boost::optional<GPlatesMaths::FiniteRotation> plate_from_rotation =
				get_equivalent_total_rotation(
						from_reconstruction_tree,
						plate_id,
						use_identity_for_missing_plate_ids);
		if (!plate_from_rotation)
		{
			return boost::none;
		}

		boost::optional<GPlatesMaths::FiniteRotation> plate_to_rotation =
				get_equivalent_total_rotation(
						to_reconstruction_tree,
						plate_id,
						use_identity_for_missing_plate_ids);
		if (!plate_to_rotation)
		{
			return boost::none;
		}

		//
		// Rotation from present day (0Ma) to time 't2' (via time 't1'):
		//
		// R(0->t2)  = R(t1->t2) * R(0->t1)
		// ...or by post-multiplying both sides by R(t1->0), and then swapping sides, this becomes...
		// R(t1->t2) = R(0->t2) * R(t1->0)
		// R(t1->t2) = R(0->t2) * inverse[R(0->t1)]
		//
		// See comments in implementation of 'GPlatesAppLogic::ReconstructUtils::get_stage_pole()'
		// for a more in-depth coverage of the above.
		//
		return compose(plate_to_rotation.get(), get_reverse(plate_from_rotation.get()));
	}

	boost::optional<GPlatesMaths::FiniteRotation>
	get_relative_stage_rotation(
			const GPlatesAppLogic::ReconstructionTree &from_reconstruction_tree,
			const GPlatesAppLogic::ReconstructionTree &to_reconstruction_tree,
			GPlatesModel::integer_plate_id_type moving_plate_id,
			GPlatesModel::integer_plate_id_type fixed_plate_id,
			bool use_identity_for_missing_plate_ids)
	{
		boost::optional<GPlatesMaths::FiniteRotation> fixed_plate_from_rotation =
				get_equivalent_total_rotation(
						from_reconstruction_tree,
						fixed_plate_id,
						use_identity_for_missing_plate_ids);
		if (!fixed_plate_from_rotation)
		{
			return boost::none;
		}

		boost::optional<GPlatesMaths::FiniteRotation> fixed_plate_to_rotation =
				get_equivalent_total_rotation(
						to_reconstruction_tree,
						fixed_plate_id,
						use_identity_for_missing_plate_ids);
		if (!fixed_plate_to_rotation)
		{
			return boost::none;
		}

		boost::optional<GPlatesMaths::FiniteRotation> moving_plate_from_rotation =
				get_equivalent_total_rotation(
						from_reconstruction_tree,
						moving_plate_id,
						use_identity_for_missing_plate_ids);
		if (!moving_plate_from_rotation)
		{
			return boost::none;
		}

		boost::optional<GPlatesMaths::FiniteRotation> moving_plate_to_rotation =
				get_equivalent_total_rotation(
						to_reconstruction_tree,
						moving_plate_id,
						use_identity_for_missing_plate_ids);
		if (!moving_plate_to_rotation)
		{
			return boost::none;
		}

		// This is the same as ReconstructUtils::get_stage_pole() but we return 'boost::none'
		// if any plate ids were not found.
		//
		//    R(t_from->t_to,F->M)
		//       = R(0->t_to,F->M) * R(t_from->0,F->M)
		//       = R(0->t_to,F->M) * inverse[R(0->t_from,F->M)]
		//       = R(0->t_to,F->A_to) * R(0->t_to,A_to->M) * inverse[R(0->t_from,F->A_from) * R(0->t_from,A_from->M)]
		//       = inverse[R(0->t_to,A_to->F)] * R(0->t_to,A_to->M) * inverse[inverse[R(0->t_from,A_from->F)] * R(0->t_from,A_from->M)]
		//       = inverse[R(0->t_to,A_to->F)] * R(0->t_to,A_to->M) * inverse[R(0->t_from,A_from->M)] * R(0->t_from,A_from->F)
		//
		// ...where 'A_from' is the anchor plate of *from_reconstruction_tree*,
		// 'A_to' is the anchor plate of *to_reconstruction_tree*,
		// 'F' is the fixed plate and 'M' is the moving plate.
		//
		return compose(
				compose(
						get_reverse(fixed_plate_to_rotation.get()),
						moving_plate_to_rotation.get()),
				get_reverse(compose(
						get_reverse(fixed_plate_from_rotation.get()),
						moving_plate_from_rotation.get())));
	}

	void
	reconstruction_tree_builder_insert_total_reconstruction_pole(
			GPlatesAppLogic::ReconstructionGraph &reconstruction_graph,
			GPlatesModel::integer_plate_id_type fixed_plate_id,
			GPlatesModel::integer_plate_id_type moving_plate_id,
			const GPlatesMaths::FiniteRotation &total_reconstruction_pole)
	{
		// We'll just say the finite rotation was always interpolated - currently this is only used
		// for display in dialogs.
		reconstruction_graph.insert_total_reconstruction_pole(
				fixed_plate_id,
				moving_plate_id,
				total_reconstruction_pole,
				true/*finite_rotation_was_interpolated*/);
	}

	GPlatesAppLogic::ReconstructionTree::non_null_ptr_type
	reconstruction_tree_builder_build_reconstruction_tree(
			GPlatesAppLogic::ReconstructionGraph &reconstruction_graph,
			GPlatesModel::integer_plate_id_type anchor_plate_id,
			const double &reconstruction_time)
	{
		// We don't specify any rotation feature collections because we don't want to require
		// the user to have to specify where their total reconstruction poles came from.
		// It could be that they didn't even come from features.
		// We also don't have a method to retrieve those features from the reconstruction tree.
		//
		// FIXME: It's useful to specify the features used to build a tree (since can then
		// build trees for different reconstruction times (as done by the ReconstructionTreeCreator)
		// but it needs to be handled in a different way to account for the fact that poles can
		// come from different sources (not just rotation features).
		return reconstruction_graph.build_tree(
				anchor_plate_id,
				reconstruction_time,
				std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref>()/*empty*/);
	}
}


void
export_reconstruction_tree()
{
	//
	// A wrapper around view access to the edges of a reconstruction tree (in a vector).
	//
	// We don't document this wrapper (using docstrings) since it's documented in "ReconstructionTree".
	bp::class_<
			GPlatesApi::reconstruction_tree_edge_vector_view_type>(
					"ReconstructionTreeEdgesView",
					bp::no_init)
		.def("__iter__",
				&GPlatesApi::reconstruction_tree_edge_vector_view_type::get_iter)
		.def("__len__",
				&GPlatesApi::reconstruction_tree_edge_vector_view_type::get_len)
		.def("__getitem__",
				&GPlatesApi::reconstruction_tree_edge_vector_view_type::get_item)
	;
	// Iterator over the view.
	bp::class_<
			GPlatesApi::reconstruction_tree_edge_vector_view_type::Iterator>(
					"ReconstructionTreeEdgesViewIterator",
					bp::no_init)
		.def("__iter__",
				&GPlatesApi::reconstruction_tree_edge_vector_view_type::Iterator::self,
				bp::return_value_policy<bp::copy_non_const_reference>())
		.def("next",
				&GPlatesApi::reconstruction_tree_edge_vector_view_type::Iterator::next)
	;

	//
	// A wrapper around view access to the edges of a reconstruction tree (in a map).
	//
	// We don't document this wrapper (using docstrings) since it's documented in "ReconstructionTree".
	bp::class_<
			GPlatesApi::reconstruction_tree_edge_map_view_type>(
					"AllReconstructionTreeEdgesView",
					bp::no_init)
		.def("__iter__",
				&GPlatesApi::reconstruction_tree_edge_map_view_type::get_iter)
		.def("__len__",
				&GPlatesApi::reconstruction_tree_edge_map_view_type::get_len)
		.def("__getitem__",
				&GPlatesApi::reconstruction_tree_edge_map_view_type::get_item)
	;
	// Iterator over the view.
	bp::class_<
			GPlatesApi::reconstruction_tree_edge_map_view_type::Iterator>(
					"AllReconstructionTreeEdgesViewIterator",
					bp::no_init)
		.def("__iter__",
				&GPlatesApi::reconstruction_tree_edge_map_view_type::Iterator::self,
				bp::return_value_policy<bp::copy_non_const_reference>())
		.def("next",
				&GPlatesApi::reconstruction_tree_edge_map_view_type::Iterator::next)
	;

	//
	// ReconstructionTree - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesAppLogic::ReconstructionTree,
			GPlatesAppLogic::ReconstructionTree::non_null_ptr_type,
			boost::noncopyable>(
					"ReconstructionTree",
					"Represents the plate-reconstruction hierarchy of total reconstruction poles at "
					"an instant in geological time.\n"
					"\n"
					"Plate motions are described in terms of relative rotations between pairs of plates. "
					"Every plate in the model moves relative to some other plate where, within each "
					"of these plate pairs, one plate is considered the *moving* plate relative to the "
					"other *fixed* plate. That *fixed* plate, in turn, moves relative to another plate "
					"thus forming a tree-like structure known as the *reconstruction tree*. "
					"Each of these *relative* rotations is an *edge* in the tree.\n"
					"\n"
					"The following diagram shows a subset of the hierarchy of relative rotations used in GPlates:\n"
					"::\n"
					"\n"
					"                  000\n"
					"                   |\n"
					"                   |  finite rotation (001 rel. 000)\n"
					"                   |\n"
					"                  001\n"
					"                   |\n"
					"                   |  finite rotation (701 rel. 001)\n"
					"                   |\n"
					"                  701(AFR)\n"
					"                  /|\\\n"
					"                 / | \\  finite rotation (802 rel. 701)\n"
					"                /  |  \\\n"
					"             201  702  802(ANT)\n"
					"              /   / \\   \\\n"
					"             /   /   \\   \\  finite rotation (801 rel. 802)\n"
					"            /   /     \\   \\\n"
					"         202  704     705  801(AUS)\n"
					"         / \\\n"
					"        /   \\\n"
					"       /     \\\n"
					"     290     291\n"
					"\n"
					"...where *000* is the anchored plate (the top of the reconstruction tree). "
					"The :class:`edge<ReconstructionTreeEdge>` *802 rel. 701* contains the rotation "
					"of *802* (the moving plate in the pair) relative to *701* (the fixed plate in the pair).\n"
					"\n"
					"An *equivalent* rotation is the rotation of a plate relative to the *anchored* "
					"plate. So the equivalent rotation of plate *802* is the "
					":func:`composition<compose_finite_rotations>` of relative rotations along the "
					"plate circuit *edge* path from anchored plate *000* to plate *802*.\n"
					"\n"
					"A *relative* rotation is the rotation of one plate relative to *another* plate "
					"(as opposed to the *anchored* plate). Note that, like *equivalent* rotations, "
					"the plate circuit *edge* path can consist of one or more edges. "
					"For example, the rotation of plate *801* relative to plate *291* follows an *edge* "
					"path that goes via plates *202*, *201*, *701* and *802*. However it should be noted "
					"that if the edge between *001* and *701* did not exist then, even though a path "
					"would still exist between *291* and *801*, the *relative* rotation (and *equivalent* "
					"rotations of *291* and *801* for that matter) would be an :meth:`identity rotation"
					"<FiniteRotation.represents_identity_rotation>`. This is because the sub-tree "
					"below *001* would not get built into the reconstruction tree and hence all plates "
					"in the sub-tree would be missing (this is what the *use_identity_for_missing_plate_ids* "
					"parameter in various functions refers to). This can happen when the rotation sequence "
					"for a moving/fixed plate pair (eg, *701*/*101*) does not span a large enough time "
					"period. You can work around this situation by setting the anchor plate (in "
					":meth:`__init__`) to the relative plate (eg, *291* in the above example).\n"
					"\n"
					"A *total* rotation is a rotation at a time in the past relative to *present day* (0Ma) - "
					"in other words *from* present day *to* a past time. *Total* rotations are "
					"handled by the *ReconstructionTree* methods :meth:`get_equivalent_total_rotation` and "
					":meth:`get_relative_total_rotation`.\n"
					"\n"
					"A *stage* rotation is a rotation at a time in the past relative to *another* time "
					"in the past. *Stage* rotations are handled by the functions "
					":func:`get_equivalent_stage_rotation` and :func:`get_relative_stage_rotation`.\n"
					"\n"
					"Alternatively all four combinations of total/stage and equivalent/relative rotations "
					"can be obtained more easily from class :class:`RotationModel` using its method "
					":meth:`RotationModel.get_rotation`.\n"
					"\n"
					"For more information on the rotation hierarchy please see "
					"`Next-generation plate-tectonic reconstructions using GPlates "
					"<http://www.gplates.org/publications.html>`_\n",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::reconstruction_tree_create,
						bp::default_call_policies(),
						(bp::arg("feature_collections"),
							bp::arg("reconstruction_time"),
							bp::arg("anchor_plate_id") = 0)),
				"__init__(feature_collections, reconstruction_time, [anchor_plate_id=0])\n"
				"  Create a plate-reconstruction hierarchy at the specified reconstruction time "
				"with *equivalent* rotations relative to the specified anchored plate.\n"
				"\n"
				"  :param feature_collections: A sequence of :class:`FeatureCollection` instances\n"
				"  :type feature_collections: Any sequence such as a ``list`` or a ``tuple``\n"
				"  :param reconstruction_time: the time at which to generate the reconstruction tree\n"
				"  :type reconstruction_time: float\n"
				"  :param anchor_plate_id: the id of the anchored plate that *equivalent* rotations "
				"are calculated with respect to\n"
				"  :type anchor_plate_id: int\n"
				"\n"
				"  *NOTE:* the anchored plate id can be any plate id (does not have to be zero). "
				"All *equivalent* rotations are calculated relative to the *anchored* plate id.\n"
				"\n"
				"  This method essentially does the following:\n"
				"  ::\n"
				"\n"
				"    def create_reconstruction_tree(feature_collections, reconstruction_time, anchor_plate_id):\n"
				"        builder = pygplates.ReconstructionTreeBuilder()\n"
				"        for feature_collection in feature_collections:\n"
				"            for feature in feature_collection:\n"
				"                trp = pygplates.interpolate_total_reconstruction_sequence(feature, reconstruction_time)\n"
				"                if trp:\n"
				"                    fixed_plate_id, moving_plate_id, interpolated_rotation = trp\n"
				"                    builder.insert_total_reconstruction_pole("
				"fixed_plate_id, moving_plate_id, interpolated_rotation)\n"
				"        return builder.build_reconstruction_tree(anchor_plate_id, reconstruction_time)\n"
				"\n"
				"  Note that the above example uses the class :class:`ReconstructionTreeBuilder` and "
				"the function :func:`interpolate_total_reconstruction_sequence`\n")
		.def("get_reconstruction_time",
				&GPlatesAppLogic::ReconstructionTree::get_reconstruction_time,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_reconstruction_time() -> float\n"
				"  Return the reconstruction time for which this *ReconstructionTree* was generated.\n"
				"\n"
				"  :rtype: float\n")
		.def("get_anchor_plate_id",
				&GPlatesAppLogic::ReconstructionTree::get_anchor_plate_id,
				"get_anchor_plate_id() -> int\n"
				"  Return the anchor plate id for which this *ReconstructionTree* was generated.\n"
				"\n"
				"  :rtype: int\n")
		.def("get_equivalent_total_rotation",
				&GPlatesApi::get_equivalent_total_rotation,
				(bp::arg("plate_id"),
					bp::arg("use_identity_for_missing_plate_ids")=true),
				"get_equivalent_total_rotation(plate_id, [bool use_identity_for_missing_plate_ids=True]) "
				"-> FiniteRotation or None\n"
				"  Return the *equivalent* finite rotation of the *plate_id* plate relative to the "
				"*anchored* plate.\n"
				"\n"
				"  :param plate_id: the plate id of plate to calculate the equivalent rotation\n"
				"  :type plate_id: int\n"
				"  :param use_identity_for_missing_plate_ids: whether to use an "
				":meth:`identity rotation<FiniteRotation.create_identity_rotation>` or return ``None`` "
				"for missing plate ids (default is to use identity rotation)\n"
				"  :type use_identity_for_missing_plate_ids: bool\n"
				"  :rtype: :class:`FiniteRotation`, or None (if *use_identity_for_missing_plate_ids* is ``False``)\n"
				"\n"
				"  The *total* in the method name indicates that the rotation is also relative to *present day*.\n"
				"\n"
				"  If there is no plate circuit path from *plate_id* to the anchor plate then an "
				":meth:`identity rotation<FiniteRotation.create_identity_rotation>` is used for the "
				"plate's rotation if *use_identity_for_missing_plate_ids* is ``True``, "
				"otherwise ``None`` is returned. See :class:`ReconstructionTree` for details on how a "
				"plate id can go missing and how to work around it. If *plate_id* matches more than "
				"one plate id in this *ReconstructionTree* then the first match is returned.\n"
				"\n"
				"  This method essentially does the following:\n"
				"  ::\n"
				"\n"
				"    def get_equivalent_total_rotation(reconstruction_tree, plate_id):\n"
				"        edge = reconstruction_tree.get_edge(plate_id)\n"
				"        if edge:\n"
				"            return edge.get_equivalent_total_rotation()\n"
				"        elif plate_id == reconstruction_tree.get_anchor_plate_id():\n"
				"            return pygplates.FiniteRotation.create_identity_rotation()\n"
				"        elif use_identity_for_missing_plate_ids:\n"
				"            return pygplates.FiniteRotation.create_identity_rotation()\n"
				"        # else returns None\n")
		.def("get_relative_total_rotation",
				&GPlatesApi::get_relative_total_rotation,
				(bp::arg("moving_plate_id"),
					bp::arg("fixed_plate_id"),
					bp::arg("use_identity_for_missing_plate_ids")=true),
				"get_relative_total_rotation(moving_plate_id, fixed_plate_id"
				", [use_identity_for_missing_plate_ids=True]) -> FiniteRotation or None\n"
				"  Return the finite rotation of the *moving_plate_id* plate relative to the "
				"*fixed_plate_id* plate.\n"
				"\n"
				"  :param moving_plate_id: the plate id of plate to calculate the relative rotation\n"
				"  :type moving_plate_id: int\n"
				"  :param fixed_plate_id: the plate id of plate that the rotation is relative to\n"
				"  :type fixed_plate_id: int\n"
				"  :param use_identity_for_missing_plate_ids: whether to use an "
				":meth:`identity rotation<FiniteRotation.create_identity_rotation>` or return ``None`` "
				"for missing plate ids (default is to use identity rotation)\n"
				"  :type use_identity_for_missing_plate_ids: bool\n"
				"  :rtype: :class:`FiniteRotation`, or None (if *use_identity_for_missing_plate_ids* is ``False``)\n"
				"\n"
				"  The *total* in the method name indicates that the rotation is also relative to *present day*.\n"
				"\n"
				"  This method is useful if *fixed_plate_id* and *moving_plate_id* have more than one "
				"edge between them. If *fixed_plate_id* is the *anchored* plate then this method gives the same "
				"result as :meth:`get_equivalent_total_rotation`. Another way to calculate this result "
				"is to create a new *ReconstructionTree* using *fixed_plate_id* as the *anchored* plate. "
				"See :class:`ReconstructionTree` for a description of some subtle differences between "
				"these two approaches.\n"
				"\n"
				"  If there is no plate circuit path from *fixed_plate_id* or *moving_plate_id* to the "
				"anchor plate then an :meth:`identity rotation<FiniteRotation.create_identity_rotation>` "
				"is used for that plate's rotation if *use_identity_for_missing_plate_ids* is ``True``, "
				"otherwise this method returns immediately with ``None``. See :class:`ReconstructionTree` "
				"for details on how a plate id can go missing and how to work around it.\n"
				"\n"
				"  This method essentially does the following:\n"
				"  ::\n"
				"\n"
				"    def get_relative_total_rotation(reconstruction_tree, moving_plate_id, fixed_plate_id):\n"
				"        fixed_plate_rotation = reconstruction_tree.get_equivalent_total_rotation(fixed_plate_id)\n"
				"        moving_plate_rotation = reconstruction_tree.get_equivalent_total_rotation(moving_plate_id)\n"
				"        if fixed_plate_rotation and moving_plate_rotation:\n"
				"            return fixed_plate_rotation.get_inverse() * moving_plate_rotation\n"
				"\n"
				"  The derivation of the relative rotation is (see :class:`FiniteRotation` for a "
				"more in-depth coverage of the maths):\n"
				"\n"
				"  The rotation from anchor plate ``Anchor`` to plate ``To`` (via plate ``From``) is...\n"
				"  ::\n"
				"\n"
				"    R(Anchor->To) = R(Anchor->From) * R(From->To)\n"
				"\n"
				"  ...or by pre-multiplying both sides by ``R(From->Anchor)`` this becomes...\n"
				"  ::\n"
				"\n"
				"    R(From->To) = R(From->Anchor) * R(Anchor->To)\n"
				"\n"
				"  ...which is the rotation *from* plate ``From`` *to* plate ``To``...\n"
				"  ::\n"
				"\n"
				"    R(From->To) = inverse[R(Anchor->From)] * R(Anchor->To)\n")
		.def("get_edge",
				&GPlatesApi::reconstruction_tree_get_edge,
				(bp::arg("moving_plate_id")),
				"get_edge(moving_plate_id) -> ReconstructionTreeEdge or None\n"
				"  Return the edge in the hierarchy (graph) of the reconstruction tree associated with "
				"the specified moving plate id, or ``None`` if *moving_plate_id* is not in the reconstruction tree.\n"
				"\n"
				"  :param moving_plate_id: the moving plate id of the edge in the reconstruction tree (graph)\n"
				"  :type moving_plate_id: int\n"
				"  :rtype: :class:`ReconstructionTreeEdge` or None\n"
				"\n"
				"  Returns ``None`` if *moving_plate_id* is the *anchored* plate, or is not found (because a "
				"total reconstruction pole with that moving plate id was not inserted into the "
				":class:`ReconstructionTreeBuilder` used to build this reconstruction tree). "
				"If *moving_plate_id* matches more than one edge then the first match is returned.\n")
		.def("get_edges",
				&GPlatesApi::reconstruction_tree_get_edges,
				"get_edges() -> AllReconstructionTreeEdgesView\n"
				"  Returns a *view*, of *all* :class:`edges<ReconstructionTreeEdge>` of this "
				"reconstruction tree, that supports iteration over the edges as well as indexing into the edges.\n"
				"\n"
				"  Note that the *view* object returned by this method is not a ``list`` or an *iterator* but, "
				"like dictionary views in Python 3, a ``list`` or *iterator* can be obtained from the *view* "
				"as in ``list(reconstruction_tree.get_edges())`` or ``iter(reconstruction_tree.get_edges())``.\n"
				"\n"
				"  The returned view provides the following operations for accessing the edges:\n"
				"\n"
				"  ================================ ==========================================================\n"
				"  Operation                        Result\n"
				"  ================================ ==========================================================\n"
				"  ``len(edges)``                   number of edges\n"
				"  ``for e in edges``               iterates over the edges *e*\n"
				"  ``edges[i]``                     the edge at index *i*\n"
				"  ================================ ==========================================================\n"
				"\n"
				"  The following example demonstrates iteration over all edges in the reconstruction tree "
				"to export relative rotations:\n"
				"  ::\n"
				"\n"
				"    with open('export.txt', 'w') as file: \n"
				"        for edge in reconstruction_tree.get_edges():\n"
				"            relative_rotation = edge.get_relative_total_rotation()\n"
				"            if relative_rotation.represents_identity_rotation():\n"
				"                # Use north pole with zero rotation to indicate identity rotation.\n"
				"                lat_lon_pole = pygplates.LatLonPoint(90, 0)\n"
				"                angle = 0\n"
				"            else:\n"
				"                pole, angle = relative_rotation.get_euler_pole_and_angle()\n"
				"                lat_lon_pole = pole.to_lat_lon_point(pole)\n"
				"            file.write('%f %f %f %f %f\\n' % (\n"
				"                edge.get_moving_plate_id(),\n"
				"                lat_lon_pole.get_latitude(),\n"
				"                lat_lon_pole.get_longitude(),\n"
				"                math.degrees(angle),\n"
				"                edge.get_fixed_plate_id()))\n")
		.def("get_anchor_plate_edges",
				&GPlatesApi::reconstruction_tree_get_anchor_plate_edges,
				"get_anchor_plate_edges() -> ReconstructionTreeEdgesView\n"
				"  Returns a *view*, of the :class:`edges<ReconstructionTreeEdge>` at the top (or root) "
				"of this reconstruction tree, that supports iteration over the edges as well as "
				"indexing into the edges.\n"
				"\n"
				"  The fixed plate id of each anchor plate edge matches the anchor plate id.\n"
				"\n"
				"  Note that the *view* object returned by this method is not a ``list`` or an *iterator* but, "
				"like dictionary views in Python 3, a ``list`` or *iterator* can be obtained from the *view* "
				"as in ``list(reconstruction_tree.get_anchor_plate_edges())`` or "
				"``iter(reconstruction_tree.get_anchor_plate_edges())``.\n"
				"\n"
				"  The returned view provides the following operations for accessing the anchor plate edges:\n"
				"\n"
				"  ================================ ==========================================================\n"
				"  Operation                        Result\n"
				"  ================================ ==========================================================\n"
				"  ``len(anchor_plate_edges)``      number of anchor plate edges\n"
				"  ``for e in anchor_plate_edges``  iterates over the anchor plate edges *e*\n"
				"  ``anchor_plate_edges[i]``        the anchor plate edge at index *i*\n"
				"  ================================ ==========================================================\n"
				"\n"
				"  The following example demonstrates top-down (starting at the anchored plate) "
				"traversal of the reconstruction tree:\n"
				"  ::\n"
				"\n"
				"    def traverse_sub_tree(edge):\n"
				"        print 'Parent plate: %d, child plate:%d' % ("
				"edge.get_fixed_plate_id(), edge.get_moving_plate_id())\n"
				"        # Recurse into the children sub-trees.\n"
				"        for child_edge in edge.get_child_edges():\n"
				"            traverse_sub_tree(child_edge)\n"
				"\n"
				"    for anchor_plate_edge in reconstruction_tree.get_anchor plate_edges():\n"
				"        traverse_sub_tree(anchor_plate_edge)\n")
	;

	// Non-member equivalent stage rotation function...
	bp::def("get_equivalent_stage_rotation",
			&GPlatesApi::get_equivalent_stage_rotation,
			(bp::arg("from_reconstruction_tree"),
				bp::arg("to_reconstruction_tree"),
				bp::arg("plate_id"),
				bp::arg("use_identity_for_missing_plate_ids")=true),
			"get_equivalent_stage_rotation(from_reconstruction_tree, to_reconstruction_tree, plate_id"
			", [use_identity_for_missing_plate_ids=True]) -> FiniteRotation or None\n"
			"  Return the finite rotation that rotates from the *anchored* plate to plate *plate_id* "
			"and from the time of *from_reconstruction_tree* to the time of *to_reconstruction_tree*.\n"
			"\n"
			"  :param from_reconstruction_tree: the reconstruction tree created for the *from* time\n"
			"  :type from_reconstruction_tree: :class:`ReconstructionTree`\n"
			"  :param to_reconstruction_tree: the reconstruction tree created for the *to* time\n"
			"  :type to_reconstruction_tree: :class:`ReconstructionTree`\n"
			"  :param plate_id: the plate id of the plate\n"
			"  :type plate_id: int\n"
			"  :param use_identity_for_missing_plate_ids: whether to use an "
			":meth:`identity rotation<FiniteRotation.create_identity_rotation>` or return ``None`` "
			"for missing plate ids (default is to use identity rotation)\n"
			"  :type use_identity_for_missing_plate_ids: bool\n"
			"  :rtype: :class:`FiniteRotation`, or None (if *use_identity_for_missing_plate_ids* is ``False``)\n"
			"  :raises: DifferentAnchoredPlatesInReconstructionTreesError if the anchor plate of "
			"both reconstruction trees is not the same plate\n"
			"\n"
			"  If the *anchored* plate of both reconstruction trees differs then "
			"*DifferentAnchoredPlatesInReconstructionTreesError* is raised. "
			"In this situation you can instead use the function :func:`get_relative_stage_rotation` "
			"and set its *fixed_plate_id* parameter to the *anchored* plate that you want.\n"
			"\n"
			"  If there is no plate circuit path from *plate_id* to the anchor plate (in either "
			"reconstruction tree) then an :meth:`identity rotation<FiniteRotation.create_identity_rotation>` "
			"is used for that plate's rotation if *use_identity_for_missing_plate_ids* is ``True``, "
			"otherwise this function returns immediately with ``None``. See :class:`ReconstructionTree` "
			"for details on how a plate id can go missing and how to work around it.\n"
			"\n"
			"  The only real advantage of this function over :func:`get_relative_stage_rotation` is "
			"it is a bit faster when the *fixed* plate is the *anchored* plate.\n"
			"\n"
			"  This function essentially does the following:\n"
			"  ::\n"
			"\n"
			"    def get_equivalent_stage_rotation(from_reconstruction_tree, to_reconstruction_tree, plate_id):\n"
			"        if from_reconstruction_tree.get_anchor_plate_id() != to_reconstruction_tree.get_anchor_plate_id():\n"
			"            raise pygplates.DifferentAnchoredPlatesInReconstructionTreesError\n"
			"        from_plate_rotation = from_reconstruction_tree.get_equivalent_total_rotation(plate_id)\n"
			"        to_plate_rotation = to_reconstruction_tree.get_equivalent_total_rotation(plate_id)\n"
			"        if from_plate_rotation and to_plate_rotation:\n"
			"            return to_plate_rotation * from_plate_rotation.get_inverse()\n"
			"\n"
			"  The derivation of the stage rotation is (see :class:`FiniteRotation` for a "
			"more in-depth coverage of the maths):\n"
			"\n"
			"  The rotation from present day (0Ma) to time ``To`` (via time ``From``) is...\n"
			"  ::\n"
			"\n"
			"    R(0->To, A->P) = R(From->To, A->P) * R(0->From, A->P)\n"
			"\n"
			"  ...or by post-multiplying both sides by ``R(From->0, A->P)`` this becomes...\n"
			"  ::\n"
			"\n"
			"    R(From->To, A->P) = R(0->To, A->P) * R(From->0, A->P)\n"
			"\n"
			"  ...which is the rotation *from* time ``From`` *to* time ``To``...\n"
			"  ::\n"
			"\n"
			"    R(From->To, A->P) = R(0->To, A->P) * inverse[R(0->From, A->P)]\n"
			"\n"
			"  ...where ``A`` is the anchored plate of both *from_reconstruction_tree* and "
			"*to_reconstruction_tree*, and ``P`` is the plate.\n");

	// Non-member relative stage rotation function...
	bp::def("get_relative_stage_rotation",
			&GPlatesApi::get_relative_stage_rotation,
			(bp::arg("from_reconstruction_tree"),
				bp::arg("to_reconstruction_tree"),
				bp::arg("moving_plate_id"),
				bp::arg("fixed_plate_id"),
				bp::arg("use_identity_for_missing_plate_ids")=true),
			"get_relative_stage_rotation(from_reconstruction_tree, to_reconstruction_tree, "
			"moving_plate_id, fixed_plate_id, [use_identity_for_missing_plate_ids]) -> FiniteRotation or None\n"
			"  Return the finite rotation that rotates from the *fixed_plate_id* plate to the *moving_plate_id* "
			"plate and from the time of *from_reconstruction_tree* to the time of *to_reconstruction_tree*.\n"
			"\n"
			"  :param from_reconstruction_tree: the reconstruction tree created for the *from* time\n"
			"  :type from_reconstruction_tree: :class:`ReconstructionTree`\n"
			"  :param to_reconstruction_tree: the reconstruction tree created for the *to* time\n"
			"  :type to_reconstruction_tree: :class:`ReconstructionTree`\n"
			"  :param moving_plate_id: the plate id of the moving plate\n"
			"  :type moving_plate_id: int\n"
			"  :param fixed_plate_id: the plate id of the fixed plate\n"
			"  :type fixed_plate_id: int\n"
			"  :param use_identity_for_missing_plate_ids: whether to use an "
			":meth:`identity rotation<FiniteRotation.create_identity_rotation>` or return ``None`` "
			"for missing plate ids (default is to use identity rotation)\n"
			"  :type use_identity_for_missing_plate_ids: bool\n"
			"  :rtype: :class:`FiniteRotation`, or None (if *use_identity_for_missing_plate_ids* is ``False``)\n"
			"\n"
			"  Note that this function will still work correctly if the *anchored* plate of both "
			"reconstruction trees differ.\n"
			"\n"
			"  If there is no plate circuit path from *fixed_plate_id* or *moving_plate_id* to the anchor "
			"plate (in either reconstruction tree) then an :meth:`identity rotation<FiniteRotation.create_identity_rotation>` "
			"is used for that plate's rotation if *use_identity_for_missing_plate_ids* is ``True``, "
			"otherwise this function returns immediately with ``None``. See :class:`ReconstructionTree` "
			"for details on how a plate id can go missing and how to work around it.\n"
			"\n"
			"  This function essentially does the following:\n"
			"  ::\n"
			"\n"
			"    def get_relative_stage_rotation(from_reconstruction_tree, to_reconstruction_tree, "
			"moving_plate_id, fixed_plate_id):\n"
			"        fixed_plate_from_rotation = from_reconstruction_tree.get_equivalent_total_rotation(fixed_plate_id)\n"
			"        fixed_plate_to_rotation = to_reconstruction_tree.get_equivalent_total_rotation(fixed_plate_id)\n"
			"        moving_plate_from_rotation = from_reconstruction_tree.get_equivalent_total_rotation(moving_plate_id)\n"
			"        moving_plate_to_rotation = to_reconstruction_tree.get_equivalent_total_rotation(moving_plate_id)\n"
			"        if fixed_plate_from_rotation and fixed_plate_to_rotation and "
			"moving_plate_from_rotation and moving_plate_to_rotation:\n"
			"            return fixed_plate_to_rotation.get_inverse() * moving_plate_to_rotation * "
			"moving_plate_from_rotation.get_inverse() * fixed_plate_from_rotation\n"
			"\n"
			"  The derivation of the relative stage rotation is (see :class:`FiniteRotation` for a "
			"more in-depth coverage of the maths):\n"
			"  ::\n"
			"\n"
			"    R(t_from->t_to, F->M)\n"
			"       = R(0->t_to, F->M) * R(t_from->0, F->M)\n"
			"       = R(0->t_to, F->M) * inverse[R(0->t_from, F->M)]\n"
			"       = R(0->t_to, F->A_to) * R(0->t_to, A_to->M) * "
			"inverse[R(0->t_from, F->A_from) * R(0->t_from, A_from->M)]\n"
			"       = inverse[R(0->t_to, A_to->F)] * R(0->t_to, A_to->M) * "
			"inverse[inverse[R(0->t_from, A_from->F)] * R(0->t_from, A_from->M)]\n"
			"       = inverse[R(0->t_to, A_to->F)] * R(0->t_to, A_to->M) * "
			"inverse[R(0->t_from, A_from->M)] * R(0->t_from, A_from->F)\n"
			"\n"
			"  ...where ``t_from`` and ``A_from`` are the time and anchor plate of *from_reconstruction_tree*, "
			"``t_to`` and ``A_to`` are the time and anchor plate of *to_reconstruction_tree*, "
			"``F`` is the fixed plate and ``M`` is the moving plate. Note that, unlike "
			":func:`get_equivalent_stage_rotation`, both reconstruction trees can have different *anchored* "
			"plates (not that this is usually the case - just that it's allowed).\n");

	// Enable boost::optional<ReconstructionTree::non_null_ptr_type> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::register_optional_conversion<GPlatesAppLogic::ReconstructionTree::non_null_ptr_type>();

	// Registers 'non-const' to 'const' conversions.
	boost::python::implicitly_convertible<
			GPlatesAppLogic::ReconstructionTree::non_null_ptr_type,
			GPlatesAppLogic::ReconstructionTree::non_null_ptr_to_const_type>();
	boost::python::implicitly_convertible<
			boost::optional<GPlatesAppLogic::ReconstructionTree::non_null_ptr_type>,
			boost::optional<GPlatesAppLogic::ReconstructionTree::non_null_ptr_to_const_type> >();

	// Register to-python conversion from ReconstructionTree::non_null_ptr_to_const_type to
	// ReconstructionTree::non_null_ptr_type since some functions return a 'const' reconstruction tree.
	GPlatesApi::PythonConverterUtils::register_to_python_const_to_non_const_non_null_intrusive_ptr_conversion<
			GPlatesAppLogic::ReconstructionTree>();

	//
	// ReconstructionTreeEdge - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesApi::ReconstructionTreeEdge>(
					"ReconstructionTreeEdge",
					"A reconstruction tree edge represents a moving/fixed plate pair in the graph of "
					"the plate-reconstruction hierarchy.\n",
					bp::no_init)
		.def("get_fixed_plate_id",
				&GPlatesApi::ReconstructionTreeEdge::get_fixed_plate_id,
				"get_fixed_plate_id() -> int\n"
				"  Return the *fixed* plate id of this edge.\n"
				"\n"
				"  :rtype: int\n")
		.def("get_moving_plate_id",
				&GPlatesApi::ReconstructionTreeEdge::get_moving_plate_id,
				"get_moving_plate_id() -> int\n"
				"  Return the *moving* plate id of this edge.\n"
				"\n"
				"  :rtype: int\n")
		.def("get_relative_total_rotation",
				&GPlatesApi::ReconstructionTreeEdge::get_relative_total_rotation,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_relative_total_rotation() -> FiniteRotation\n"
				"  Return finite rotation of the *moving* plate of this edge relative to the *fixed* "
				"plated id of this edge.\n"
				"\n"
				"  :rtype: :class:`FiniteRotation`\n"
				"\n"
				"  The *total* in the method name indicates that the rotation is also relative to *present day*.\n")
		.def("get_equivalent_total_rotation",
				&GPlatesApi::ReconstructionTreeEdge::get_equivalent_total_rotation,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_equivalent_total_rotation() -> FiniteRotation\n"
				"  Return *equivalent* (relative to the anchor plate) finite rotation of the *moving* plate of this edge.\n"
				"\n"
				"  :rtype: :class:`FiniteRotation`\n"
				"\n"
				"  The *total* in the method name indicates that the rotation is also relative to *present day*.\n"
				"\n"
				"  This method returns the *precomputed* equivalent of the following (see "
				":class:`FiniteRotation` for the maths):\n"
				"  ::\n"
				"\n"
				"    def get_equivalent_total_rotation(edge):\n"
				"        finite_rotation = pygplates.FiniteRotation.create_identity_rotation()\n"
				"        while edge:\n"
				"            finite_rotation = edge.get_relative_total_rotation() * finite_rotation\n"
				"            edge = edge.get_parent_edge()\n"
				"        return finite_rotation\n")
		.def("get_parent_edge",
				&GPlatesApi::reconstruction_tree_edge_get_parent_edge,
				"get_parent_edge() -> ReconstructionTreeEdge or None\n"
				"  Return the parent edge of this edge, or ``None`` if this edge has no parent.\n"
				"\n"
				"  This method can be used to traverse the plate circuit from an arbitrary plate "
				"(moving plate id on an edge) to the anchor plate (terminating at ``None``) and "
				"visiting moving/fixed relative plate :class:`rotations<FiniteRotation>` along the way.\n"
				"\n"
				"  :rtype: :class:`ReconstructionTreeEdge` or None\n"
				"\n"
				"  The parent edge is one step closer to the top (or root) of the reconstruction tree "
				"(closer to the anchor plate). The moving plate id of the parent edge matches the "
				"fixed plate id of this edge.\n"
				"\n"
				"  ``None`` is returned if this edge is already at the top of the reconstruction tree "
				"(if its fixed plate id is the anchor plate id).\n"
				"\n"
				"  The following example demonstrates following the plate circuit path from a plate "
				"(moving plate of an edge) to the anchor plate (at the top/root of the reconstruction tree):\n"
				"  ::\n"
				"\n"
				"    while True:\n"
				"        print 'Plate: %d' % edge.get_moving_plate_id()\n"
				"        if not edge.get_parent_edge():\n"
				"            print 'Anchored plate: %d' % edge.get_fixed_plate_id()\n"
				"            break\n"
				"        edge = edge.get_parent_edge()\n")
		.def("get_child_edges",
				&GPlatesApi::reconstruction_tree_edge_get_child_edges,
				"get_child_edges() -> ReconstructionTreeEdgesView\n"
				"  Returns a *view*, of the child :class:`edges<ReconstructionTreeEdge>` of this edge, "
				"that supports iteration over the child edges as well as indexing into the child edges.\n"
				"\n"
				"  A child edge is one step further from the top (or root) of the reconstruction tree "
				"(further from the anchor plate). The fixed plate id of each child edge matches the "
				"moving plate id of this edge.\n"
				"\n"
				"  Note that the *view* object returned by this method is not a ``list`` or an *iterator* but, "
				"like dictionary views in Python 3, a ``list`` or *iterator* can be obtained from the *view* "
				"as in ``list(edge.get_child_edges())`` or ``iter(edge.get_child_edges())``.\n"
				"\n"
				"  The returned view provides the following operations for accessing the child edges:\n"
				"\n"
				"  =========================== ==========================================================\n"
				"  Operation                    Result\n"
				"  =========================== ==========================================================\n"
				"  ``len(child_edges)``         number of child edges\n"
				"  ``for e in child_edges``     iterates over the child edges *e*\n"
				"  ``child_edges[i]``           the child edge at index *i*\n"
				"  =========================== ==========================================================\n"
				"\n"
				"  The following example demonstrates top-down (starting at the anchored plate) "
				"traversal of the reconstruction tree:\n"
				"  ::\n"
				"\n"
				"    def traverse_sub_tree(edge):\n"
				"        print 'Parent plate: %d, child plate:%d' % ("
				"edge.get_fixed_plate_id(), edge.get_moving_plate_id())\n"
				"        # Recurse into the children sub-trees.\n"
				"        for child_edge in edge.get_child_edges():\n"
				"            traverse_sub_tree(child_edge)\n"
				"\n"
				"    for anchor_plate_edge in reconstruction_tree.get_anchor plate_edges():\n"
				"        traverse_sub_tree(anchor_plate_edge)\n")
	;

	// Enable boost::optional<GPlatesApi::ReconstructionTreeEdge> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::register_optional_conversion<GPlatesApi::ReconstructionTreeEdge>();

	//
	// ReconstructionTreeBuilder - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesAppLogic::ReconstructionGraph,
			boost::noncopyable>(
					"ReconstructionTreeBuilder",
					"Enable incremental building of a reconstruction tree by inserting *total reconstruction poles*.\n"
					"\n"
					"The following example demonstrates the general procedure for incrementally building "
					"a :class:`ReconstructionTree`:\n"
					"::\n"
					"\n"
					"  builder = pygplates.ReconstructionTreeBuilder()\n"
					"  ...\n"
					"  builder.insert_total_reconstruction_pole(fixed_plate_id1, moving_plate_id1, "
					"total_rotation_moving_plate_id1_relative_fixed_plate_id1)\n"
					"  builder.insert_total_reconstruction_pole(fixed_plate_id2, moving_plate_id2, "
					"total_rotation_moving_plate_id2_relative_fixed_plate_id2)\n"
					"  ...\n"
					"  reconstruction_tree = builder.build_reconstruction_tree(anchor_plate_id, reconstruction_time)\n"
					"\n"
					"The :meth:`ReconstructionTree.__init__` method of class :class:`ReconstructionTree` "
					"uses *ReconstructionTreeBuilder* to create itself from rotation :class:`features<Feature>`.\n")
		.def("insert_total_reconstruction_pole",
				&GPlatesApi::reconstruction_tree_builder_insert_total_reconstruction_pole,
				(bp::arg("fixed_plate_id"), bp::arg("moving_plate_id"), bp::arg("total_reconstruction_pole")),
				"insert_total_reconstruction_pole(fixed_plate_id, moving_plate_id, total_reconstruction_pole)\n"
				"  Insert the *total reconstruction pole* associated with the plate pair *moving_plate_id* "
				"and *fixed_plate_id* plate.\n"
				"\n"
				"  :param fixed_plate_id: the fixed plate id of the *total reconstruction pole*\n"
				"  :type fixed_plate_id: int\n"
				"  :param moving_plate_id: the moving plate id of the *total reconstruction pole*\n"
				"  :type moving_plate_id: int\n"
				"  :param total_reconstruction_pole: the *total reconstruction pole*\n"
				"  :type total_reconstruction_pole: :class:`FiniteRotation`\n"
				"\n"
				"  The *total reconstruction pole* is also associated with the reconstruction time of "
				"the :class:`ReconstructionTree` that will be built by :meth:`build_reconstruction_tree`.\n"
				"\n"
				"  See the functions :func:`interpolate_finite_rotations`, "
				":func:`interpolate_total_reconstruction_pole` and "
				":func:`interpolate_total_reconstruction_sequence` for typical ways to generate a "
				"*total reconstruction pole*.\n")
		.def("build_reconstruction_tree",
				&GPlatesApi::reconstruction_tree_builder_build_reconstruction_tree,
				(bp::arg("anchor_plate_id"), bp::arg("reconstruction_time")),
				"build_reconstruction_tree(anchor_plate_id, reconstruction_time) -> ReconstructionTree\n"
				"  Builds a :class:`ReconstructionTree` from the *total reconstruction poles* inserted "
				"via :meth:`insert_total_reconstruction_pole`.\n"
				"\n"
				"  :param anchor_plate_id: the anchored plate id of the reconstruction tree\n"
				"  :type anchor_plate_id: int\n"
				"  :param reconstruction_time: the reconstruction time of *all* the total reconstruction "
				"poles inserted\n"
				"  :type reconstruction_time: float\n"
				"\n"
				"  The top (root) of the tree is the plate *anchor_plate_id*. "
				"The *total reconstruction poles* inserted via :meth:`insert_total_reconstruction_pole` "
				"are all assumed to be for the time *reconstruction_time* - although this is not checked.\n")
	;

	// Enable boost::optional<ReconstructionGraph> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::register_optional_conversion<GPlatesAppLogic::ReconstructionGraph>();
}

#endif // GPLATES_NO_PYTHON
