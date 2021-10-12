/* $Id$ */


/**
 * \file 
 * $Revision$
 * $Date$
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
 
#ifndef GPLATES_OPENGL_GLVOLATILEOBJECT_H
#define GPLATES_OPENGL_GLVOLATILEOBJECT_H

#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>


namespace GPlatesOpenGL
{
	/**
	 * An object allocated from an object cache - it is volatile because it can
	 * be recycled, by the object cache, for another client.
	 *
	 * 'ObjectType' should have 'shared_ptr_type' and 'weak_ptr_type' nested typedefs
	 * which behave like boost::shared_ptr and boost::weak_ptr.
	 */
	template <typename ObjectType>
	class GLVolatileObject
	{
	public:
		/**
		 *  Constructor.
		 *
		 * Creates a weak reference to @a object.
		 * The @a volatile_token is used to receive notification that the object cache
		 * has recycled the object.
		 */
		GLVolatileObject(
				const typename ObjectType::shared_ptr_type &object,
				const boost::shared_ptr<void> &volatile_token);

		/**
		 * Default constructor.
		 *
		 * Does not reference a object (@a get_object will return false).
		 */
		GLVolatileObject()
		{  }


		/**
		 * Returns the referenced object if it's still available.
		 *
		 * If the object is not available (false is returned) then it means the
		 * object was recycled by an object cache request and a new volatile object
		 * will need to be allocated from the object cache.
		 *
		 * Returns false if the referenced object is no longer available because either:
		 * 1) it has been recycled by the object cache, or
		 * 2) it has been destroyed.
		 * The most likely reason is the first (recycled by object cache).
		 *
		 * The returned shared object reference (if false was not returned) will
		 * prevent 'this' volatile object from being recycled by the object cache.
		 * So it should be used temporarily and then destroyed to allow it to be recycled.
		 * For example, if it's used for rendering the scene then it should be discarded
		 * once the scene has finished rendering.
		 *
		 * If the returned boost::optional is true then the ObjectType::shared_ptr_type
		 * is guaranteed to be non-null.
		 */
		boost::optional<typename ObjectType::shared_ptr_type>
		get_object();


		/**
		 * Marks this object as invalid so that @a get_object will return false.
		 */
		void
		invalidate();

	private:
		typename ObjectType::weak_ptr_type d_object;
		boost::weak_ptr<void> d_volatile_token;
	};
}


namespace GPlatesOpenGL
{
	////////////////////
	// Implementation //
	////////////////////


	template <typename ObjectType>
	GLVolatileObject<ObjectType>::GLVolatileObject(
			const typename ObjectType::shared_ptr_type &object,
			const boost::shared_ptr<void> &volatile_token) :
		d_object(object),
		d_volatile_token(volatile_token)
	{
	}


	template <typename ObjectType>
	boost::optional<typename ObjectType::shared_ptr_type>
	GLVolatileObject<ObjectType>::get_object()
	{
		// Has our object been recycled by the object cache ?
		if (d_volatile_token.expired())
		{
			return boost::none;
		}

		// Get a shared pointer to our object.
		typename ObjectType::shared_ptr_type object = d_object.lock();

		// Has our object actually been destroyed for some reason ?
		if (!object)
		{
			return boost::none;
		}

		return object;
	}


	template <typename ObjectType>
	void
	GLVolatileObject<ObjectType>::invalidate()
	{
		d_volatile_token.reset();
	}
}

#endif // GPLATES_OPENGL_GLVOLATILEOBJECT_H
