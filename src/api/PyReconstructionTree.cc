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

#include "PythonConverterUtils.h"

#include "global/python.h"
// This is not included by <boost/python.hpp>.
// Also we must include this after <boost/python.hpp> which means after "global/python.h".
#include <boost/python/stl_iterator.hpp>

#include "app-logic/ReconstructionTree.h"
#include "app-logic/ReconstructionTreeCreator.h"
#include "app-logic/ReconstructionTreeEdge.h"

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

	const GPlatesMaths::FiniteRotation
	reconstruction_tree_get_equivalent_total_rotation(
			const GPlatesAppLogic::ReconstructionTree &reconstruction_tree,
			GPlatesModel::integer_plate_id_type moving_plate_id)
	{
		const std::pair<
				GPlatesMaths::FiniteRotation,
				GPlatesAppLogic::ReconstructionTree::ReconstructionCircumstance> result =
						reconstruction_tree.get_composed_absolute_rotation(moving_plate_id);

		// Currently only returning finite rotation (and not whether 0, 1 or more moving plate id
		// matches were found). Zero matches returns identity rotation. Two or more return first match.
		return result.first;
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
					"an instant in geological time.\n",
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
				"__init__(feature_collections, reconstruction_time[, anchor_plate_id=0])\n"
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
				"  ::\n"
				"\n"
				"    reconstruction_tree = pygplates.ReconstructionTree(feature_collections, reconstruction_time)\n")
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
				&GPlatesApi::reconstruction_tree_get_equivalent_total_rotation,
				"get_equivalent_total_rotation(plate_id) -> FiniteRotation\n"
				"  Return *equivalent* finite rotation of the *plate_id* plate relative to the anchor plate.\n"
				"\n"
				"  :param plate_id: the plate id of plate to calculate the equivalent rotation\n"
				"  :type plate_id: int\n"
				"  :rtype: :class:`FiniteRotation`\n"
				"\n"
				"  The *total* in the method name indicates that the rotation is also relative to *present day*.\n"
				"\n"
				"  If *plate_id* is not found (because a total reconstruction pole with that moving "
				"plate id was not inserted into the :class:`ReconstructionTreeBuilder` used to build "
				"this reconstruction tree) then the returned finite rotation represents the identity rotation "
				"(zero-angle rotation). If *plate_id* matches more than one moving plate id in this "
				"*ReconstructionTree* then the first match is returned.\n")
		.def("get_edge",
				&GPlatesApi::reconstruction_tree_get_edge,
				"get_edge(moving_plate_id) -> ReconstructionTreeEdge or None\n"
				"  Return the edge in the hierarchy (graph) of the reconstruction tree associated with "
				"the specified moving plate id.\n"
				"\n"
				"  :param moving_plate_id: the moving plate id of the edge in the reconstruction tree (graph)\n"
				"  :type moving_plate_id: int\n"
				"  :rtype: :class:`ReconstructionTreeEdge` or None\n"
				"\n"
				"  Returns ``None`` if *moving_plate_id* is not found (because a total reconstruction "
				"pole with that moving plate id was not inserted into the :class:`ReconstructionTreeBuilder` "
				"used to build this reconstruction tree). If *moving_plate_id* matches more than one "
				"edge then the first match is returned.\n")
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
				"  The returned view provides the following operations for accessing the points:\n"
				"\n"
				"  ================================ ==========================================================\n"
				"  Operation                        Result\n"
				"  ================================ ==========================================================\n"
				"  ``len(edges)``                   number of edges\n"
				"  ``for e in edges``               iterates over the edges *e*\n"
				"  ``edges[i]``                     the edge at index *i*\n"
				"  ================================ ==========================================================\n"
				"\n"
				"  The following example demonstrates some uses of the above operations:\n"
				"  ::\n"
				"\n"
				"    for edge in reconstruction_tree.get_edges():\n"
				"        ...\n")
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
				"as in ``list(reconstruction_tree.get_child_edges())`` or ``iter(reconstruction_tree.get_child_edges())``.\n"
				"\n"
				"  The returned view provides the following operations for accessing the points:\n"
				"\n"
				"  ================================ ==========================================================\n"
				"  Operation                        Result\n"
				"  ================================ ==========================================================\n"
				"  ``len(anchor_plate_edges)``      number of anchor plate edges\n"
				"  ``for e in anchor_plate_edges``  iterates over the anchor plate edges *e*\n"
				"  ``anchor_plate_edges[i]``        the anchor plate edge at index *i*\n"
				"  ================================ ==========================================================\n"
				"\n"
				"  The following example demonstrates some uses of the above operations:\n"
				"  ::\n"
				"\n"
				"    for anchor_plate_edge in reconstruction_tree.get_anchor plate_edges():\n"
				"        ...\n")
	;

	// Enable boost::optional<ReconstructionTree::non_null_ptr_type> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::python_optional<GPlatesAppLogic::ReconstructionTree::non_null_ptr_type>();

	// Registers 'non-const' to 'const' conversions.
	boost::python::implicitly_convertible<
			GPlatesAppLogic::ReconstructionTree::non_null_ptr_type,
			GPlatesAppLogic::ReconstructionTree::non_null_ptr_to_const_type>();
	boost::python::implicitly_convertible<
			boost::optional<GPlatesAppLogic::ReconstructionTree::non_null_ptr_type>,
			boost::optional<GPlatesAppLogic::ReconstructionTree::non_null_ptr_to_const_type> >();

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
				"  The *total* in the method name indicates that the rotation is also relative to *present day*.\n")
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
				"(if its fixed plate id is the anchor plate id).\n")
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
				"  The returned view provides the following operations for accessing the points:\n"
				"\n"
				"  =========================== ==========================================================\n"
				"  Operation                    Result\n"
				"  =========================== ==========================================================\n"
				"  ``len(child_edges)``         number of child edges\n"
				"  ``for e in child_edges``     iterates over the child edges *e*\n"
				"  ``child_edges[i]``           the child edge at index *i*\n"
				"  =========================== ==========================================================\n"
				"\n"
				"  The following example demonstrates some uses of the above operations:\n"
				"  ::\n"
				"\n"
				"    for child_edge in edge.get_child_edges():\n"
				"        ...\n")
	;

	// Enable boost::optional<GPlatesApi::ReconstructionTreeEdge> to be passed to and from python.
	GPlatesApi::PythonConverterUtils::python_optional<GPlatesApi::ReconstructionTreeEdge>();
}

#endif // GPLATES_NO_PYTHON
