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

#ifndef GPLATES_API_PYTHONVARIABLEFUNCTIONARGUMENTS_H
#define GPLATES_API_PYTHONVARIABLEFUNCTIONARGUMENTS_H

#include <map>
#include <sstream>
#include <string>
#include <typeinfo>
#include <vector>
#include <boost/call_traits.hpp>
#include <boost/optional.hpp>
#include <boost/static_assert.hpp>
#include <boost/tuple/tuple.hpp>
#include <QDebug>

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/python.h"


#if !defined(GPLATES_NO_PYTHON)

namespace GPlatesApi
{
	namespace VariableArguments
	{
		/**
		 * A sequence of position arguments.
		 */
		typedef std::vector<boost::python::object> positional_arguments_type;

		/**
		 * A map of keyword argument names to values.
		 */
		typedef std::map<std::string, boost::python::object> keyword_arguments_type;


		/**
		 * Copies the specified python positional and keyword arguments to standard containers
		 * std::vector and std::map.
		 */
		void
		get_positional_and_keywords_args(
				positional_arguments_type &positional_args,
				keyword_arguments_type &keyword_args,
				boost::python::tuple positional_args_tuple,
				boost::python::dict keyword_args_dict);


		/**
		 * Extracts explicit (named and typed) arguments of a function and also returns a variable
		 * number of optional positional and keyword arguments.
		 *
		 * This enables a raw function (see boost::python::raw_function) of the following signature...
		 *
		 *   def raw_function(*args, **kwargs):
		 *       ...
		 *
		 * ...to be treated as a specific function with *explicitly* named arguments, and optional
		 * variable-length positional (*args) and keyword (**kwargs) arguments.
		 *
		 * This is useful when extracting arguments for a C++ function wrapped using
		 * boost::python::raw_function.
		 * The following shows a pure python function (for illustration) and its C++ equivalent...
		 *
		 *
		 *  def function(arg1, arg2="default_arg2", arg3=True, **kwargs):
		 *      arg4 = kwargs.get('arg4', 1.0)
		 *      arg5 = kwargs.get('arg5', 'default')
		 *      ...
		 *
		 *
		 * ...note that we chose only to have a variable number of keyword (**kwargs) arguments and
		 * *not* a variable number of positional (*args) arguments.
		 * The equivalent wrapped C++ function is...
		 *
		 *
		 * 	bp::object
		 *  function(
		 *      bp::tuple positional_args,
		 *      bp::dict keyword_args)
		 *  {
		 *    VariableArguments::keyword_arguments_type unused_keyword_args;
		 *  
		 *    int arg1;
		 *    QString arg2;
		 *    bool arg3;
		 *  
		 *    boost::tie(arg1, arg2, arg3) =
		 *      VariableArguments::get_explicit_args< boost::tuple<int, QString, bool> >(
		 *          positional_args,
		 *          keyword_args,
		 *          boost::make_tuple(
		 *              "arg1",
		 *              "arg2",
		 *              "arg3"),
		 *          boost::tuples::make_tuple(QString("default_arg2"), true),
		 *          boost::none, // unused_positional_args
		 *          unused_keyword_args);
		 *
		 *    double arg4 = VariableArguments::extract_or_default<double>(unused_keyword_args, "arg4", 1.0)
		 *    QString arg5 = VariableArguments::extract_or_default<QString>(unused_keyword_args, "arg5", "default")
		 *
		 *    ...
		 *
		 *    // We must return a value (required by 'bp::raw_function') so just return Py_None.
		 *    return bp::object();
		 *  }
		 *
		 *  boost::python::def("function", bp::raw_function(&function));
		 * 
		 *
		 * @a explicit_arg_names should be a boost::tuple of strings containing the names of the
		 * explicit arguments (in the above example these are "arg1", "arg2" and "arg3").
		 * @a default_args should be a boost::tuple of the default arguments - if there are no
		 * default arguments this should be the empty tuple created by boost::make_tuple().
		 * The first template parameter ExplicitArgsTupleType should be a boost::tuple of the
		 * *types* of the explicit arguments.
		 * The size of ExplicitArgsTupleType and ExplicitArgNamesTupleType should be equal.
		 * The size of ExplicitArgsTupleType should be greater than or equal to DefaultArgsTupleType.
		 *
		 * The returned boost::tuple contains the values of the explicit arguments.
		 *
		 * If @a unused_positional_args is boost::none then a python error is raised if there are
		 * more positional arguments than explicit arguments, otherwise it will contain the
		 * unused (non-explicit) positional arguments. In the above example there are three explicit
		 * arguments arg1, arg2 and arg3 - so a fourth positional argument would raise an error.
		 * If @a unused_keyword_args is boost::none then a python error is raised if any keyword
		 * argument does not match an explicit arguments (eg, arg1, arg2 or arg3), otherwise it will
		 * contain the unused (non-explicit) keyword arguments.
		 *
		 * @a unused_keyword_args, in particular, is useful for extracting a variable number of
		 * keyword arguments that are not explicitly listed as the function's usual arguments.
		 * See arg4 and arg5 in the above example.
		 */
		template <class ExplicitArgsTupleType, class ExplicitArgNamesTupleType, class DefaultArgsTupleType>
		ExplicitArgsTupleType
		get_explicit_args(
				boost::python::tuple positional_args,
				boost::python::dict keyword_args,
				const ExplicitArgNamesTupleType &explicit_arg_names,
				const DefaultArgsTupleType &default_args,
				boost::optional<positional_arguments_type &> unused_positional_args,
				boost::optional<keyword_arguments_type &> unused_keyword_args);


