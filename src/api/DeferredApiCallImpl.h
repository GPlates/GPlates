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

#ifndef GPLATES_API_DEFERREDAPICALLIMPL_H
#define GPLATES_API_DEFERREDAPICALLIMPL_H

#include <boost/bind.hpp>
#include <boost/type_traits.hpp>
#include <boost/utility/enable_if.hpp>

#include "PythonInterpreterLocker.h"
#include "PythonInterpreterUnlocker.h"

#include "utils/DeferredCallEvent.h"
#include "utils/FunctionTypes.h"


namespace GPlatesApi
{
	/**
	 * Beware! Template dragons lie within!
	 *
	 * This namespace contains the implementation details for the macro
	 * @a GPLATES_DEFERRED_API_CALL as defined in "DeferredApiCall.h". This header
	 * is not intended for public use; refer to "DeferredApiCall.h" instead.
	 */
	namespace DeferredApiCallImpl
	{
		typedef DeferredApiCall::no_wrap no_wrap;
		typedef DeferredApiCall::ref ref;
		typedef DeferredApiCall::cref cref;

		template<int I, class ArgReferenceWrappingsType>
		struct get_wrapping_helper
		{
			typedef typename ArgReferenceWrappingsType::template get<I - 1>::type type;
		};

		template<class ArgReferenceWrappingsType>
		struct get_wrapping_helper<0, ArgReferenceWrappingsType>
		{
			typedef no_wrap type;
		};

		template<class ComponentTypes, class ArgReferenceWrappingsType>
		struct Impl
		{
			/**
			 * Extracts the wrapping appropriate for the @a I-th component (i.e. the
			 * {@code (I - 1)}-th parameter.
			 */
			template<int I>
			struct get_wrapping
			{
				typedef typename get_wrapping_helper<I, ArgReferenceWrappingsType>::type type;
			};

			////

			template<typename T, class Wrapping>
			struct wrap_arg_type_helper;

			template<typename T>
			struct wrap_arg_type_helper<T, no_wrap>
			{
				typedef T type;
			};

			template<typename T>
			struct wrap_arg_type_helper<T, ref>
			{
				typedef typename boost::add_reference<T>::type type;
			};

			template<typename T>
			struct wrap_arg_type_helper<T, cref>
			{
			private:

				typedef typename boost::add_const<T>::type ConstT;

			public:

				typedef typename boost::add_reference<ConstT>::type type;
			};

			/**
			 * Ensures that the parameter types of the synthesised function have the
			 * correct const-ness and reference-ness depending on what reference wrapping
			 * has been selected for the corresponding parameter.
			 */
			template<typename T, int I>
			struct wrap_arg_type
			{
				typedef typename wrap_arg_type_helper<
					T,
					typename get_wrapping<I>::type
				>::type type;
			};

			/**
			 * Extracts the @a I-th component and passes it through @a wrap_arg_type.
			 */
			template<int I>
			struct at
			{
			private:

				typedef typename boost::mpl::at<ComponentTypes, boost::mpl::int_<I> >::type unwrapped_type;

			public:

				typedef typename wrap_arg_type<unwrapped_type, I>::type type;
			};

			////

			template<typename T, class Wrapping>
			struct wrapped_value_type_helper;

			template<typename T>
			struct wrapped_value_type_helper<T, no_wrap>
			{
				typedef T type;
			};

			template<typename T>
			struct wrapped_value_type_helper<T, ref>
			{
				typedef boost::reference_wrapper<
					typename boost::remove_reference<T>::type
				> type;
			};

			template<typename T>
			struct wrapped_value_type_helper<T, cref>
			{
				typedef boost::reference_wrapper<
					const typename boost::remove_reference<T>::type
				> type;
			};

			template<int I, typename T>
			struct wrapped_value_type
			{
				typedef typename wrapped_value_type_helper<
					T,
					typename get_wrapping<I>::type
				>::type type;
			};

