/* $Id: PropertyName.h 880 2006-10-09 08:16:28Z matty $ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date: 2006-10-09 18:16:28 +1000 (Mon, 09 Oct 2006) $
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

#include "PropertyName.h"


bool
GPlatesModel::PropertyName::is_loaded(
		const UnicodeString &s) {
	return StringSet::instance(StringSetInstantiations::PROPERTY_NAME)->contains(s);
}
