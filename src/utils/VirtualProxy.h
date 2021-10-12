/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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

#ifndef GPLATES_UTILS_VIRTUALPROXY_H
#define GPLATES_UTILS_VIRTUALPROXY_H

#include <boost/scoped_ptr.hpp>

// class VirtualProxy delays the creation of its template type
// until it's used. Basically VirtualProxy behaves like a pointer
// that creates the pointee object when it's first dereferenced.
// A second template parameter is the factory used to create
// the pointee object.
namespace GPlatesUtils
{
	// Default factory uses default constructor of pointee.
	template <class Type>
	class DefaultFactory
	{
		public:
			Type* create() const { return new Type(); }
	};

	// VirtualProxy template class (non-copyable due to boost::scoped_ptr).
	// 'Type' is pointee object that's created when VirtualProxy is first dereferenced.
	// 'Factory' is used to create pointee 'Type' and must have a method create()
	// that returns a pointer to 'Type' allocated using operator new.
	// 'Factory' must be copy-constructable.
	template < class Type,  class Factory = DefaultFactory<Type> >
	class VirtualProxy
	{
	public:
		VirtualProxy(const Factory& factory = Factory())
			: d_factory(factory)
		{
		}

		// Indirection operator (first call will create instance of Type).
		Type& operator*() const
		{
			if (!d_type_ptr)
			{
				d_type_ptr.reset( d_factory.create() );
			}

			return *d_type_ptr;
		}

		// Indirection operator (first call will create instance of Type).
		Type* operator->() const { return &operator*(); }

	private:
		mutable boost::scoped_ptr<Type> d_type_ptr;
		Factory d_factory;
	};
}
#endif // GPLATES_UTILS_VIRTUALPROXY_H
