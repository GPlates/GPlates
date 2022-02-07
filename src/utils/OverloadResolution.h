/* $Id$ */

/**
 * \file
 * This file provides facilities to assist in the resolution of overloaded functions.
 *
 * Most recent change:
 *   $Date$
 *
 * Copyright (C) 2010 The University of Sydney, Australia
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

#ifndef GPLATES_UTILS_OVERLOADRESOLUTION_H
#define GPLATES_UTILS_OVERLOADRESOLUTION_H

#include <map>
#include <QString>
#include <boost/mpl/if.hpp>

#include "HasFunction.h"


namespace GPlatesUtils
{
	namespace OverloadResolution
	{
		// Note: The current implementation of overload resolution works for functions
		// with up to 5 parameters.

		struct NonMemberFunction;

		namespace OverloadResolutionInternals
		{
			struct NullType;

			// Full 5 parameter version for member functions.
			template
			<
				class Class,
				typename Ret,
				typename Arg1,
				typename Arg2,
				typename Arg3,
				typename Arg4,
				typename Arg5
			>
			struct DeduceMemberFunctionPointerType
			{
				typedef Ret (Class::*Type)(Arg1, Arg2, Arg3, Arg4, Arg5);
			};

			// 5 parameter version for non-member functions.
			template
			<
				typename Ret,
				typename Arg1,
				typename Arg2,
				typename Arg3,
				typename Arg4,
				typename Arg5
			>
			struct DeduceMemberFunctionPointerType<NonMemberFunction, Ret, Arg1, Arg2, Arg3, Arg4, Arg5>
			{
				typedef Ret (*Type)(Arg1, Arg2, Arg3, Arg4, Arg5);
			};

			// 4 parameter version for member functions.
			template
			<
				class Class,
				typename Ret,
				typename Arg1,
				typename Arg2,
				typename Arg3,
				typename Arg4
			>
			struct DeduceMemberFunctionPointerType<Class, Ret, Arg1, Arg2, Arg3, Arg4, NullType>
			{
				typedef Ret (Class::*Type)(Arg1, Arg2, Arg3, Arg4);
			};

			// 4 parameter version for non-member functions.
			template
			<
				typename Ret,
				typename Arg1,
				typename Arg2,
				typename Arg3,
				typename Arg4
			>
			struct DeduceMemberFunctionPointerType<NonMemberFunction, Ret, Arg1, Arg2, Arg3, Arg4, NullType>
			{
				typedef Ret (*Type)(Arg1, Arg2, Arg3, Arg4);
			};

			// 3 parameter version for member functions.
			template
			<
				class Class,
				typename Ret,
				typename Arg1,
				typename Arg2,
				typename Arg3
			>
			struct DeduceMemberFunctionPointerType<Class, Ret, Arg1, Arg2, Arg3, NullType, NullType>
			{
				typedef Ret (Class::*Type)(Arg1, Arg2, Arg3);
			};

			// 3 parameter version for non-member functions.
			template
			<
				typename Ret,
				typename Arg1,
				typename Arg2,
				typename Arg3
			>
			struct DeduceMemberFunctionPointerType<NonMemberFunction, Ret, Arg1, Arg2, Arg3, NullType, NullType>
			{
				typedef Ret (*Type)(Arg1, Arg2, Arg3);
			};

			// 2 parameter version for member functions.
			template
			<
				class Class,
				typename Ret,
				typename Arg1,
				typename Arg2
			>
			struct DeduceMemberFunctionPointerType<Class, Ret, Arg1, Arg2, NullType, NullType, NullType>
			{
				typedef Ret (Class::*Type)(Arg1, Arg2);
			};

			// 2 parameter version for non-member functions.
			template
			<
				typename Ret,
				typename Arg1,
				typename Arg2
			>
			struct DeduceMemberFunctionPointerType<NonMemberFunction, Ret, Arg1, Arg2, NullType, NullType, NullType>
			{
				typedef Ret (*Type)(Arg1, Arg2);
			};

			// 1 parameter version for member functions.
			template
			<
				class Class,
				typename Ret,
				typename Arg1
			>
			struct DeduceMemberFunctionPointerType<Class, Ret, Arg1, NullType, NullType, NullType, NullType>
			{
				typedef Ret (Class::*Type)(Arg1);
			};

			// 1 parameter version for non-member functions.
			template
			<
				typename Ret,
				typename Arg1
			>
			struct DeduceMemberFunctionPointerType<NonMemberFunction, Ret, Arg1, NullType, NullType, NullType, NullType>
			{
				typedef Ret (*Type)(Arg1);
			};

			// 0 parameter version for member functions.
			template
			<
				class Class,
				typename Ret
			>
			struct DeduceMemberFunctionPointerType<Class, Ret, NullType, NullType, NullType, NullType, NullType>
			{
				typedef Ret (Class::*Type)();
			};

			// 0 parameter version for non-member functions.
			template
			<
				typename Ret
			>
			struct DeduceMemberFunctionPointerType<NonMemberFunction, Ret, NullType, NullType, NullType, NullType, NullType>
			{
				typedef Ret (*Type)();
			};
		}

		/**
		 * Holds up to 5 types to specify the types of the parameters of a function.
		 */
		template
		<
			typename _Arg1 = OverloadResolutionInternals::NullType,
			typename _Arg2 = OverloadResolutionInternals::NullType,
			typename _Arg3 = OverloadResolutionInternals::NullType,
			typename _Arg4 = OverloadResolutionInternals::NullType,
			typename _Arg5 = OverloadResolutionInternals::NullType
		>
		struct Params
		{
			typedef _Arg1 Arg1;
			typedef _Arg2 Arg2;
			typedef _Arg3 Arg3;
			typedef _Arg4 Arg4;
			typedef _Arg5 Arg5;
		};

		/**
		 * Assists in getting a function pointer to a function that is overloaded.
		 *
		 * Suppose we wish to use an overloaded function in a lambda expression:
		 *
		 *		typedef std::map<int, int> map_type;
		 *		map_type m;
		 *		boost::lambda::bind(
		 *				&map_type::find, // ERROR: erase is overloaded.
		 *				boost::ref(m),
		 *				boost::lambda::_1);
		 * 
		 * When a function is overloaded, the compiler can't tell which of the
		 * overloads you mean, because it can't read your mind. What you can do,
		 * however, is initialise a function pointer variable with a type that is an
		 * exact match with the overload that you are interested in. This approach can
		 * be quite verbose and syntactically ugly.
		 *
		 * resolve() works by providing a function that takes as its parameter a
		 * function pointer with a type that is exactly the overload you want; you
		 * customise resolve() by giving it, as template parameters, the class (for a
		 * member function), the return type and a list of parameters. For example, the
		 * above example can be rewritten as:
		 *
		 *		boost::lambda::bind(
		 *				resolve<map_type, map_type::iterator, Params<const int &> >(&map_type::find),
		 *				boost::ref(m),
		 *				boost::lambda::_1);
		 *
		 * Params can take up to 5 template arguments. For an overload that takes no
		 * parameters, just give Params no template arguments.
		 *
		 * For an overloaded non-member function, specify NonMemberFunction as the
		 * first template argument to resolve(). (Yes, this is ugly, but we need C++0x
		 * to write function templates with default template parameters.)
		 */
		template
		<
			class Class,
			typename Ret,
			typename Params
		>
		inline
		typename OverloadResolutionInternals::DeduceMemberFunctionPointerType<
				Class, Ret, typename Params::Arg1, typename Params::Arg2,
				typename Params::Arg3, typename Params::Arg4, typename Params::Arg5>::Type
		resolve(
				typename OverloadResolutionInternals::DeduceMemberFunctionPointerType<
				Class, Ret, typename Params::Arg1, typename Params::Arg2,
				typename Params::Arg3, typename Params::Arg4, typename Params::Arg5>::Type fp)
		{
			return fp;
		}

		
		// For commonly used overloaded functions in the STL/Qt, we provide a more
		// convenient form of resolve that takes the type of the function pointer as
		// template parameter. The type of the function pointer can be retrieved from
		// the templated struct mem_fn_types.
		//
		// Also note that the Microsoft implementation of the STL sometimes has
		// non-standard parameters and return types; note also that sometimes this
		// varies between versions of Visual Studio!
		
		template<class Class>
		struct mem_fn_types
		{
		};

		HAS_MEMBER_FUNCTION(erase, HasEraseMember)

		template<class MapType>
		struct mem_fn_types_for_maps
		{
			typedef MapType map_type;
			typedef typename map_type::iterator iterator_type;
			typedef typename map_type::const_iterator const_iterator_type;
			typedef typename map_type::key_type key_type;

			// erase() - use 'erase1' (eg, mem_fn_types<...>::erase1).
			//
			// We use function/method signature detection code to determine signature of 'map_type::erase'
			// because it varies quite a bit across different compilers (and versions) between:
			//  - iterator_type (map_type::*)(iterator_type)
			//  - iterator_type (map_type::*)(const_iterator_type)
			//  - void (map_type::*)(iterator_type)
			//
			// Previously we had preprocessor ifdef's based on different compilers but it got too fiddly
			// between MSVC, G++ and CLANG and different versions within each having different signatures.
			//
			// Update: Looks like we need at least one ifdef though :-)
			// At least don't have to worry about compiler versions - not yet anyway :-)
#ifdef _MSC_VER
			// For MSVC we need to inherit 'map_type' and bring its 'erase()' method into scope of derived class
			// so that, when we check the method type, it doesn't find 'erase' in the base class of 'map_type'
			// and hence have a different method signature (eg, 'map_base_type::erase') and hence bypass
			// our method detection code.
			struct CheckMapEraseType : public map_type { using map_type::erase; };
#else
			// The above inherit 'map_type' trick doesn't work for G++ and CLANG.
			// Instead just use the map type itself as the check type.
			// This may only work is 'map_type' itself doesn't inherit from a base class (containing 'erase()').
			// So far it seems this is the case.
			typedef map_type CheckMapEraseType;
#endif
			typedef typename boost::mpl::if_<
					// See if erase method has signature 'iterator_type (map_type::*)(iterator_type)'...
					HasEraseMember<CheckMapEraseType, iterator_type (CheckMapEraseType::*)(iterator_type)>,
							iterator_type (map_type::*)(iterator_type),
							// See if erase method has signature 'iterator_type (map_type::*)(const_iterator_type)'...
							typename boost::mpl::if_<
									HasEraseMember<CheckMapEraseType, iterator_type (CheckMapEraseType::*)(const_iterator_type)>,
											iterator_type (map_type::*)(const_iterator_type),
											// Default to signature 'void (map_type::*)(iterator_type)'...
											void (map_type::*)(iterator_type)
							>::type
					>::type erase1;
			
			// non-const find().
			typedef iterator_type (map_type::*find)(const key_type &);

			// const find().
			typedef const_iterator_type (map_type::*const_find)(const key_type &) const;
		};

		template<typename T, typename U>
		struct mem_fn_types<std::map<T, U> > :
				public mem_fn_types_for_maps<std::map<T, U> >
		{
		};

		template<typename T, typename U>
		struct mem_fn_types<std::multimap<T, U> > :
				public mem_fn_types_for_maps<std::multimap<T, U> >
		{
		};

		template<>
		struct mem_fn_types<QString>
		{
			typedef QString & (QString::*prepend1)(const QString &);
		};

		template<typename FunctionPointerType>
		inline
		FunctionPointerType
		resolve(
				FunctionPointerType fp)
		{
			return fp;
		}
	}  // namespace OverloadResolution
}

#endif  // GPLATES_UTILS_OVERLOADRESOLUTION_H
