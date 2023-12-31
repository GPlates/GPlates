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

			// NOTE: The GPGIM namespace is not part of the feature readers but is placed here
			// in order to re-use a lot of the XML parsing machinery when reading the GPGIM XML file.
			const char *
			gpgim_namespace();

			const char *
			gpml_namespace();

			const char *
			gml_namespace();
			
			const char *
			xsi_namespace();


			// NOTE: The GPGIM namespace is not part of the feature readers but is placed here
			// in order to re-use a lot of the XML parsing machinery when reading the GPGIM XML file.
			const char *
			gpgim_standard_alias();

			const char *
			gpml_standard_alias();

			const char *
			gml_standard_alias();

			const char *
			xsi_standard_alias();
		}


		//
		// Standard namespaces.
		//


		const GPlatesUtils::UnicodeString &
		get_gpgim_namespace();

		const GPlatesUtils::UnicodeString &
		get_gpml_namespace();

		const GPlatesUtils::UnicodeString &
		get_gml_namespace();

		const GPlatesUtils::UnicodeString &
		get_xsi_namespace();


		const QString &
		get_gpgim_namespace_qstring();

		const QString &
		get_gpml_namespace_qstring();

		const QString &
		get_gml_namespace_qstring();

		const QString &
		get_xsi_namespace_qstring();


		//
		// Standard namespace aliases.
		//


		const GPlatesUtils::UnicodeString &
		get_gpgim_standard_alias();

		const GPlatesUtils::UnicodeString &
		get_gpml_standard_alias();

		const GPlatesUtils::UnicodeString &
		get_gml_standard_alias();

		const GPlatesUtils::UnicodeString &
		get_xsi_standard_alias();


		const QString &
		get_gpgim_standard_alias_qstring();

		const QString &
		get_gpml_standard_alias_qstring();

		const QString &
		get_gml_standard_alias_qstring();

		const QString &
		get_xsi_standard_alias_qstring();


		/**
		 * Returns the standard namespace alias for the given namespace URI.
		 *
		 * NOTE: Returns "gpml" for any namespace URI that is not recognised.
		 */
		StringSet::SharedIterator
		get_standard_alias_for_namespace(
				const GPlatesUtils::UnicodeString &namespace_uri);


		/**
		 * Returns the namespace URI for the given standard namespace alias.
		 *
		 * NOTE: Returns namespace associated with the "gpml" alias if the specified alias is not recognised. 
		 */
		StringSet::SharedIterator
		get_namespace_for_standard_alias(
				const GPlatesUtils::UnicodeString &namespace_alias);
	}
}

#endif  // GPLATES_UTILS_XMLNAMESPACES_H
