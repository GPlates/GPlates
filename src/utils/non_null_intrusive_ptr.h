#ifndef NON_NULL_INTRUSIVE_PTR_HPP_INCLUDED
#define NON_NULL_INTRUSIVE_PTR_HPP_INCLUDED

//
//  non_null_intrusive_ptr.h
//
// This header file was derived from the header file "intrusive_ptr.hpp".
//
//  Copyright (c) 2001, 2002 Peter Dimov
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
//  See http://www.boost.org/libs/smart_ptr/intrusive_ptr.html for
//  documentation for "intrusive_ptr.hpp".
//

#include <boost/config.hpp>

#ifdef BOOST_MSVC  // moved here to work around VC++ compiler crash
# pragma warning(push)
# pragma warning(disable:4284) // odd return type for operator->
#endif

#include <boost/assert.hpp>
#include <boost/detail/workaround.hpp>

#include <functional>           // for std::less
#include <iosfwd>               // for std::basic_ostream
#include <boost/intrusive_ptr.hpp>


namespace GPlatesUtils
{

// This exception is thrown when the null_handler_type fails to handle a NULL pointer.
struct UnhandledNullPointerException {  };

//
//  non_null_intrusive_ptr
//
//  A smart pointer that uses intrusive reference counting.
//
//  It cannot have a "NULL" target.
//
//  Relies on unqualified calls to
//  
//      void intrusive_ptr_add_ref(T * p);
//      void intrusive_ptr_release(T * p);
//
//  The object is responsible for destroying itself.
//

template<class T, class H> class non_null_intrusive_ptr
{
private:

    typedef non_null_intrusive_ptr this_type;

public:

    typedef T element_type;
    typedef H null_handler_type;

    non_null_intrusive_ptr(T * p, H const & handle_null, bool add_ref = true): p_(p)
    {
        if(p_ == 0) {
            // Give 'handle_null' a chance to handle the situation.
            handle_null();

            // If 'handle_null' doesn't handle the situation, fall back to throwing an exception.
            throw UnhandledNullPointerException();
	}
        if(add_ref) intrusive_ptr_add_ref(p_);
    }

#if !defined(BOOST_NO_MEMBER_TEMPLATES) || defined(BOOST_MSVC6_MEMBER_TEMPLATES)

    template<class U, class I> non_null_intrusive_ptr(non_null_intrusive_ptr<U, I> const & rhs): p_(rhs.get())
    {
        intrusive_ptr_add_ref(p_);
    }

#endif

    non_null_intrusive_ptr(non_null_intrusive_ptr const & rhs): p_(rhs.p_)
    {
        intrusive_ptr_add_ref(p_);
    }

    ~non_null_intrusive_ptr()
    {
        intrusive_ptr_release(p_);
    }

#if !defined(BOOST_NO_MEMBER_TEMPLATES) || defined(BOOST_MSVC6_MEMBER_TEMPLATES)

    template<class U, class I> non_null_intrusive_ptr & operator=(non_null_intrusive_ptr<U, I> const & rhs)
    {
        this_type(rhs).swap(*this);
        return *this;
    }

#endif

    non_null_intrusive_ptr & operator=(non_null_intrusive_ptr const & rhs)
    {
        this_type(rhs).swap(*this);
        return *this;
    }

    T * get() const
    {
        return p_;
    }

    T & operator*() const
    {
        return *p_;
    }

    T * operator->() const
    {
        return p_;
    }

#if defined(__SUNPRO_CC) && BOOST_WORKAROUND(__SUNPRO_CC, <= 0x530)

    operator bool () const
    {
        return p_ != 0;
    }

#elif defined(__MWERKS__) && BOOST_WORKAROUND(__MWERKS__, BOOST_TESTED_AT(0x3003))
    typedef T * (this_type::*unspecified_bool_type)() const;
    
    operator unspecified_bool_type() const // never throws
    {
        return p_ == 0? 0: &this_type::get;
    }

#else 

    typedef T * this_type::*unspecified_bool_type;

    operator unspecified_bool_type () const
    {
        return p_ == 0? 0: &this_type::p_;
    }

#endif

    // operator! is a Borland-specific workaround
    bool operator! () const
    {
        return p_ == 0;
    }

    void swap(non_null_intrusive_ptr & rhs)
    {
        T * tmp = p_;
        p_ = rhs.p_;
        rhs.p_ = tmp;
    }

private:

    T * p_;
};

template<class T, class H, class U, class I> inline bool operator==(non_null_intrusive_ptr<T, H> const & a, non_null_intrusive_ptr<U, I> const & b)
{
    return a.get() == b.get();
}

template<class T, class H, class U, class I> inline bool operator!=(non_null_intrusive_ptr<T, H> const & a, non_null_intrusive_ptr<U, I> const & b)
{
    return a.get() != b.get();
}

template<class T, class H, class U> inline bool operator==(non_null_intrusive_ptr<T, H> const & a, boost::intrusive_ptr<U> const & b)
{
    return a.get() == b.get();
}

template<class T, class H, class U> inline bool operator!=(non_null_intrusive_ptr<T, H> const & a, boost::intrusive_ptr<U> const & b)
{
    return a.get() != b.get();
}

template<class T, class U, class I> inline bool operator==(boost::intrusive_ptr<T> const & a, non_null_intrusive_ptr<U, I> const & b)
{
    return a.get() == b.get();
}

template<class T, class U, class I> inline bool operator!=(boost::intrusive_ptr<T> const & a, non_null_intrusive_ptr<U, I> const & b)
{
    return a.get() != b.get();
}

template<class T, class H> inline bool operator==(non_null_intrusive_ptr<T, H> const & a, T * b)
{
    return a.get() == b;
}

template<class T, class H> inline bool operator!=(non_null_intrusive_ptr<T, H> const & a, T * b)
{
    return a.get() != b;
}

template<class T, class H> inline bool operator==(T * a, non_null_intrusive_ptr<T, H> const & b)
{
    return a == b.get();
}

template<class T, class H> inline bool operator!=(T * a, non_null_intrusive_ptr<T, H> const & b)
{
    return a != b.get();
}

#if __GNUC__ == 2 && __GNUC_MINOR__ <= 96

// Resolve the ambiguity between our op!= and the one in rel_ops

template<class T, class H> inline bool operator!=(non_null_intrusive_ptr<T, H> const & a, non_null_intrusive_ptr<T, H> const & b)
{
    return a.get() != b.get();
}

#endif

template<class T, class H> inline bool operator<(non_null_intrusive_ptr<T, H> const & a, non_null_intrusive_ptr<T, H> const & b)
{
    return std::less<T *>()(a.get(), b.get());
}

template<class T, class H> void swap(non_null_intrusive_ptr<T, H> & lhs, non_null_intrusive_ptr<T, H> & rhs)
{
    lhs.swap(rhs);
}

template<class T, class H> boost::intrusive_ptr<T> get_intrusive_ptr(non_null_intrusive_ptr<T, H> const & p)
{
    boost::intrusive_ptr<T> ip(p.get());
    return ip;
}

// mem_fn support

template<class T, class H> T * get_pointer(non_null_intrusive_ptr<T, H> const & p)
{
    return p.get();
}

template<class T, class H, class U, class I> non_null_intrusive_ptr<T, H> static_pointer_cast(non_null_intrusive_ptr<U, I> const & p)
{
    return static_cast<T *>(p.get());
}

template<class T, class H, class U, class I> non_null_intrusive_ptr<T, H> const_pointer_cast(non_null_intrusive_ptr<U, I> const & p)
{
    return const_cast<T *>(p.get());
}

template<class T, class H, class U, class I> non_null_intrusive_ptr<T, H> dynamic_pointer_cast(non_null_intrusive_ptr<U, I> const & p)
{
    return dynamic_cast<T *>(p.get());
}

// operator<<

#if defined(__GNUC__) &&  (__GNUC__ < 3)

template<class Y, class Z> std::ostream & operator<< (std::ostream & os, non_null_intrusive_ptr<Y, Z> const & p)
{
    os << p.get();
    return os;
}

#else

# if defined(BOOST_MSVC) && BOOST_WORKAROUND(BOOST_MSVC, <= 1200 && __SGI_STL_PORT)
// MSVC6 has problems finding std::basic_ostream through the using declaration in namespace _STL
using std::basic_ostream;
template<class E, class T, class Y, class Z> basic_ostream<E, T> & operator<< (basic_ostream<E, T> & os, non_null_intrusive_ptr<Y, Z> const & p)
# else
template<class E, class T, class Y, class Z> std::basic_ostream<E, T> & operator<< (std::basic_ostream<E, T> & os, non_null_intrusive_ptr<Y, Z> const & p)
# endif 
{
    os << p.get();
    return os;
}

#endif

} // namespace GPlatesUtils

#ifdef BOOST_MSVC
# pragma warning(pop)
#endif    

#endif  // #ifndef NON_NULL_INTRUSIVE_PTR_HPP_INCLUDED
