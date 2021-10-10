/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007, 2008 The University of Sydney, Australia
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


namespace GPlatesModel {

	class StringSetSingletons {

	public:

		static
		GPlatesUtils::StringSet &
		feature_type_instance();

		static
		GPlatesUtils::StringSet &
		property_name_instance();

		static
		GPlatesUtils::StringSet &
		template_type_parameter_type_instance();

		static
		GPlatesUtils::StringSet &
		text_content_instance();

		static
		GPlatesUtils::StringSet &
		xml_attribute_name_instance();

		static
		GPlatesUtils::StringSet &
		xml_attribute_value_instance();

		static
		GPlatesUtils::StringSet &
		xml_namespace_instance();

		static
		GPlatesUtils::StringSet &
		xml_namespace_alias_instance();

		static
		GPlatesUtils::StringSet &
		xml_element_name_instance();

		static
		GPlatesUtils::StringSet &
		enumeration_content_instance();

		static
		GPlatesUtils::StringSet &
		enumeration_type_instance();

	private:

		// This constructor should never be defined, because we don't want to allow
		// instantiation of this class.
		StringSetSingletons();

		static GPlatesUtils::StringSet *s_feature_type_instance;
		static GPlatesUtils::StringSet *s_property_name_instance;
		static GPlatesUtils::StringSet *s_template_type_parameter_type_instance;
		static GPlatesUtils::StringSet *s_text_content_instance;
		static GPlatesUtils::StringSet *s_xml_attribute_name_instance;
		static GPlatesUtils::StringSet *s_xml_attribute_value_instance;
		static GPlatesUtils::StringSet *s_xml_namespace_instance;
		static GPlatesUtils::StringSet *s_xml_namespace_alias_instance;
		static GPlatesUtils::StringSet *s_xml_element_name_instance;
		static GPlatesUtils::StringSet *s_enumeration_content_instance;
		static GPlatesUtils::StringSet *s_enumeration_type_instance;
	};

}

#endif  // GPLATES_MODEL_STRINGSETSINGLETONS_H
