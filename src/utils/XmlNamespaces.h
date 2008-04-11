#ifndef GPLATES_UTILS_XMLNAMESPACES_H
#define GPLATES_UTILS_XMLNAMESPACES_H

#include <QString>
#include "StringSet.h"
#include "UnicodeStringUtils.h"

namespace GPlatesUtils
{
	namespace XmlNamespaces
	{
		/**
		 * Standard namespaces.
		 */
		static const QString GPML_NAMESPACE = "http://www.gplates.org/gplates";
		static const QString GML_NAMESPACE = "http://www.opengis.net/gml";
		static const QString XSI_NAMESPACE = "http://www.w3.org/XMLSchema-instance";

		/**
		 * Standard namespace aliases.
		 */
		static const QString GPML_STANDARD_ALIAS = "gpml";
		static const QString GML_STANDARD_ALIAS = "gml";
		static const QString XSI_STANDARD_ALIAS = "xsi";


		/**
		 * Return the standard namespace for the given namespace URI.  Returns
		 * "gpml" for any namespace URI that is not recognised.
		 */
		inline
		StringSet::SharedIterator
		get_standard_alias_for_namespace(
				const StringSet::SharedIterator &namespace_uri)
		{
			QString ns = GPlatesUtils::make_qstring_from_icu_string(*namespace_uri);
			GPlatesUtils::StringSet& aliases = 
				GPlatesModel::StringSetSingletons::xml_namespace_alias_instance();
			if (ns == GML_NAMESPACE) {
				return aliases.insert(
						GPlatesUtils::make_icu_string_from_qstring(GML_STANDARD_ALIAS));
			}
			if (ns == XSI_NAMESPACE) {
				return aliases.insert(
						GPlatesUtils::make_icu_string_from_qstring(XSI_STANDARD_ALIAS));
			}
			return aliases.insert(
					GPlatesUtils::make_icu_string_from_qstring(GPML_STANDARD_ALIAS));
		}
	}
}

#endif  // GPLATES_UTILS_XMLNAMESPACES_H
