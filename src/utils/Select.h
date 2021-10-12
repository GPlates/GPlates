/* $Id$ */

/**
 * @file 
 * Contains the definition of the Select template class.
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

#ifndef GPLATES_UTILS_SELECT_H
#define GPLATES_UTILS_SELECT_H

namespace GPlatesUtils
{
	/**
	 * Select allows for compile-time type selection based on the value of a
	 * compile-time boolean expression.
	 *
	 * Select is based on the class of the same name in Alexandrescu, A.
	 * "Modern C++ Design".
	 */
	template<bool Condition, typename TrueType, typename FalseType>
	struct Select
	{
		typedef TrueType result;
	};

	template<typename TrueType, typename FalseType>
	struct Select<false, TrueType, FalseType>
	{
		typedef FalseType result;
	};
}

#endif  // GPLATES_UTILS_SELECT_H
