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
 
#ifndef GPLATES_OPENGL_GLARRAY_H
#define GPLATES_OPENGL_GLARRAY_H

#include <cstddef> // For std::size_t
#include <utility>
#include <vector>
#include <boost/iterator/iterator_traits.hpp>
#include <boost/shared_array.hpp>
#include <opengl/OpenGL.h>

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"


namespace GPlatesOpenGL
{
	/**
	 * A utility class to wrap arrays in OpenGL (such as vertex and index arrays) and
	 * retains ownership of the array data (possibly shared ownership) but can
	 * only access the array via a 'void' pointer.
	 */
	class GLArray
	{
	public:
		/**
		 * Constructor.
		 *
		 * The array data is copied into an internal array.
		 *
		 * @a ElementType is the type (or structure) of the data passed in.
		 * For example it could be a vertex structure for a vertex array or
		 * a GL_UNSIGNED_SHORT for a vertex index array.
		 */
		template <class ElementType>
		explicit
		GLArray(
				const std::vector<ElementType> &elements) :
			d_array(new ArrayStorage<ElementType>(elements))
		{  }

		/**
		 * Constructor.
		 *
		 * The array data is copied into an internal array.
		 *
		 * @a ElementForwardIter is an iterator of the type (or structure) of the data passed in.
		 * For example it could be a vertex structure for a vertex array or
		 * a GL_UNSIGNED_SHORT for a vertex index array.
		 */
		template <typename ElementForwardIter>
		GLArray(
				ElementForwardIter begin,
				ElementForwardIter end) :
			d_array(
					new ArrayStorage<typename boost::iterator_value<ElementForwardIter>::type>(
							begin, end))
		{  }

		/**
		 * Constructor.
		 *
		 * The array data is shared internally and hence is less expensive
		 * than the constructor accepting a std::vector.
		 *
		 * @a ElementType is the type (or structure) of the data passed in.
		 * For example it could be a vertex structure for a vertex array or
		 * a GL_UNSIGNED_SHORT for a vertex index array.
		 */
		template <class ElementType>
		explicit
		GLArray(
				const boost::shared_array<ElementType> &elements) :
			d_array(new ArrayStorage<ElementType>(elements))
		{  }


		/**
		 * Returns the opaque void pointer to the internal array.
		 */
		const void *
		get_array() const
		{
			return d_array->opaque_array;
		}

	private:
		//! Keeps a void pointer to the array.
		struct Array :
				public GPlatesUtils::ReferenceCount<Array>
		{
			typedef GPlatesUtils::non_null_intrusive_ptr<Array> non_null_ptr_type;
			typedef GPlatesUtils::non_null_intrusive_ptr<const Array> non_null_ptr_to_const_type;

			virtual
			~Array()
			{  }

			void *opaque_array;
		};

		//! Stores the actual elements.
		template <class ElementType>
		struct ArrayStorage :
				public Array
		{
			explicit
			ArrayStorage(
					const std::vector<ElementType> &elements)
			{
				initialise_storage(elements.begin(), elements.end());

				// Store void pointer in base class.
				opaque_array = array_storage.get();
			}

			template <typename ElementForwardIter>
			ArrayStorage(
					ElementForwardIter begin,
					ElementForwardIter end)
			{
				initialise_storage(begin, end);

				// Store void pointer in base class.
				opaque_array = array_storage.get();
			}

			explicit
			ArrayStorage(
					const boost::shared_array<ElementType> &elements) :
				array_storage(elements)
			{
				// Store void pointer in base class.
				opaque_array = array_storage.get();
			}


		private:
			boost::shared_array<ElementType> array_storage;


			template <typename ElementForwardIter>
			void
			initialise_storage(
					ElementForwardIter begin,
					ElementForwardIter end)
			{
				// Allocate storage.
				const std::size_t num_elements = std::distance(begin, end);
				array_storage.reset(new ElementType[num_elements]);

				// Copy the elements into our internal array.
				ElementType *const dst = array_storage.get();
				ElementForwardIter src = begin;
				for (std::size_t n = 0; n < num_elements; ++n, ++src)
				{
					dst[n] = *src;
				}
			}
		};


		/**
		 * The array data.
		 */
		Array::non_null_ptr_type d_array;
	};
}

#endif // GPLATES_OPENGL_GLARRAY_H
