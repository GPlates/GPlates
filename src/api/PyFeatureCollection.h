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

#ifndef GPLATES_API_PYFEATURECOLLECTION_H
#define GPLATES_API_PYFEATURECOLLECTION_H

#include <vector>
#include <boost/optional.hpp>
#include <boost/variant.hpp>
#include <QString>

#include "file-io/File.h"

#include "global/python.h"

#include "model/FeatureCollectionHandle.h"


#if !defined(GPLATES_NO_PYTHON)

namespace GPlatesApi
{
	/**
	 * A convenience class for receiving a feature collection function argument as either:
	 *  (1) a feature collection, or
	 *  (2) a filename (read into a feature collection), or
	 *  (3) a feature (loaded into a feature collection), or
	 *  (4) a sequence of features - eg, a list or tuple (loaded into a feature collection).
	 *
	 * To get an instance of @a FeatureCollectionFunctionArgument you can either:
	 *  (1) specify FeatureCollectionFunctionArgument directly as a function argument type
	 *      (in the C++ function being wrapped), or
	 *  (2) use boost::python::extract<FeatureCollectionFunctionArgument>().
	 */
	class FeatureCollectionFunctionArgument
	{
	public:

		/**
		 * Types of function argument.
		 */
		typedef boost::variant<
				GPlatesModel::FeatureCollectionHandle::non_null_ptr_type,
				QString,
				GPlatesModel::FeatureHandle::non_null_ptr_type,
				boost::python::object/*sequence of features*/> function_argument_type;


		/**
		 * Returns true if @a python_function_argument is convertible to an instance of this class.
		 */
		static
		bool
		is_convertible(
				boost::python::object python_function_argument);


		explicit
		FeatureCollectionFunctionArgument(
				boost::python::object python_function_argument);


		explicit
		FeatureCollectionFunctionArgument(
				const function_argument_type &function_argument);

		/**
		 * Returns function argument as a feature collection (for passing to python).
		 */
		boost::python::object
		to_python() const;

		/**
		 * Return the function argument as a feature collection.
		 */
		GPlatesModel::FeatureCollectionHandle::non_null_ptr_type
		get_feature_collection() const;

		/**
		 * Return the function argument as a file object.
		 *
		 * If feature collection did not come from a file then it will have an empty filename.
		 */
		GPlatesFileIO::File::non_null_ptr_type
		get_file() const;

	private:

		static
		GPlatesFileIO::File::non_null_ptr_type
		initialise_feature_collection(
				const function_argument_type &function_argument);


		GPlatesFileIO::File::non_null_ptr_type d_feature_collection;
	};


	/**
	 * A convenience class for receiving one or more feature collections from a function argument as either:
	 *  (1) a feature collection, or
	 *  (2) a filename (read into a feature collection), or
	 *  (3) a feature (loaded into a feature collection), or
	 *  (4) a sequence of features (loaded into a feature collection), or
	 *  (5) a sequence of any combination of (1)-(4) above.
	 *
	 * To get an instance of @a FeatureCollectionSequenceFunctionArgument you can either:
	 *  (1) specify FeatureCollectionSequenceFunctionArgument directly as a function argument type
	 *      (in the C++ function being wrapped), or
	 *  (2) use boost::python::extract<FeatureCollectionSequenceFunctionArgument>().
	 */
	class FeatureCollectionSequenceFunctionArgument
	{
	public:

		/**
		 * Types of function argument.
		 */
		typedef boost::variant<
				FeatureCollectionFunctionArgument,
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


		explicit
		FeatureCollectionSequenceFunctionArgument(
				boost::python::object python_function_argument);


		explicit
		FeatureCollectionSequenceFunctionArgument(
				const function_argument_type &function_argument);

		explicit
		FeatureCollectionSequenceFunctionArgument(
				const std::vector<FeatureCollectionFunctionArgument> &feature_collections);

		/**
		 * Returns function argument as a boost::python::list of feature collections (for passing to python).
		 */
		boost::python::object
		to_python() const;

		/**
		 * Return the individual feature collection function arguments.
		 */
		const std::vector<FeatureCollectionFunctionArgument> &
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

	private:

		static
		void
		initialise_feature_collections(
				std::vector<FeatureCollectionFunctionArgument> &feature_collections,
				const function_argument_type &function_argument);


		std::vector<FeatureCollectionFunctionArgument> d_feature_collections;
	};
}

#endif   // GPLATES_NO_PYTHON

#endif // GPLATES_API_PYFEATURECOLLECTION_H