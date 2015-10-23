/* $Id$ */

/**
 * \file 
 * StringSetSingleton typedef for names of timescales, as used by the GpmlAge property value.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2015 The University of Sydney, Australia
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

#ifndef GPLATES_PROPERTYVALUES_TIMESCALENAME_H
#define GPLATES_PROPERTYVALUES_TIMESCALENAME_H

#include "model/StringSetSingletons.h"
#include "model/StringContentTypeGenerator.h"

namespace GPlatesPropertyValues {

	class TimescaleNameFactory {

	public:
		static
		GPlatesUtils::StringSet &
		instance()
		{
			return GPlatesModel::StringSetSingletons::timescale_name_instance();
		}

	private:
		TimescaleNameFactory();

	};

	/**
	 * StringSetSingleton typedef for common names of timescales e.g. ICC2012, GTS2004.
	 * These will crop up frequently in the gpml:Age property value.
	 */
	typedef GPlatesModel::StringContentTypeGenerator<TimescaleNameFactory> TimescaleName;

}

#endif  // GPLATES_PROPERTYVALUES_TIMESCALENAME_H
