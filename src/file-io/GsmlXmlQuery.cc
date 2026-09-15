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

#include <algorithm>
#include <map>
#include <utility>
#include <boost/optional.hpp>
#include <QDebug>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>

#include "GsmlXmlQuery.h"

#include "GsmlConst.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"


namespace
{
	//! A namespace URI and a local name: one step of a path, or one open element.
	typedef std::pair<QString, QString> qualified_name_type;

	//! The namespaces one element declares.
	typedef QXmlStreamNamespaceDeclarations namespace_declarations_type;


	/**
	 * Parses "//prefix:name" or "/prefix:a/prefix:b/..." into @a steps.
	 *
	 * Returns whether the path is anchored at the document element.
	 */
	bool
	parse_path(
			const QString &path,
			std::vector<qualified_name_type> &steps)
	{
		// Every path is a literal in the reader's tables, so a malformed one, or one using a
		// prefix the reader does not know, is a programming error.
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				path.startsWith('/'),
				GPLATES_ASSERTION_SOURCE);
		const bool anchored = !path.startsWith("//");

		const QStringList names = path.mid(anchored ? 1 : 2).split('/');
		for (const QString &name : names)
		{
			const int colon = name.indexOf(':');
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					colon > 0 && colon + 1 < name.size(),
					GPLATES_ASSERTION_SOURCE);

			const boost::optional<QString> uri =
					GPlatesFileIO::GsmlConst::namespace_uri(name.left(colon));
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					uri,
					GPLATES_ASSERTION_SOURCE);

			steps.push_back(qualified_name_type(uri.get(), name.mid(colon + 1)));
		}

		return anchored;
	}


	/**
	 * Whether the innermost open element is selected by the path.
	 */
	bool
	matches(
			const std::vector<qualified_name_type> &open_elements,
			const std::vector<qualified_name_type> &steps,
			bool anchored)
	{
		if (anchored
			? open_elements.size() != steps.size()
			: open_elements.size() < steps.size())
		{
			return false;
		}

		return std::equal(steps.begin(), steps.end(), open_elements.end() - steps.size());
	}


	void
	declare_namespace(
			QXmlStreamWriter &writer,
			const QString &prefix,
			const QString &namespace_uri)
	{
		if (prefix.isEmpty())
		{
			writer.writeDefaultNamespace(namespace_uri);
		}
		else
		{
			writer.writeNamespace(namespace_uri, prefix);
		}
	}


	/**
	 * Serialises the element the reader is positioned on (a start element) together with
	 * everything inside it, leaving the reader on its end element.
	 *
	 * The fragment declares every namespace in scope on its root, under the prefixes the
	 * source used (the handlers match some of them textually), so it can be parsed on its own.
	 * @a scopes holds the declarations of each open element, outermost first.
	 */
	QByteArray
	copy_element(
			QXmlStreamReader &reader,
			const std::vector<namespace_declarations_type> &scopes)
	{
		QByteArray fragment;
		QXmlStreamWriter writer(&fragment);

		// The effective declarations: the innermost declaration of a prefix wins. They are
		// written before the root's start tag, which Qt then attaches them to. The root's own
		// declarations are among them, so it does not declare them again below.
		std::map<QString, QString> namespaces_in_scope;
		for (const namespace_declarations_type &declarations : scopes)
		{
			for (const QXmlStreamNamespaceDeclaration &declaration : declarations)
			{
				namespaces_in_scope[declaration.prefix().toString()] =
						declaration.namespaceUri().toString();
			}
		}
		for (const auto &prefix_and_uri : namespaces_in_scope)
		{
			declare_namespace(writer, prefix_and_uri.first, prefix_and_uri.second);
		}

		int depth = 0;
		for (;;)
		{
			if (reader.isStartElement())
			{
				if (depth > 0)
				{
					for (const QXmlStreamNamespaceDeclaration &declaration :
						reader.namespaceDeclarations())
					{
						declare_namespace(
								writer,
								declaration.prefix().toString(),
								declaration.namespaceUri().toString());
					}
				}
				// The tag and its attributes are written under the source's own qualified names.
				// Given a namespace URI instead, the writer would choose the prefix itself - the
				// last one it saw bound to that URI - so a document binding two prefixes to one
				// namespace would come out under the other one, and the handlers' textual
				// matching ("<gml:posList") would miss it.
				writer.writeStartElement(reader.qualifiedName().toString());
				for (const QXmlStreamAttribute &attribute : reader.attributes())
				{
					writer.writeAttribute(
							attribute.qualifiedName().toString(),
							attribute.value().toString());
				}
				++depth;
			}
			else if (reader.isEndElement())
			{
				writer.writeEndElement();
				if (--depth == 0)
				{
					break;
				}
			}
			else
			{
				// Characters (and CDATA), comments, processing instructions, entity references.
				writer.writeCurrentToken(reader);
			}

			reader.readNext();
			if (reader.atEnd())
			{
				// Not well-formed (or truncated): the caller reports it.
				break;
			}
		}

		return fragment;
	}


	void
	warn_if_not_well_formed(
			const QXmlStreamReader &reader)
	{
		if (reader.hasError())
		{
			qWarning() << "GeoSciML: XML is not well-formed at line" << reader.lineNumber()
					<< ":" << reader.errorString();
		}
	}
}


