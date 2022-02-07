/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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

#ifndef GPLATES_SCRIBE_SCRIBECONSTRUCTOBJECT_H
#define GPLATES_SCRIBE_SCRIBECONSTRUCTOBJECT_H

#include <boost/noncopyable.hpp>
#include <boost/preprocessor/arithmetic/inc.hpp>
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/enum_shifted.hpp>
#include <boost/preprocessor/enum_shifted_params.hpp>
#include <boost/preprocessor/repetition/repeat_from_to.hpp>

#include "ScribeAccess.h"
#include "ScribeExceptions.h"

#include "global/GPlatesAssert.h"


/**
 * The maximum number of object constructor arguments supported in ConstructOjbect<>::construct_object().
 */
#define GPLATES_SCRIBE_CONSTRUCT_MAX_CONSTRUCTOR_ARGS \
		GPLATES_SCRIBE_ACCESS_CONSTRUCT_MAX_CONSTRUCTOR_ARGS


namespace GPlatesScribe
{
	/**
	 * Wrapper around a (possibly) un-initialised, or un-constructed, object.
	 *
	 * This is used in 'transcribe_construct_data()' to construct a transcribed object when loading
	 * from an archive. It is typically used for object types that don't have a default constructor.
	 *
	 * When loading from an archive a ConstructObject instance, passed in 'transcribe_construct_data()',
	 * is initially un-initialised memory - then calling 'ConstructObject<>::construct()' will
	 * construct its internal object - after which the object is initialised and can be accessed.
	 * When saving to an archive a ConstructObject instance always contains an initialised/constructed
	 * object (it is essentially just a reference to the existing object being transcribed/saved).
	 */
	template <typename ObjectType>
	class ConstructObject :
			private boost::noncopyable
	{
	public:

		/**
		 * Returns a reference to the internal object.
		 *
		 * Throws exception if the object has not yet been constructed.
		 */
		ObjectType &
		get_object()
		{
			GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
					d_is_object_initialised,
					GPLATES_ASSERTION_SOURCE,
					"Attempted to access uninitialised object.");

			return *d_object;
		}

		/**
		 * Returns a reference to the internal object.
		 *
		 * Throws exception if the object has not yet been constructed.
		 */
		ObjectType &
		operator*()
		{
			return get_object();
		}

		/**
		 * Returns a pointer to the internal object.
		 *
		 * Throws exception if the object has not yet been constructed.
		 */
		ObjectType *
		operator->()
		{
			return &get_object();
		}


		/**
		 * Constructs the internal object using the default constructor of 'ObjectType'.
		 *
		 * NOTE: If the 'ObjectType' constructor is private then you'll need the following friend
		 * declaration in your 'ObjectType' class:
		 *
		 *    friend class GPlatesScribe::Access;
		 */
		void
		construct_object()
		{
			GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
					!d_is_object_initialised,
					GPLATES_ASSERTION_SOURCE,
					"Attempted to construct an object that has already been constructed.");

			Access::construct_object(d_object);

			d_is_object_initialised = true;
		}


		//
		// Constructs the internal object using non-default constructors of 'ObjectType'
		// accepting a variable number of constructor arguments.
		//
		// For example...
		//
		//   construct_object(a, b, c, ...)
		//
		// ...where a, b, c, etc are constructor arguments for type ObjectType.
		//
		// NOTE: If a constructor argument is a *non-const* reference then you'll need to transport
		// it via boost::ref() - this is because 'construct_object()' arguments are *const* references.
		// Note that the same applies to boost::in_place().
		// For example, with the 'ObjectType' constructor signature:
		//
		//   ObjectType::ObjectType(A &, const B &)
		//
		// ...we can construct as follows...
		//
		//   A a = ...;
		//   B b = ...;
		//   ...
		//   construct_object(boost::ref(a), b, ...)
		//
		//
		// Note that this is similar to using boost::in_place() to transport a variable number of
		// constructor arguments, and is provided as a convenience as a slightly simpler
		// alternative to using boost::in_place() while enabling the client to simply declare
		// 'friend class GPlatesScribe::Access' in their client class.
		//
		// NOTE: If the 'ObjectType' constructor is private then you'll need the following friend
		// declaration in your 'ObjectType' class:
		//
		//    friend class GPlatesScribe::Access;
		//

		//
		// The following preprocessor macros generate the following code:
		//
		//    template <typename A1>
		//    void
		//    construct_object(
		//        const A1 &a1);
		//
		//    template <typename A1, typename A2>
		//    void
		//    construct_object(
		//        const A1 &a1,
		//        const A2 &a2);
		//
		//    template <typename A1, typename A2, typename A3>
		//    void
		//    construct_object(
		//        const A1 &a1,
		//        const A2 &a2,
		//        const A3 &a3);
		//
		//    ...ETC
		//

		#define GPLATES_SCRIBE_CONSTRUCT_OBJECT_PARAM(z, i, _) \
				BOOST_PP_CAT(const A, i) &BOOST_PP_CAT(a, i)

		#define GPLATES_SCRIBE_CONSTRUCT_OBJECT(z, n, _) \
				template <BOOST_PP_ENUM_SHIFTED_PARAMS(BOOST_PP_INC(n), typename A)> \
				void \
				construct_object( \
						BOOST_PP_ENUM_SHIFTED(BOOST_PP_INC(n), GPLATES_SCRIBE_CONSTRUCT_OBJECT_PARAM, _) ) \
				{ \
					GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>( \
							!d_is_object_initialised, \
							GPLATES_ASSERTION_SOURCE, \
							"Attempted to construct an object that has already been constructed."); \
					\
					Access::construct_object( \
							d_object, \
							BOOST_PP_ENUM_SHIFTED_PARAMS(BOOST_PP_INC(n), a)); \
					\
					d_is_object_initialised = true; \
				} \
				/**/

		// Create 'construct_object()' overloads.
		BOOST_PP_REPEAT_FROM_TO( \
				1, \
				BOOST_PP_INC(GPLATES_SCRIBE_CONSTRUCT_MAX_CONSTRUCTOR_ARGS), \
				GPLATES_SCRIBE_CONSTRUCT_OBJECT, \
				_)

		#undef GPLATES_SCRIBE_CONSTRUCT_OBJECT_PARAM
		#undef GPLATES_SCRIBE_CONSTRUCT_OBJECT


		/**
		 * Returns the address of the internal object.
		 *
		 * The internal object can be initialised/constructed or uninitialised.
		 * NOTE: Hence the returned pointer should not be dereferenced - if you need to
		 * dereference use @a get_object, or operator*() or operator->(), instead since they
		 * check that the object is initialised.
		 */
		ObjectType *
		get_object_address() const
		{
			return d_object;
		}

	protected:

		ConstructObject(
				ObjectType *object_,
				bool is_object_initialised_) :
			d_object(object_),
			d_is_object_initialised(is_object_initialised_)
		{  }

		bool
		is_object_initialised() const
		{
			return d_is_object_initialised;
		}

	private:

		ObjectType *d_object;
		bool d_is_object_initialised;

	};
}

#endif // GPLATES_SCRIBE_SCRIBECONSTRUCTOBJECT_H
