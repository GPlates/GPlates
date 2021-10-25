/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2016 The University of Sydney, Australia
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

#ifndef GPLATES_UTILS_HASFUNCTION_H
#define GPLATES_UTILS_HASFUNCTION_H

#include <boost/mpl/bool.hpp>


//
// The following macros provide meta-functions that check if a function (HAS_FUNCTION)
// or class method (HAD_MEMBER_FUNCTION) exists with a particular signature.
//
// Each meta-function has a nested boolean 'value' and a nested boost MPL boolean constant 'type'.
//
// For example:
//
//   struct MyClass
//   {
//       int get();
//   };
//
//   HAS_MEMBER_FUNCTION(get, HasGetMember)
//   
//   /* The return type of MyClass::get() - defaults to 'double' - not the best example, but shows the idea. */
//   typedef typename boost::mpl::if_<
//       HasGetMember<MyClass, int (MyClass::*)()>,
//       int,
//       double
//           >::type get_return_type;
//
// A better example would use boost enable_if to enable one of multiple functions that call MyClass::get() based on its signature.
//
// The implementation is based on:
//    http://stackoverflow.com/questions/257288/is-it-possible-to-write-a-c-template-to-check-for-a-functions-existence/264088#264088
//
#define HAS_FUNCTION(function_name, MetaFunctionName)                                \
	template <typename FunctionSignatureType>                                        \
	struct MetaFunctionName                                                          \
	{                                                                                \
		typedef char yes[1];                                                         \
		typedef char no[2];                                                          \
                                                                                     \
		template <typename U, U>                                                     \
		struct TypeCheck;                                                            \
                                                                                     \
		template <typename T>                                                        \
		static                                                                       \
		yes &                                                                        \
		check_function(                                                              \
				TypeCheck<FunctionSignatureType, &function_name> *);                 \
                                                                                     \
		template <typename T>                                                        \
		static                                                                       \
		no &                                                                         \
		check_function(...);                                                         \
                                                                                     \
                                                                                     \
		static const bool value = sizeof(check_function<void>(0)) == sizeof(yes);    \
                                                                                     \
		typedef boost::mpl::bool_<value> type;                                       \
	};


#define HAS_MEMBER_FUNCTION(method_name, MetaFunctionName)                           \
	template <class ClassType, typename MethodSignatureType>                         \
	struct MetaFunctionName                                                          \
	{                                                                                \
		typedef char yes[1];                                                         \
		typedef char no[2];                                                          \
                                                                                     \
		template <typename U, U>                                                     \
		struct TypeCheck;                                                            \
                                                                                     \
		template <typename T>                                                        \
		static                                                                       \
		yes &                                                                        \
		check_method(                                                                \
				TypeCheck<MethodSignatureType, &T::method_name> *);                  \
                                                                                     \
		template <typename T>                                                        \
		static                                                                       \
		no &                                                                         \
		check_method(...);                                                           \
                                                                                     \
                                                                                     \
		static const bool value = sizeof(check_method<ClassType>(0)) == sizeof(yes); \
                                                                                     \
		typedef boost::mpl::bool_<value> type;                                       \
	};

#endif // GPLATES_UTILS_HASFUNCTION_H
