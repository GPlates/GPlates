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
GPlatesUtils::XmlNamespaces::Internals::gpgim_namespace()
{
	return "http://www.gplates.org/gpgim";
}


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
GPlatesUtils::XmlNamespaces::Internals::gpgim_standard_alias()
{
	return "gpgim";
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


const GPlatesUtils::UnicodeString &
GPlatesUtils::XmlNamespaces::get_gpgim_namespace()
{
	static const GPlatesUtils::UnicodeString GPGIM_NAMESPACE = Internals::gpgim_namespace();
	return GPGIM_NAMESPACE;
}


const GPlatesUtils::UnicodeString &
GPlatesUtils::XmlNamespaces::get_gpml_namespace()
{
	static const GPlatesUtils::UnicodeString GPML_NAMESPACE = Internals::gpml_namespace();
	return GPML_NAMESPACE;
}


const GPlatesUtils::UnicodeString &
GPlatesUtils::XmlNamespaces::get_gml_namespace()
{
	static const GPlatesUtils::UnicodeString GML_NAMESPACE = Internals::gml_namespace();
	return GML_NAMESPACE;
}


const GPlatesUtils::UnicodeString &
GPlatesUtils::XmlNamespaces::get_xsi_namespace()
{
	static const GPlatesUtils::UnicodeString XSI_NAMESPACE = Internals::xsi_namespace();
	return XSI_NAMESPACE;
}


const QString &
GPlatesUtils::XmlNamespaces::get_gpgim_namespace_qstring()
{
	static const QString GPGIM_NAMESPACE = Internals::gpgim_namespace();
	return GPGIM_NAMESPACE;
}


const QString &
GPlatesUtils::XmlNamespaces::get_gpml_namespace_qstring()
{
	static const QString GPML_NAMESPACE = Internals::gpml_namespace();
	return GPML_NAMESPACE;
}


const QString &
GPlatesUtils::XmlNamespaces::get_gml_namespace_qstring()
{
	static const QString GML_NAMESPACE = Internals::gml_namespace();
	return GML_NAMESPACE;
}


const QString &
GPlatesUtils::XmlNamespaces::get_xsi_namespace_qstring()
{
	static const QString XSI_NAMESPACE = Internals::xsi_namespace();
	return XSI_NAMESPACE;
}


const GPlatesUtils::UnicodeString &
GPlatesUtils::XmlNamespaces::get_gpgim_standard_alias()
{
	static const GPlatesUtils::UnicodeString GPGIM_STANDARD_ALIAS = Internals::gpgim_standard_alias();
	return GPGIM_STANDARD_ALIAS;
}


const GPlatesUtils::UnicodeString &
GPlatesUtils::XmlNamespaces::get_gpml_standard_alias()
{
	static const GPlatesUtils::UnicodeString GPML_STANDARD_ALIAS = Internals::gpml_standard_alias();
	return GPML_STANDARD_ALIAS;
}


const GPlatesUtils::UnicodeString &
GPlatesUtils::XmlNamespaces::get_gml_standard_alias()
{
	static const GPlatesUtils::UnicodeString GML_STANDARD_ALIAS = Internals::gml_standard_alias();
	return GML_STANDARD_ALIAS;
}


const GPlatesUtils::UnicodeString &
GPlatesUtils::XmlNamespaces::get_xsi_standard_alias()
{
	static const GPlatesUtils::UnicodeString XSI_STANDARD_ALIAS = Internals::xsi_standard_alias();
	return XSI_STANDARD_ALIAS;
}


const QString &
GPlatesUtils::XmlNamespaces::get_gpgim_standard_alias_qstring()
{
	static const QString GPGIM_STANDARD_ALIAS = Internals::gpgim_standard_alias();
	return GPGIM_STANDARD_ALIAS;
}


const QString &
GPlatesUtils::XmlNamespaces::get_gpml_standard_alias_qstring()
{
	static const QString GPML_STANDARD_ALIAS = Internals::gpml_standard_alias();
	return GPML_STANDARD_ALIAS;
}


const QString &
GPlatesUtils::XmlNamespaces::get_gml_standard_alias_qstring()
{
	static const QString GML_STANDARD_ALIAS = Internals::gml_standard_alias();
	return GML_STANDARD_ALIAS;
}


const QString &
GPlatesUtils::XmlNamespaces::get_xsi_standard_alias_qstring()
{
	static const QString XSI_STANDARD_ALIAS = Internals::xsi_standard_alias();
	return XSI_STANDARD_ALIAS;
}


GPlatesUtils::StringSet::SharedIterator
GPlatesUtils::XmlNamespaces::get_standard_alias_for_namespace(
		const GPlatesUtils::UnicodeString &namespace_uri)
{
	GPlatesUtils::StringSet& aliases = 
		GPlatesModel::StringSetSingletons::xml_namespace_alias_instance();
	if (namespace_uri == get_gml_namespace())
	{
		return aliases.insert(get_gml_standard_alias());
	}
	else if (namespace_uri == get_xsi_namespace())
	{
		return aliases.insert(get_xsi_standard_alias());
	}
	// NOTE: The GPGIM namespace is not part of the feature readers but is placed here
	// in order to re-use a lot of the XML parsing machinery when reading the GPGIM XML file.
	else if (namespace_uri == get_gpgim_namespace())
	{
		return aliases.insert(get_gpgim_standard_alias());
	}
	else
	{
		// Return gpml as default if URI not recognised.
		return aliases.insert(get_gpml_standard_alias());
	}
}


GPlatesUtils::StringSet::SharedIterator
GPlatesUtils::XmlNamespaces::get_namespace_for_standard_alias(
		const GPlatesUtils::UnicodeString &namespace_alias)
{
	GPlatesUtils::StringSet& aliases = 
		GPlatesModel::StringSetSingletons::xml_namespace_instance();
	if (namespace_alias == get_gpml_standard_alias())
	{
		return aliases.insert(get_gpml_namespace());
	}
	else if (namespace_alias == get_gml_standard_alias())
	{
		return aliases.insert(get_gml_namespace());
	}
	else if (namespace_alias == get_xsi_standard_alias())
	{
		return aliases.insert(get_xsi_namespace());
	}
	// NOTE: The GPGIM namespace is not part of the feature readers but is placed here
	// in order to re-use a lot of the XML parsing machinery when reading the GPGIM XML file.
	else if (namespace_alias == get_gpgim_standard_alias())
	{
		return aliases.insert(get_gpgim_namespace());
	}
	else
	{
		// Return gpml namespace URI as default if alias not recognised.
		return aliases.insert(get_gpml_namespace());
	}
}

