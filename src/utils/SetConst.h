/* $Id$ */


/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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
 
#ifndef GPLATES_UTILS_SETCONST_H
#define GPLATES_UTILS_SETCONST_H

#include <boost/type_traits/add_const.hpp>
#include <boost/type_traits/remove_const.hpp>


namespace GPlatesUtils
{
	/**
	 * SetConst adds top level const-ness to T if Const is true, and removes any
	 * top level const-ness from T if Const is false.
	 */
	template<class T, bool Const>
	struct SetConst
	{
		typedef typename boost::remove_const<T>::type type;
	};

	template<class T>
	struct SetConst<T, true>
	{
		typedef typename boost::add_const<T>::type type;
	};
}

#endif // GPLATES_UTILS_SETCONST_H
