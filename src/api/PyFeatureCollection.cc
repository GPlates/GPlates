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

#include "PyFeatureCollectionFileFormatRegistry.h"
#include "PyFeatureCollectionFunctionArgument.h"
#include "PythonConverterUtils.h"
#include "PythonExtractUtils.h"
#include "PythonHashDefVisitor.h"
#include "PythonPickle.h"

#include "file-io/FeatureCollectionFileFormatRegistry.h"
#include "file-io/ReadErrorAccumulation.h"

#include "global/python.h"

#include "model/FeatureHandle.h"
#include "model/FeatureCollectionHandle.h"

#include "utils/ReferenceCount.h"


namespace bp = boost::python;


namespace GPlatesApi
{
	/**
	 * Enumeration to determine how features are returned.
	 */
	namespace FeatureReturn
	{
		enum Value
		{
			EXACTLY_ONE, // Returns a single element only if there's one match to the query.
			FIRST,       // Returns the first element that matches the query.
			ALL          // Returns all elements that matches the query.
		};
	};


	bp::object
	feature_collection_handle_read(
			bp::object filename_object)
	{
		GPlatesFileIO::FeatureCollectionFileFormat::Registry registry;

		// Use the function in "PyFeatureCollectionFileFormatRegistry.h"...
		return read_feature_collections(registry, filename_object);
	}

	void
	feature_collection_handle_write(
			GPlatesModel::FeatureCollectionHandle::non_null_ptr_type feature_collection,
			const QString &filename)
	{
		GPlatesFileIO::FeatureCollectionFileFormat::Registry registry;

		// Use the function in "PyFeatureCollectionFileFormatRegistry.h"...
		write_feature_collection(registry, feature_collection, filename);
	}

	GPlatesModel::FeatureCollectionHandle::non_null_ptr_type
	feature_collection_handle_create(
			bp::object features_object)
	{
		if (features_object == bp::object()/*Py_None*/)
		{
			// Return an empty feature collection.
			return GPlatesModel::FeatureCollectionHandle::create();
		}

		// Before we use 'FeatureCollectionFunctionArgument' to extract the feature collection
		// we check that 'features_object' is not already a feature collection.
		// If it is then we need to perform a shallow copy (as is typical of Python constructors
		// such as 'new_list = list(old_list)'). We do this before trying 'FeatureCollectionFunctionArgument'
		// since it will accept a feature collection directly and return it as the same feature collection object
		// and so there will be no shallow copy.
		bp::extract<GPlatesModel::FeatureCollectionHandle::non_null_ptr_type> extract_feature_collection(features_object);
		if (extract_feature_collection.check())
		{
			GPlatesModel::FeatureCollectionHandle::non_null_ptr_type feature_collection = extract_feature_collection();

			// Create a feature collection to add the features to.
			GPlatesModel::FeatureCollectionHandle::non_null_ptr_type new_feature_collection =
					GPlatesModel::FeatureCollectionHandle::create();

			// Copy the features into the new feature collection.
			GPlatesModel::FeatureCollectionHandle::iterator feature_collection_iter = feature_collection->begin();
			GPlatesModel::FeatureCollectionHandle::iterator feature_collection_end = feature_collection->end();
			for ( ; feature_collection_iter != feature_collection_end; ++feature_collection_iter)
			{
				GPlatesModel::FeatureHandle::non_null_ptr_type feature = *feature_collection_iter;
				new_feature_collection->add(feature);
			}

			return new_feature_collection;
		}

		bp::extract<FeatureCollectionFunctionArgument> extract_feature_collection_function_argument(features_object);
		if (!extract_feature_collection_function_argument.check())
		{
			PyErr_SetString(PyExc_TypeError,
					"Expected an optional filename, or sequence of features, or a single feature");
			bp::throw_error_already_set();
		}

		FeatureCollectionFunctionArgument feature_collection_function_argument = extract_feature_collection_function_argument();

		return feature_collection_function_argument.get_feature_collection();
	}

	/**
	 * Clone an existing feature collection.
	 *
	 * NOTE: We don't use FeatureHandle::clone() because it currently does a shallow copy
	 * instead of a deep copy.
	 * FIXME: Once FeatureHandle has been updated to use the same revisioning system as
	 * TopLevelProperty and PropertyValue then just delegate directly to FeatureCollectionHandle::clone().
	 */
	const GPlatesModel::FeatureCollectionHandle::non_null_ptr_type
	feature_collection_handle_clone(
			GPlatesModel::FeatureCollectionHandle &feature_collection_handle)
	{
		GPlatesModel::FeatureCollectionHandle::non_null_ptr_type cloned_feature_collection =
				GPlatesModel::FeatureCollectionHandle::create();

		// Iterate over the feature of the feature collection and clone them.
		GPlatesModel::FeatureCollectionHandle::iterator features_iter = feature_collection_handle.begin();
		GPlatesModel::FeatureCollectionHandle::iterator features_end = feature_collection_handle.end();
		for ( ; features_iter != features_end; ++features_iter)
		{
			GPlatesModel::FeatureHandle::non_null_ptr_type feature = *features_iter;

			GPlatesModel::FeatureHandle::non_null_ptr_type cloned_feature =
					GPlatesModel::FeatureHandle::create(feature->feature_type());

			// Iterate over the properties of the feature and clone them.
			GPlatesModel::FeatureHandle::iterator properties_iter = feature->begin();
			GPlatesModel::FeatureHandle::iterator properties_end = feature->end();
			for ( ; properties_iter != properties_end; ++properties_iter)
			{
				GPlatesModel::TopLevelProperty::non_null_ptr_type feature_property = *properties_iter;

				cloned_feature->add(feature_property->clone());
			}

			cloned_feature_collection->add(cloned_feature);
		}

		// Copy the tags also.
		cloned_feature_collection->tags() = feature_collection_handle.tags();

		return cloned_feature_collection;
	}