		/**
		 * A convenience wrapper around boost::python::extract that raises a 'TypeError', if
		 * extraction fails, mentioning the argument name and the C++ object type in the error message.
		 *
		 * NOTE: Unlike boost::python::extract this method does *not* mention the python type
		 * in the error message. So this method is mainly useful when using the unused keyword
		 * arguments returned by @a get_explicit_args.
		 */
		template <typename ArgumentType>
		ArgumentType
		extract(
				boost::python::object python_argument,
				const std::string &argument_name);


		/**
		 * Returns extracted C++ object type from python object in @a keyword_args with the
		 * name @a argument_name (if found), otherwise none.
		 *
		 * NOTE: Unlike boost::python::extract this method does *not* mention the python type
		 * in the error message. So this method is mainly useful when using the unused keyword
		 * arguments returned by @a get_explicit_args.
		 */
		template <typename ArgumentType>
		boost::optional<ArgumentType>
		extract(
				const keyword_arguments_type &keyword_args,
				const std::string &argument_name);

		/**
		 * Same as similar @a extract function but returns @a default_argument if @a argument_name
		 * is not found in @a keyword_args.
		 *
		 * boost::call_traits is used in case ArgumentType is a reference - in which case the returned
		 * value will reference either the C++ object wrapped in the python object or @a default_argument.
		 */
		template <typename ArgumentType>
		typename boost::call_traits<ArgumentType>::value_type
		extract_or_default(
				const keyword_arguments_type &keyword_args,
				const std::string &argument_name,
				typename boost::call_traits<ArgumentType>::param_type default_argument);


		/**
		 * Same as similar @a extract function but also removes the argument from @a keyword_args if found.
		 *
		 * This is useful when you want to check/extract all supported keywords and then ensure
		 * that there are no keywords remaining (see @a raise_python_error_if_unused).
		 */
		template <typename ArgumentType>
		boost::optional<ArgumentType>
		extract_and_remove(
				keyword_arguments_type &keyword_args,
				const std::string &argument_name);

		/**
		 * Same as similar @a extract_and_remove function but returns @a default_argument if
		 * @a argument_name is not found in @a keyword_args.
		 *
		 * boost::call_traits is used in case ArgumentType is a reference - in which case the returned
		 * value will reference either the C++ object wrapped in the python object or @a default_argument.
		 */
		template <typename ArgumentType>
		typename boost::call_traits<ArgumentType>::value_type
		extract_and_remove_or_default(
				keyword_arguments_type &keyword_args,
				const std::string &argument_name,
				typename boost::call_traits<ArgumentType>::param_type default_argument);


		/**
		 * Raises a python error if the size of @a positional_args exceeds @a num_used_positional_args.
		 */
		void
		raise_python_error_if_unused(
				const positional_arguments_type &positional_args,
				unsigned int num_used_positional_args);


		/**
		 * Raises a python error if @a unused_keyword_args is not empty.
		 *
		 * The error message mentions one of the keywords as unexpected.
		 */
		void
		raise_python_error_if_unused(
				const keyword_arguments_type &unused_keyword_args);
	}
}

