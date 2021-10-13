// The contents of the file after the #else was copied from Boost 1.35.
// If the user's version of Boost has fpclassify.hpp, it is used; otherwise this
// version from Boost 1.35 is used.
// This file is in system-fixes because Boost 1.34 does not have this file.
#include "global/config.h"

#ifdef BOOST_MATH_SPECIAL_FUNCTIONS_FPCLASSIFY_HPP_PATH
#	include BOOST_MATH_SPECIAL_FUNCTIONS_FPCLASSIFY_HPP_PATH
#else // BOOST_MATH_SPECIAL_FUNCTIONS_FPCLASSIFY_HPP_PATH

//  Copyright John Maddock 2005-2006.
//  Use, modification and distribution are subject to the
//  Boost Software License, Version 1.0. (See accompanying file
//  LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)

#ifndef BOOST_MATH_FPCLASSIFY_HPP
#define BOOST_MATH_FPCLASSIFY_HPP

#include <math.h>
#include <cmath>
#include <boost/limits.hpp>
#include <boost/math/tools/real_cast.hpp>
#include <boost/type_traits/is_floating_point.hpp>
// GPlates note: The following include has been commented out from the original
// fpclassify.hpp file and the following #define was copied over from math_fwd.hpp.
// #include <boost/math/special_functions/math_fwd.hpp>
#define BOOST_NO_MACRO_EXPAND /**/

#if defined(_MSC_VER) || defined(__BORLANDC__)
#include <float.h>
#endif

#ifdef BOOST_NO_STDC_NAMESPACE
  namespace std{ using ::abs; using ::fabs; }
#endif

#ifndef FP_NORMAL

#define FP_ZERO        0
#define FP_NORMAL      1
#define FP_INFINITE    2
#define FP_NAN         3
#define FP_SUBNORMAL   4

#else

#define BOOST_HAS_FPCLASSIFY

#ifndef fpclassify
#  if (defined(__GLIBCPP__) || defined(__GLIBCXX__)) && defined(_GLIBCXX_USE_C99_MATH) && !(defined(_GLIBCXX_USE_C99_FP_MACROS_DYNAMIC) && (_GLIBCXX_USE_C99_FP_MACROS_DYNAMIC != 0))
#     define BOOST_FPCLASSIFY_PREFIX ::std::
#  else
#     undef BOOST_HAS_FPCLASSIFY
#     define BOOST_FPCLASSIFY_PREFIX
#  endif
#elif (defined(__HP_aCC) && !defined(__hppa))
// aCC 6 appears to do "#define fpclassify fpclassify" which messes us up a bit!
#  define BOOST_FPCLASSIFY_PREFIX ::
#else
#  define BOOST_FPCLASSIFY_PREFIX
#endif

#ifdef __MINGW32__
#  undef BOOST_HAS_FPCLASSIFY
#endif

#endif