			template<typename T>
			static
			inline
			T
			wrap_value(
					T &value,
					no_wrap)
			{
				return value;
			}

			template<typename T>
			static
			inline
			boost::reference_wrapper<T>
			wrap_value(
					T &value,
					ref)
			{
				return boost::ref(value);
			}

			template<typename T>
			static
			inline
			boost::reference_wrapper<const T>
			wrap_value(
					T &value,
					cref)
			{
				return boost::cref(value);
			}

			/**
			 * Applies the specified reference wrapping for the {@code (I - 1)}-th
			 * function parameter of value @a value.
			 *
			 * Note that because the type of @a value is {@a code T &}, the template
			 * parameter @a T is never a reference type.
			 */
			template<int I, typename T>
			static
			inline
			typename wrapped_value_type<I, T>::type
			wrap_value(
					T &value)
			{
				typedef typename get_wrapping<I>::type wrapping;
				return wrap_value(value, wrapping());
			}
		};

		/**
		 * There are specialisations of @a DeferredApiCall for each possible value of
		 * @a FunctionArity (currently 0 to 10).
		 *
		 * Each specialisation has a public static function @a deferred_api_call that
		 * binds the @a FunctionPtr to the provided arguments and posts a
		 * @a DeferredCallEvent to the @a qApp singleton on the main GUI thread.
		 *
		 * The reason for this template hocus-pocus is that Boost.Python's def
		 * function, used for exposing C++ functions to Python, expects real functions
		 * and you cannot give it a function object. This is why the @a FunctionPtr is
		 * baked in at compile time to generate a particular version of
		 * @a deferred_api_call that binds @a FunctionPtr.
		 */
		template<typename FunctionPtrType, FunctionPtrType FunctionPtr, class ArgReferenceWrappingsType, std::size_t FunctionArity>
		struct DeferredApiCall;

		// Arity = 0.
		template<typename FunctionPtrType, FunctionPtrType FunctionPtr, class ArgReferenceWrappingsType>
		struct DeferredApiCall<FunctionPtrType, FunctionPtr, ArgReferenceWrappingsType, 0>
		{
			typedef typename GPlatesUtils::FunctionTypes::component_types<FunctionPtrType>::types types;
			typedef Impl<types, ArgReferenceWrappingsType> impl;

			typedef typename impl::template at<0>::type (*function_type)();

			static
			typename impl::template at<0>::type
			deferred_api_call()
			{
				// Make sure we lose the GIL.
				PythonInterpreterLocker interpreter_locker;
				PythonInterpreterUnlocker interpreter_unlocker;

				return GPlatesUtils::DeferCall<typename impl::template at<0>::type>::defer_call(
						boost::bind(
							FunctionPtr),
						true);
			}
		};

		// Arity = 1.
		template<typename FunctionPtrType, FunctionPtrType FunctionPtr, class ArgReferenceWrappingsType>
		struct DeferredApiCall<FunctionPtrType, FunctionPtr, ArgReferenceWrappingsType, 1>
		{
			typedef typename GPlatesUtils::FunctionTypes::component_types<FunctionPtrType>::types types;
			typedef Impl<types, ArgReferenceWrappingsType> impl;

			typedef typename impl::template at<0>::type (*function_type)(
					typename impl::template at<1>::type);

			static
			typename impl::template at<0>::type
			deferred_api_call(
					typename impl::template at<1>::type a1)
			{
				// Make sure we lose the GIL.
				PythonInterpreterLocker interpreter_locker;
				PythonInterpreterUnlocker interpreter_unlocker;

				return GPlatesUtils::DeferCall<typename impl::template at<0>::type>::defer_call(
						boost::bind(
							FunctionPtr,
							impl::template wrap_value<1>(a1)),
						true);
			}
		};