std::vector<QByteArray>
GPlatesFileIO::GsmlXmlQuery::find_elements(
		const QByteArray &xml_data,
		const QString &path)
{
	std::vector<qualified_name_type> steps;
	const bool anchored = parse_path(path, steps);

	std::vector<QByteArray> elements;

	std::vector<qualified_name_type> open_elements;
	std::vector<namespace_declarations_type> scopes;

	QXmlStreamReader reader(xml_data);
	while (!reader.atEnd())
	{
		reader.readNext();

		if (reader.isStartElement())
		{
			open_elements.push_back(
					qualified_name_type(
							reader.namespaceUri().toString(),
							reader.name().toString()));
			scopes.push_back(reader.namespaceDeclarations());

			if (matches(open_elements, steps, anchored))
			{
				const QByteArray element = copy_element(reader, scopes);
				// The reader is now on the element's end element - unless the XML ended
				// early, in which case the copy is cut short too, and the outer loop ends.
				if (!reader.hasError())
				{
					elements.push_back(element);
				}
				open_elements.pop_back();
				scopes.pop_back();
			}
		}
		else if (reader.isEndElement())
		{
			open_elements.pop_back();
			scopes.pop_back();
		}
	}
	warn_if_not_well_formed(reader);

	return elements;
}


QStringList
GPlatesFileIO::GsmlXmlQuery::find_attribute_values(
		const QByteArray &xml_data,
		const QString &name)
{
	QStringList values;

	QXmlStreamReader reader(xml_data);
	while (!reader.atEnd())
	{
		reader.readNext();

		if (reader.isStartElement() &&
			reader.attributes().hasAttribute(name))
		{
			values.push_back(reader.attributes().value(name).toString());
		}
	}
	warn_if_not_well_formed(reader);

	return values;
}


QByteArray
GPlatesFileIO::GsmlXmlQuery::wrap_element(
		const QByteArray &xml_data,
		const QString &qualified_name)
{
	const int colon = qualified_name.indexOf(':');
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			colon > 0 && colon + 1 < qualified_name.size(),
			GPLATES_ASSERTION_SOURCE);

	const QString prefix = qualified_name.left(colon);
	const boost::optional<QString> uri = GsmlConst::namespace_uri(prefix);
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			uri,
			GPLATES_ASSERTION_SOURCE);

	QByteArray wrapped;
	wrapped += "<" + qualified_name.toUtf8() +
			" xmlns:" + prefix.toUtf8() + "=\"" + uri->toUtf8() + "\">";
	wrapped += xml_data;
	wrapped += "</" + qualified_name.toUtf8() + ">";

	return wrapped;
}
