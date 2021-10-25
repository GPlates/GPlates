/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007, 2008, 2009 The University of Sydney, Australia
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

#ifndef GPLATES_MODEL_STRINGSETSINGLETONS_H
#define GPLATES_MODEL_STRINGSETSINGLETONS_H

#include "utils/StringSet.h"
#include "utils/IdStringSet.h"


namespace GPlatesModel
{
	namespace StringSetSingletons
	{
		GPlatesUtils::IdStringSet &
		feature_id_instance();

		GPlatesUtils::StringSet &
		feature_type_instance();

		GPlatesUtils::StringSet &
		property_name_instance();

		GPlatesUtils::StringSet &
		structural_type_instance();

		GPlatesUtils::StringSet &
		text_content_instance();

		GPlatesUtils::StringSet &
		xml_attribute_name_instance();

		GPlatesUtils::StringSet &
		xml_attribute_value_instance();

		GPlatesUtils::StringSet &
		xml_namespace_instance();

		GPlatesUtils::StringSet &
		xml_namespace_alias_instance();

		GPlatesUtils::StringSet &
		xml_element_name_instance();

		GPlatesUtils::StringSet &
		enumeration_content_instance();

		GPlatesUtils::StringSet &
		enumeration_type_instance();

		// Empty structs just so we can get different instances of StringSet returned
		// from different *_instance() functions.
		struct FeatureTypeInstance { };
		struct PropertyNameInstance { };
		struct StructuralTypeInstance { };
		struct TextContentInstance { };
		struct XMLAttributeNameInstance { };
		struct XMLAttributeValueInstance { };
		struct XMLNamespaceInstance { };
		struct XMLNamespaceAliasInstance { };
		struct XMLElementNameInstance { };
		struct EnumerationContentInstance { };
		struct EnumerationTypeInstance { };

	}
}

#endif  // GPLATES_MODEL_STRINGSETSINGLETONS_H