////////////////////
// Implementation //
////////////////////

namespace GPlatesApi
{
	namespace VariableArguments
	{
		template <typename ArgumentType>
		ArgumentType
		extract(
				boost::python::object python_argument,
				const std::string &argument_name)
		{
			// Note that 'ArgumentType' could be a reference.
			boost::python::extract<ArgumentType> extract_arg(python_argument);
			if (!extract_arg.check())
			{
				std::ostringstream oss;
				oss << "Unable to convert function argument '" << argument_name
					<< "' to C++ type '" << typeid(ArgumentType).name() << "'";

				PyErr_SetString(PyExc_TypeError, oss.str().c_str());
				boost::python::throw_error_already_set();
			}

			return extract_arg();
		}


		template <typename ArgumentType>
		boost::optional<ArgumentType>
		extract(
				const keyword_arguments_type &keyword_args,
				const std::string &argument_name)
		{
			keyword_arguments_type::const_iterator keywords_args_iter =
					keyword_args.find(argument_name);
			if (keywords_args_iter == keyword_args.end())
			{
				return boost::none;
			}

			return extract<ArgumentType>(keywords_args_iter->second, argument_name);
		}


		template <typename ArgumentType>
		typename boost::call_traits<ArgumentType>::value_type
		extract_or_default(
				const keyword_arguments_type &keyword_args,
				const std::string &argument_name,
				typename boost::call_traits<ArgumentType>::param_type default_argument)
		{
			keyword_arguments_type::const_iterator keywords_args_iter =
					keyword_args.find(argument_name);
			if (keywords_args_iter == keyword_args.end())
			{
				return default_argument;
			}

			return extract<ArgumentType>(keywords_args_iter->second, argument_name);
		}


		template <typename ArgumentType>
		boost::optional<ArgumentType>
		extract_and_remove(
				keyword_arguments_type &keyword_args,
				const std::string &argument_name)
		{
			keyword_arguments_type::iterator keywords_args_iter = keyword_args.find(argument_name);
			if (keywords_args_iter == keyword_args.end())
			{
				return boost::none;
			}

			keyword_args.erase(keywords_args_iter);

			return extract<ArgumentType>(keywords_args_iter->second, argument_name);
		}

		template <typename ArgumentType>
		typename boost::call_traits<ArgumentType>::value_type
		extract_and_remove_or_default(
				keyword_arguments_type &keyword_args,
				const std::string &argument_name,
				typename boost::call_traits<ArgumentType>::param_type default_argument)
		{
			keyword_arguments_type::iterator keywords_args_iter = keyword_args.find(argument_name);
			if (keywords_args_iter == keyword_args.end())
			{
				return default_argument;
			}

			keyword_args.erase(keywords_args_iter);

			return extract<ArgumentType>(keywords_args_iter->second, argument_name);
		}


		namespace Implementation
		{
			// Get explicit arguments from the keywords arguments.
			//
			// Need a class template instead of function template since the latter cannot be
			// partially specialised.
			//
			// Primary template declaration.
			template <
					class ExplicitArgsConsType,
					class ExplicitArgNamesConsType,
					class DefaultArgsConsType,
					int num_explicit_args_remaining,
					int num_required_explicit_args_remaining>
			struct GetExplicitArgsFromKeywordArgs;


			// Finished processing all *required* and *optional* explicit arguments from
			// keyword arguments - so we're done.
			//
			// This is a full specialisation.
			template <>
			struct GetExplicitArgsFromKeywordArgs<
					boost::tuples::null_type,
					boost::tuples::null_type,
					boost::tuples::null_type,
					0/*num_explicit_args_remaining*/,
					0/*num_required_explicit_args_remaining*/>
			{
				static
				boost::tuples::null_type
				get(
						keyword_arguments_type &unused_keyword_args,
						const boost::tuples::null_type &explicit_arg_names,
						const boost::tuples::null_type &default_args)
				{
					return boost::tuples::null_type();
				}
			};


