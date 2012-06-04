/* $Id$ */

/**
 * \file
 * Defines the Singleton class.
 *
 * Most recent change:
 *   $Date$
 *
 * Copyright (C) 2009, 2010 The University of Sydney, Australia
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

#ifndef GPLATES_UTILS_SINGLETON_H
#define GPLATES_UTILS_SINGLETON_H

#include <cstdlib>
#include <boost/checked_delete.hpp>
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
#ifdef GPLATES_SINGLETON_THREADSAFE
#include <QMutexLocker>
#endif

#include "global/GPlatesAssert.h"
#include "global/LogException.h"
#include "global/PreconditionViolationError.h"


namespace GPlatesUtils
{
	/**
	 * Singleton creation policy - allocates/constructs using operator new.
	 */
	template <typename T>
	class CreateUsingNew
	{
	public:
		static
		T *
		create_instance()
		{
			return new T();
		}

		static
		void
		destroy_instance(
				T *t)
		{
			boost::checked_delete(t);
		}
	};


	/**
	 * Singleton lifetime policy - schedules singleton for destruction in reverse order of creation.
	 */
	template <typename T>
	class DefaultLifetime
	{
	public:
		static
		void
		on_dead_reference()
		{
			throw GPlatesGlobal::LogException(
					GPLATES_EXCEPTION_SOURCE,	
					"Access to singleton after destruction disallowed.");
		}

		static
		void
		schedule_for_destruction(
				T *singleton_instance,
				void (*destruction_function_ptr)())
		{
			std::atexit(destruction_function_ptr);
		}
	};

	struct DefaultInstanceTag;


	/**
	 * Base class for types that are singletons. For an explanation of singletons,
	 * see the Design Patterns book.
	 * 
	 * A class T that derives from Singleton<T> will have a static instance()
	 * function that returns a reference to the one and only instance of T. The
	 * returned object cannot be copied.
	 * This is the standard approach to defining a singleton.
	 * An alternative approach, that does not require sub-classing, is to use the
	 * 'InstanceTag' template parameter to effectively change the singleton *type*.
	 * The class named as the 'InstanceTag' parameter is not instantiated or otherwise used;
	 * the parameter is merely provided to select, at compile time, which of the many instances
	 * of T, instance() is to return.
	 *
	 * If it should not be possible for client code to create an instance of T,
	 * insert the GPLATES_SINGLETON_CONSTRUCTOR_DECL(T) macro at the top of the
	 * definition of T. This declares (but does not define) the constructor as protected,
	 * and makes CreateUsingNew<T> a friend class so that it can construct an instance of T.
	 * The constructor of T will need to be defined elsewhere.
	 *
	 * Alternatively, use GPLATES_SINGLETON_CONSTRUCTOR_DEF(T) to declare the
	 * constructor as well as define it with an empty body. (Note the
	 * distinction between a 'declaration' and a 'definition' in C++.)
	 *
	 * If the client *should* be allowed to create an instance of T then use
	 * GPLATES_SINGLETON_PUBLIC_CONSTRUCTOR_DECL and GPLATES_SINGLETON_PUBLIC_CONSTRUCTOR_DEF instead.
	 * Note the 'PUBLIC' in the macro name.
	 * In this case you'll also need to inherit from Singleton<T> in order that Singleton<T>
	 * can be instantiated as in:
	 *
	 * class MySingleton :
	 *       public Singleton<MySingleton>
	 *
	 * This is how a derived class can allow creation of a singleton on the C runtime stack
	 * in order to control the lifetime of the singleton (ie, limited to the scope surrounding
	 * the singleton object).
	 * Note that 'instance()' can still be called to retrieve the singleton object while
	 * it is in scope on the C runtime stack.
	 *
	 * It is also possible to obtain a singleton of an existing class T by calling
	 *
	 *     Singleton<T>::instance()
	 *
	 * Note that this implementation of a singleton is not thread-safe. Define
	 * GPLATES_SINGLETON_THREADSAFE before including this header if this is required.
	 *
	 * Ideas on the policy classes where mainly obtained from:
	 *  http://www.codeproject.com/Articles/4750/Singleton-Pattern-A-review-and-analysis-of-existin
	 */
	template <
			typename T,
			template <typename> class CreationPolicy = CreateUsingNew,
			template <typename> class LifetimePolicy = DefaultLifetime,
			class InstanceTag = DefaultInstanceTag >
	class Singleton :
			private boost::noncopyable
	{

	public:

		/**
		 * Returns a reference to the single instance of T. If the instance has not
		 * been created yet, the instance is created using SingletonFactory.
		 */
		static
		T&
		instance()
		{
#if defined( GPLATES_SINGLETON_THREADSAFE )
			QMutexLocker locker;
#endif
			if (!s_instance_ptr)
			{
				if (s_singleton_destroyed)
				{
					s_singleton_destroyed = false;

					// Singleton has already been destroyed so either throw an exception or
					// allow a new singleton instance to be created (by doing nothing).
					LifetimePolicy<T>::on_dead_reference();
				}

				s_instance_ptr = CreationPolicy<T>::create_instance();

				// Note that even though the singleton instance is scheduled for destruction
				// (presumably at exit) it is still possible for derived singleton class 'T'
				// to allow instantiation on the C runtime stack (see constructor for more details).
				LifetimePolicy<T>::schedule_for_destruction(s_instance_ptr, &destroy);
			}

			return *s_instance_ptr;
		}

	protected:

		static
		void
		destroy()
		{
			if (s_instance_ptr)
			{
				CreationPolicy<T>::destroy_instance(s_instance_ptr);
				s_instance_ptr = NULL;
				s_singleton_destroyed = true;
			}
		}

		/**
		 * Only the derived singleton class 'T' can instantiate Singleton directly.
		 *
		 * This is also how a derived class can allow creation of a singleton on the C runtime stack
		 * in order to control the lifetime of the singleton (ie, limited to the scope surrounding
		 * the singleton object).
		 * Note that @a instance can still be called to retrieve the singleton object while it is in scope.
		 *
		 * Use the macros GPLATES_SINGLETON_CONSTRUCTOR_DECL, GPLATES_SINGLETON_CONSTRUCTOR_DEF,
		 * GPLATES_SINGLETON_PUBLIC_CONSTRUCTOR_DECL, GPLATES_SINGLETON_PUBLIC_CONSTRUCTOR_DEF
		 * in your derived class to control whether clients can instantiate derived singleton class 'T'
		 * on the C runtime stack or not.
		 */
		Singleton()
		{
			// If this constructor is being called by @a instance then @a s_instance_ptr will be NULL.
			// If this constructor is being called directly by the derived class 'T' constructor
			// then make sure @a instance has not already been called.
			// In the second case clients are allowed to create 'T' on the C runtime stack.
			GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
					!s_instance_ptr && !s_singleton_destroyed,
					GPLATES_EXCEPTION_SOURCE);

			s_instance_ptr = static_cast<T *>(this);
			s_singleton_destroyed = false;
		}

		~Singleton()
		{
			s_instance_ptr = NULL;
			s_singleton_destroyed = true;
		}

	private:

		static T *s_instance_ptr;
		static bool s_singleton_destroyed;

	};
}


