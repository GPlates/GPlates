/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006 The GPlates Consortium
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef GPLATES_MODEL_STRINGSETSINGLETONS_H
#define GPLATES_MODEL_STRINGSETSINGLETONS_H

#include <map>
#include "util/StringSet.h"

namespace GPlatesModel {

	class StringSetSingletons {
	public:
		static
		GPlatesUtil::StringSet &
		feature_type_instance();

		static
		GPlatesUtil::StringSet &
		property_name_instance();
	private:
		// This constructor should never be defined, because we don't want to allow
		// instantiation of this class.
		StringSetSingletons();

		static GPlatesUtil::StringSet *s_feature_type_instance;
		static GPlatesUtil::StringSet *s_property_name_instance;
	};

}

#endif  // GPLATES_MODEL_STRINGSETSINGLETONS_H