		// Arity = 2.
		template<typename FunctionPtrType, FunctionPtrType FunctionPtr, class ArgReferenceWrappingsType>
		struct DeferredApiCall<FunctionPtrType, FunctionPtr, ArgReferenceWrappingsType, 2>
		{
			typedef typename GPlatesUtils::FunctionTypes::component_types<FunctionPtrType>::types types;
			typedef Impl<types, ArgReferenceWrappingsType> impl;

			typedef typename impl::template at<0>::type (*function_type)(
					typename impl::template at<1>::type,
					typename impl::template at<2>::type);

			static
			typename impl::template at<0>::type
			deferred_api_call(
					typename impl::template at<1>::type a1,
					typename impl::template at<2>::type a2)
			{
				// Make sure we lose the GIL.
				PythonInterpreterLocker interpreter_locker;
				PythonInterpreterUnlocker interpreter_unlocker;

				return GPlatesUtils::DeferCall<typename impl::template at<0>::type>::defer_call(
						boost::bind(
							FunctionPtr,
							impl::template wrap_value<1>(a1),
							impl::template wrap_value<2>(a2)),
						true);
			}
		};

		// Arity = 3.
		template<typename FunctionPtrType, FunctionPtrType FunctionPtr, class ArgReferenceWrappingsType>
		struct DeferredApiCall<FunctionPtrType, FunctionPtr, ArgReferenceWrappingsType, 3>
		{
			typedef typename GPlatesUtils::FunctionTypes::component_types<FunctionPtrType>::types types;
			typedef Impl<types, ArgReferenceWrappingsType> impl;

			typedef typename impl::template at<0>::type (*function_type)(
					typename impl::template at<1>::type,
					typename impl::template at<2>::type,
					typename impl::template at<3>::type);

			static
			typename impl::template at<0>::type
			deferred_api_call(
					typename impl::template at<1>::type a1,
					typename impl::template at<2>::type a2,
					typename impl::template at<3>::type a3)
			{
				// Make sure we lose the GIL.
				PythonInterpreterLocker interpreter_locker;
				PythonInterpreterUnlocker interpreter_unlocker;

				return GPlatesUtils::DeferCall<typename impl::template at<0>::type>::defer_call(
						boost::bind(
							FunctionPtr,
							impl::template wrap_value<1>(a1),
							impl::template wrap_value<2>(a2),
							impl::template wrap_value<3>(a3)),
						true);
			}
		};

		// Arity = 4.
		template<typename FunctionPtrType, FunctionPtrType FunctionPtr, class ArgReferenceWrappingsType>
		struct DeferredApiCall<FunctionPtrType, FunctionPtr, ArgReferenceWrappingsType, 4>
		{
			typedef typename GPlatesUtils::FunctionTypes::component_types<FunctionPtrType>::types types;
			typedef Impl<types, ArgReferenceWrappingsType> impl;

			typedef typename impl::template at<0>::type (*function_type)(
					typename impl::template at<1>::type,
					typename impl::template at<2>::type,
					typename impl::template at<3>::type,
					typename impl::template at<4>::type);

			static
			typename impl::template at<0>::type
			deferred_api_call(
					typename impl::template at<1>::type a1,
					typename impl::template at<2>::type a2,
					typename impl::template at<3>::type a3,
					typename impl::template at<4>::type a4)
			{
				// Make sure we lose the GIL.
				PythonInterpreterLocker interpreter_locker;
				PythonInterpreterUnlocker interpreter_unlocker;

				return GPlatesUtils::DeferCall<typename impl::template at<0>::type>::defer_call(
						boost::bind(
							FunctionPtr,
							impl::template wrap_value<1>(a1),
							impl::template wrap_value<2>(a2),
							impl::template wrap_value<3>(a3),
							impl::template wrap_value<4>(a4)),
						true);
			}
		};

		// Arity = 5.
		template<typename FunctionPtrType, FunctionPtrType FunctionPtr, class ArgReferenceWrappingsType>
		struct DeferredApiCall<FunctionPtrType, FunctionPtr, ArgReferenceWrappingsType, 5>
		{
			typedef typename GPlatesUtils::FunctionTypes::component_types<FunctionPtrType>::types types;
			typedef Impl<types, ArgReferenceWrappingsType> impl;