	//
	// Support for "__getitem__".
	//
	// But we only allowing indexing with an index. We don't allow slices because it makes less
	// sense (since indexing features is really just an alternative to iterating in a 'for' loop).
	//
	bp::object
	feature_collection_handle_get_item(
			GPlatesModel::FeatureCollectionHandle &feature_collection_handle,
			boost::python::object feature_index_object)
	{
		// The feature index should be an integer.
		bp::extract<long> extract_feature_index(feature_index_object);
		if (!extract_feature_index.check())
		{
			PyErr_SetString(PyExc_TypeError, "Invalid feature index type, must be an integer (slices not allowed)");
			bp::throw_error_already_set();

			return bp::object();  // Cannot get here.
		}

		long feature_index = extract_feature_index();
		if (feature_index < 0)
		{
			feature_index += feature_collection_handle.size();
		}

		if (feature_index >= boost::numeric_cast<long>(feature_collection_handle.size()) ||
			feature_index < 0)
		{
			PyErr_SetString(PyExc_IndexError, "Feature index out of range");
			bp::throw_error_already_set();
		}

		GPlatesModel::FeatureCollectionHandle::iterator feature_iter = feature_collection_handle.begin();
		// NOTE: 'feature_iter' is not random access, so must increment 'feature_index' times...
		std::advance(feature_iter, feature_index);
		GPlatesModel::FeatureHandle::non_null_ptr_type feature = *feature_iter;

		return bp::object(feature);
	}

	// Temporarily comment out until we merge the python-model-revisions branch into this (python-api) branch because
	// currently '*feature_iter = feature' does not do anything (since '*feature_iter' just returns a non-null pointer).
#if 0
	//
	// Support for "__setitem__".
	//
	// But we only allowing indexing with an index. We don't allow slices because it makes less
	// sense (since indexing features is really just an alternative to iterating in a 'for' loop
	// to get a feature, modify it and then set it back in the feature collection at same index).
	//
	void
	feature_collection_handle_set_item(
			GPlatesModel::FeatureCollectionHandle &feature_collection_handle,
			boost::python::object feature_index_object,
			GPlatesModel::FeatureHandle::non_null_ptr_type feature)
	{
		// Feature index should be an integer.
		bp::extract<long> extract_feature_index(feature_index_object);
		if (!extract_feature_index.check())
		{
			PyErr_SetString(PyExc_TypeError, "Invalid feature index type, must be an integer (slices not allowed)");
			bp::throw_error_already_set();
		}

		long feature_index = extract_feature_index();
		if (feature_index < 0)
		{
			feature_index += feature_collection_handle.size();
		}

		if (feature_index >= boost::numeric_cast<long>(feature_collection_handle.size()) ||
			feature_index < 0)
		{
			PyErr_SetString(PyExc_IndexError, "Feature index out of range");
			bp::throw_error_already_set();
		}

		GPlatesModel::FeatureCollectionHandle::iterator feature_iter = feature_collection_handle.begin();
		// NOTE: 'feature_iter' is not random access, so must increment 'index' times...
		std::advance(feature_iter, feature_index);
		*feature_iter = feature;
	}
#endif

	void
	feature_collection_handle_add(
			GPlatesModel::FeatureCollectionHandle &feature_collection_handle,
			bp::object feature_object)
	{
		// See if a single feature.
		bp::extract<GPlatesModel::FeatureHandle::non_null_ptr_type> extract_feature(feature_object);
		if (extract_feature.check())
		{
			GPlatesModel::FeatureHandle::non_null_ptr_type feature = extract_feature();
			feature_collection_handle.add(feature);

			return;
		}

		// Try a sequence of features next.
		std::vector<GPlatesModel::FeatureHandle::non_null_ptr_type> features;
		PythonExtractUtils::extract_iterable(features, feature_object, "Expected Feature or sequence of Feature's");

		for (GPlatesModel::FeatureHandle::non_null_ptr_type feature : features)
		{
			feature_collection_handle.add(feature);
		}
	}

