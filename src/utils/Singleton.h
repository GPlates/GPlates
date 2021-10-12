/* $Id$ */

/**
 * \file
 * Defines the Singleton class.
 *
 * Most recent change:
 *   $Date$
 *
 * Copyright (C) 2007 The University of Sydney, Australia
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
#include <QMutexLocker>

namespace GPlatesUtils
{
	/**
	 * Base class for types that are singletons. Subclasses will then have
	 * a static instance() function that will return a reference to the
	 * singleton object; this object cannot be copied.
	 *
	 * If it should not be possible to independently create objects of
	 * subclasses, insert the GPLATES_SINGLETON_CONSTRUCTOR_DEF(ClassName)
	 * macro at the top of the subclass definition. This defines (but does
	 * not implement) the constructor as protected, and adds makes
	 * Singleton<T> a friend class so that the instance() function can
	 * construct such the singleton. Alternatively, use
	 * GPLATES_SINGLETON_CONSTRUCTOR_IMPL(ClassName) to define and
	 * implement the constructor as protected, and add the friend statement.
	 *
	 * It is also possible to obtain a singleton of an existing class T by calling
	 *
	 *     Singleton<T>::instance()
	 *
	 * Note that this implementation of a singleton is not thread-safe. Use
	 * ThreadsafeSingleton where this is required.
	 */
	template<class T>
	class Singleton : public boost::noncopyable
	{
		public:
			static
			T&
			instance()
			{
				static T _instance;
				return _instance;
			}
	};

	/**
	 * Base class for types that are singletons. Subclasses will then have
	 * a static instance() function that will return a reference to the
	 * singleton object; this object cannot be copied.
	 *
	 * If it should not be possible to independently create objects of
	 * subclasses, insert the GPLATES_SINGLETON_CONSTRUCTOR_DEF(ClassName)
	 * macro at the top of the subclass definition. This defines (but does
	 * not implement) the constructor as protected, and adds makes
	 * Singleton<T> a friend class so that the instance() function can
	 * construct such the singleton. Alternatively, use
	 * GPLATES_SINGLETON_CONSTRUCTOR_IMPL(ClassName) to define and
	 * implement the constructor as protected, and add the friend statement.
	 *
	 * It is also possible to obtain a singleton of an existing class T by calling
	 *
	 *     ThreadsafeSingleton<T>::instance()
	 *
	 * Note that this implementation of a singleton IS thread-safe (i.e.
	 * it is safe for there to be simultaneous invocations of instance()).
	 */
	template<class T>
	class ThreadsafeSingleton : public boost::noncopyable
	{
		public:
			static
			T&
			instance()
			{
				QMutexLocker mutex_locker;
				static T _instance;
				return _instance;
			}
	};
}

#define GPLATES_SINGLETON_CONSTRUCTOR_DEF(T) \
	protected: \
		T(); \
		friend class GPlatesUtils::Singleton<T>; \
		friend class GPlatesUtils::ThreadsafeSingleton<T>;

#define GPLATES_SINGLETON_CONSTRUCTOR_IMPL(T) \
	protected: \
		T() { } \
		friend class GPlatesUtils::Singleton<T>; \
		friend class GPlatesUtils::ThreadsafeSingleton<T>;

#endif  // GPLATES_UTILS_SINGLETON_H