			typedef typename impl::template at<0>::type (*function_type)(
					typename impl::template at<1>::type,
					typename impl::template at<2>::type,
					typename impl::template at<3>::type,
					typename impl::template at<4>::type,
					typename impl::template at<5>::type);

			static
			typename impl::template at<0>::type
			deferred_api_call(
					typename impl::template at<1>::type a1,
					typename impl::template at<2>::type a2,
					typename impl::template at<3>::type a3,
					typename impl::template at<4>::type a4,
					typename impl::template at<5>::type a5)
			{
				// Make sure we lose the GIL.
				PythonInterpreterLocker interpreter_locker;
				PythonInterpreterUnlocker interpreter_unlocker;

				return GPlatesUtils::DeferCall<typename impl::template at<0>::type>::defer_call(
						boost::bind(
							FunctionPtr,
							impl::template wrap_value<1>(a1),
							impl::template wrap_value<2>(a2),
							impl::template wrap_value<3>(a3),
							impl::template wrap_value<4>(a4),
							impl::template wrap_value<5>(a5)),
						true);
			}
		};

		// Arity = 6.
		template<typename FunctionPtrType, FunctionPtrType FunctionPtr, class ArgReferenceWrappingsType>
		struct DeferredApiCall<FunctionPtrType, FunctionPtr, ArgReferenceWrappingsType, 6>
		{
			typedef typename GPlatesUtils::FunctionTypes::component_types<FunctionPtrType>::types types;
			typedef Impl<types, ArgReferenceWrappingsType> impl;

			typedef typename impl::template at<0>::type (*function_type)(
					typename impl::template at<1>::type,
					typename impl::template at<2>::type,
					typename impl::template at<3>::type,
					typename impl::template at<4>::type,
					typename impl::template at<5>::type,
					typename impl::template at<6>::type);

			static
			typename impl::template at<0>::type
			deferred_api_call(
					typename impl::template at<1>::type a1,
					typename impl::template at<2>::type a2,
					typename impl::template at<3>::type a3,
					typename impl::template at<4>::type a4,
					typename impl::template at<5>::type a5,
					typename impl::template at<6>::type a6)
			{
				// Make sure we lose the GIL.
				PythonInterpreterLocker interpreter_locker;
				PythonInterpreterUnlocker interpreter_unlocker;

				return GPlatesUtils::DeferCall<typename impl::template at<0>::type>::defer_call(
						boost::bind(
							FunctionPtr,
							impl::template wrap_value<1>(a1),
							impl::template wrap_value<2>(a2),
							impl::template wrap_value<3>(a3),
							impl::template wrap_value<4>(a4),
							impl::template wrap_value<5>(a5),
							impl::template wrap_value<6>(a6)),
						true);
			}
		};

		// Arity = 7.
		template<typename FunctionPtrType, FunctionPtrType FunctionPtr, class ArgReferenceWrappingsType>
		struct DeferredApiCall<FunctionPtrType, FunctionPtr, ArgReferenceWrappingsType, 7>
		{
			typedef typename GPlatesUtils::FunctionTypes::component_types<FunctionPtrType>::types types;
			typedef Impl<types, ArgReferenceWrappingsType> impl;

			typedef typename impl::template at<0>::type (*function_type)(
					typename impl::template at<1>::type,
					typename impl::template at<2>::type,
					typename impl::template at<3>::type,
					typename impl::template at<4>::type,
					typename impl::template at<5>::type,
					typename impl::template at<6>::type,
					typename impl::template at<7>::type);