	void
	feature_collection_handle_remove(
			GPlatesModel::FeatureCollectionHandle &feature_collection_handle,
			bp::object feature_query_object)
	{
		// See if a single feature type.
		bp::extract<GPlatesModel::FeatureType> extract_feature_type(feature_query_object);
		if (extract_feature_type.check())
		{
			const GPlatesModel::FeatureType feature_type = extract_feature_type();

			// Search for the feature type.
			GPlatesModel::FeatureCollectionHandle::iterator features_iter = feature_collection_handle.begin();
			GPlatesModel::FeatureCollectionHandle::iterator features_end = feature_collection_handle.end();
			for ( ; features_iter != features_end; ++features_iter)
			{
				GPlatesModel::FeatureHandle::non_null_ptr_type collection_feature = *features_iter;

				if (feature_type == collection_feature->feature_type())
				{
					// Note that removing a feature does not prevent us from incrementing to the next feature.
					feature_collection_handle.remove(features_iter);
				}
			}

			return;
		}

		// See if a single feature ID.
		bp::extract<GPlatesModel::FeatureId> extract_feature_id(feature_query_object);
		if (extract_feature_id.check())
		{
			const GPlatesModel::FeatureId feature_id = extract_feature_id();

			// Search for the feature ID.
			GPlatesModel::FeatureCollectionHandle::iterator features_iter = feature_collection_handle.begin();
			GPlatesModel::FeatureCollectionHandle::iterator features_end = feature_collection_handle.end();
			for ( ; features_iter != features_end; ++features_iter)
			{
				GPlatesModel::FeatureHandle::non_null_ptr_type collection_feature = *features_iter;

				if (feature_id == collection_feature->feature_id())
				{
					// Note that removing a feature does not prevent us from incrementing to the next feature.
					feature_collection_handle.remove(features_iter);
				}
			}

			return;
		}

		// See if a single feature.
		bp::extract<GPlatesModel::FeatureHandle::non_null_ptr_type> extract_feature(feature_query_object);
		if (extract_feature.check())
		{
			GPlatesModel::FeatureHandle::non_null_ptr_type feature = extract_feature();

			// Search for the feature.
			// Note: This searches for the same feature *instance* - it does not compare values of
			// two different feature instances.
			GPlatesModel::FeatureCollectionHandle::iterator features_iter = feature_collection_handle.begin();
			GPlatesModel::FeatureCollectionHandle::iterator features_end = feature_collection_handle.end();
			for ( ; features_iter != features_end; ++features_iter)
			{
				GPlatesModel::FeatureHandle::non_null_ptr_type collection_feature = *features_iter;

				// Compare pointers not pointed-to-objects.
				if (feature == collection_feature)
				{
					feature_collection_handle.remove(features_iter);
					return;
				}
			}

			// Raise the 'ValueError' python exception if the feature was not found.
			PyErr_SetString(PyExc_ValueError, "Feature instance not found");
			bp::throw_error_already_set();
		}

		// See if a single predicate callable.
		if (PyObject_HasAttrString(feature_query_object.ptr(), "__call__"))
		{
			GPlatesModel::FeatureCollectionHandle::iterator features_iter = feature_collection_handle.begin();
			GPlatesModel::FeatureCollectionHandle::iterator features_end = feature_collection_handle.end();
			for ( ; features_iter != features_end; ++features_iter)
			{
				GPlatesModel::FeatureHandle::non_null_ptr_type collection_feature = *features_iter;

				// See if current feature matches the query.
				// Feature query is a callable predicate...
				if (bp::extract<bool>(feature_query_object(collection_feature)))
				{
					// Note that removing a feature does not prevent us from incrementing to the next feature.
					feature_collection_handle.remove(features_iter);
				}
			}

			return;
		}

		const char *type_error_string = "Expected FeatureType, or FeatureId, or Feature, or predicate, "
				"or a sequence of any combination of them";

		// Try an iterable sequence next.
		typedef std::vector<bp::object> feature_queries_seq_type;
		feature_queries_seq_type feature_queries_seq;
		PythonExtractUtils::extract_iterable(feature_queries_seq, feature_query_object, type_error_string);

		typedef std::vector<GPlatesModel::FeatureType> feature_types_seq_type;
		feature_types_seq_type feature_types_seq;

		typedef std::vector<GPlatesModel::FeatureId> feature_ids_seq_type;
		feature_ids_seq_type feature_ids_seq;

		typedef std::vector<GPlatesModel::FeatureHandle::non_null_ptr_type> features_seq_type;
		features_seq_type features_seq;

		typedef std::vector<bp::object> predicates_seq_type;
		predicates_seq_type predicates_seq;

		// Extract the different feature query types into their own arrays.
		feature_queries_seq_type::const_iterator feature_queries_iter = feature_queries_seq.begin();
		feature_queries_seq_type::const_iterator feature_queries_end = feature_queries_seq.end();
		for ( ; feature_queries_iter != feature_queries_end; ++feature_queries_iter)
		{
			const bp::object feature_query = *feature_queries_iter;

			// See if a feature type.
			bp::extract<GPlatesModel::FeatureType> extract_feature_type_element(feature_query);
			if (extract_feature_type_element.check())
			{
				const GPlatesModel::FeatureType feature_type = extract_feature_type_element();
				feature_types_seq.push_back(feature_type);
				continue;
			}

			// See if a feature id.
			bp::extract<GPlatesModel::FeatureId> extract_feature_id_element(feature_query);
			if (extract_feature_id_element.check())
			{
				const GPlatesModel::FeatureId feature_id = extract_feature_id_element();
				feature_ids_seq.push_back(feature_id);
				continue;
			}

			// See if a feature.
			bp::extract<GPlatesModel::FeatureHandle::non_null_ptr_type> extract_feature_element(feature_query);
			if (extract_feature_element.check())
			{
				GPlatesModel::FeatureHandle::non_null_ptr_type feature = extract_feature_element();
				features_seq.push_back(feature);
				continue;
			}

			// See if a predicate callable.
			if (PyObject_HasAttrString(feature_query.ptr(), "__call__"))
			{
				predicates_seq.push_back(feature_query);
				continue;
			}

			// Unexpected feature query type so raise an error.
			PyErr_SetString(PyExc_TypeError, type_error_string);
			bp::throw_error_already_set();
		}

		//
		// Process features first to avoid unnecessarily throwing ValueError exception.
		//

		// Remove duplicate feature pointers.
		features_seq.erase(
				std::unique(features_seq.begin(), features_seq.end()),
				features_seq.end());

		if (!features_seq.empty())
		{
			// Search for the features.
			GPlatesModel::FeatureCollectionHandle::iterator features_iter = feature_collection_handle.begin();
			GPlatesModel::FeatureCollectionHandle::iterator features_end = feature_collection_handle.end();
			for ( ; features_iter != features_end; ++features_iter)
			{
				GPlatesModel::FeatureHandle::non_null_ptr_type collection_feature = *features_iter;

				// Compare pointers not pointed-to-objects.
				features_seq_type::iterator features_seq_iter =
						std::find(features_seq.begin(), features_seq.end(), collection_feature);
				if (features_seq_iter != features_seq.end())
				{
					// Remove the feature from the collection.
					// Note that removing a feature does not prevent us from incrementing to the next feature.
					feature_collection_handle.remove(features_iter);
					// Record that we have removed this feature.
					features_seq.erase(features_seq_iter);
				}
			}

			// Raise the 'ValueError' python exception if not all features were found.
			if (!features_seq.empty())
			{
				PyErr_SetString(PyExc_ValueError, "Not all feature instances were found");
				bp::throw_error_already_set();
			}
		}

		//
		// Process feature types next.
		//

		// Remove duplicate feature types.
		feature_types_seq.erase(
				std::unique(feature_types_seq.begin(), feature_types_seq.end()),
				feature_types_seq.end());

		if (!feature_types_seq.empty())
		{
			// Search for the feature types.
			GPlatesModel::FeatureCollectionHandle::iterator features_iter = feature_collection_handle.begin();
			GPlatesModel::FeatureCollectionHandle::iterator features_end = feature_collection_handle.end();
			for ( ; features_iter != features_end; ++features_iter)
			{
				GPlatesModel::FeatureHandle::non_null_ptr_type collection_feature = *features_iter;

				feature_types_seq_type::iterator feature_types_seq_iter = std::find(
						feature_types_seq.begin(),
						feature_types_seq.end(),
						collection_feature->feature_type());
				if (feature_types_seq_iter != feature_types_seq.end())
				{
					// Remove the feature from the collection.
					// Note that removing a feature does not prevent us from incrementing to the next feature.
					feature_collection_handle.remove(features_iter);
				}
			}
		}

		//
		// Process feature IDs next.
		//

		// Remove duplicate feature IDs.
		feature_ids_seq.erase(
				std::unique(feature_ids_seq.begin(), feature_ids_seq.end()),
				feature_ids_seq.end());

		if (!feature_ids_seq.empty())
		{
			// Search for the feature IDs.
			GPlatesModel::FeatureCollectionHandle::iterator features_iter = feature_collection_handle.begin();
			GPlatesModel::FeatureCollectionHandle::iterator features_end = feature_collection_handle.end();
			for ( ; features_iter != features_end; ++features_iter)
			{
				GPlatesModel::FeatureHandle::non_null_ptr_type collection_feature = *features_iter;

				feature_ids_seq_type::iterator feature_ids_seq_iter = std::find(
						feature_ids_seq.begin(),
						feature_ids_seq.end(),
						collection_feature->feature_id());
				if (feature_ids_seq_iter != feature_ids_seq.end())
				{
					// Remove the feature from the collection.
					// Note that removing a feature does not prevent us from incrementing to the next feature.
					feature_collection_handle.remove(features_iter);
				}
			}
		}

		//
		// Process predicate callables next.
		//

		if (!predicates_seq.empty())
		{
			// Search for matching predicate callables.
			GPlatesModel::FeatureCollectionHandle::iterator features_iter = feature_collection_handle.begin();
			GPlatesModel::FeatureCollectionHandle::iterator features_end = feature_collection_handle.end();
			for ( ; features_iter != features_end; ++features_iter)
			{
				GPlatesModel::FeatureHandle::non_null_ptr_type collection_feature = *features_iter;

				// Test each predicate callable.
				predicates_seq_type::const_iterator predicates_seq_iter = predicates_seq.begin();
				predicates_seq_type::const_iterator predicates_seq_end = predicates_seq.end();
				for ( ; predicates_seq_iter != predicates_seq_end; ++predicates_seq_iter)
				{
					bp::object predicate = *predicates_seq_iter;

					// See if current feature matches the query.
					// Feature query is a callable predicate...
					if (bp::extract<bool>(predicate(collection_feature)))
					{
						// Note that removing a feature does not prevent us from incrementing to the next feature.
						feature_collection_handle.remove(features_iter);
						break;
					}
				}
			}
		}
	}

