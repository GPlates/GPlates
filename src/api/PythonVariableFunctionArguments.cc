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

#include "PythonVariableFunctionArguments.h"


void
GPlatesApi::VariableArguments::get_positional_and_keywords_args(
		positional_arguments_type &positional_args,
		keyword_arguments_type &keyword_args,
		boost::python::tuple positional_args_tuple,
		boost::python::dict keyword_args_dict)
{
	// Copy positional args from python tuple into the vector.
	const unsigned int num_positional_args = boost::python::len(positional_args_tuple);
	for (unsigned int p = 0; p < num_positional_args; ++p)
	{
		positional_args.push_back(positional_args_tuple[p]);
	}

	// Copy keyword args from python dict into the map.
	boost::python::list keywords_arg_keys = keyword_args_dict.keys();
	const unsigned int num_keyword_args = boost::python::len(keywords_arg_keys);
	for (unsigned int k = 0; k < num_keyword_args; ++k)
	{
		const boost::python::object key = keywords_arg_keys[k];

		keyword_args.insert(
				keyword_arguments_type::value_type(
						boost::python::extract<std::string>(key),
						keyword_args_dict[key]));
	}
}


void
GPlatesApi::VariableArguments::raise_python_error_if_unused(
		const positional_arguments_type &positional_args,
		unsigned int num_used_positional_args)
{
	if (positional_args.size() > num_used_positional_args)
	{
		std::ostringstream oss;
		oss << "Function takes at most " << num_used_positional_args
			<< " arguments (" << positional_args.size() <<" given)";

		PyErr_SetString(PyExc_TypeError, oss.str().c_str());
		boost::python::throw_error_already_set();
	}
}


void
GPlatesApi::VariableArguments::raise_python_error_if_unused(
		const keyword_arguments_type &unused_keyword_args)
{
	if (!unused_keyword_args.empty())
	{
		// Pick an arbitrary unused keyword argument and raise an error specifying it.
		std::ostringstream oss;
		oss << "Function got an unexpected keyword argument '"
			<< unused_keyword_args.begin()->first << "'";

		PyErr_SetString(PyExc_TypeError, oss.str().c_str());
		boost::python::throw_error_already_set();
	}
}