			static
			typename impl::template at<0>::type
			deferred_api_call(
					typename impl::template at<1>::type a1,
					typename impl::template at<2>::type a2,
					typename impl::template at<3>::type a3,
					typename impl::template at<4>::type a4,
					typename impl::template at<5>::type a5,
					typename impl::template at<6>::type a6,
					typename impl::template at<7>::type a7)
			{
				// Make sure we lose the GIL.
				PythonInterpreterLocker interpreter_locker;
				PythonInterpreterUnlocker interpreter_unlocker;

				return GPlatesUtils::DeferCall<typename impl::template at<0>::type>::defer_call(
						boost::bind(
							FunctionPtr,
							impl::template wrap_value<1>(a1),
							impl::template wrap_value<2>(a2),
							impl::template wrap_value<3>(a3),
							impl::template wrap_value<4>(a4),
							impl::template wrap_value<5>(a5),
							impl::template wrap_value<6>(a6),
							impl::template wrap_value<7>(a7)),
						true);
			}
		};

		// Arity = 8.
		template<typename FunctionPtrType, FunctionPtrType FunctionPtr, class ArgReferenceWrappingsType>
		struct DeferredApiCall<FunctionPtrType, FunctionPtr, ArgReferenceWrappingsType, 8>
		{
			typedef typename GPlatesUtils::FunctionTypes::component_types<FunctionPtrType>::types types;
			typedef Impl<types, ArgReferenceWrappingsType> impl;

			typedef typename impl::template at<0>::type (*function_type)(
					typename impl::template at<1>::type,
					typename impl::template at<2>::type,
					typename impl::template at<3>::type,
					typename impl::template at<4>::type,
					typename impl::template at<5>::type,
					typename impl::template at<6>::type,
					typename impl::template at<7>::type,
					typename impl::template at<8>::type);

			static
			typename impl::template at<0>::type
			deferred_api_call(
					typename impl::template at<1>::type a1,
					typename impl::template at<2>::type a2,
					typename impl::template at<3>::type a3,
					typename impl::template at<4>::type a4,
					typename impl::template at<5>::type a5,
					typename impl::template at<6>::type a6,
					typename impl::template at<7>::type a7,
					typename impl::template at<8>::type a8)
			{
				// Make sure we lose the GIL.
				PythonInterpreterLocker interpreter_locker;
				PythonInterpreterUnlocker interpreter_unlocker;

				return GPlatesUtils::DeferCall<typename impl::template at<0>::type>::defer_call(
						boost::bind(
							FunctionPtr,
							impl::template wrap_value<1>(a1),
							impl::template wrap_value<2>(a2),
							impl::template wrap_value<3>(a3),
							impl::template wrap_value<4>(a4),
							impl::template wrap_value<5>(a5),
							impl::template wrap_value<6>(a6),
							impl::template wrap_value<7>(a7),
							impl::template wrap_value<8>(a8)),
						true);
			}
		};

		// Arity = 9.
		template<typename FunctionPtrType, FunctionPtrType FunctionPtr, class ArgReferenceWrappingsType>
		struct DeferredApiCall<FunctionPtrType, FunctionPtr, ArgReferenceWrappingsType, 9>
		{
			typedef typename GPlatesUtils::FunctionTypes::component_types<FunctionPtrType>::types types;
			typedef Impl<types, ArgReferenceWrappingsType> impl;

			typedef typename impl::template at<0>::type (*function_type)(
					typename impl::template at<1>::type,
					typename impl::template at<2>::type,
					typename impl::template at<3>::type,
					typename impl::template at<4>::type,
					typename impl::template at<5>::type,
					typename impl::template at<6>::type,
					typename impl::template at<7>::type,
					typename impl::template at<8>::type,
					typename impl::template at<9>::type);

