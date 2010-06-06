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
 
#ifndef GPLATES_UTILS_COPYCONST_H
#define GPLATES_UTILS_COPYCONST_H

#include <boost/type_traits/add_const.hpp>
#include <boost/type_traits/remove_const.hpp>


namespace GPlatesUtils
{
	/**
	 * CopyConst takes the const-ness of SrcType and applies it to DstType.
	 */
	template<class SrcType, class DstType>
	struct CopyConst
	{
		typedef typename boost::remove_const<DstType>::type type;
	};

	template<class SrcType, class DstType>
	struct CopyConst<const SrcType, DstType>
	{
		typedef typename boost::add_const<DstType>::type type;
	};
}

#endif // GPLATES_UTILS_COPYCONST_H
