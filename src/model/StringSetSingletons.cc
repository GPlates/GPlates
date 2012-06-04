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

#include "StringSetSingletons.h"
#include "utils/Singleton.h"

GPlatesUtils::IdStringSet &
GPlatesModel::StringSetSingletons::feature_id_instance()
{
	return GPlatesUtils::Singleton<GPlatesUtils::IdStringSet>::instance();
}

GPlatesUtils::StringSet &
GPlatesModel::StringSetSingletons::feature_type_instance()
{
	return GPlatesUtils::Singleton<
		GPlatesUtils::StringSet,
		GPlatesUtils::CreateUsingNew,
		GPlatesUtils::DefaultLifetime,
		FeatureTypeInstance>::instance();
}

GPlatesUtils::StringSet &
GPlatesModel::StringSetSingletons::property_name_instance()
{
	return GPlatesUtils::Singleton<
		GPlatesUtils::StringSet,
		GPlatesUtils::CreateUsingNew,
		GPlatesUtils::DefaultLifetime,
		PropertyNameInstance>::instance();
}

GPlatesUtils::StringSet &
GPlatesModel::StringSetSingletons::template_type_parameter_type_instance()
{
	return GPlatesUtils::Singleton<
		GPlatesUtils::StringSet,
		GPlatesUtils::CreateUsingNew,
		GPlatesUtils::DefaultLifetime,
		TemplateTypeParameterTypeInstance>::instance();
}

GPlatesUtils::StringSet &
GPlatesModel::StringSetSingletons::text_content_instance()
{
	return GPlatesUtils::Singleton<
		GPlatesUtils::StringSet,
		GPlatesUtils::CreateUsingNew,
		GPlatesUtils::DefaultLifetime,
		TextContentInstance>::instance();
}

GPlatesUtils::StringSet &
GPlatesModel::StringSetSingletons::xml_attribute_name_instance()
{
	return GPlatesUtils::Singleton<
		GPlatesUtils::StringSet,
		GPlatesUtils::CreateUsingNew,
		GPlatesUtils::DefaultLifetime,
		XMLAttributeNameInstance>::instance();
}

GPlatesUtils::StringSet &
GPlatesModel::StringSetSingletons::xml_attribute_value_instance()
{
	return GPlatesUtils::Singleton<
		GPlatesUtils::StringSet,
		GPlatesUtils::CreateUsingNew,
		GPlatesUtils::DefaultLifetime,
		XMLAttributeValueInstance>::instance();
}

GPlatesUtils::StringSet &
GPlatesModel::StringSetSingletons::xml_namespace_instance()
{
	return GPlatesUtils::Singleton<
		GPlatesUtils::StringSet,
		GPlatesUtils::CreateUsingNew,
		GPlatesUtils::DefaultLifetime,
		XMLNamespaceInstance>::instance();
}

GPlatesUtils::StringSet &
GPlatesModel::StringSetSingletons::xml_namespace_alias_instance()
{
	return GPlatesUtils::Singleton<
		GPlatesUtils::StringSet,
		GPlatesUtils::CreateUsingNew,
		GPlatesUtils::DefaultLifetime,
		XMLNamespaceAliasInstance>::instance();
}

GPlatesUtils::StringSet &
GPlatesModel::StringSetSingletons::xml_element_name_instance()
{
	return GPlatesUtils::Singleton<
		GPlatesUtils::StringSet,
		GPlatesUtils::CreateUsingNew,
		GPlatesUtils::DefaultLifetime,
		XMLElementNameInstance>::instance();
}

GPlatesUtils::StringSet &
GPlatesModel::StringSetSingletons::enumeration_content_instance()
{
	return GPlatesUtils::Singleton<
		GPlatesUtils::StringSet,
		GPlatesUtils::CreateUsingNew,
		GPlatesUtils::DefaultLifetime,
		EnumerationContentInstance>::instance();
}

GPlatesUtils::StringSet &
GPlatesModel::StringSetSingletons::enumeration_type_instance()
{
	return GPlatesUtils::Singleton<
		GPlatesUtils::StringSet,
		GPlatesUtils::CreateUsingNew,
		GPlatesUtils::DefaultLifetime,
		EnumerationTypeInstance>::instance();
}