			// Finished processing all *required* explicit arguments but there's still *optional*
			// explicit arguments to process.
			//
			// This is a partial specialisation, which is why we had to use a class instead of a function.
			template <
					class ExplicitArgsConsType,
					class ExplicitArgNamesConsType,
					class DefaultArgsConsType,
					int num_explicit_args_remaining>
			struct GetExplicitArgsFromKeywordArgs<
					ExplicitArgsConsType,
					ExplicitArgNamesConsType,
					DefaultArgsConsType,
					num_explicit_args_remaining,
					0/*num_required_explicit_args_remaining*/>
			{
				static
				ExplicitArgsConsType
				get(
						keyword_arguments_type &unused_keyword_args,
						const ExplicitArgNamesConsType &explicit_arg_names,
						const DefaultArgsConsType &default_args)
				{
					// Number of explicit arguments must match number of default arguments.
					BOOST_STATIC_ASSERT(boost::tuples::length<ExplicitArgsConsType>::value ==
							boost::tuples::length<DefaultArgsConsType>::value);

					// Look for the current explicit argument in the unused keyword arguments.
					const std::string explicit_arg_name(explicit_arg_names.get_head());
					keyword_arguments_type::iterator explicit_arg_iter = unused_keyword_args.find(explicit_arg_name);

					typedef typename ExplicitArgsConsType::head_type explicit_arg_type;

					// Note that 'explicit_arg_type' could be a reference if client specified it as such
					// in the original boost::tuple (because they wanted reference to C++ object
					// wrapped inside python object instead of a copy).
					explicit_arg_type arg = (explicit_arg_iter != unused_keyword_args.end())
							// Extract from unused keyword arguments...
							? extract<explicit_arg_type>(explicit_arg_iter->second, explicit_arg_name)
							// Use default value (since not found in unused keyword arguments)...
							: default_args.get_head();

					//qDebug() << "Optional explicit argument is a"
					//	<< ((explicit_arg_iter != unused_keyword_args.end()) ? "keyword" : "default")
					//	<< "argument";

					// Remove from the map of unused keywords arguments.
					if (explicit_arg_iter != unused_keyword_args.end())
					{
						unused_keyword_args.erase(explicit_arg_iter);
					}

					return ExplicitArgsConsType(
							arg,
							GetExplicitArgsFromKeywordArgs<
									typename ExplicitArgsConsType::tail_type,
									typename ExplicitArgNamesConsType::tail_type,
									typename DefaultArgsConsType::tail_type,
									num_explicit_args_remaining - 1,
									0/*num_required_explicit_args_remaining*/>::get(
											unused_keyword_args,
											explicit_arg_names.get_tail(),
											default_args.get_tail()));
				}
			};


			// Primary template definition.
			template <
					class ExplicitArgsConsType,
					class ExplicitArgNamesConsType,
					class DefaultArgsConsType,
					int num_explicit_args_remaining,
					int num_required_explicit_args_remaining>
			struct GetExplicitArgsFromKeywordArgs
			{
				static
				ExplicitArgsConsType
				get(
						keyword_arguments_type &unused_keyword_args,
						const ExplicitArgNamesConsType &explicit_arg_names,
						const DefaultArgsConsType &default_args)
				{
					// Number of explicit arguments must greater-than-or-equal to number of default arguments.
					BOOST_STATIC_ASSERT(boost::tuples::length<ExplicitArgsConsType>::value >=
							boost::tuples::length<DefaultArgsConsType>::value);

					//qDebug() << "Required explicit argument is a keyword argument";

					// Look for the current explicit argument in the unused keyword arguments.
					const std::string explicit_arg_name(explicit_arg_names.get_head());
					keyword_arguments_type::iterator explicit_arg_iter = unused_keyword_args.find(explicit_arg_name);

					// Throw error if could not find required argument as a keyword.
					if (explicit_arg_iter == unused_keyword_args.end())
					{
						std::ostringstream oss;
						oss << "Function is missing required argument '" << explicit_arg_name << "'";

						PyErr_SetString(PyExc_TypeError, oss.str().c_str());
						boost::python::throw_error_already_set();
					}

					typedef typename ExplicitArgsConsType::head_type explicit_arg_type;

					// Note that 'explicit_arg_type' could be a reference if client specified it as such
					// in the original boost::tuple (because they wanted reference to C++ object
					// wrapped inside python object instead of a copy).
					explicit_arg_type arg =
							extract<explicit_arg_type>(explicit_arg_iter->second, explicit_arg_name);

					// Remove from the map of unused keywords arguments.
					unused_keyword_args.erase(explicit_arg_iter);

					return ExplicitArgsConsType(
							arg,
							GetExplicitArgsFromKeywordArgs<
									typename ExplicitArgsConsType::tail_type,
									typename ExplicitArgNamesConsType::tail_type,
									DefaultArgsConsType, // Use full tuple since haven't started using default args
									num_explicit_args_remaining - 1,
									num_required_explicit_args_remaining - 1>::get(
											unused_keyword_args,
											explicit_arg_names.get_tail(),
											default_args)); // Use full tuple since haven't started using default args
				}
			};


