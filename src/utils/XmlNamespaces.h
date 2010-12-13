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

#ifndef GPLATES_UTILS_XMLNAMESPACES_H
#define GPLATES_UTILS_XMLNAMESPACES_H

#include <QString>

#include "StringSet.h"
#include "UnicodeStringUtils.h"


namespace GPlatesUtils
{
	namespace XmlNamespaces
	{
		namespace Internals
		{
			// These need to be functions because C++ does not otherwise guarantee the
			// order of initialisation of non-local static objects.

			const char *
			gpml_namespace();

			const char *
			gml_namespace();
			
			const char *
			xsi_namespace();

			const char *
			gpml_standard_alias();

			const char *
			gml_standard_alias();

			const char *
			xsi_standard_alias();
		}


		/**
		 * Standard namespaces.
		 */
		const GPlatesUtils::UnicodeString GPML_NAMESPACE = Internals::gpml_namespace();
		const GPlatesUtils::UnicodeString GML_NAMESPACE = Internals::gml_namespace();
		const GPlatesUtils::UnicodeString XSI_NAMESPACE = Internals::xsi_namespace();

		const QString GPML_NAMESPACE_QSTRING = Internals::gpml_namespace();
		const QString GML_NAMESPACE_QSTRING = Internals::gml_namespace();
		const QString XSI_NAMESPACE_QSTRING = Internals::xsi_namespace();


		/**
		 * Standard namespace aliases.
		 */
		const GPlatesUtils::UnicodeString GPML_STANDARD_ALIAS = Internals::gpml_standard_alias();
		const GPlatesUtils::UnicodeString GML_STANDARD_ALIAS = Internals::gml_standard_alias();
		const GPlatesUtils::UnicodeString XSI_STANDARD_ALIAS = Internals::xsi_standard_alias();

		const QString GPML_STANDARD_ALIAS_QSTRING = Internals::gpml_standard_alias();
		const QString GML_STANDARD_ALIAS_QSTRING = Internals::gml_standard_alias();
		const QString XSI_STANDARD_ALIAS_QSTRING = Internals::xsi_standard_alias();


		/**
		 * Returns the standard namespace alias for the given namespace URI.
		 * Returns "gpml" for any namespace URI that is not recognised.
		 */
		StringSet::SharedIterator
		get_standard_alias_for_namespace(
				const GPlatesUtils::UnicodeString &namespace_uri);


		/**
		 * Returns the namespace URI when given a standard namespace alias.
		 * Returns @a namespace_alias if the alias is not recognised. 
		 */
		StringSet::SharedIterator
		get_namespace_for_standard_alias(
				const GPlatesUtils::UnicodeString &namespace_alias);
	}
}

#endif  // GPLATES_UTILS_XMLNAMESPACES_H