			static
			typename impl::template at<0>::type
			deferred_api_call(
					typename impl::template at<1>::type a1,
					typename impl::template at<2>::type a2,
					typename impl::template at<3>::type a3,
					typename impl::template at<4>::type a4,
					typename impl::template at<5>::type a5,
					typename impl::template at<6>::type a6,
					typename impl::template at<7>::type a7,
					typename impl::template at<8>::type a8,
					typename impl::template at<9>::type a9)
			{
				// Make sure we lose the GIL.
				PythonInterpreterLocker interpreter_locker;
				PythonInterpreterUnlocker interpreter_unlocker;

				return GPlatesUtils::DeferCall<typename impl::template at<0>::type>::defer_call(
						boost::bind(
							FunctionPtr,
							impl::template wrap_value<1>(a1),
							impl::template wrap_value<2>(a2),
							impl::template wrap_value<3>(a3),
							impl::template wrap_value<4>(a4),
							impl::template wrap_value<5>(a5),
							impl::template wrap_value<6>(a6),
							impl::template wrap_value<7>(a7),
							impl::template wrap_value<8>(a8),
							impl::template wrap_value<9>(a9)),
						true);
			}
		};

		// Arity = 10.
		template<typename FunctionPtrType, FunctionPtrType FunctionPtr, class ArgReferenceWrappingsType>
		struct DeferredApiCall<FunctionPtrType, FunctionPtr, ArgReferenceWrappingsType, 10>
		{
			typedef typename GPlatesUtils::FunctionTypes::component_types<FunctionPtrType>::types types;
			typedef Impl<types, ArgReferenceWrappingsType> impl;

			typedef typename impl::template at<0>::type (*function_type)(
					typename impl::template at<1>::type,
					typename impl::template at<2>::type,
					typename impl::template at<3>::type,
					typename impl::template at<4>::type,
					typename impl::template at<5>::type,
					typename impl::template at<6>::type,
					typename impl::template at<7>::type,
					typename impl::template at<8>::type,
					typename impl::template at<9>::type,
					typename impl::template at<10>::type);

			static
			typename impl::template at<0>::type
			deferred_api_call(
					typename impl::template at<1>::type a1,
					typename impl::template at<2>::type a2,
					typename impl::template at<3>::type a3,
					typename impl::template at<4>::type a4,
					typename impl::template at<5>::type a5,
					typename impl::template at<6>::type a6,
					typename impl::template at<7>::type a7,
					typename impl::template at<8>::type a8,
					typename impl::template at<9>::type a9,
					typename impl::template at<10>::type a10)
			{
				// Make sure we lose the GIL.
				PythonInterpreterLocker interpreter_locker;
				PythonInterpreterUnlocker interpreter_unlocker;

				return GPlatesUtils::DeferCall<typename impl::template at<0>::type>::defer_call(
						boost::bind(
							FunctionPtr,
							impl::template wrap_value<1>(a1),
							impl::template wrap_value<2>(a2),
							impl::template wrap_value<3>(a3),
							impl::template wrap_value<4>(a4),
							impl::template wrap_value<5>(a5),
							impl::template wrap_value<6>(a6),
							impl::template wrap_value<7>(a7),
							impl::template wrap_value<8>(a8),
							impl::template wrap_value<9>(a9),
							impl::template wrap_value<10>(a10)),
						true);
			}
		};

