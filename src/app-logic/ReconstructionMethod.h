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
#ifndef GPLATES_APP_LOGIC_RECONSTRUCTIONMETHOD_H
#define GPLATES_APP_LOGIC_RECONSTRUCTIONMETHOD_H

#include <boost/optional.hpp>

#include "property-values/EnumerationContent.h"


namespace GPlatesAppLogic
{
	namespace ReconstructionMethod
	{
		/**
		 * An enumeration of different ways to reconstruct a geometry.
		 */
		enum Type
		{
			BY_PLATE_ID,
			HALF_STAGE_ROTATION,

			NUM_TYPES
		};

		/**
		 * Returns the corresponding string value for the given enumeration.
		 * @a reconstruction_method must be one of BY_PLATE_ID or
		 * HALF_STAGE_ROTATION.
		 */
		GPlatesPropertyValues::EnumerationContent
		get_enum_as_string(
				Type reconstruction_method);

		/**
		 * Returns the corresponding enumeration value for the given string;
		 * returns boost::none if @a string is not recognised.
		 */
		boost::optional<Type>
		get_string_as_enum(
				const GPlatesPropertyValues::EnumerationContent &string);
	}
}
	
#endif  // GPLATES_APP_LOGIC_RECONSTRUCTIONMETHOD_H