	bp::object
	feature_collection_handle_get_feature(
			GPlatesModel::FeatureCollectionHandle &feature_collection_handle,
			bp::object feature_query_object,
			FeatureReturn::Value feature_return)
	{
		boost::optional<GPlatesModel::FeatureType> feature_type;
		boost::optional<GPlatesModel::FeatureId> feature_id;

		// See if feature query is a feature type.
		bp::extract<GPlatesModel::FeatureType> extract_feature_type(feature_query_object);
		if (extract_feature_type.check())
		{
			feature_type = extract_feature_type();
		}
		else
		{
			// See if feature query is a feature id.
			bp::extract<GPlatesModel::FeatureId> extract_feature_id(feature_query_object);
			if (extract_feature_id.check())
			{
				feature_id = extract_feature_id();
			}
		}

		if (feature_return == FeatureReturn::EXACTLY_ONE)
		{
			boost::optional<GPlatesModel::FeatureHandle::non_null_ptr_type> feature;

			// Search for the feature.
			GPlatesModel::FeatureCollectionHandle::iterator features_iter = feature_collection_handle.begin();
			GPlatesModel::FeatureCollectionHandle::iterator features_end = feature_collection_handle.end();
			for ( ; features_iter != features_end; ++features_iter)
			{
				GPlatesModel::FeatureHandle::non_null_ptr_type collection_feature = *features_iter;

				// See if current feature matches the query.
				bool feature_query_result = false;
				if (feature_type)
				{
					feature_query_result = (feature_type.get() == collection_feature->feature_type());
				}
				else if (feature_id)
				{
					feature_query_result = (feature_id.get() == collection_feature->feature_id());
				}
				else
				{
					// Property query is a callable predicate...
					feature_query_result = bp::extract<bool>(feature_query_object(collection_feature));
				}

				if (feature_query_result)
				{
					if (feature)
					{
						// Found two features matching same query but expecting only one.
						return bp::object()/*Py_None*/;
					}

					feature = collection_feature;
				}
			}

			// Return exactly one found feature (if found).
			if (feature)
			{
				return bp::object(feature.get());
			}
		}
		else if (feature_return == FeatureReturn::FIRST)
		{
			// Search for the feature.
			GPlatesModel::FeatureCollectionHandle::iterator features_iter = feature_collection_handle.begin();
			GPlatesModel::FeatureCollectionHandle::iterator features_end = feature_collection_handle.end();
			for ( ; features_iter != features_end; ++features_iter)
			{
				GPlatesModel::FeatureHandle::non_null_ptr_type collection_feature = *features_iter;

				// See if current feature matches the query.
				bool feature_query_result = false;
				if (feature_type)
				{
					feature_query_result = (feature_type.get() == collection_feature->feature_type());
				}
				else if (feature_id)
				{
					feature_query_result = (feature_id.get() == collection_feature->feature_id());
				}
				else
				{
					// Property query is a callable predicate...
					feature_query_result = bp::extract<bool>(feature_query_object(collection_feature));
				}

				if (feature_query_result)
				{
					// Return first found.
					return bp::object(collection_feature);
				}
			}
		}
		else
		{
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					feature_return == FeatureReturn::ALL,
					GPLATES_ASSERTION_SOURCE);

			bp::list features;

			// Search for the features.
			GPlatesModel::FeatureCollectionHandle::iterator features_iter = feature_collection_handle.begin();
			GPlatesModel::FeatureCollectionHandle::iterator features_end = feature_collection_handle.end();
			for ( ; features_iter != features_end; ++features_iter)
			{
				GPlatesModel::FeatureHandle::non_null_ptr_type collection_feature = *features_iter;

				// See if current feature matches the query.
				bool feature_query_result = false;
				if (feature_type)
				{
					feature_query_result = (feature_type.get() == collection_feature->feature_type());
				}
				else if (feature_id)
				{
					feature_query_result = (feature_id.get() == collection_feature->feature_id());
				}
				else
				{
					// Property query is a callable predicate...
					feature_query_result = bp::extract<bool>(feature_query_object(collection_feature));
				}

				if (feature_query_result)
				{
					features.append(collection_feature);
				}
			}

			// Returned list could be empty if no features matched.
			return features;
		}

