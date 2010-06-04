/* $Id$ */

/**
 * \file
 *
 * Most recent change:
 *   $Date$
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

#include "XmlNamespaces.h"

#include "model/StringSetSingletons.h"


const char *
GPlatesUtils::XmlNamespaces::Internals::gpml_namespace()
{
	return "http://www.gplates.org/gplates";
}


const char *
GPlatesUtils::XmlNamespaces::Internals::gml_namespace()
{
	return "http://www.opengis.net/gml";
}


const char *
GPlatesUtils::XmlNamespaces::Internals::xsi_namespace()
{
	return "http://www.w3.org/XMLSchema-instance";
}


const char *
GPlatesUtils::XmlNamespaces::Internals::gpml_standard_alias()
{
	return "gpml";
}


const char *
GPlatesUtils::XmlNamespaces::Internals::gml_standard_alias()
{
	return "gml";
}


const char *
GPlatesUtils::XmlNamespaces::Internals::xsi_standard_alias()
{
	return "xsi";
}


GPlatesUtils::StringSet::SharedIterator
GPlatesUtils::XmlNamespaces::get_standard_alias_for_namespace(
		const UnicodeString &namespace_uri)
{
	GPlatesUtils::StringSet& aliases = 
		GPlatesModel::StringSetSingletons::xml_namespace_alias_instance();
	if (namespace_uri == GML_NAMESPACE)
	{
		return aliases.insert(GML_STANDARD_ALIAS);
	}
	else if (namespace_uri == XSI_NAMESPACE)
	{
		return aliases.insert(XSI_STANDARD_ALIAS);
	}
	else
	{
		// Return gpml as default if URI not recognised.
		return aliases.insert(GPML_STANDARD_ALIAS);
	}
}


GPlatesUtils::StringSet::SharedIterator
GPlatesUtils::XmlNamespaces::get_namespace_for_standard_alias(
		const UnicodeString &namespace_alias)
{
	GPlatesUtils::StringSet& aliases = 
		GPlatesModel::StringSetSingletons::xml_namespace_alias_instance();
	if (namespace_alias == GPML_STANDARD_ALIAS)
	{
		return aliases.insert(GPML_NAMESPACE);
	}
	else if (namespace_alias == GML_STANDARD_ALIAS)
	{
		return aliases.insert(GML_NAMESPACE);
	}
	else if (namespace_alias == XSI_STANDARD_ALIAS)
	{
		return aliases.insert(XSI_STANDARD_ALIAS);
	}
	else
	{
		// Return alias argument as default if not recognised.
		return aliases.insert(namespace_alias);
	}
}