			// Get explicit arguments from the positional arguments.
			//
			// Need a class template instead of function template since the latter cannot be
			// partially specialised.
			//
			// Primary template declaration.
			template <
					class ExplicitArgsConsType,
					class ExplicitArgNamesConsType,
					class DefaultArgsConsType,
					int num_explicit_args_remaining,
					int num_required_explicit_args_remaining>
			struct GetExplicitArgsFromPositionalArgs;


			// Finished processing all *required* and *optional* explicit arguments from
			// positional arguments - so we're done.
			//
			// This is a full specialisation.
			template <>
			struct GetExplicitArgsFromPositionalArgs<
					boost::tuples::null_type,
					boost::tuples::null_type,
					boost::tuples::null_type,
					0/*num_explicit_args_remaining*/,
					0/*num_required_explicit_args_remaining*/>
			{
				static
				boost::tuples::null_type
				get(
						const positional_arguments_type &positional_args,
						keyword_arguments_type &unused_keyword_args,
						const boost::tuples::null_type &explicit_arg_names,
						const boost::tuples::null_type &default_args,
						unsigned int positional_arg_index)
				{
					return boost::tuples::null_type();
				}
			};


			// Finished processing all *required* explicit arguments but there's still *optional*
			// explicit arguments to process.
			//
			// This is a partial specialisation, which is why we had to use a class instead of a function.
			template <
					class ExplicitArgsConsType,
					class ExplicitArgNamesConsType,
					class DefaultArgsConsType,
					int num_explicit_args_remaining>
			struct GetExplicitArgsFromPositionalArgs<
					ExplicitArgsConsType,
					ExplicitArgNamesConsType,
					DefaultArgsConsType,
					num_explicit_args_remaining,
					0/*num_required_explicit_args_remaining*/>
			{
				static
				ExplicitArgsConsType
				get(
						const positional_arguments_type &positional_args,
						keyword_arguments_type &unused_keyword_args,
						const ExplicitArgNamesConsType &explicit_arg_names,
						const DefaultArgsConsType &default_args,
						unsigned int positional_arg_index)
				{
					// Number of explicit arguments must match number of default arguments.
					BOOST_STATIC_ASSERT(boost::tuples::length<ExplicitArgsConsType>::value ==
							boost::tuples::length<DefaultArgsConsType>::value);

					// We've retrieved all *required* explicit arguments (ones that don't have
					// default values) from positional arguments.
					// Now use up the remaining positional arguments, if any, for the optional
					// explicit arguments (ones that have default values).

					// However if there's no remaining positional arguments to consume then
					// switch to processing keywords arguments.
					// Note that we still have *optional* explicit arguments so either keyword
					// arguments will get used for them or they'll assume their default values.
					if (positional_arg_index == positional_args.size())
					{
						return GetExplicitArgsFromKeywordArgs<
								ExplicitArgsConsType,
								ExplicitArgNamesConsType,
								DefaultArgsConsType,
								num_explicit_args_remaining,
								0/*num_required_explicit_args_remaining*/>::get(
										unused_keyword_args,
										explicit_arg_names,
										default_args);
					}

					//qDebug() << "Optional explicit argument is a positional argument";

					const std::string explicit_arg_name(explicit_arg_names.get_head());

					typedef typename ExplicitArgsConsType::head_type explicit_arg_type;

					// Note that 'explicit_arg_type' could be a reference if client specified it as such
					// in the original boost::tuple (because they wanted reference to C++ object
					// wrapped inside python object instead of a copy).
					explicit_arg_type arg =
							extract<explicit_arg_type>(
									positional_args[positional_arg_index],
									explicit_arg_name);

					return ExplicitArgsConsType(
							arg,
							GetExplicitArgsFromPositionalArgs<
									typename ExplicitArgsConsType::tail_type,
									typename ExplicitArgNamesConsType::tail_type,
									typename DefaultArgsConsType::tail_type, // We ignored current default value
									num_explicit_args_remaining - 1,
									0/*num_required_explicit_args_remaining*/>::get(
											positional_args,
											unused_keyword_args,
											explicit_arg_names.get_tail(),
											default_args.get_tail(), // We ignored current default value
											positional_arg_index + 1));
				}
			};


