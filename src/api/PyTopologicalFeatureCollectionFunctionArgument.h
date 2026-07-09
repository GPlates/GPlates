/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#ifndef GPLATES_API_PYTOPOLOGICALFEATURECOLLECTIONFUNCTIONARGUMENT_H
#define GPLATES_API_PYTOPOLOGICALFEATURECOLLECTIONFUNCTIONARGUMENT_H

#include <tuple>
#include <vector>
#include <boost/optional.hpp>
#include <boost/variant.hpp>
#include <QString>

#include "PyFeatureCollectionFunctionArgument.h"
#include "PyResolveTopologyParameters.h"

#include "global/python.h"


namespace GPlatesApi
{
	/**
	 * A convenience class for receiving either:
	 *  (1) a feature collection function argument, or
	 *  (2) a feature collection function argument and ResolveTopologyParameters argument (eg, as a 2-tuple).
	 *
	 * And the feature collection function argument can be either:
	 *  (1) a feature collection, or
	 *  (2) a filename (read into a feature collection), or
	 *  (3) a feature (loaded into a feature collection), or
	 *  (4) a sequence of features - eg, a list or tuple (loaded into a feature collection).
	 *
	 * To get an instance of @a TopologicalFeatureCollectionFunctionArgument you can either:
	 *  (1) specify TopologicalFeatureCollectionFunctionArgument directly as a function argument type
	 *      (in the C++ function being wrapped), or
	 *  (2) use boost::python::extract<TopologicalFeatureCollectionFunctionArgument>().
	 */
	class TopologicalFeatureCollectionFunctionArgument
	{
	public:

		/**
		 * Types of function argument.
		 */
		typedef boost::variant<
				FeatureCollectionFunctionArgument,
				boost::python::object/*2-sequence (FeatureCollectionFunctionArgument, ResolveTopologyParameters)*/>
						function_argument_type;


		/**
		 * Returns true if @a python_function_argument is convertible to an instance of this class.
		 */
		static
		bool
		is_convertible(
				boost::python::object python_function_argument);


		static
		TopologicalFeatureCollectionFunctionArgument
		create(
				boost::python::object python_function_argument);


		static
		TopologicalFeatureCollectionFunctionArgument
		create(
				const function_argument_type &function_argument);


		/**
		 * Return the function argument as a feature collection.
		 */
		GPlatesModel::FeatureCollectionHandle::non_null_ptr_type
		get_feature_collection() const
		{
			return d_feature_collection.get_feature_collection();
		}

		/**
		 * Return the function argument as a file object.
		 *
		 * If feature collection did not come from a file then it will have an empty filename.
		 */
		GPlatesFileIO::File::non_null_ptr_type
		get_file() const
		{
			return d_feature_collection.get_file();
		}

		/**
		 * Return the optional resolved topology parameters to use for this feature collection.
		 *
		 * If this feature collection was not associated with a @a ResolveTopologyParameters then boost::none is returned.
		 */
		boost::optional<ResolveTopologyParameters::non_null_ptr_to_const_type>
		get_resolve_topology_parameters() const
		{
			return d_resolve_topology_parameters;
		}

	private:

		static
		std::tuple<
				FeatureCollectionFunctionArgument,
				boost::optional<ResolveTopologyParameters::non_null_ptr_to_const_type>>
		create_feature_collection(
				const function_argument_type &function_argument);


		TopologicalFeatureCollectionFunctionArgument(
				const FeatureCollectionFunctionArgument &feature_collection,
				boost::optional<ResolveTopologyParameters::non_null_ptr_to_const_type> resolve_topology_parameters) :
			d_feature_collection(feature_collection),
			d_resolve_topology_parameters(resolve_topology_parameters)
		{  }

		FeatureCollectionFunctionArgument d_feature_collection;
		boost::optional<ResolveTopologyParameters::non_null_ptr_to_const_type> d_resolve_topology_parameters;
	};


	/**
	 * A convenience class for receiving one or more *topological* feature collection function arguments.
	 *
	 * Each *topological* feature collection function argument can receive either:
	 *  (1) a regular feature collection function argument, or
	 *  (2) a regular feature collection function argument and ResolveTopologyParameters argument (eg, as a 2-tuple).
	 *
	 * And each regular feature collection function argument can be either:
	 *  (1) a feature collection, or
	 *  (2) a filename (read into a feature collection), or
	 *  (3) a feature (loaded into a feature collection), or
	 *  (4) a sequence of features - eg, a list or tuple (loaded into a feature collection).
	 *
	 * To get an instance of @a TopologicalFeatureCollectionFunctionArgument you can either:
	 *  (1) specify TopologicalFeatureCollectionFunctionArgument directly as a function argument type
	 *      (in the C++ function being wrapped), or
	 *  (2) use boost::python::extract<TopologicalFeatureCollectionFunctionArgument>().
	 */
	class TopologicalFeatureCollectionSequenceFunctionArgument
	{
	public:

		/**
		 * Types of function argument.
		 */
		typedef boost::variant<
				TopologicalFeatureCollectionFunctionArgument,
				boost::python::object/*sequence*/> function_argument_type;


		/**
		 * Returns true if @a python_function_argument is convertible to an instance of this class.
		 *
		 * This also checks if the function argument is a valid sequence of feature collections / filenames.
		 */
		static
		bool
		is_convertible(
				boost::python::object python_function_argument);


		static
		TopologicalFeatureCollectionSequenceFunctionArgument
		create(
				boost::python::object python_function_argument);


		static
		TopologicalFeatureCollectionSequenceFunctionArgument
		create(
				const function_argument_type &function_argument);


		/**
		 * Return the individual feature collection function arguments.
		 */
		const std::vector<TopologicalFeatureCollectionFunctionArgument> &
		get_feature_collection_function_arguments() const
		{
			return d_feature_collections;
		}

		/**
		 * Return the function argument as a sequence of feature collections appended to @a feature_collections.
		 */
		void
		get_feature_collections(
				std::vector<GPlatesModel::FeatureCollectionHandle::non_null_ptr_type> &feature_collections) const;

		/**
		 * Return the function argument as a sequence of file object appended to @a feature_collection_files.
		 *
		 * Any feature collections that did not come from files will have empty filenames.
		 */
		void
		get_files(
				std::vector<GPlatesFileIO::File::non_null_ptr_type> &feature_collection_files) const;

		/**
		 * Return the function argument as a sequence of @a ResolveTopologyParameters appended to @a resolve_topology_parameters.
		 *
		 * Any feature collections that were not associated with a @a ResolveTopologyParameters will be boost::none.
		 *
		 * Returns true if any feature collections are associated with a @a ResolveTopologyParameters.
		 *
		 * Note: The returned size of @a resolve_topology_parameters will be increased by the size of @a get_feature_collections (and @a get_files).
		 */
		bool
		get_resolve_topology_parameters(
				std::vector<boost::optional<ResolveTopologyParameters::non_null_ptr_to_const_type>> &resolve_topology_parameters) const;

	private:

		static
		std::vector<TopologicalFeatureCollectionFunctionArgument>
		create_feature_collections(
				const function_argument_type &function_argument);


		explicit
		TopologicalFeatureCollectionSequenceFunctionArgument(
				const std::vector<TopologicalFeatureCollectionFunctionArgument> &feature_collections) :
			d_feature_collections(feature_collections)
		{  }

		std::vector<TopologicalFeatureCollectionFunctionArgument> d_feature_collections;
	};
}

#endif // GPLATES_API_PYTOPOLOGICALFEATURECOLLECTIONFUNCTIONARGUMENT_H