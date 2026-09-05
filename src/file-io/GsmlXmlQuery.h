/* $Id$ */

/**
 * \file
 * $Revision$
 * $Date$
 *
 * Copyright (C) 2026 The University of Sydney, Australia
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

#ifndef GPLATES_FILEIO_GSMLXMLQUERY_H
#define GPLATES_FILEIO_GSMLXMLQUERY_H

#include <vector>
#include <QByteArray>
#include <QString>
#include <QStringList>

namespace GPlatesFileIO
{
	/**
	 * The XPath-lite the GeoSciML reader runs over XML text, using Qt Core alone.
	 *
	 * The reader used QtXmlPatterns (QXmlQuery) for this until Qt6 removed that module. Its
	 * queries are all plain path expressions (see "GsmlPropertyDef.h"), and its handlers
	 * consume each result as serialised text - re-parsing it with QXmlStreamReader, scanning
	 * it for "<gml:posList" - so a result is a complete XML fragment that still uses the
	 * source's prefixes.
	 */
	namespace GsmlXmlQuery
	{
		/**
		 * Returns every element that @a path selects in @a xml_data, in document order, each
		 * serialised as a standalone fragment: every namespace in scope is declared on its
		 * root, under the prefix the source used.
		 *
		 * @a path is either "//prefix:name" - every element with that namespace and local
		 * name, wherever it is - or "/prefix:a/prefix:b/..." - the document element must be
		 * 'a', a child of it 'b', and so on, and the innermost elements are returned. A "//"
		 * path may also have several steps, matched against the end of an element's ancestry.
		 * An element inside a returned element is not returned again on its own.
		 *
		 * Prefixes are resolved through @a GsmlConst::namespace_uri and matching is on
		 * namespace URI and local name, so the document's own prefixes do not matter.
		 */
		std::vector<QByteArray>
		find_elements(
				const QByteArray &xml_data,
				const QString &path);

		/**
		 * Returns the value of the un-namespaced attribute @a name on every element of
		 * @a xml_data that has it, in document order.
		 */
		QStringList
		find_attribute_values(
				const QByteArray &xml_data,
				const QString &name);

		/**
		 * Returns @a xml_data (a fragment, as returned by @a find_elements) enclosed in a new
		 * element @a qualified_name ("prefix:name"), whose prefix is declared on it so that
		 * the result is still a standalone fragment.
		 */
		QByteArray
		wrap_element(
				const QByteArray &xml_data,
				const QString &qualified_name);
	}
}

#endif  // GPLATES_FILEIO_GSMLXMLQUERY_H