			// Primary template definition.
			template <
					class ExplicitArgsConsType,
					class ExplicitArgNamesConsType,
					class DefaultArgsConsType,
					int num_explicit_args_remaining,
					int num_required_explicit_args_remaining>
			struct GetExplicitArgsFromPositionalArgs
			{
				static
				ExplicitArgsConsType
				get(
						const positional_arguments_type &positional_args,
						keyword_arguments_type &unused_keyword_args,
						const ExplicitArgNamesConsType &explicit_arg_names,
						const DefaultArgsConsType &default_args,
						unsigned int positional_arg_index)
				{
					// Number of explicit arguments must greater-than-or-equal to number of default arguments.
					BOOST_STATIC_ASSERT(boost::tuples::length<ExplicitArgsConsType>::value >=
							boost::tuples::length<DefaultArgsConsType>::value);

					// If there's no remaining positional arguments to consume then
					// switch to processing keywords arguments.
					// Note that we still have *required* explicit arguments to fill so we're
					// hoping the keyword arguments will satisfy them.
					if (positional_arg_index == positional_args.size())
					{
						return GetExplicitArgsFromKeywordArgs<
								ExplicitArgsConsType,
								ExplicitArgNamesConsType,
								DefaultArgsConsType,
								num_explicit_args_remaining,
								num_required_explicit_args_remaining>::get(
										unused_keyword_args,
										explicit_arg_names,
										default_args);
					}

					//qDebug() << "Required explicit argument is a positional argument";

					const std::string explicit_arg_name(explicit_arg_names.get_head());

					typedef typename ExplicitArgsConsType::head_type explicit_arg_type;

					// Note that 'explicit_arg_type' could be a reference if client specified it as such
					// in the original boost::tuple (because they wanted reference to C++ object
					// wrapped inside python object instead of a copy).
					explicit_arg_type arg =
							extract<explicit_arg_type>(
									positional_args[positional_arg_index],
									explicit_arg_name);

					return ExplicitArgsConsType(
							arg,
							GetExplicitArgsFromPositionalArgs<
									typename ExplicitArgsConsType::tail_type,
									typename ExplicitArgNamesConsType::tail_type,
									DefaultArgsConsType, // Use full tuple since haven't started using default args
									num_explicit_args_remaining - 1,
									num_required_explicit_args_remaining - 1>::get(
											positional_args,
											unused_keyword_args,
											explicit_arg_names.get_tail(),
											default_args, // Use full tuple since haven't started using default args
											positional_arg_index + 1));
				}
			};


			// boost::tuples::cons terminator.
			inline
			void
			check_positional_keyword_overlap(
					const keyword_arguments_type &unused_keyword_args,
					const boost::tuples::null_type &explicit_arg_names,
					unsigned int num_explicit_arg_names_left_to_check)
			{
				GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
						num_explicit_arg_names_left_to_check == 0,
						GPLATES_ASSERTION_SOURCE);
			}

			/**
			 * Throw an error if a keyword argument and a positional argument both
			 * have the same argument name (means user specified same argument twice).
			 */
			template <class ExplicitArgNamesConsType>
			void
			check_positional_keyword_overlap(
					const keyword_arguments_type &unused_keyword_args,
					const ExplicitArgNamesConsType &explicit_arg_names,
					unsigned int num_explicit_arg_names_left_to_check)
			{
				if (num_explicit_arg_names_left_to_check == 0)
				{
					return;
				}

				// If the current explicit argument name matches a keyword then throw an error.
				const std::string explicit_arg_name(explicit_arg_names.get_head());
				if (unused_keyword_args.find(explicit_arg_name) != unused_keyword_args.end())
				{
					std::ostringstream oss;
					oss << "Function got multiple values for keyword argument '"
						<< explicit_arg_name << "'";

					PyErr_SetString(PyExc_TypeError, oss.str().c_str());
					boost::python::throw_error_already_set();
				}

				check_positional_keyword_overlap(
						unused_keyword_args,
						explicit_arg_names.get_tail(),
						num_explicit_arg_names_left_to_check - 1);
			}


