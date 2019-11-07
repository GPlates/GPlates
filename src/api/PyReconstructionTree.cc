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
#include <boost/foreach.hpp>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>

#include "PyReconstructionTree.h"

#include "PyFeatureCollection.h"
#include "PyGPlatesModule.h"
#include "PyInterpolationException.h"
#include "PythonConverterUtils.h"
#include "PythonHashDefVisitor.h"

#include "global/GPlatesAssert.h"
#include "global/python.h"

#include "app-logic/ReconstructionGraph.h"
#include "app-logic/ReconstructionGraphBuilder.h"
#include "app-logic/ReconstructionTree.h"
#include "app-logic/ReconstructionTreeCreator.h"

#include "maths/MathsUtils.h"

#include "model/FeatureCollectionHandle.h"
#include "model/types.h"

#include "property-values/GeoTimeInstant.h"


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
				const GPlatesAppLogic::ReconstructionTree::Edge &edge) :
			d_reconstruction_tree(reconstruction_tree),
			d_edge(edge)
		{  }

		GPlatesModel::integer_plate_id_type
		get_fixed_plate_id() const
		{
			return d_edge.get_fixed_plate();
		}

		GPlatesModel::integer_plate_id_type
		get_moving_plate_id() const
		{
			return d_edge.get_moving_plate();
		}

		const GPlatesMaths::FiniteRotation &
		get_relative_total_rotation() const
		{
			return d_edge.get_relative_rotation();
		}

		const GPlatesMaths::FiniteRotation &
		get_equivalent_total_rotation() const
		{
			return d_edge.get_composed_absolute_rotation();
		}

		// Helper methods...

		GPlatesAppLogic::ReconstructionTree::non_null_ptr_type
		get_reconstruction_tree() const
		{
			return d_reconstruction_tree;
		}

		const GPlatesAppLogic::ReconstructionTree::Edge &
		get_edge() const
		{
			return d_edge;
		}

	private:
		// Keep the reconstruction tree alive while we're referencing it so not left with dangling pointer.
		GPlatesAppLogic::ReconstructionTree::non_null_ptr_type d_reconstruction_tree;

		// The edge (owned by the reconstruction tree).
		const GPlatesAppLogic::ReconstructionTree::Edge &d_edge;
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
			const GPlatesAppLogic::ReconstructionTree::Edge &
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
		 * 'const GPlatesAppLogic::ReconstructionTree::Edge &' to its equivalent wrapper
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

				const GPlatesAppLogic::ReconstructionTree::Edge &edge =
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
		// Support for "__getitem__".
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
				const GPlatesAppLogic::ReconstructionTree::Edge &edge =
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
	const GPlatesAppLogic::ReconstructionTree::Edge &
	edge_from_vector_iterator(
			IteratorType iter)
	{
		return *iter;
	}

	//! Typedef for reconstruction tree edge *vector* sequence view.
	typedef ReconstructionTreeEdgeSequenceView<
			GPlatesAppLogic::ReconstructionTree::edge_list_type::const_iterator,
			&edge_from_vector_iterator<GPlatesAppLogic::ReconstructionTree::edge_list_type::const_iterator> >
					reconstruction_tree_edge_vector_view_type;


	template <typename IteratorType>
	const GPlatesAppLogic::ReconstructionTree::Edge &
	edge_from_map_iterator(
			IteratorType iter)
	{
		return *iter->second;
	}

	//! Typedef for reconstruction tree edge *map* sequence view.
	typedef ReconstructionTreeEdgeSequenceView<
			GPlatesAppLogic::ReconstructionTree::edge_map_type::const_iterator,
			&edge_from_map_iterator<GPlatesAppLogic::ReconstructionTree::edge_map_type::const_iterator> >
					reconstruction_tree_edge_map_view_type;


	// This function moved out of class ReconstructionTreeEdge due to cyclic dependency.
	boost::optional<ReconstructionTreeEdge>
	reconstruction_tree_edge_get_parent_edge(
			const ReconstructionTreeEdge &reconstruction_tree_edge)
	{
		const GPlatesAppLogic::ReconstructionTree::Edge *parent_edge =
				reconstruction_tree_edge.get_edge().get_parent_edge();
		if (parent_edge == NULL)
		{
			return boost::none;
		}

		return ReconstructionTreeEdge(reconstruction_tree_edge.get_reconstruction_tree(), *parent_edge);
	}

	// This function moved out of class ReconstructionTreeEdge due to cyclic dependency.
	reconstruction_tree_edge_vector_view_type
	reconstruction_tree_edge_get_child_edges(
			const ReconstructionTreeEdge &reconstruction_tree_edge)
	{
		return reconstruction_tree_edge_vector_view_type(
				reconstruction_tree_edge.get_reconstruction_tree(),
				reconstruction_tree_edge.get_edge().get_child_edges().begin(),
				reconstruction_tree_edge.get_edge().get_child_edges().end());
	}


	/**
	 * DEPRECATED - Creating a ReconstructionTree directly from rotation features is now deprecated.
	 *              We'll still allow it but it is no longer documented.
	 *              It is recommended users create ReconstructionTree's using a RotationModel.
	 */
	const GPlatesAppLogic::ReconstructionTree::non_null_ptr_type
	deprecated_reconstruction_tree_create(
			const FeatureCollectionSequenceFunctionArgument &rotation_features,
			const GPlatesPropertyValues::GeoTimeInstant &reconstruction_time,
			GPlatesModel::integer_plate_id_type anchor_plate_id = 0)
	{
		// Time must not be distant past/future.
		GPlatesGlobal::Assert<InterpolationException>(
				reconstruction_time.is_real(),
				GPLATES_ASSERTION_SOURCE,
				"Time values cannot be distant-past (float('inf')) or distant-future (float('-inf')).");

		// Copy feature collections into a vector.
		std::vector<GPlatesModel::FeatureCollectionHandle::non_null_ptr_type> feature_collections;
		rotation_features.get_feature_collections(feature_collections);

		// Convert the feature collections to weak refs.
		std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> feature_collection_refs;
		BOOST_FOREACH(
				GPlatesModel::FeatureCollectionHandle::non_null_ptr_type feature_collection,
				feature_collections)
		{
			feature_collection_refs.push_back(feature_collection->reference());
		}

		// Normally we wouldn't create a ReconstructionGraph each time a ReconstructionTree is created
		// (because a single ReconstructionGraph can create many ReconstructionTree's at different times).
		// However this is a deprecated function, so users should plan to stop using it
		// (in favour of RotationModel.get_reconstruction_tree()).
		const GPlatesAppLogic::ReconstructionGraph::non_null_ptr_to_const_type reconstruction_graph =
				GPlatesAppLogic::create_reconstruction_graph(
						feature_collection_refs,
						false/*extend_total_reconstruction_poles_to_distant_past*/);

		return GPlatesAppLogic::ReconstructionTree::create(
				reconstruction_graph,
				reconstruction_time.value(),
				anchor_plate_id);
	}

	boost::optional<ReconstructionTreeEdge>
	reconstruction_tree_get_edge(
			GPlatesAppLogic::ReconstructionTree::non_null_ptr_type reconstruction_tree,
			GPlatesModel::integer_plate_id_type moving_plate_id)
	{
		const GPlatesAppLogic::ReconstructionTree::edge_map_type &all_edges = reconstruction_tree->get_all_edges();
		GPlatesAppLogic::ReconstructionTree::edge_map_type::const_iterator edge_iter = all_edges.find(moving_plate_id);
		if (edge_iter == all_edges.end())
		{
			// No match.
			return boost::none;
		}

		const GPlatesAppLogic::ReconstructionTree::Edge &edge = *edge_iter->second;
		return ReconstructionTreeEdge(reconstruction_tree, edge);
	}

	reconstruction_tree_edge_map_view_type
	reconstruction_tree_get_edges(
			GPlatesAppLogic::ReconstructionTree::non_null_ptr_type reconstruction_tree)
	{
		return reconstruction_tree_edge_map_view_type(
				reconstruction_tree,
				reconstruction_tree->get_all_edges().begin(),
				reconstruction_tree->get_all_edges().end());
	}

	reconstruction_tree_edge_vector_view_type
	reconstruction_tree_get_anchor_plate_edges(
			GPlatesAppLogic::ReconstructionTree::non_null_ptr_type reconstruction_tree)
	{
		return reconstruction_tree_edge_vector_view_type(
				reconstruction_tree,
				reconstruction_tree->get_anchor_plate_edges().begin(),
				reconstruction_tree->get_anchor_plate_edges().end());
	}

	boost::optional<GPlatesMaths::FiniteRotation>
	get_equivalent_total_rotation(
			const GPlatesAppLogic::ReconstructionTree &reconstruction_tree,
			GPlatesModel::integer_plate_id_type moving_plate_id,
			bool use_identity_for_missing_plate_ids)
	{
		const boost::optional<GPlatesMaths::FiniteRotation> finite_rotation =
				reconstruction_tree.get_composed_absolute_rotation_or_none(moving_plate_id);
		if (!finite_rotation)
		{
			if (use_identity_for_missing_plate_ids)
			{
				return GPlatesMaths::FiniteRotation::create_identity_rotation();
			}

			return boost::none;
		}

		return finite_rotation.get();
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
						false/*use_identity_for_missing_plate_ids*/);
		if (!equivalent_plate_rotation)
		{
			return use_identity_for_missing_plate_ids
					? GPlatesMaths::FiniteRotation::create_identity_rotation()
					: boost::optional<GPlatesMaths::FiniteRotation>();
		}

		boost::optional<GPlatesMaths::FiniteRotation> equivalent_relative_plate_rotation =
				get_equivalent_total_rotation(
						reconstruction_tree,
						fixed_plate_id,
						false/*use_identity_for_missing_plate_ids*/);
		if (!equivalent_relative_plate_rotation)
		{
			return use_identity_for_missing_plate_ids
					? GPlatesMaths::FiniteRotation::create_identity_rotation()
					: boost::optional<GPlatesMaths::FiniteRotation>();
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
						false/*use_identity_for_missing_plate_ids*/);
		if (!plate_from_rotation)
		{
			return use_identity_for_missing_plate_ids
					? GPlatesMaths::FiniteRotation::create_identity_rotation()
					: boost::optional<GPlatesMaths::FiniteRotation>();
		}

		boost::optional<GPlatesMaths::FiniteRotation> plate_to_rotation =
				get_equivalent_total_rotation(
						to_reconstruction_tree,
						plate_id,
						false/*use_identity_for_missing_plate_ids*/);
		if (!plate_to_rotation)
		{
			return use_identity_for_missing_plate_ids
					? GPlatesMaths::FiniteRotation::create_identity_rotation()
					: boost::optional<GPlatesMaths::FiniteRotation>();
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
						false/*use_identity_for_missing_plate_ids*/);
		if (!fixed_plate_from_rotation)
		{
			return use_identity_for_missing_plate_ids
					? GPlatesMaths::FiniteRotation::create_identity_rotation()
					: boost::optional<GPlatesMaths::FiniteRotation>();
		}

		boost::optional<GPlatesMaths::FiniteRotation> fixed_plate_to_rotation =
				get_equivalent_total_rotation(
						to_reconstruction_tree,
						fixed_plate_id,
						false/*use_identity_for_missing_plate_ids*/);
		if (!fixed_plate_to_rotation)
		{
			return use_identity_for_missing_plate_ids
					? GPlatesMaths::FiniteRotation::create_identity_rotation()
					: boost::optional<GPlatesMaths::FiniteRotation>();
		}

		boost::optional<GPlatesMaths::FiniteRotation> moving_plate_from_rotation =
				get_equivalent_total_rotation(
						from_reconstruction_tree,
						moving_plate_id,
						false/*use_identity_for_missing_plate_ids*/);
		if (!moving_plate_from_rotation)
		{
			return use_identity_for_missing_plate_ids
					? GPlatesMaths::FiniteRotation::create_identity_rotation()
					: boost::optional<GPlatesMaths::FiniteRotation>();
		}

		boost::optional<GPlatesMaths::FiniteRotation> moving_plate_to_rotation =
				get_equivalent_total_rotation(
						to_reconstruction_tree,
						moving_plate_id,
						false/*use_identity_for_missing_plate_ids*/);
		if (!moving_plate_to_rotation)
		{
			return use_identity_for_missing_plate_ids
					? GPlatesMaths::FiniteRotation::create_identity_rotation()
					: boost::optional<GPlatesMaths::FiniteRotation>();
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


	//
	// Class ReconstructionTreeBuilder is now DEPRECATED - see comment at bp::class_ below (for reasons why).
	//

	void
	deprecated_reconstruction_tree_builder_insert_total_reconstruction_pole(
			GPlatesAppLogic::ReconstructionGraphBuilder &reconstruction_graph_builder,
			GPlatesModel::integer_plate_id_type fixed_plate_id,
			GPlatesModel::integer_plate_id_type moving_plate_id,
			const GPlatesMaths::FiniteRotation &total_reconstruction_pole)
	{
		// We only have a total reconstruction pole at a particular reconstruction time
		// (and we don't know that time), so create a sequence containing the same pole both
		// infinitely far into the future and infinitely far in the distant past (this is now supported)
		// such that any reconstruction time will return that pole (total rotation).
		GPlatesAppLogic::ReconstructionGraphBuilder::total_reconstruction_pole_type total_reconstruction_sequence;
		total_reconstruction_sequence.push_back(
				GPlatesAppLogic::ReconstructionGraphBuilder::total_reconstruction_pole_time_sample_type(
						GPlatesPropertyValues::GeoTimeInstant::create_distant_future(),
						total_reconstruction_pole));
		total_reconstruction_sequence.push_back(
				GPlatesAppLogic::ReconstructionGraphBuilder::total_reconstruction_pole_time_sample_type(
						GPlatesPropertyValues::GeoTimeInstant::create_distant_past(),
						total_reconstruction_pole));

		reconstruction_graph_builder.insert_total_reconstruction_sequence(
				fixed_plate_id,
				moving_plate_id,
				total_reconstruction_sequence);
	}

	GPlatesAppLogic::ReconstructionTree::non_null_ptr_type
	deprecated_reconstruction_tree_builder_build_reconstruction_tree(
			GPlatesAppLogic::ReconstructionGraphBuilder &reconstruction_graph_builder,
			GPlatesModel::integer_plate_id_type anchor_plate_id,
			const GPlatesPropertyValues::GeoTimeInstant &reconstruction_time)
	{
		// Time must not be distant past/future.
		GPlatesGlobal::Assert<InterpolationException>(
				reconstruction_time.is_real(),
				GPLATES_ASSERTION_SOURCE,
				"Time values cannot be distant-past (float('inf')) or distant-future (float('-inf')).");

		// FIXME: It's useful to specify the features used to build a tree (since can then
		// build trees for different reconstruction times (as done by the ReconstructionTreeCreator)
		// but it needs to be handled in a different way to account for the fact that poles can
		// come from different sources (not just rotation features).
		const GPlatesAppLogic::ReconstructionGraph::non_null_ptr_to_const_type reconstruction_graph =
				reconstruction_graph_builder.build_graph();

		return GPlatesAppLogic::ReconstructionTree::create(
				reconstruction_graph,
				reconstruction_time.value(),
				anchor_plate_id);
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
		.def(
#if PY_MAJOR_VERSION < 3
				"next",
#else
				"__next__",
#endif
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
		.def(
#if PY_MAJOR_VERSION < 3
				"next",
#else
				"__next__",
#endif
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
					"See :ref:`pygplates_foundations_plate_reconstruction_hierarchy`.\n"
					"\n"
					"Total rotations are handled by the methods :meth:`get_equivalent_total_rotation` and "
					":meth:`get_relative_total_rotation`.\n"
					"\n"
					"Stage rotations are handled by the functions :meth:`get_equivalent_stage_rotation` and "
					":meth:`get_relative_stage_rotation`.\n"
					"\n"
					".. note:: All four combinations of total/stage and equivalent/relative rotations "
					"can be obtained more easily from class :class:`RotationModel` using its method "
					":meth:`RotationModel.get_rotation`.\n",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		// DEPRECATED - Creating a ReconstructionTree directly from rotation features is now deprecated.
		//              We'll still allow it but it is no longer documented.
		//              It is recommended users create ReconstructionTree's using a RotationModel...
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::deprecated_reconstruction_tree_create,
						bp::default_call_policies(),
						(bp::arg("rotation_features"),
							bp::arg("reconstruction_time"),
							bp::arg("anchor_plate_id") = 0)),
				"__init__()\n"
				"\n"
				"  .. deprecated:: 25\n"
				"     Use :meth:`RotationModel.get_reconstruction_tree` instead.\n")
		.def("get_equivalent_stage_rotation",
				&GPlatesApi::get_equivalent_stage_rotation,
				(bp::arg("from_reconstruction_tree"),
					bp::arg("to_reconstruction_tree"),
					bp::arg("plate_id"),
					bp::arg("use_identity_for_missing_plate_ids")=true),
				"get_equivalent_stage_rotation(from_reconstruction_tree, to_reconstruction_tree, plate_id"
				", [use_identity_for_missing_plate_ids=True])\n"
				// Documenting 'staticmethod' here since Sphinx cannot introspect boost-python function
				// (like it can a pure python function) and we cannot document it in first (signature) line
				// because it messes up Sphinx's signature recognition...
				"  [*staticmethod*] Return the finite rotation that rotates from the *anchored* plate to plate *plate_id* "
				"and from the time of *from_reconstruction_tree* to the time of *to_reconstruction_tree*.\n"
				"\n"
				"  :param from_reconstruction_tree: the reconstruction tree created for the *from* time\n"
				"  :type from_reconstruction_tree: :class:`ReconstructionTree`\n"
				"  :param to_reconstruction_tree: the reconstruction tree created for the *to* time\n"
				"  :type to_reconstruction_tree: :class:`ReconstructionTree`\n"
				"  :param plate_id: the plate id of the plate\n"
				"  :type plate_id: int\n"
				"  :param use_identity_for_missing_plate_ids: whether to return an "
				":meth:`identity rotation<FiniteRotation.create_identity_rotation>` or return ``None`` "
				"for missing plate ids (default is to use identity rotation)\n"
				"  :type use_identity_for_missing_plate_ids: bool\n"
				"  :rtype: :class:`FiniteRotation`, or None (if *use_identity_for_missing_plate_ids* is ``False``)\n"
				"  :raises: DifferentAnchoredPlatesInReconstructionTreesError if the anchor plate of "
				"both reconstruction trees is not the same plate\n"
				"\n"
				"  Get the stage rotation of plate 802 (relative to anchor plate) from 20Ma to 15Ma:\n"
				"  ::\n"
				"\n"
				"    reconstruction_tree_at_20Ma = pygplates.ReconstructionTree(rotation_features, 20)\n"
				"    reconstruction_tree_at_15Ma = pygplates.ReconstructionTree(rotation_features, 15)\n"
				"    stage_rotation = pygplates.ReconstructionTree.get_equivalent_stage_rotation(\n"
				"        reconstruction_tree_at_20Ma,\n"
				"        reconstruction_tree_at_15Ma,\n"
				"        802)\n"
				"\n"
				"    #...or even better...\n"
				"\n"
				"    rotation_model = pygplates.RotationModel(rotation_features)\n"
				"    stage_rotation = rotation_model.get_rotation(15, 802, 20)\n"
				"\n"
				"  If the *anchored* plate of both reconstruction trees differs then "
				"*DifferentAnchoredPlatesInReconstructionTreesError* is raised. "
				"In this situation you can instead use :meth:`get_relative_stage_rotation` "
				"and set its *fixed_plate_id* parameter to the *anchored* plate that you want.\n"
				"\n"
				"  If there is no plate circuit path from *plate_id* to the anchor plate (in either "
				"reconstruction tree) then an :meth:`identity rotation<FiniteRotation.create_identity_rotation>` "
				"is returned if *use_identity_for_missing_plate_ids* is ``True``, otherwise ``None`` is returned. "
				"See :ref:`pygplates_foundations_plate_reconstruction_hierarchy` for details on how a plate id can "
				"go missing and how to work around it.\n"
				"\n"
				"  The only real advantage of this function over :meth:`get_relative_stage_rotation` is "
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
				"  .. note:: See :ref:`pygplates_foundations_equivalent_stage_rotation` for the derivation of the stage rotation.\n")
		.staticmethod("get_equivalent_stage_rotation")
		.def("get_relative_stage_rotation",
				&GPlatesApi::get_relative_stage_rotation,
				(bp::arg("from_reconstruction_tree"),
					bp::arg("to_reconstruction_tree"),
					bp::arg("moving_plate_id"),
					bp::arg("fixed_plate_id"),
					bp::arg("use_identity_for_missing_plate_ids")=true),
				"get_relative_stage_rotation(from_reconstruction_tree, to_reconstruction_tree, "
				"moving_plate_id, fixed_plate_id, [use_identity_for_missing_plate_ids])\n"
				// Documenting 'staticmethod' here since Sphinx cannot introspect boost-python function
				// (like it can a pure python function) and we cannot document it in first (signature) line
				// because it messes up Sphinx's signature recognition...
				"  [*staticmethod*] Return the finite rotation that rotates from the *fixed_plate_id* plate to the *moving_plate_id* "
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
				"  :param use_identity_for_missing_plate_ids: whether to return an "
				":meth:`identity rotation<FiniteRotation.create_identity_rotation>` or return ``None`` "
				"for missing plate ids (default is to use identity rotation)\n"
				"  :type use_identity_for_missing_plate_ids: bool\n"
				"  :rtype: :class:`FiniteRotation`, or None (if *use_identity_for_missing_plate_ids* is ``False``)\n"
				"\n"
				"  Get the stage rotation of plate 802 (relative to plate 291) from 20Ma to 15Ma:\n"
				"  ::\n"
				"\n"
				"    reconstruction_tree_at_20Ma = pygplates.ReconstructionTree(rotation_features, 20)\n"
				"    reconstruction_tree_at_15Ma = pygplates.ReconstructionTree(rotation_features, 15)\n"
				"    stage_rotation = pygplates.ReconstructionTree.get_relative_stage_rotation(\n"
				"        reconstruction_tree_at_20Ma,\n"
				"        reconstruction_tree_at_15Ma,\n"
				"        802,\n"
				"        291)\n"
				"\n"
				"    #...or even better...\n"
				"\n"
				"    rotation_model = pygplates.RotationModel(rotation_features)\n"
				"    stage_rotation = rotation_model.get_rotation(15, 802, 20, 291)\n"
				"\n"
				"  .. note:: This function, unlike :meth:`get_equivalent_stage_rotation`, will still work "
				"correctly if the *anchored* plate of both reconstruction trees differ (not that this is "
				"usually the case - just that it's allowed).\n"
				"\n"
				"  If there is no plate circuit path from *fixed_plate_id* or *moving_plate_id* to the anchor "
				"plate (in either reconstruction tree) then an :meth:`identity rotation<FiniteRotation.create_identity_rotation>` "
				"is returned if *use_identity_for_missing_plate_ids* is ``True``, otherwise ``None`` is returned. "
				"See :ref:`pygplates_foundations_plate_reconstruction_hierarchy` for details on how a plate id can "
				"go missing and how to work around it.\n"
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
				"  .. note:: See :ref:`pygplates_foundations_relative_stage_rotation` for the derivation of the stage rotation.\n")
		.staticmethod("get_relative_stage_rotation")
		.def("get_reconstruction_time",
				&GPlatesAppLogic::ReconstructionTree::get_reconstruction_time,
				"get_reconstruction_time()\n"
				"  Return the reconstruction time for which this *ReconstructionTree* was generated.\n"
				"\n"
				"  :rtype: float\n")
		.def("get_anchor_plate_id",
				&GPlatesAppLogic::ReconstructionTree::get_anchor_plate_id,
				"get_anchor_plate_id()\n"
				"  Return the anchor plate id for which this *ReconstructionTree* was generated.\n"
				"\n"
				"  :rtype: int\n")
		.def("get_equivalent_total_rotation",
				&GPlatesApi::get_equivalent_total_rotation,
				(bp::arg("plate_id"),
					bp::arg("use_identity_for_missing_plate_ids")=true),
				"get_equivalent_total_rotation(plate_id, [bool use_identity_for_missing_plate_ids=True])\n"
				"  Return the *equivalent* finite rotation of the *plate_id* plate relative to the "
				"*anchored* plate.\n"
				"\n"
				"  :param plate_id: the plate id of plate to calculate the equivalent rotation\n"
				"  :type plate_id: int\n"
				"  :param use_identity_for_missing_plate_ids: whether to return an "
				":meth:`identity rotation<FiniteRotation.create_identity_rotation>` or return ``None`` "
				"for missing plate ids (default is to use identity rotation)\n"
				"  :type use_identity_for_missing_plate_ids: bool\n"
				"  :rtype: :class:`FiniteRotation`, or None (if *use_identity_for_missing_plate_ids* is ``False``)\n"
				"\n"
				"  Get the rotation of plate 802 (relative to the anchor plate) at 10Ma (relative to present-day):\n"
				"  ::\n"
				"\n"
				"    reconstruction_tree = pygplates.ReconstructionTree(rotation_features, 10)\n"
				"    finite_rotation = reconstruction_tree.get_equivalent_total_rotation(802)\n"
				"\n"
				"    #...or even better...\n"
				"\n"
				"    rotation_model = pygplates.RotationModel(rotation_features)\n"
				"    stage_rotation = rotation_model.get_rotation(10, 802)\n"
				"\n"
				"  The *total* in the method name indicates that the rotation is also relative to *present day*.\n"
				"\n"
				"  If there is no plate circuit path from *plate_id* to the anchor plate then an "
				":meth:`identity rotation<FiniteRotation.create_identity_rotation>` is returned "
				"if *use_identity_for_missing_plate_ids* is ``True``, otherwise ``None`` is returned. "
				"See :ref:`pygplates_foundations_plate_reconstruction_hierarchy` for details on how a plate id can "
				"go missing and how to work around it.\n"
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
				"        # else returns None\n"
				"\n"
				"  .. note:: See :ref:`pygplates_foundations_equivalent_total_rotation` for the derivation of the rotation.\n")
		.def("get_relative_total_rotation",
				&GPlatesApi::get_relative_total_rotation,
				(bp::arg("moving_plate_id"),
					bp::arg("fixed_plate_id"),
					bp::arg("use_identity_for_missing_plate_ids")=true),
				"get_relative_total_rotation(moving_plate_id, fixed_plate_id"
				", [use_identity_for_missing_plate_ids=True])\n"
				"  Return the finite rotation of the *moving_plate_id* plate relative to the "
				"*fixed_plate_id* plate.\n"
				"\n"
				"  :param moving_plate_id: the plate id of plate to calculate the relative rotation\n"
				"  :type moving_plate_id: int\n"
				"  :param fixed_plate_id: the plate id of plate that the rotation is relative to\n"
				"  :type fixed_plate_id: int\n"
				"  :param use_identity_for_missing_plate_ids: whether to return an "
				":meth:`identity rotation<FiniteRotation.create_identity_rotation>` or return ``None`` "
				"for missing plate ids (default is to use identity rotation)\n"
				"  :type use_identity_for_missing_plate_ids: bool\n"
				"  :rtype: :class:`FiniteRotation`, or None (if *use_identity_for_missing_plate_ids* is ``False``)\n"
				"\n"
				"  Get the rotation of plate 802 (relative to plate 291) at 10Ma (relative to present-day):\n"
				"  ::\n"
				"\n"
				"    reconstruction_tree = pygplates.ReconstructionTree(rotation_features, 10)\n"
				"    finite_rotation = reconstruction_tree.get_relative_total_rotation(802, 291)\n"
				"\n"
				"    #...or even better...\n"
				"\n"
				"    rotation_model = pygplates.RotationModel(rotation_features)\n"
				"    stage_rotation = rotation_model.get_rotation(10, 802, fixed_plate_id=291)\n"
				"\n"
				"  The *total* in the method name indicates that the rotation is also relative to *present day*.\n"
				"\n"
				"  If *fixed_plate_id* is the *anchored* plate then this method gives the same "
				"result as :meth:`get_equivalent_total_rotation`. Another way to calculate this result "
				"is to create a new *ReconstructionTree* using *fixed_plate_id* as the *anchored* plate. "
				"See :ref:`pygplates_foundations_plate_reconstruction_hierarchy` for a description of some "
				"subtle differences between these two approaches.\n"
				"\n"
				"  If there is no plate circuit path from *fixed_plate_id* or *moving_plate_id* to the "
				"anchor plate then an :meth:`identity rotation<FiniteRotation.create_identity_rotation>` "
				"is returned if *use_identity_for_missing_plate_ids* is ``True``, otherwise ``None`` is returned. "
				"See :ref:`pygplates_foundations_plate_reconstruction_hierarchy` for details on how a plate id can "
				"go missing and how to work around it.\n"
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
				"  .. note:: See :ref:`pygplates_foundations_relative_total_rotation` for the derivation of the rotation.\n")
		.def("get_edge",
				&GPlatesApi::reconstruction_tree_get_edge,
				(bp::arg("moving_plate_id")),
				"get_edge(moving_plate_id)\n"
				"  Return the edge in the hierarchy (graph) of the reconstruction tree associated with "
				"the specified moving plate id, or ``None`` if *moving_plate_id* is not in the reconstruction tree.\n"
				"\n"
				"  :param moving_plate_id: the moving plate id of the edge in the reconstruction tree (graph)\n"
				"  :type moving_plate_id: int\n"
				"  :rtype: :class:`ReconstructionTreeEdge` or None\n"
				"\n"
				"  Returns ``None`` if *moving_plate_id* is the *anchored* plate, or is not found "
				"(not in this reconstruction tree).\n")
		.def("get_edges",
				&GPlatesApi::reconstruction_tree_get_edges,
				"get_edges()\n"
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
				"            relative_pole_latitude, relative_pole_longitude, relative_angle_degrees = (\n"
				"                relative_rotation.get_lat_lon_euler_pole_and_angle_degrees())\n"
				"            file.write('%f %f %f %f %f\\n' % (\n"
				"                edge.get_moving_plate_id(),\n"
				"                relative_pole_latitude,\n"
				"                relative_pole_longitude,\n"
				"                relative_angle_degrees,\n"
				"                edge.get_fixed_plate_id()))\n")
		.def("get_anchor_plate_edges",
				&GPlatesApi::reconstruction_tree_get_anchor_plate_edges,
				"get_anchor_plate_edges()\n"
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
				"    for anchor_plate_edge in reconstruction_tree.get_anchor_plate_edges():\n"
				"        traverse_sub_tree(anchor_plate_edge)\n")
		// Make hash and comparisons based on C++ object identity (not python object identity)...
		.def(GPlatesApi::ObjectIdentityHashDefVisitor())
	;

	// Register to/from Python conversions of non_null_intrusive_ptr<> including const/non-const and boost::optional.
	GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<GPlatesAppLogic::ReconstructionTree>();

	//
	// ReconstructionTreeEdge - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesApi::ReconstructionTreeEdge>(
					"ReconstructionTreeEdge",
					"A reconstruction tree edge represents a moving/fixed plate pair in the graph of "
					"the plate-reconstruction hierarchy. See :ref:`pygplates_foundations_plate_reconstruction_hierarchy`.\n",
					bp::no_init)
		.def("get_fixed_plate_id",
				&GPlatesApi::ReconstructionTreeEdge::get_fixed_plate_id,
				"get_fixed_plate_id()\n"
				"  Return the *fixed* plate id of this edge.\n"
				"\n"
				"  :rtype: int\n")
		.def("get_moving_plate_id",
				&GPlatesApi::ReconstructionTreeEdge::get_moving_plate_id,
				"get_moving_plate_id()\n"
				"  Return the *moving* plate id of this edge.\n"
				"\n"
				"  :rtype: int\n")
		.def("get_relative_total_rotation",
				&GPlatesApi::ReconstructionTreeEdge::get_relative_total_rotation,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_relative_total_rotation()\n"
				"  Return finite rotation of the *moving* plate of this edge relative to the *fixed* "
				"plated id of this edge.\n"
				"\n"
				"  :rtype: :class:`FiniteRotation`\n"
				"\n"
				"  The *total* in the method name indicates that the rotation is also relative to *present day*.\n")
		.def("get_equivalent_total_rotation",
				&GPlatesApi::ReconstructionTreeEdge::get_equivalent_total_rotation,
				bp::return_value_policy<bp::copy_const_reference>(),
				"get_equivalent_total_rotation()\n"
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
				"get_parent_edge()\n"
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
				"get_child_edges()\n"
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
				"    for anchor_plate_edge in reconstruction_tree.get_anchor_plate_edges():\n"
				"        traverse_sub_tree(anchor_plate_edge)\n")
	;

	// Enable boost::optional<GPlatesApi::ReconstructionTreeEdge> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::register_optional_conversion<GPlatesApi::ReconstructionTreeEdge>();

	//
	// ReconstructionTreeBuilder - DEPRECATED.
	//
	// NOTE: This class is deprecated now ReconstructionTree is no longer created by visiting
	//       rotation features - instead a ReconstructionGraph is created by visiting rotation features
	//       on a ReconstructionGraphBuilder, and then any number of ReconstructionTree's are created
	//       from the one ReconstructionGraph.
	//       We could create a Python class for ReconstructionGraph (but this is essentially what
	//       RotationModel already is) and a Python class for ReconstructionGraphBuilder.
	//       But for now we'll just stick to RotationModel (which accepts rotation features).
	//       
	//       So while we still support this class (for those users still using it) we do not document it.
	//
	bp::class_<
			GPlatesAppLogic::ReconstructionGraphBuilder,
			boost::noncopyable>(
					"ReconstructionTreeBuilder",
					bp::init<>())
		.def("insert_total_reconstruction_pole",
				&GPlatesApi::deprecated_reconstruction_tree_builder_insert_total_reconstruction_pole,
				(bp::arg("fixed_plate_id"), bp::arg("moving_plate_id"), bp::arg("total_reconstruction_pole")))
		.def("build_reconstruction_tree",
				&GPlatesApi::deprecated_reconstruction_tree_builder_build_reconstruction_tree,
				(bp::arg("anchor_plate_id"), bp::arg("reconstruction_time")))
	;

	// NOTE: We currently do not enable boost::optional<ReconstructionGraphBuilder> to be passed to and from python.
	//       This is because ReconstructionGraphBuilder would need to be copy-constructable (which it's not).
	//       But "boost::optional<ReconstructionGraphBuilder>" is not used anywhere.
	//       And besides, this class is now deprecated.
	//
	//GPlatesApi::PythonConverterUtils::register_optional_conversion<GPlatesAppLogic::ReconstructionGraphBuilder>();
}

#endif // GPLATES_NO_PYTHON