namespace boost{ 

#if defined(BOOST_HAS_FPCLASSIFY) || defined(isnan)
//
// This must not be located in any namespace under boost::math
// otherwise we can get into an infinite loop if isnan is
// a #define for "isnan" !
//
namespace math_detail{

template <class T>
inline bool is_nan_helper(T t, const boost::true_type&)
{
#ifdef isnan
   return isnan(t);
#else // BOOST_HAS_FPCLASSIFY
   return (BOOST_FPCLASSIFY_PREFIX fpclassify(t) == FP_NAN);
#endif
}

template <class T>
inline bool is_nan_helper(T t, const boost::false_type&)
{
   return false;
}

}

#endif // defined(BOOST_HAS_FPCLASSIFY) || defined(isnan)

namespace math{

namespace detail{

template <class T>
inline int fpclassify_imp BOOST_NO_MACRO_EXPAND(T t, const mpl::true_&)
{
   // whenever possible check for Nan's first:
#ifdef BOOST_HAS_FPCLASSIFY
   if(::boost::math_detail::is_nan_helper(t, ::boost::is_floating_point<T>()))
      return FP_NAN;
#elif defined(isnan)
   if(boost::math_detail::is_nan_helper(t, ::boost::is_floating_point<T>()))
      return FP_NAN;
#elif defined(_MSC_VER) || defined(__BORLANDC__)
   if(::_isnan(boost::math::tools::real_cast<double>(t)))
      return FP_NAN;
#endif
   // std::fabs broken on a few systems especially for long long!!!!
   T at = (t < T(0)) ? -t : t;

   // Use a process of exclusion to figure out
   // what kind of type we have, this relies on
   // IEEE conforming reals that will treat
   // Nan's as unordered.  Some compilers
   // don't do this once optimisations are
   // turned on, hence the check for nan's above.
   if(at <= (std::numeric_limits<T>::max)())
   {
      if(at >= (std::numeric_limits<T>::min)())
         return FP_NORMAL;
      return (at != 0) ? FP_SUBNORMAL : FP_ZERO;
   }
   else if(at > (std::numeric_limits<T>::max)())
      return FP_INFINITE;
   return FP_NAN;
}

template <class T>
inline int fpclassify_imp BOOST_NO_MACRO_EXPAND(T t, const mpl::false_&)
{
   // 
   // An unknown type with no numeric_limits support,
   // so what are we supposed to do we do here?
   //
   return t == 0 ? FP_ZERO : FP_NORMAL;
}

}  // namespace detail

template <class T>
inline int fpclassify BOOST_NO_MACRO_EXPAND(T t)
{
#ifdef BOOST_NO_LIMITS_COMPILE_TIME_CONSTANTS
   if(std::numeric_limits<T>::is_specialized)
      return detail::fpclassify_imp(t, mpl::true_());
   return detail::fpclassify_imp(t, mpl::false_());
#else
   return detail::fpclassify_imp(t, mpl::bool_< ::std::numeric_limits<T>::is_specialized>());
#endif
}

#if defined(BOOST_HAS_FPCLASSIFY)
inline int fpclassify BOOST_NO_MACRO_EXPAND(float t)
{
   return BOOST_FPCLASSIFY_PREFIX fpclassify(t);
}
inline int fpclassify BOOST_NO_MACRO_EXPAND(double t)
{
   return BOOST_FPCLASSIFY_PREFIX fpclassify(t);
}
#if !defined(__CYGWIN__) && !defined(__HP_aCC) && !defined(BOOST_INTEL) && !defined(BOOST_NO_NATIVE_LONG_DOUBLE_FP_CLASSIFY)
// The native fpclassify broken for long doubles with aCC
// use portable one instead....
inline int fpclassify BOOST_NO_MACRO_EXPAND(long double t)
{
   return BOOST_FPCLASSIFY_PREFIX fpclassify(t);
}
#endif

#elif defined(_MSC_VER)
// This only works for type double, for both float
// and long double it gives misleading answers.
inline int fpclassify BOOST_NO_MACRO_EXPAND(double t)
{
   switch(::_fpclass(t))
   {
   case _FPCLASS_SNAN /* Signaling NaN */ :
   case _FPCLASS_QNAN /* Quiet NaN */ :
      return FP_NAN;
   case _FPCLASS_NINF /*Negative infinity ( �INF) */ :
   case _FPCLASS_PINF /* Positive infinity (+INF) */ :
      return FP_INFINITE;
   case _FPCLASS_NN /* Negative normalized non-zero */ :
   case _FPCLASS_PN /* Positive normalized non-zero */ :
      return FP_NORMAL;
   case _FPCLASS_ND /* Negative denormalized */:
   case _FPCLASS_PD /* Positive denormalized */ :
      return FP_SUBNORMAL;
   case _FPCLASS_NZ /* Negative zero ( � 0) */ :
   case _FPCLASS_PZ /* Positive 0 (+0) */ :
      return FP_ZERO;
   default:
      /**/ ;
   }
   return FP_NAN;  // should never get here!!!
}
#endif

template <class T>
inline bool isfinite BOOST_NO_MACRO_EXPAND(T z)
{
   int t = (::boost::math::fpclassify)(z);
   return (t != FP_NAN) && (t != FP_INFINITE);
}

template <class T>
inline bool isinf BOOST_NO_MACRO_EXPAND(T t)
{
   return (::boost::math::fpclassify)(t) == FP_INFINITE;
}

template <class T>
inline bool isnan BOOST_NO_MACRO_EXPAND(T t)
{
   return (::boost::math::fpclassify)(t) == FP_NAN;
}
#ifdef isnan
template <> inline bool isnan BOOST_NO_MACRO_EXPAND<float>(float t){ return ::boost::math_detail::is_nan_helper(t, boost::true_type()); }
template <> inline bool isnan BOOST_NO_MACRO_EXPAND<double>(double t){ return ::boost::math_detail::is_nan_helper(t, boost::true_type()); }
template <> inline bool isnan BOOST_NO_MACRO_EXPAND<long double>(long double t){ return ::boost::math_detail::is_nan_helper(t, boost::true_type()); }
#elif defined(BOOST_MSVC)
#  pragma warning(push)
#  pragma warning(disable: 4800) // forcing value to bool 'true' or 'false' 
#  pragma warning(disable: 4244) // conversion from 'long double' to 'double',
// No possible loss of data because they are same size.
template <> inline bool isnan BOOST_NO_MACRO_EXPAND<float>(float t){ return _isnan(t); }
template <> inline bool isnan BOOST_NO_MACRO_EXPAND<double>(double t){ return _isnan(t); }
template <> inline bool isnan BOOST_NO_MACRO_EXPAND<long double>(long double t){ return _isnan(t); }
#pragma warning (pop)
#endif

template <class T>
inline bool isnormal BOOST_NO_MACRO_EXPAND(T t)
{
   return (::boost::math::fpclassify)(t) == FP_NORMAL;
}

} // namespace math
} // namespace boost

#endif // BOOST_MATH_FPCLASSIFY_HPP

#endif // BOOST_MATH_SPECIAL_FUNCTIONS_FPCLASSIFY_HPP_PATH