/**
 * Adds a default (protected) constructor and friend declaration.
 * You provide the constructor implementation.
 *
 * Clients can *not* instantiate singleton objects 'T' on the C runtime stack.
 *
 * Use this when the 'CreationPolicy' template parameter is the default 'CreateUsingNew<T>'.
 * Otherwise you'll need to explicitly add the constructor and friend declarations.
 */
#define GPLATES_SINGLETON_CONSTRUCTOR_DECL(T) \
	protected: \
		T(); \
		friend class GPlatesUtils::CreateUsingNew<T>;

/**
 * Adds a default (protected) constructor implementation and friend declaration.
 *
 * Clients can *not* instantiate singleton objects 'T' on the C runtime stack.
 *
 * Use this when the 'CreationPolicy' template parameter is the default 'CreateUsingNew<T>'.
 * Otherwise you'll need to explicitly add the constructor and friend declarations.
 */
#define GPLATES_SINGLETON_CONSTRUCTOR_DEF(T) \
	protected: \
		T() { } \
		friend class GPlatesUtils::CreateUsingNew<T>;


/**
 * Adds a default (public) constructor and friend declaration.
 * You provide the constructor implementation.
 *
 * Clients *can* instantiate singleton objects 'T' on the C runtime stack.
 * This is useful as a means to control singleton lifetime to a specific scope.
 *
 * Use this when the 'CreationPolicy' template parameter is the default 'CreateUsingNew<T>'.
 * Otherwise you'll need to explicitly add the constructor and friend declarations.
 */
#define GPLATES_SINGLETON_PUBLIC_CONSTRUCTOR_DECL(T) \
	public: \
		T(); \
		friend class GPlatesUtils::CreateUsingNew<T>;

/**
 * Adds a default (public) constructor implementation and friend declaration.
 *
 * Clients *can* instantiate singleton objects 'T' on the C runtime stack.
 * This is useful as a means to control singleton lifetime to a specific scope.
 *
 * Use this when the 'CreationPolicy' template parameter is the default 'CreateUsingNew<T>'.
 * Otherwise you'll need to explicitly add the constructor and friend declarations.
 */
#define GPLATES_SINGLETON_PUBLIC_CONSTRUCTOR_DEF(T) \
	public: \
		T() { } \
		friend class GPlatesUtils::CreateUsingNew<T>;


//
// Implementation
//

template <
		typename T,
		template <typename> class CreationPolicy,
		template <typename> class LifetimePolicy,
		class InstanceTag >
T *
GPlatesUtils::Singleton<T, CreationPolicy, LifetimePolicy, InstanceTag>::s_instance_ptr = NULL;

template <
		typename T,
		template <typename> class CreationPolicy,
		template <typename> class LifetimePolicy,
		class InstanceTag >
bool
GPlatesUtils::Singleton<T, CreationPolicy, LifetimePolicy, InstanceTag>::s_singleton_destroyed = false;

#endif  // GPLATES_UTILS_SINGLETON_H