			// Metafunction to determine the boost::tuples::cons type for boost::tuple type 'TupleType'.
			//
			// We can't just use boost::tuple::inherited since boost::tuples::null_type does
			// not have 'inherited' as a nested typedef.
			template <class TupleType>
			struct GetTupleCons
			{
				typedef typename TupleType::inherited type;
			};

			template <>
			struct GetTupleCons<boost::tuples::null_type>
			{
				typedef boost::tuples::null_type type;
			};
		}


		template <class ExplicitArgsTupleType, class ExplicitArgNamesTupleType, class DefaultArgsTupleType>
		ExplicitArgsTupleType
		get_explicit_args(
				boost::python::tuple positional_args_tuple,
				boost::python::dict keyword_args_dict,
				const ExplicitArgNamesTupleType &explicit_arg_names,
				const DefaultArgsTupleType &default_args,
				boost::optional<positional_arguments_type &> unused_positional_args,
				boost::optional<keyword_arguments_type &> unused_keyword_args)
		{
			const unsigned int num_explicit_args = boost::tuples::length<ExplicitArgsTupleType>::value;
			const unsigned int num_explicit_arg_names = boost::tuples::length<ExplicitArgNamesTupleType>::value;
			const unsigned int num_default_args = boost::tuples::length<DefaultArgsTupleType>::value;

			// Number of explicit arguments must match number of associated argument names.
			BOOST_STATIC_ASSERT(num_explicit_args == num_explicit_arg_names);

			// There must not be more default arguments than explicit arguments.
			BOOST_STATIC_ASSERT(num_explicit_args >= num_default_args);

			const unsigned int min_required_explicit_args = num_explicit_args - num_default_args;

			// Copy positional and keywords args from python tuple and dict into a vector and map.
			// Copy keyword args from python dict into the unused keywords args map.
			// If we use any keyword args for any explicit args then we'll remove them as we go.
			positional_arguments_type positional_args;
			keyword_arguments_type keyword_args;
			get_positional_and_keywords_args(
					positional_args,
					keyword_args,
					positional_args_tuple,
					keyword_args_dict);

			// The number of positional arguments that will be used to satisfy explicit arguments.
			const unsigned int num_explicit_positional_args =
					(positional_args.size() > num_explicit_args)
					? num_explicit_args
					: positional_args.size();

			// Throw error if any keyword arguments overlap with explicit positional arguments.
			Implementation::check_positional_keyword_overlap<
					Implementation::GetTupleCons<ExplicitArgNamesTupleType>::type>(
							keyword_args,
							explicit_arg_names,
							num_explicit_positional_args);

			// Return uses implicit conversion constructor of 'boost::tuple' (from 'boost::tuples::cons').
			const ExplicitArgsTupleType explicit_args =
					Implementation::GetExplicitArgsFromPositionalArgs<
							Implementation::GetTupleCons<ExplicitArgsTupleType>::type,
							Implementation::GetTupleCons<ExplicitArgNamesTupleType>::type,
							Implementation::GetTupleCons<DefaultArgsTupleType>::type,
							num_explicit_args,
							min_required_explicit_args>::get(
									positional_args,
									keyword_args,
									explicit_arg_names,
									default_args,
									0); // start at positional index zero

			// If unused positional arguments are not allowed then raise an error if any were unused.
			if (!unused_positional_args)
			{
				// Note that we did *not* clear the used positional arguments as we processed them.
				raise_python_error_if_unused(positional_args, num_explicit_args);
			}

			// If unused keywords arguments are not allowed then raise an error if any were unused.
			if (!unused_keyword_args)
			{
				// Note that we *did* clear the used keyword arguments as we processed them.
				raise_python_error_if_unused(keyword_args);
			}

			if (unused_positional_args)
			{
				// We didn't clear the used positional arguments as we processed them.
				// So skip over the used positional arguments.
				unused_positional_args->assign(
						positional_args.begin() + num_explicit_positional_args,
						positional_args.end());
			}

			if (unused_keyword_args)
			{
				// Only the unused keywords remain.
				unused_keyword_args->swap(keyword_args);
			}

			return explicit_args;
		}
	}
}

#endif   //GPLATES_NO_PYTHON)

#endif // GPLATES_API_PYTHONVARIABLEFUNCTIONARGUMENTS_H