		/**
		 * The template parameter @a FunctionPtrType is deduced using @a make_wrapper
		 * below, and once we have a @a Wrapper object instantiated with the correct
		 * @a FunctionPtrType, we can then call @a wrap, which takes the
		 * @a FunctionPtr itself as a template parameter. See comments above for why
		 * the function pointer needs to be baked in at compile time.
		 */
		template<typename FunctionPtrType>
		struct Wrapper
		{
#if defined(_MSC_VER) && _MSC_VER <= 1400
			// Workaround for Visual Studio 2005. See else case for description.
			// Function pointers are valid template parameters, but Visual Studio 2005
			// seems to have trouble with template parameters that are pointers to
			// non-member functions whose type is a template parameter of the enclosing
			// class. It seems to have no such problem with structs that have such a
			// template parameter - I'm putting this down to a compiler bug. Note also
			// that it works fine with pointers to member functions.
			template<void *FunctionPtr, class ArgReferenceWrappingsType>
			inline
			typename DeferredApiCall<
				FunctionPtrType,
				static_cast<FunctionPtrType>(FunctionPtr),
				ArgReferenceWrappingsType,
				GPlatesUtils::FunctionTypes::function_arity<FunctionPtrType>::value
			>::function_type
			wrap(
					ArgReferenceWrappingsType) const
			{
				return &DeferredApiCall<
					FunctionPtrType,
					static_cast<FunctionPtrType>(FunctionPtr),
					ArgReferenceWrappingsType,
					GPlatesUtils::FunctionTypes::function_arity<FunctionPtrType>::value
				>::deferred_api_call;
			}
#else
			/**
			 * Returns a pointer to a function that binds @a FunctionPtr to its arguments
			 * and posts it to the @a qApp singleton for execution on the main GUI thread.
			 *
			 * Why is there an unnamed parameter of type @a ArgReferenceWrappingsType you
			 * ask? It's because this function is called by the macro
			 * @a GPLATES_DEFERRED_API_CALL. If the user specifies more than one template
			 * argument to ArgReferenceWrappings, it's going to have a comma between the
			 * template arguments, which screws up the macro call. The usual way to fix
			 * that is to put everything into parentheses, but unfortunately, having
			 * parentheses in the template argument list isn't allowed. So, what we do
			 * instead is pass in the parenthesised expression as the argument to this
			 * function and let the compiler deduce @a ArgReferenceWrappingsType.
			 */
			template<FunctionPtrType FunctionPtr, class ArgReferenceWrappingsType>
			inline
			typename DeferredApiCall<
				FunctionPtrType,
				FunctionPtr,
				ArgReferenceWrappingsType,
				GPlatesUtils::FunctionTypes::function_arity<FunctionPtrType>::value
			>::function_type
			wrap(
					ArgReferenceWrappingsType) const
			{
				return &DeferredApiCall<
					FunctionPtrType,
					FunctionPtr,
					ArgReferenceWrappingsType,
					GPlatesUtils::FunctionTypes::function_arity<FunctionPtrType>::value
				>::deferred_api_call;
			}
#endif
		};

		// The same as Wrapper but for member function pointers.
		template<typename FunctionPtrType>
		struct MemberFunctionWrapper
		{
			template<FunctionPtrType FunctionPtr, class ArgReferenceWrappingsType>
			inline
			typename DeferredApiCall<
				FunctionPtrType,
				FunctionPtr,
				ArgReferenceWrappingsType,
				GPlatesUtils::FunctionTypes::function_arity<FunctionPtrType>::value
			>::function_type
			wrap(
					ArgReferenceWrappingsType) const
			{
				return &DeferredApiCall<
					FunctionPtrType,
					FunctionPtr,
					ArgReferenceWrappingsType,
					GPlatesUtils::FunctionTypes::function_arity<FunctionPtrType>::value
				>::deferred_api_call;
			}
		};

		/**
		 * This function here is used to deduce the type of @a function_ptr for
		 * pointers to non-member functions.
		 */
		template<typename FunctionPtrType>
		inline
		Wrapper<FunctionPtrType>
		make_wrapper(
				FunctionPtrType function_ptr,
				typename boost::disable_if< boost::is_member_function_pointer<FunctionPtrType> >::type *dummy = NULL)
		{
			return Wrapper<FunctionPtrType>();
		}

		/**
		 * This function here is used to deduce the type of @a function_ptr for
		 * pointers to member functions.
		 */
		template<typename FunctionPtrType>
		inline
		MemberFunctionWrapper<FunctionPtrType>
		make_wrapper(
				FunctionPtrType function_ptr,
				typename boost::enable_if< boost::is_member_function_pointer<FunctionPtrType> >::type *dummy = NULL)
		{
			return MemberFunctionWrapper<FunctionPtrType>();
		}
	}
}

#endif  // GPLATES_API_DEFERREDAPICALLIMPL_H
