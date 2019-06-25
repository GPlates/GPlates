/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2019 The University of Sydney, Australia
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

#ifndef GPLATES_API_PYQUALIFIEDXMLNAMES_H
#define GPLATES_API_PYQUALIFIEDXMLNAMES_H

#include "global/python.h"

#include "property-values/StructuralType.h"


#if !defined(GPLATES_NO_PYTHON)

namespace GPlatesApi
{
	/**
	 * Register a property value type as a structural type.
	 *
	 * In Python these are represented as the class object associated with the C++ property type, and
	 * in C++ these are represented as 'GPlatesPropertyValues::StructuralType'.
	 *
	 * For example, for Python <-> C++ we can have pygplates.GmlLineString <-> "gml:LineString".
	 *
	 * So a C++ function that has a GPlatesPropertyValues::StructuralType argument type will accept
	 * pygplates.GmlLineString, etc. And a C++ function returning a GPlatesPropertyValues::StructuralType
	 * will return a pygplates.GmlLineString, etc, to Python.
	 *
	 * To register this conversion, call 'GPlatesApi::register_structural_type<GPlatesPropertyValues::GmlLineString>()'.
	 * Note that the type must already have been exported and it must have a static 'STRUCTURAL_TYPE' data member
	 * of type 'GPlatesPropertyValues::StructuralType'.
	 */
	template <class Type>
	void
	register_structural_type();


	//
	// Implementation.
	//

	namespace Implementation
	{
		void
		register_structural_type(
			const boost::python::type_info &type,
			const GPlatesPropertyValues::StructuralType &structural_type);
	}

	template <class Type>
	void
	register_structural_type()
	{
		Implementation::register_structural_type(
			boost::python::type_id<Type>(),
			Type::STRUCTURAL_TYPE);
	}
}

#endif   // GPLATES_NO_PYTHON

#endif // GPLATES_API_PYQUALIFIEDXMLNAMES_H
