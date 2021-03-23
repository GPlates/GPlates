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

#include <algorithm>
#include <iterator>
#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/optional.hpp>
#include <QString>

#include "PyTopologicalFeatureCollectionFunctionArgument.h"

#include "PythonConverterUtils.h"
#include "PythonExtractUtils.h"
#include "PythonHashDefVisitor.h"

#include "global/python.h"

#include "model/FeatureHandle.h"
#include "model/FeatureCollectionHandle.h"

#include "utils/ReferenceCount.h"


namespace bp = boost::python;


namespace GPlatesApi
{
	/**
	 * A from-python converter from @a FeatureCollectionSequenceFunctionArgument and optional
	 * @a ResolveTopologyParameters to a @a TopologicalFeatureCollectionSequenceFunctionArgument.
	 */
	struct ConversionTopologicalFeatureCollectionFunctionArgument :
			private boost::noncopyable
	{
		static
		void *
		convertible(
				PyObject *obj)
		{
			namespace bp = boost::python;

			if (TopologicalFeatureCollectionFunctionArgument::is_convertible(
				bp::object(bp::handle<>(bp::borrowed(obj)))))
			{
				return obj;
			}

			return NULL;
		}

		static
		void
		construct(
				PyObject *obj,
				boost::python::converter::rvalue_from_python_stage1_data *data)
		{
			namespace bp = boost::python;

			void *const storage = reinterpret_cast<
					bp::converter::rvalue_from_python_storage<
							TopologicalFeatureCollectionFunctionArgument> *>(
									data)->storage.bytes;

			new (storage) TopologicalFeatureCollectionFunctionArgument(
					TopologicalFeatureCollectionFunctionArgument::create(
							bp::object(bp::handle<>(bp::borrowed(obj)))));

			data->convertible = storage;
		}
	};


	/**
	 * Registers a from-python converter from @a FeatureCollectionSequenceFunctionArgument and optional
	 * @a ResolveTopologyParameters to a @a TopologicalFeatureCollectionSequenceFunctionArgument.
	 */
	void
	register_conversion_topological_feature_collection_function_argument()
	{
		// Register function argument types variant.
		PythonConverterUtils::register_variant_conversion<
				TopologicalFeatureCollectionFunctionArgument::function_argument_type>();

		// NOTE: We don't define a to-python conversion.

		// From python conversion.
		bp::converter::registry::push_back(
				&ConversionTopologicalFeatureCollectionFunctionArgument::convertible,
				&ConversionTopologicalFeatureCollectionFunctionArgument::construct,
				bp::type_id<TopologicalFeatureCollectionFunctionArgument>());
	}


	/**
	 * A from-python converter from a sequence of (@a FeatureCollectionFunctionArgument and optional
	 * @a ResolveTopologyParameters) to a @a TopologicalFeatureCollectionSequenceFunctionArgument.
	 */
	struct ConversionTopologicalFeatureCollectionSequenceFunctionArgument :
			private boost::noncopyable
	{
		static
		void *
		convertible(
				PyObject *obj)
		{
			namespace bp = boost::python;

			if (TopologicalFeatureCollectionSequenceFunctionArgument::is_convertible(
				bp::object(bp::handle<>(bp::borrowed(obj)))))
			{
				return obj;
			}

			return NULL;
		}

		static
		void
		construct(
				PyObject *obj,
				boost::python::converter::rvalue_from_python_stage1_data *data)
		{
			namespace bp = boost::python;

			void *const storage = reinterpret_cast<
					bp::converter::rvalue_from_python_storage<
							TopologicalFeatureCollectionSequenceFunctionArgument> *>(
									data)->storage.bytes;

			new (storage) TopologicalFeatureCollectionSequenceFunctionArgument(
					TopologicalFeatureCollectionSequenceFunctionArgument::create(
							bp::object(bp::handle<>(bp::borrowed(obj)))));

			data->convertible = storage;
		}
	};


