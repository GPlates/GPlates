/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2007, 2008 The University of Sydney, Australia
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

#ifndef GPLATES_PROPERTYVALUES_ENUMERATIONCONTENT_H
#define GPLATES_PROPERTYVALUES_ENUMERATIONCONTENT_H

#include "model/StringSetSingletons.h"
#include "model/StringContentTypeGenerator.h"

namespace GPlatesPropertyValues {

	class EnumerationContentFactory {

	public:
		static
		GPlatesUtils::StringSet &
		instance()
		{
			return GPlatesModel::StringSetSingletons::enumeration_content_instance();
		}

	private:
		EnumerationContentFactory();
	};

	typedef GPlatesModel::StringContentTypeGenerator<EnumerationContentFactory> EnumerationContent;
}

#endif  // GPLATES_PROPERTYVALUES_ENUMERATIONCONTENT_H
