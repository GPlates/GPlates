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

#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>

#include "PythonConverterUtils.h"

#include "global/python.h"
// This is not included by <boost/python.hpp>.
// Also we must include this after <boost/python.hpp> which means after "global/python.h".
#include <boost/python/stl_iterator.hpp>

#include "app-logic/ReconstructionTree.h"
#include "app-logic/ReconstructionTreeCreator.h"

#include "model/FeatureCollectionHandle.h"
#include "model/types.h"


#if !defined(GPLATES_NO_PYTHON)

namespace bp = boost::python;


namespace GPlatesApi
{
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

DISABLE_GCC_WARNING("-Wshadow")
	// Default argument overloads of 'GPlatesAppLogic::create_reconstruction_tree'.
	BOOST_PYTHON_FUNCTION_OVERLOADS(
			reconstruction_tree_create_overloads,
			reconstruction_tree_create, 2, 3)
ENABLE_GCC_WARNING("-Wshadow")

	const GPlatesMaths::FiniteRotation
	reconstruction_tree_get_equivalent_total_rotation(
			GPlatesAppLogic::ReconstructionTree::non_null_ptr_type reconstruction_tree,
			GPlatesModel::integer_plate_id_type moving_plate_id)
	{
		const std::pair<
				GPlatesMaths::FiniteRotation,
				GPlatesAppLogic::ReconstructionTree::ReconstructionCircumstance> result =
						reconstruction_tree->get_composed_absolute_rotation(moving_plate_id);

		return result.first;
	}
}


void
export_reconstruction_tree()
{
	//
	// ReconstructionTree
	//
	bp::class_<
			GPlatesAppLogic::ReconstructionTree,
			GPlatesAppLogic::ReconstructionTree::non_null_ptr_type,
			boost::noncopyable>(
					"ReconstructionTree",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						&GPlatesApi::reconstruction_tree_create,
						bp::default_call_policies(),
						(bp::arg("feature_collections"),
							bp::arg("reconstruction_time"),
							bp::arg("anchor_plate_id") = 0)))
		.def("get_equivalent_total_rotation", &GPlatesApi::reconstruction_tree_get_equivalent_total_rotation)
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
}

#endif // GPLATES_NO_PYTHON
