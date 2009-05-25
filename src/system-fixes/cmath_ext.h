#ifndef CMATH_EXTENDED
#define CMATH_EXTENDED

//
// Include this header instead of <cmath> if you want to use 'std::isnan'.
// It makes sure std::isnan exists on all compilers.
// If isnan is defined as a macro then it's moved to function in std namespace.
//

#include <cmath>

// Visual studio compiler does not support isnan.
#ifdef _MSC_VER
#include <cfloat>
#define isnan _isnan
#define isinf !_finite
#endif // _MSC_VER

#ifdef isnan

// If isnan macro is currently defined after including <cmath> then
// we need to use same trick as gnu compiler to move it into the std namespace.
// We do this because isnan is part of C99 which is not required to be implemented
// in C++ standard and we want to be able to use std::isnan for all compilers.

namespace std_impl
{
  template<typename Tp>
    inline int
    capture_isnan(Tp f) { return isnan(f); }
}

#undef isnan

namespace std
{
  template<typename Tp>
    inline int
    isnan(Tp f) { return std_impl::capture_isnan(f); }
}
#endif // ifdef isnan

#ifdef isinf

namespace std_impl
{
  template<typename Tp>
    inline int
    capture_isinf(Tp f) { return isinf(f); }
}

#undef isinf

namespace std
{
  template<typename Tp>
    inline int
    isinf(Tp f) { return std_impl::capture_isinf(f); }
}


#endif // ifdef isinf

#endif // CMATH_EXTENDED
