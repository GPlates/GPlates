/* $Id$ */

/**
 * \file
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 *
 * Copyright (C) 2011 The University of Sydney, Australia
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

#ifndef GPLATES_FILEIO_GSMLCONST_H
#define GPLATES_FILEIO_GSMLCONST_H

#include <boost/optional.hpp>
#include <QString>

namespace GPlatesFileIO
{
	/**
	 * The XML namespaces of the GeoSciML 2.0 documents (and their WFS envelopes) the reader
	 * accepts, keyed by the prefixes the reader's own path expressions use for them.
	 */
	namespace GsmlConst
	{
		/**
		 * Returns the namespace URI that @a prefix stands for in the reader's path expressions
		 * (see "GsmlPropertyDef.h") and element wrappers, or none if it is not one of them.
		 *
		 * These are the reader's prefixes, not a document's: @a GsmlXmlQuery matches on the
		 * URI, so a document may bind whatever prefixes it likes.
		 */
		inline
		boost::optional<QString>
		namespace_uri(
				const QString &prefix)
		{
			static const struct
			{
				const char *prefix;
				const char *uri;
			} NAMESPACES[] =
			{
				{ "xsi", "http://www.w3.org/2001/XMLSchema-instance" },
				{ "gml", "http://www.opengis.net/gml" },
				{ "wfs", "http://www.opengis.net/wfs" },
				{ "gsml", "urn:cgi:xmlns:CGI:GeoSciML:2.0" },
				{ "sa", "http://www.opengis.net/sampling/1.0" },
				{ "om", "http://www.opengis.net/om/1.0" },
				{ "cgu", "urn:cgi:xmlns:CGI:Utilities:1.0" },
				{ "xlink", "http://www.w3.org/1999/xlink" },
				{ "gpml", "http://www.gplates.org/gplates" }
			};

			for (const auto &ns : NAMESPACES)
			{
				if (prefix == ns.prefix)
				{
					return QString(ns.uri);
				}
			}

			return boost::none;
		}
	}
}

#endif  // GPLATES_FILEIO_GSMLCONST_H
