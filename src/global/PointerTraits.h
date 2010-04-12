/* $Id$ */

/**
 * \file 
 * Contains the definition of the PointerTraits template class.
 *
 * Most recent change:
 *   $Date$
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

#ifndef GPLATES_GLOBAL_POINTERTRAITS_H
#define GPLATES_GLOBAL_POINTERTRAITS_H

#include "utils/non_null_intrusive_ptr.h"

namespace GPlatesGlobal 
{
	namespace PointerTraitsInternal
	{
		template<class T>
		struct PointerTraitsBase
		{
			typedef GPlatesUtils::non_null_intrusive_ptr<T> non_null_ptr_type;
		};
	}

	/**
	 * PointerTraits provides type information about smart pointers to GPlates
	 * objects. The use of this traits class helps to reduce the number of header
	 * dependencies required.
	 *
	 * For example, inside the Foo class, there may be the following public
	 * typedef:
	 *
	 *		typedef GPlatesUtils::non_null_intrusive_ptr<Foo> non_null_ptr_type;
	 *
	 * Suppose the Bar class has an instance variable of type
	 * Foo::non_null_ptr_type. Even if the Bar header file does not contain code
	 * that calls members of the Foo class, it is necessary to include the Foo
	 * header file, because otherwise Foo::non_null_ptr_type cannot be resolved.
	 * This poses a problem if including the Foo header file is impossible due to
	 * cyclic dependencies (in particular, when dealing with templated classes).
	 *
	 * Instead, the Bar class should declare the instance variable to be of type
	 * GPlatesGlobal::PointerTraits<Foo>::non_null_ptr_type. In this case, it
	 * suffices for the Bar header file to have a forward declaration of Foo and
	 * to include this PointerTraits header file.
	 *
	 * If it is desirable for there to be other smart pointers to Foo objects, it
	 * will be necessary to write a specialisation of PointerTraits for Foo.
	 * If appropriate, the specialisation for Foo should also publicly inherit
	 * from PointerTraitsInternal::PointerTraitsBase<Foo>. For example:
	 * 
	 *		template<>
	 *		struct PointerTraits<Foo> :
	 *			public PointerTraitsInternal::PointerTraitsBase<Foo>
	 *		{
	 *			typedef boost::intrusive_ptr<Foo> maybe_null_ptr_type;
	 *			// ... other typedefs here ...
	 *		};
	 *
	 */
	template<class T>
	struct PointerTraits :
		public PointerTraitsInternal::PointerTraitsBase<T>
	{
		// non_null_ptr_type defined in base class.
		// non_null_ptr_to_const_type is not defined. Use PointerTraits<const T>::non_null_ptr_type.
	};
}

#endif // GPLATES_GLOBAL_POINTERTRAITS_H
