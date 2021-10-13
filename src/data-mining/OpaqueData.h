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

#ifndef GPLATESDATAMINING_OPAQUEDATA_H
#define GPLATESDATAMINING_OPAQUEDATA_H

#include <QString>

#include <boost/variant.hpp>

namespace GPlatesDataMining
{
	/*
	* Use a pointer of dummy class data member to define empty_data_type,
	* which will be used when there is no valid data in OpaqueData type
	*/
	struct dummy{ };
    typedef int dummy::*empty_data_type;
    empty_data_type const EmptyData = static_cast<empty_data_type>(0);

	/*
	* The definition of opaque data type. Opaque data is an old name.
	* http://en.wikipedia.org/wiki/Opaque_data_type
	* Basically, we use opaque data here to unify interface and data type.
	*/
	typedef boost::variant
		<
			empty_data_type,
			bool ,
			int ,
			unsigned ,
			char ,
			float ,
			double ,
			QString
		 > OpaqueData;
}

#endif