		return bp::object()/*Py_None*/;
	}
}


void
export_feature_collection()
{
	// An enumeration nested within 'pygplates' (ie, current) module.
	bp::enum_<GPlatesApi::FeatureReturn::Value>("FeatureReturn")
			.value("exactly_one", GPlatesApi::FeatureReturn::EXACTLY_ONE)
			.value("first", GPlatesApi::FeatureReturn::FIRST)
			.value("all", GPlatesApi::FeatureReturn::ALL);

	//
	// FeatureCollection - docstrings in reStructuredText (see http://sphinx-doc.org/rest.html).
	//
	bp::class_<
			GPlatesModel::FeatureCollectionHandle,
			GPlatesModel::FeatureCollectionHandle::non_null_ptr_type,
			boost::noncopyable>(
					"FeatureCollection",
					"A feature collection aggregates a set of features into a collection. This is traditionally "
					"so that a group of features can be loaded, saved or unloaded in a single operation.\n"
					"\n"
					"For example, to read coastline features from a file:\n"
					"::\n"
					"\n"
					"  coastline_feature_collection = pygplates.FeatureCollection('coastlines.gpml')\n"
					"\n"
					"And to write coastline features to a file:\n"
					"::\n"
					"\n"
					"  coastline_feature_collection = pygplates.FeatureCollection(coastline_features)\n"
					"  coastline_feature_collection.write('coastlines.gpml')\n"
					"\n"
					"The following *feature collection* file formats are currently supported:\n"
					"\n"
					"=============================== ======================= ============== =================\n"
					"File Format                     Filename Extension      Supports Read  Supports Write\n"
					"=============================== ======================= ============== =================\n"
					"GPlates Markup Language         '.gpml'                 Yes            Yes\n"
					"Compressed GPML                 '.gpmlz' or '.gpml.gz'  Yes            Yes\n"
					"PLATES4 line                    '.dat' or '.pla'        Yes            Yes\n"
					"PLATES4 rotation                '.rot'                  Yes            Yes\n"
					"GPlates rotation                '.grot'                 Yes            Yes\n"
					"ESRI Shapefile                  '.shp'                  Yes            Yes\n"
					"GeoJSON                         '.geojson' or '.json'   Yes            Yes\n"
					"GeoPackage                      '.gpkg'                 Yes            Yes\n"
					"OGR GMT                         '.gmt'                  Yes            Yes\n"
					"GMT xy                          '.xy'                   No             Yes\n"
					"GMAP Virtual Geomagnetic Poles  '.vgp'                  Yes            No\n"
					"=============================== ======================= ============== =================\n"
					"\n"
					"In the future, support will be added to enable users to implement and register "
					"readers/writers for other file formats (or their own non-standard file formats).\n"
					"\n"
					"The following operations for accessing the features are supported:\n"
					"\n"
					"=========================== ==========================================================\n"
					"Operation                   Result\n"
					"=========================== ==========================================================\n"
					"``len(fc)``                 number of features in feature collection *fc*\n"
					"``for f in fc``             iterates over the features *f* in feature collection *fc*\n"
					"``fc[i]``                   the feature of *fc* at index *i*\n"
					"=========================== ==========================================================\n"
					"\n"
					"For example:\n"
					"::\n"
					"\n"
					"  num_features = len(feature_collection)\n"
					"  features_in_collection = [feature for feature in feature_collection]\n"
					"  # assert(num_features == len(features_in_collection))\n"
					"\n"
					".. note:: A feature collection can be deep copied using :meth:`clone`.\n"
					"\n"
					".. versionchanged:: 0.31\n"
					"   Can index a feature in feature collection *fc* with ``fc[i]``.\n",
					// We need this (even though "__init__" is defined) since
					// there is no publicly-accessible default constructor...
					bp::no_init)
		.def("__init__",
				bp::make_constructor(
						GPlatesApi::feature_collection_handle_create,
						bp::default_call_policies(),
						(bp::arg("features") = bp::object()/*Py_None*/)),
				"__init__([features])\n"
				"  Create a new feature collection instance.\n"
				"\n"
				"  :param features: an optional filename, or sequence of features, or a single feature\n"
				"  :type features: string, or a sequence (eg, ``list`` or ``tuple``) of :class:`Feature`, "
				"or a single :class:`Feature`\n"
				"  :raises: OpenFileForReadingError if file is not readable (if filename specified)\n"
				"  :raises: FileFormatNotSupportedError if file format (identified by the filename "
				"extension) does not support reading (when filename specified)\n"
				"\n"
				"  To create a new feature collection from a file: \n"
				"  ::\n"
				"\n"
				"    coastline_feature_collection = pygplates.FeatureCollection('coastlines.gpml')\n"
				"\n"
				"  To create a new feature collection from a sequence of :class:`features<Feature>`:\n"
				"  ::\n"
				"\n"
				"    feature_collection = pygplates.FeatureCollection([feature1, feature2])\n"
				"    \n"
				"    # ...is the equivalent of...\n"
				"    \n"
				"    feature_collection = pygplates.FeatureCollection()\n"
				"    feature_collection.add(feature1)\n"
				"    feature_collection.add(feature2)\n"
				"\n"
				"  .. note:: Since a :class:`FeatureCollection` is an iterable sequence of features it can be "
				"used in the *features* argument.\n"
				"\n"
				"     This creates a shallow copy much in the same way as with a Python list "
				"(for example ``shallow_copy_list = list(original_list)``):\n"
				"     ::\n"
				"\n"
				"       shallow_copy_feature_collection = pygplates.FeatureCollection(original_feature_collection)\n"
				"\n"
				"       # Modifying the collection/list of features in the shallow copy will not affect the original...\n"
				"       shallow_copy_feature_collection.add(...)\n"
				"       # assert(len(shallow_copy_feature_collection) != len(original_feature_collection))\n"
				"\n"
				"       # Modifying the actual feature data in the collection will affect both feature collections\n"
				"       # since the feature data is shared by both collections...\n"
				"       for feature in original_feature_collection:\n"
				"           # Changing the reconstruction plate ID affects both original and shallow copy collections.\n"
				"           feature.set_reconstruction_plate_id(...)\n")
		.def("read",
				&GPlatesApi::feature_collection_handle_read,
				(bp::arg("filename")),
				"read(filename)\n"
				// Documenting 'staticmethod' here since Sphinx cannot introspect boost-python function
				// (like it can a pure python function) and we cannot document it in first (signature) line
				// because it messes up Sphinx's signature recognition...
				"  [*staticmethod*] Reads one or more feature collections (from one or more files).\n"
				"\n"
				"  :param filename: the name of the file (or files) to read\n"
				"  :type filename: string, or sequence of strings\n"
				"  :rtype: :class:`FeatureCollection`, list of :class:`FeatureCollection`\n"
				"  :raises: OpenFileForReadingError if any file is not readable\n"
				"  :raises: FileFormatNotSupportedError if any file format (identified by a filename "
				"extension) does not support reading\n"
				"\n"
				"  ::\n"
				"\n"
				"    feature_collection = pygplates.FeatureCollection.read(filename)\n"
				"\n"
				"  ...although for a single file the following is even easier:\n"
				"  ::\n"
				"\n"
				"    feature_collection = pygplates.FeatureCollection(filename)\n"
				"\n"
				"  Multiple files can also be read:\n"
				"  ::\n"
				"\n"
				"    for feature_collection in pygplates.FeatureCollection.read([filename1, filename2]):\n"
				"        ...\n")
		.staticmethod("read")
		.def("write",
				&GPlatesApi::feature_collection_handle_write,
				(bp::arg("filename")),
				"write(filename)\n"
				"  Writes this feature collection to the file with name *filename*.\n"
				"\n"
				"  :param filename: the name of the file to write\n"
				"  :type filename: string\n"
				"  :raises: OpenFileForWritingError if the file is not writable\n"
				"  :raises: FileFormatNotSupportedError if the file format (identified by the filename "
				"extension) does not support writing\n"
				"\n"
				"  ::\n"
				"\n"
				"    feature_collection.write(filename)\n")
		.def("clone",
				&GPlatesApi::feature_collection_handle_clone,
				"clone()\n"
				"  Create a duplicate of this feature collection instance.\n"
				"\n"
				"  :rtype: :class:`FeatureCollection`\n"
				"\n"
				"  This creates a new :class:`FeatureCollection` instance with cloned versions of this "
				"collection's features. And the cloned features (in the cloned collection) are each "
				"created with a unique :class:`FeatureId`.\n")
		.def("__iter__", bp::iterator<GPlatesModel::FeatureCollectionHandle>())
		.def("__len__", &GPlatesModel::FeatureCollectionHandle::size)
		.def("__getitem__", &GPlatesApi::feature_collection_handle_get_item)
	// Temporarily comment out until we merge the python-model-revisions branch into this (python-api) branch because
	// currently '*feature_iter = feature' does not do anything (since '*feature_iter' just returns a non-null pointer).
#if 0
		.def("__setitem__", &GPlatesApi::feature_collection_handle_set_item)
#endif
		// Pickle support...
		.def(GPlatesApi::PythonPickle::PickleDefVisitor<GPlatesModel::FeatureCollectionHandle::non_null_ptr_type>())
		.def("add",
				&GPlatesApi::feature_collection_handle_add,
				(bp::arg("feature")),
				"add(feature)\n"
				"  Adds one or more features to this collection.\n"
				"\n"
				"  :param feature: one or more features to add\n"
				"  :type feature: :class:`Feature` or sequence (eg, ``list`` or ``tuple``) of :class:`Feature`\n"
				"\n"
				"  A feature collection is an *unordered* collection of features "
				"so there is no concept of where a feature is inserted in the sequence of features.\n"
				"\n"
				"  ::\n"
				"\n"
				"    feature_collection.add(feature)\n"
				"    feature_collection.add([feature1, feature2])\n")
		.def("remove",
				&GPlatesApi::feature_collection_handle_remove,
				(bp::arg("feature_query")),
				"remove(feature_query)\n"
				"  Removes features from this collection.\n"
				"\n"
				"  :param feature_query: one or more feature types, feature IDs, feature instances or "
				"predicate functions that determine which features to remove\n"
				"  :type feature_query: :class:`FeatureType`, or :class:`FeatureId`, or :class:`Feature`, "
				"or callable (accepting single :class:`Feature` argument), or a sequence (eg, ``list`` or ``tuple``) "
				"of any combination of them\n"
				"  :raises: ValueError if any specified :class:`Feature` is not currently a feature in this collection\n"
				"\n"
				"  All features matching any :class:`FeatureType`, :class:`FeatureId` or predicate callable "
				"(if any specified) will be removed. Any specified :class:`FeatureType`, :class:`FeatureId` "
				"or predicate callable that does not match a feature in this collection is ignored. "
				"However if any specified :class:`Feature` is not currently a feature in this collection "
				"then the ``ValueError`` exception is raised - note that the same :class:`Feature` *instance* "
				"must have previously been added (in other words the feature *values* are not compared - "
				"it actually looks for the same feature *instance*).\n"
				"\n"
				"  ::\n"
				"\n"
				"    feature_collection.remove(feature_id)\n"
				"    feature_collection.remove(pygplates.FeatureType.gpml_volcano)\n"
				"    feature_collection.remove([\n"
				"        pygplates.FeatureType.gpml_volcano,\n"
				"        pygplates.FeatureType.gpml_isochron])\n"
				"    \n"
				"    for feature in feature_collection:\n"
				"        if predicate(feature):\n"
				"            feature_collection.remove(feature)\n"
				"    feature_collection.remove([feature for feature in feature_collection if predicate(feature)])\n"
				"    feature_collection.remove(predicate)\n"
				"    \n"
				"    # Mix different query types.\n"
				"    # Remove a specific 'feature' instance and any features of type 'gpml:Isochron'...\n"
				"    feature_collection.remove([feature, pygplates.FeatureType.gpml_isochron])\n"
				"    \n"
				"    # Remove features of type 'gpml:Isochron' with reconstruction plate IDs less than 700...\n"
				"    feature_collection.remove(\n"
				"        lambda feature: feature.get_feature_type() == pygplates.FeatureType.gpml_isochron and\n"
				"                         feature.get_reconstruction_plate_id() < 700)\n"
				"    \n"
				"    # Remove features of type 'gpml:Volcano' and 'gpml:Isochron'...\n"
				"    feature_collection.remove([\n"
				"        lambda feature: feature.get_feature_type() == pygplates.FeatureType.gpml_volcano,\n"
				"        pygplates.FeatureType.gpml_isochron])\n"
				"    feature_collection.remove(\n"
				"        lambda feature: feature.get_feature_type() == pygplates.FeatureType.gpml_volcano or\n"
				"                         feature.get_feature_type() == pygplates.FeatureType.gpml_isochron)\n")
		.def("get",
				&GPlatesApi::feature_collection_handle_get_feature,
				(bp::arg("feature_query"),
						bp::arg("feature_query") = GPlatesApi::FeatureReturn::EXACTLY_ONE),
				"get(feature_query, [feature_return=FeatureReturn.exactly_one])\n"
				"  Returns one or more features matching a feature type, feature id or predicate.\n"
				"\n"
				"  :param feature_query: the feature type, feature id or predicate function that matches the feature "
				"(or features) to get\n"
				"  :type feature_query: :class:`FeatureType`, or :class:`FeatureId`, or callable "
				"(accepting single :class:`Feature` argument)\n"
				"  :param feature_return: whether to return exactly one feature, the first feature or "
				"all matching features\n"
				"  :type feature_return: *FeatureReturn.exactly_one*, *FeatureReturn.first* or *FeatureReturn.all*\n"
				"  :rtype: :class:`Feature`, or ``list`` of :class:`Feature`, or None\n"
				"\n"
				"  The following table maps *feature_return* values to return values:\n"
				"\n"
				"  ======================================= ==============\n"
				"  FeatureReturn Value                      Description\n"
				"  ======================================= ==============\n"
				"  exactly_one                             Returns a :class:`Feature` only if "
				"*feature_query* matches exactly one feature, otherwise ``None`` is returned.\n"
				"  first                                   Returns the first :class:`Feature` that matches "
				"*feature_query* - however note that a feature collection is an *unordered* collection of "
				"features. If no features match then ``None`` is returned.\n"
				"  all                                     Returns a ``list`` of :class:`features<Feature>` "
				"matching *feature_query*. If no features match then the returned list will be empty.\n"
				"  ======================================= ==============\n"
				"\n"
				"  ::\n"
				"\n"
				"    isochron_feature_type = pygplates.FeatureType.gpml_isochron\n"
				"    exactly_one_isochron = feature_collection.get(isochron_feature_type)\n"
				"    first_isochron = feature_collection.get(isochron_feature_type, pygplates.FeatureReturn.first)\n"
				"    all_isochrons = feature_collection.get(isochron_feature_type, pygplates.FeatureReturn.all)\n"
				"    \n"
				"    feature_matching_id = feature_collection.get(feature_id)\n"
				"    \n"
				"    # Using a predicate function that returns true if feature's type is 'gpml:Isochron' and \n"
				"    # reconstruction plate ID is less than 700.\n"
				"    recon_plate_id_less_700_isochrons = feature_collection.get(\n"
				"        lambda feature: feature.get_feature_type() == pygplates.FeatureType.gpml_isochron and\n"
				"                        feature.get_reconstruction_plate_id() < 700,\n"
				"        pygplates.FeatureReturn.all)\n")
	;

	// Register to/from Python conversions of non_null_intrusive_ptr<> including const/non-const and boost::optional.
	GPlatesApi::PythonConverterUtils::register_all_conversions_for_non_null_intrusive_ptr<GPlatesModel::FeatureCollectionHandle>();
}