	/**
	 * Registers a from-python converter from a sequence of (@a FeatureCollectionFunctionArgument and optional
	 * @a ResolveTopologyParameters) to a @a TopologicalFeatureCollectionSequenceFunctionArgument.
	 */
	void
	register_conversion_topological_feature_collection_sequence_function_argument()
	{
		// Register function argument types variant.
		PythonConverterUtils::register_variant_conversion<
				TopologicalFeatureCollectionSequenceFunctionArgument::function_argument_type>();

		// NOTE: We don't define a to-python conversion.

		// From python conversion.
		bp::converter::registry::push_back(
				&ConversionTopologicalFeatureCollectionSequenceFunctionArgument::convertible,
				&ConversionTopologicalFeatureCollectionSequenceFunctionArgument::construct,
				bp::type_id<TopologicalFeatureCollectionSequenceFunctionArgument>());
	}
}


bool
GPlatesApi::TopologicalFeatureCollectionFunctionArgument::is_convertible(
		bp::object python_function_argument)
{
	// Test all supported types (in function_argument_type) except the bp::object (since that's a sequence).
	if (bp::extract<FeatureCollectionFunctionArgument>(python_function_argument).check())
	{
		return true;
	}

	//
	// Else it's a boost::python::object so we're expecting it to be a
	// 2-sequence (FeatureCollectionFunctionArgument, ResolveTopologyParameters)
	// which requires further checking.
	//

	// Should be a sequence of size 2.
	boost::optional<unsigned int> sequence_size = PythonExtractUtils::check_sequence<bp::object>(python_function_argument);
	if (!sequence_size ||
		sequence_size.get() != 2)
	{
		return false;
	}

	// Extract the two sequence objects so we can check their type.
	std::vector<bp::object> sequence_of_objects;
	PythonExtractUtils::extract_sequence(sequence_of_objects, python_function_argument);

	// Check we have a FeatureCollectionFunctionArgument and a ResolveTopologyParameters.
	if (!bp::extract<FeatureCollectionFunctionArgument>(sequence_of_objects[0]).check() ||
		!bp::extract<ResolveTopologyParameters::non_null_ptr_type>(sequence_of_objects[1]).check())
	{
		return false;
	}

	return true;
}


GPlatesApi::TopologicalFeatureCollectionFunctionArgument
GPlatesApi::TopologicalFeatureCollectionFunctionArgument::create(
		bp::object python_function_argument)
{
	const auto data = create_feature_collection(bp::extract<function_argument_type>(python_function_argument));

	return TopologicalFeatureCollectionFunctionArgument(std::get<0>(data), std::get<1>(data));
}


GPlatesApi::TopologicalFeatureCollectionFunctionArgument
GPlatesApi::TopologicalFeatureCollectionFunctionArgument::create(
		const function_argument_type &function_argument)
{
	const auto data = create_feature_collection(function_argument);

	return TopologicalFeatureCollectionFunctionArgument(std::get<0>(data), std::get<1>(data));
}


std::tuple<GPlatesApi::FeatureCollectionFunctionArgument, GPlatesApi::ResolveTopologyParameters::non_null_ptr_to_const_type>
GPlatesApi::TopologicalFeatureCollectionFunctionArgument::create_feature_collection(
		const function_argument_type &function_argument)
{
	if (const FeatureCollectionFunctionArgument *feature_collection_function_argument =
		boost::get<FeatureCollectionFunctionArgument>(&function_argument))
	{
		return std::make_tuple(*feature_collection_function_argument, ResolveTopologyParameters::create());
	}
	else
	{
		const bp::object sequence = boost::get<bp::object>(function_argument);

		//
		// The 2-sequence (FeatureCollectionFunctionArgument, ResolveTopologyParameters).
		//
		// Note that we've already checked that it's a 2-sequence containing the above two types.
		// We're just extracting it now.
		//

		std::vector<bp::object> sequence_of_objects;
		PythonExtractUtils::extract_sequence(sequence_of_objects, sequence);

		FeatureCollectionFunctionArgument feature_collection =
				bp::extract<FeatureCollectionFunctionArgument>(sequence_of_objects[0]);
		ResolveTopologyParameters::non_null_ptr_type resolve_topology_parameters =
				bp::extract<ResolveTopologyParameters::non_null_ptr_type>(sequence_of_objects[1]);

		return std::make_tuple(feature_collection, resolve_topology_parameters);
	}
}


