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

#include "StringSetSingletons.h"
#include "util/StringSet.h"


GPlatesUtil::StringSet &
GPlatesModel::StringSetSingletons::feature_type_instance() {
	if (s_feature_type_instance == NULL) {
		s_feature_type_instance = new GPlatesUtil::StringSet();
	}
	return *s_feature_type_instance;
}

GPlatesUtil::StringSet &
GPlatesModel::StringSetSingletons::property_name_instance() {
	if (s_property_name_instance == NULL) {
		s_property_name_instance = new GPlatesUtil::StringSet();
	}
	return *s_property_name_instance;
}

GPlatesUtil::StringSet *GPlatesModel::StringSetSingletons::s_feature_type_instance = NULL;
GPlatesUtil::StringSet *GPlatesModel::StringSetSingletons::s_property_name_instance = NULL;
