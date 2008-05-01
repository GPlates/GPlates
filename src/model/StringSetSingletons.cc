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

#include "StringSetSingletons.h"
#include "utils/StringSet.h"


GPlatesUtils::StringSet &
GPlatesModel::StringSetSingletons::feature_type_instance() {
	if (s_feature_type_instance == NULL) {
		s_feature_type_instance = new GPlatesUtils::StringSet();
	}
	return *s_feature_type_instance;
}

GPlatesUtils::StringSet &
GPlatesModel::StringSetSingletons::property_name_instance() {
	if (s_property_name_instance == NULL) {
		s_property_name_instance = new GPlatesUtils::StringSet();
	}
	return *s_property_name_instance;
}

GPlatesUtils::StringSet &
GPlatesModel::StringSetSingletons::template_type_parameter_type_instance() {
	if (s_template_type_parameter_type_instance == NULL) {
		s_template_type_parameter_type_instance = new GPlatesUtils::StringSet();
	}
	return *s_template_type_parameter_type_instance;
}

GPlatesUtils::StringSet &
GPlatesModel::StringSetSingletons::text_content_instance() {
	if (s_text_content_instance == NULL) {
		s_text_content_instance = new GPlatesUtils::StringSet();
	}
	return *s_text_content_instance;
}

GPlatesUtils::StringSet &
GPlatesModel::StringSetSingletons::xml_attribute_name_instance() {
	if (s_xml_attribute_name_instance == NULL) {
		s_xml_attribute_name_instance = new GPlatesUtils::StringSet();
	}
	return *s_xml_attribute_name_instance;
}

GPlatesUtils::StringSet &
GPlatesModel::StringSetSingletons::xml_attribute_value_instance() {
	if (s_xml_attribute_value_instance == NULL) {
		s_xml_attribute_value_instance = new GPlatesUtils::StringSet();
	}
	return *s_xml_attribute_value_instance;
}

GPlatesUtils::StringSet &
GPlatesModel::StringSetSingletons::xml_namespace_instance() {
	if (s_xml_namespace_instance == NULL) {
		s_xml_namespace_instance = new GPlatesUtils::StringSet();
	}
	return *s_xml_namespace_instance;
}

GPlatesUtils::StringSet &
GPlatesModel::StringSetSingletons::xml_namespace_alias_instance() {
	if (s_xml_namespace_alias_instance == NULL) {
		s_xml_namespace_alias_instance = new GPlatesUtils::StringSet();
	}
	return *s_xml_namespace_alias_instance;
}

GPlatesUtils::StringSet &
GPlatesModel::StringSetSingletons::xml_element_name_instance() {
	if (s_xml_element_name_instance == NULL) {
		s_xml_element_name_instance = new GPlatesUtils::StringSet();
	}
	return *s_xml_element_name_instance;
}

GPlatesUtils::StringSet &
GPlatesModel::StringSetSingletons::enumeration_content_instance() {
	if (s_enumeration_content_instance == NULL) {
		s_enumeration_content_instance = new GPlatesUtils::StringSet();
	}
	return *s_enumeration_content_instance;
}

GPlatesUtils::StringSet &
GPlatesModel::StringSetSingletons::enumeration_type_instance() {
	if (s_enumeration_type_instance == NULL) {
		s_enumeration_type_instance = new GPlatesUtils::StringSet();
	}
	return *s_enumeration_type_instance;
}

GPlatesUtils::StringSet *GPlatesModel::StringSetSingletons::s_feature_type_instance = NULL;
GPlatesUtils::StringSet *GPlatesModel::StringSetSingletons::s_property_name_instance = NULL;
GPlatesUtils::StringSet *GPlatesModel::StringSetSingletons::s_template_type_parameter_type_instance = NULL;
GPlatesUtils::StringSet *GPlatesModel::StringSetSingletons::s_text_content_instance = NULL;
GPlatesUtils::StringSet *GPlatesModel::StringSetSingletons::s_xml_attribute_name_instance = NULL;
GPlatesUtils::StringSet *GPlatesModel::StringSetSingletons::s_xml_attribute_value_instance = NULL;
GPlatesUtils::StringSet *GPlatesModel::StringSetSingletons::s_xml_namespace_instance = NULL;
GPlatesUtils::StringSet *GPlatesModel::StringSetSingletons::s_xml_namespace_alias_instance = NULL;
GPlatesUtils::StringSet *GPlatesModel::StringSetSingletons::s_xml_element_name_instance = NULL;
GPlatesUtils::StringSet *GPlatesModel::StringSetSingletons::s_enumeration_content_instance = NULL;
GPlatesUtils::StringSet *GPlatesModel::StringSetSingletons::s_enumeration_type_instance = NULL;