bool
GPlatesApi::TopologicalFeatureCollectionSequenceFunctionArgument::is_convertible(
		bp::object python_function_argument)
{
	// Test all supported types (in function_argument_type) except the bp::object (since that's a sequence).
	if (bp::extract<TopologicalFeatureCollectionFunctionArgument>(python_function_argument).check())
	{
		return true;
	}

	// Else it's a boost::python::object so we're expecting it to be a sequence of
	// TopologicalFeatureCollectionFunctionArgument's which requires further checking.
	return static_cast<bool>(
			PythonExtractUtils::check_sequence<TopologicalFeatureCollectionFunctionArgument>(python_function_argument));
}


GPlatesApi::TopologicalFeatureCollectionSequenceFunctionArgument
GPlatesApi::TopologicalFeatureCollectionSequenceFunctionArgument::create(
		boost::python::object python_function_argument)
{
	return create(
			boost::python::extract<function_argument_type>(python_function_argument));
}


GPlatesApi::TopologicalFeatureCollectionSequenceFunctionArgument
GPlatesApi::TopologicalFeatureCollectionSequenceFunctionArgument::create(
		const function_argument_type &function_argument)
{
	return TopologicalFeatureCollectionSequenceFunctionArgument(
			create_feature_collections(function_argument));
}


std::vector<GPlatesApi::TopologicalFeatureCollectionFunctionArgument>
GPlatesApi::TopologicalFeatureCollectionSequenceFunctionArgument::create_feature_collections(
		const function_argument_type &function_argument)
{
	if (const TopologicalFeatureCollectionFunctionArgument *feature_collection_function_argument =
		boost::get<TopologicalFeatureCollectionFunctionArgument>(&function_argument))
	{
		return std::vector<GPlatesApi::TopologicalFeatureCollectionFunctionArgument>(1, *feature_collection_function_argument);
	}
	else
	{
		//
		// A sequence of feature collections and/or filenames (and their optional resolved topology parameters).
		//

		const bp::object sequence = boost::get<bp::object>(function_argument);

		// Use convenience class 'TopologicalFeatureCollectionFunctionArgument' to access the feature collections.
		std::vector<GPlatesApi::TopologicalFeatureCollectionFunctionArgument> feature_collections;
		PythonExtractUtils::extract_sequence(feature_collections, sequence);

		return feature_collections;
	}
}


void
GPlatesApi::TopologicalFeatureCollectionSequenceFunctionArgument::get_feature_collections(
		std::vector<GPlatesModel::FeatureCollectionHandle::non_null_ptr_type> &feature_collections) const
{
	for (const TopologicalFeatureCollectionFunctionArgument &feature_collection : d_feature_collections)
	{
		feature_collections.push_back(feature_collection.get_feature_collection());
	}
}


void
GPlatesApi::TopologicalFeatureCollectionSequenceFunctionArgument::get_files(
		std::vector<GPlatesFileIO::File::non_null_ptr_type> &feature_collection_files) const
{
	for (const TopologicalFeatureCollectionFunctionArgument &feature_collection : d_feature_collections)
	{
		feature_collection_files.push_back(feature_collection.get_file());
	}
}


void
GPlatesApi::TopologicalFeatureCollectionSequenceFunctionArgument::get_resolved_topology_parameters(
		std::vector<ResolveTopologyParameters::non_null_ptr_to_const_type> &resolved_topology_parameters) const
{
	for (const TopologicalFeatureCollectionFunctionArgument &feature_collection : d_feature_collections)
	{
		resolved_topology_parameters.push_back(feature_collection.get_resolve_topology_parameters());
	}
}


void
export_topological_feature_collection_function_argument()
{
	// Registers a from-python converter from FeatureCollectionSequenceFunctionArgument and optional
	// ResolveTopologyParameters to a TopologicalFeatureCollectionSequenceFunctionArgument.
	GPlatesApi::register_conversion_topological_feature_collection_function_argument();

	// Registers a from-python converter from a sequence of (FeatureCollectionFunctionArgument and optional
	// ResolveTopologyParameters) to a TopologicalFeatureCollectionSequenceFunctionArgument.
	GPlatesApi::register_conversion_topological_feature_collection_sequence_function_argument();
}
