/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2014 The University of Sydney, Australia
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

#include <string>

#include "ScribeArchiveCommon.h"

#include "ScribeExceptions.h"

#include "global/GPlatesAssert.h"


namespace
{
	//
	// These validation functions are based on http://www.w3.org/TR/REC-xml/#NT-NameChar
	//

	bool
	is_valid_xml_name_start_char(
			char c)
	{
		return (c >= 'A' && c <= 'Z') ||
				(c >= 'a' && c <= 'z') ||
				c == '_'
				// Seems QXmlStreamReader does not accept the ':' character...
				/* || c == ':'*/;
	}

	bool
	is_valid_xml_name_char(
			char c)
	{
		return is_valid_xml_name_start_char(c) ||
				(c >= '0' && c <= '9') ||
				c == '-' ||
				c == '.';
	}
}


QString
GPlatesScribe::ArchiveCommon::get_xml_element_name(
		std::string xml_element_name,
		bool validate_all_chars)
{
	GPlatesGlobal::Assert<Exceptions::InvalidXmlElementName>(
			!xml_element_name.empty() &&
				xml_element_name[0] != '\0',
			GPLATES_ASSERTION_SOURCE);

	if (validate_all_chars)
	{
		for (unsigned int n = 0; n < xml_element_name.length(); ++n)
		{
			GPlatesGlobal::Assert<Exceptions::InvalidXmlElementName>(
					is_valid_xml_name_char(xml_element_name[n]),
					GPLATES_ASSERTION_SOURCE,
					xml_element_name);
		}
	}

	// If the start char is a valid XML name character but not as the first character
	// then prefix the string with an underscore.
	if (!is_valid_xml_name_start_char(xml_element_name[0]))
	{
		xml_element_name.insert(xml_element_name.begin(), '_');
	}

	return QString::fromLatin1(xml_element_name.data(), xml_element_name.length());
}
