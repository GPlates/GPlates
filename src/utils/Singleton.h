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

#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
#ifdef GPLATES_SINGLETON_THREADSAFE
#include <QMutexLocker>
#endif

namespace GPlatesUtils
{
	template<typename T>
	class DefaultSingletonFactory
	{
	public:
		T *
		create_instance()
		{
			return new T();
		}
	};

	struct DefaultInstance;

	/**
	 * Base class for types that are singletons. For an explanation of singletons,
	 * see the Design Patterns book.
	 * 
	 * A class T that derives from Singleton<T> will have a static instance()
	 * function that returns a reference to the one and only instance of T. The
	 * returned object cannot be copied.
	 *
	 * If T does not have a default constructor, it is necessary to supply the
	 * type of a factory class as the SingletonFactory template parameter.
	 *
	 * If it is desirable to have more than one instance of T, supply the Instance
	 * template parameter. The class named as the Instance parameter is not
	 * instantiated or otherwise used; the parameter is merely provided to select,
	 * at compile time, which of the many instances of T instance() is to return.
	 *
	 * If it should not be possible for client code to create instances of T,
	 * insert the GPLATES_SINGLETON_CONSTRUCTOR_DECL(T) macro at the top of the
	 * definition of T. This declares (but does not define) the constructor as
	 * protected, and makes DefaultSingletonFactory<T> a friend class so that it
	 * can construct an instance of T. The constructor of T will need to be
	 * defined elsewhere.
	 *
	 * Alternatively, use GPLATES_SINGLETON_CONSTRUCTOR_DEF(T) to declare the
	 * constructor as well as define it with an empty body. (Note the
	 * distinction between a 'declaration' and a 'definition' in C++.)
	 *
	 * It is also possible to obtain a singleton of an existing class T by calling
	 *
	 *     Singleton<T>::instance()
	 *
	 * Note that this implementation of a singleton is not thread-safe. Define
	 * GPLATES_SINGLETON_THREADSAFE before including this header if this is required.
	 */
	template<typename T, class SingletonFactory = DefaultSingletonFactory<T>, class Instance = DefaultInstance>
	class Singleton :
			public boost::noncopyable
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
				SingletonFactory factory;
				s_instance_ptr.reset(factory.create_instance());
			}
			return *s_instance_ptr;
		}

	protected:

		// It should not be possible to create an instance of Singleton directly.
		Singleton()
		{
		}
	
	private:

		static boost::scoped_ptr<T> s_instance_ptr;

	};
}

template<class T, class SingletonFactory, class Instance>
boost::scoped_ptr<T>
GPlatesUtils::Singleton<T, SingletonFactory, Instance>::s_instance_ptr(NULL);

#define GPLATES_SINGLETON_CONSTRUCTOR_DECL(T) \
	protected: \
		T(); \
		friend class GPlatesUtils::DefaultSingletonFactory<T>;

#define GPLATES_SINGLETON_CONSTRUCTOR_DEF(T) \
	protected: \
		T() { } \
		friend class GPlatesUtils::DefaultSingletonFactory<T>;

#endif  // GPLATES_UTILS_SINGLETON_H
