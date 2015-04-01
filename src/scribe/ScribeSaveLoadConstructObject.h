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

#ifndef GPLATES_SCRIBE_SCRIBESAVELOADCONSTRUCTOBJECT_H
#define GPLATES_SCRIBE_SCRIBESAVELOADCONSTRUCTOBJECT_H

#include <new>
#include <boost/type_traits/aligned_storage.hpp>
#include <boost/type_traits/alignment_of.hpp>
#include <boost/type_traits/remove_const.hpp>

#include "ScribeConstructObject.h"
#include "ScribeExceptions.h"


namespace GPlatesScribe
{
	/**
	 * Used when saving a @a ConstructObject to an archive.
	 *
	 * Objects being saved are, obviously, already constructed so this class is designed to mirror
	 * the loading of @a ConstructObject in order to keep the load and save paths synchronised.
	 */
	template <typename ObjectType>
	class SaveConstructObject :
			public ConstructObject<ObjectType>
	{
	public:

		SaveConstructObject(
				const ObjectType &object) :
			ConstructObject<ObjectType>(const_cast<ObjectType *>(&object), true/*is_object_initialised*/)
		{  }
	};


	/**
	 * Used when loading a @a ConstructObject from an archive onto the C runtime stack.
	 *
	 * An instance of @a LoadConstructObjectOnStack should be a local variable (on the stack)
	 * and, initially, it contains an internal un-initialised object of type 'ObjectType'.
	 */
	template <typename ObjectType>
	class LoadConstructObjectOnStack :
			public ConstructObject<ObjectType>
	{
	public:

		LoadConstructObjectOnStack() :
			ConstructObject<ObjectType>(address(), false/*is_object_initialised*/)
		{  }

		/**
		 * Destructs the internal object of type ObjectType if it has been constructed.
		 */
		~LoadConstructObjectOnStack()
		{
			if (this->is_object_initialised())
			{
				// Call destructor on internal object.
				address()->~ObjectType();
			}
		}

	private:

		//! Typedef for uninitialised storage for the object of type 'ObjectType'.
		typedef boost::aligned_storage<
				sizeof(ObjectType),
				boost::alignment_of<ObjectType>::value>
						uninitialised_object_storage_type;

		uninitialised_object_storage_type d_uninitialised_object_storage;


		/**
		 * Returns address of internal object.
		 */
		ObjectType *
		address()
		{
			return static_cast<ObjectType *>(d_uninitialised_object_storage.address());
		}
	};


	/**
	 * Used when loading a @a ConstructObject from an archive onto the memory heap.
	 *
	 * Initially an instance of @a LoadConstructObjectOnHeap contains an internal un-initialised
	 * object of type 'ObjectType'.
	 */
	template <typename ObjectType>
	class LoadConstructObjectOnHeap :
			public ConstructObject<ObjectType>
	{
	public:

		LoadConstructObjectOnHeap() :
			ConstructObject<ObjectType>(allocate_object(), false/*is_object_initialised*/),
			d_released(false)
		{  }

		/**
		 * Releases allocated memory of the internal object if it was never constructed.
		 */
		~LoadConstructObjectOnHeap()
		{
			if (this->is_object_initialised())
			{
				// If the client has not released the internal object then delete it (deallocates and destructs).
				if (!d_released)
				{
					delete this->get_object_address();
				}
			}
			else
			{
				// Free un-initialised internal object's memory.
				// Note that this is different than calling 'delete object' which also calls destructor.
				deallocate_object(this->get_object_address());
			}
		}

		/**
		 * Release ownership of the internal object (must be initialised).
		 *
		 * NOTE: The caller is responsible for 'delete'ing the returned object.
		 * Failure to do so will result in a memory leak.
		 */
		ObjectType *
		release()
		{
			GPlatesGlobal::Assert<Exceptions::ScribeLibraryError>(
					this->is_object_initialised(),
					GPLATES_ASSERTION_SOURCE,
					"Attempted to release uninitialised object.");

			d_released = true;

			return this->get_object_address();
		}

	private:

		bool d_released;


		/**
		 * Allocates space for the internal object on the heap.
		 */
		ObjectType *
		allocate_object()
		{
			void *object_memory = operator new(sizeof(ObjectType));
			if (object_memory == NULL)
			{
				throw std::bad_alloc();
			}

			return static_cast<ObjectType *>(object_memory);
		}

		/**
		 * Deallocates the internal object from the heap.
		 */
		void
		deallocate_object(
				ObjectType *object_ptr)
		{
			// Note that this is different than calling 'delete object' which also calls destructor.
			operator delete(
					// Const-cast because operator delete expects a non-const pointer...
					const_cast<typename boost::remove_const<ObjectType>::type *>(object_ptr));
		}
	};
}

#endif // GPLATES_SCRIBE_SCRIBESAVELOADCONSTRUCTOBJECT_H
