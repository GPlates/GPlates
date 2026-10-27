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

#include <vector>
#include <QByteArray>
#include <QString>
#include <QStringList>
#include <QXmlStreamReader>
#include <gtest/gtest.h>

#include "file-io/GsmlXmlQuery.h"

#include "global/AssertionFailureException.h"


// The GeoSciML reader's XPath-lite (see GsmlXmlQuery.h). What matters, beyond selecting the
// right elements, is that each result is a fragment that parses on its own and keeps the
// prefixes the source used - the reader's handlers re-parse the fragments and match some
// prefixes textually.
namespace
{
	using GPlatesFileIO::GsmlXmlQuery::find_attribute_values;
	using GPlatesFileIO::GsmlXmlQuery::find_elements;
	using GPlatesFileIO::GsmlXmlQuery::wrap_element;

	const char *const GSML_NS = "urn:cgi:xmlns:CGI:GeoSciML:2.0";

	const QByteArray DOCUMENT =
			"<wfs:FeatureCollection xmlns:wfs=\"http://www.opengis.net/wfs\""
			" xmlns:gml=\"http://www.opengis.net/gml\""
			" xmlns:gsml=\"urn:cgi:xmlns:CGI:GeoSciML:2.0\""
			" xmlns:xlink=\"http://www.w3.org/1999/xlink\">"
			"<gml:featureMember>"
			"<gsml:UnclassifiedFeature gml:id=\"f1\">"
			"<gml:name>one</gml:name>"
			"<gsml:shape><gml:Point srsName=\"EPSG:4326\"><gml:pos>1 2</gml:pos></gml:Point>"
			"</gsml:shape>"
			"</gsml:UnclassifiedFeature>"
			"</gml:featureMember>"
			"<gml:featureMember>"
			"<gsml:UnclassifiedFeature gml:id=\"f2\">"
			"<gml:name>two</gml:name>"
			"<gsml:observationMethod xlink:href=\"urn:x\"/>"
			"<gml:name>alias &amp; more</gml:name>"
			"</gsml:UnclassifiedFeature>"
			"</gml:featureMember>"
			"</wfs:FeatureCollection>";

	/**
	 * The root element of a fragment parsed on its own, plus whether the whole fragment
	 * was well-formed.
	 */
	struct ParsedRoot
	{
		QString namespace_uri;
		QString local_name;
		QString qualified_name;
		bool well_formed;
	};

	ParsedRoot
	parse_root(
			const QByteArray &fragment)
	{
		ParsedRoot root;
		QXmlStreamReader reader(fragment);
		if (reader.readNextStartElement())
		{
			root.namespace_uri = reader.namespaceUri().toString();
			root.local_name = reader.name().toString();
			root.qualified_name = reader.qualifiedName().toString();
		}
		while (!reader.atEnd())
		{
			reader.readNext();
		}
		root.well_formed = !reader.hasError();
		return root;
	}
}


TEST(GsmlXmlQueryTest, descendant_path_returns_each_element_separately)
{
	std::vector<QByteArray> members = find_elements(DOCUMENT, "//gml:featureMember");
	ASSERT_EQ(2u, members.size());
	EXPECT_TRUE(members[0].contains("gml:id=\"f1\""));
	EXPECT_TRUE(members[1].contains("gml:id=\"f2\""));

	// Both names of the second feature come back on their own.
	std::vector<QByteArray> names = find_elements(DOCUMENT, "//gml:name");
	ASSERT_EQ(3u, names.size());
	EXPECT_TRUE(names[2].contains(">alias &amp; more<"));

	// A multi-step "//" path is matched against the end of the element's ancestry.
	EXPECT_EQ(1u, find_elements(DOCUMENT, "//gsml:shape/gml:Point").size());
	EXPECT_EQ(0u, find_elements(DOCUMENT, "//gml:featureMember/gml:Point").size());
}


TEST(GsmlXmlQueryTest, anchored_path_starts_at_the_document_element)
{
	EXPECT_EQ(2u, find_elements(DOCUMENT, "/wfs:FeatureCollection/gml:featureMember").size());
	EXPECT_EQ(0u, find_elements(DOCUMENT, "/gml:featureMember").size());

	// Run against a fragment, the fragment's root is the document element.
	const std::vector<QByteArray> members = find_elements(DOCUMENT, "//gml:featureMember");
	ASSERT_EQ(2u, members.size());
	EXPECT_EQ(1u, find_elements(members[0], "/gml:featureMember/gsml:UnclassifiedFeature").size());
	EXPECT_EQ(0u, find_elements(members[0], "/gsml:UnclassifiedFeature").size());
}


TEST(GsmlXmlQueryTest, matches_namespace_uri_not_prefix)
{
	// The same document with every prefix renamed and gml made the default namespace.
	const QByteArray renamed =
			"<w:FeatureCollection xmlns:w=\"http://www.opengis.net/wfs\""
			" xmlns=\"http://www.opengis.net/gml\" xmlns:g=\"urn:cgi:xmlns:CGI:GeoSciML:2.0\">"
			"<featureMember><g:UnclassifiedFeature><name>one</name></g:UnclassifiedFeature>"
			"</featureMember>"
			"</w:FeatureCollection>";

	EXPECT_EQ(1u, find_elements(renamed, "/wfs:FeatureCollection/gml:featureMember").size());
	EXPECT_EQ(1u, find_elements(renamed, "//gml:name").size());
	EXPECT_EQ(1u, find_elements(renamed, "//gsml:UnclassifiedFeature").size());
}


TEST(GsmlXmlQueryTest, fragments_stand_alone_with_the_source_prefixes)
{
	const std::vector<QByteArray> shapes = find_elements(DOCUMENT, "//gsml:shape");
	ASSERT_EQ(1u, shapes.size());
	const QByteArray &shape = shapes[0];

	// The prefixes were declared on an ancestor, and are declared again on the fragment.
	EXPECT_TRUE(shape.startsWith("<gsml:shape "));
	EXPECT_TRUE(shape.contains("<gml:Point srsName=\"EPSG:4326\">"));
	EXPECT_TRUE(shape.contains("xmlns:gml=\"http://www.opengis.net/gml\""));
	EXPECT_TRUE(shape.endsWith("</gsml:shape>"));

	const ParsedRoot root = parse_root(shape);
	EXPECT_TRUE(root.well_formed);
	EXPECT_EQ(GSML_NS, root.namespace_uri);
	EXPECT_EQ("shape", root.local_name);
	EXPECT_EQ("gsml:shape", root.qualified_name);

	// So a fragment can be queried again.
	EXPECT_EQ(1u, find_elements(shape, "/gsml:shape/gml:Point").size());

	// Attributes keep their prefixes, and resolve.
	const std::vector<QByteArray> methods = find_elements(DOCUMENT, "//gsml:observationMethod");
	ASSERT_EQ(1u, methods.size());
	EXPECT_TRUE(methods[0].contains("xlink:href=\"urn:x\""));
	EXPECT_TRUE(parse_root(methods[0]).well_formed);
}


TEST(GsmlXmlQueryTest, keeps_the_source_prefix_when_two_prefixes_share_a_namespace)
{
	// A second prefix bound to the GML namespace, sorting after "gml". Copying by namespace
	// URI would re-prefix every GML element under whichever binding the writer saw last.
	const QByteArray document =
			"<wfs:FeatureCollection xmlns:wfs=\"http://www.opengis.net/wfs\""
			" xmlns:gml=\"http://www.opengis.net/gml\""
			" xmlns:gml3=\"http://www.opengis.net/gml\""
			" xmlns:gsml=\"urn:cgi:xmlns:CGI:GeoSciML:2.0\">"
			"<gsml:shape>"
			"<gml:Point gml:id=\"p1\" gml3:remoteSchema=\"s\"><gml3:pos>1 2</gml3:pos></gml:Point>"
			"</gsml:shape>"
			"</wfs:FeatureCollection>";

	const std::vector<QByteArray> shapes = find_elements(document, "//gsml:shape");
	ASSERT_EQ(1u, shapes.size());
	const QByteArray &shape = shapes[0];

	// Elements and attributes keep the prefix the source gave each of them.
	EXPECT_TRUE(shape.contains("<gml:Point gml:id=\"p1\" gml3:remoteSchema=\"s\">"));
	EXPECT_TRUE(shape.contains("<gml3:pos>1 2</gml3:pos>"));
	EXPECT_TRUE(parse_root(shape).well_formed);

	// And both prefixes still resolve to the namespace when the fragment is queried.
	EXPECT_EQ(1u, find_elements(shape, "/gsml:shape/gml:Point/gml:pos").size());
}


TEST(GsmlXmlQueryTest, nested_hit_is_returned_inside_its_enclosing_hit_only)
{
	const QByteArray nested =
			"<gsml:x xmlns:gsml=\"urn:cgi:xmlns:CGI:GeoSciML:2.0\"><gsml:x>inner</gsml:x></gsml:x>";

	const std::vector<QByteArray> hits = find_elements(nested, "//gsml:x");
	ASSERT_EQ(1u, hits.size());
	EXPECT_EQ(2, hits[0].count("<gsml:x"));
}


TEST(GsmlXmlQueryTest, namespaces_declared_on_inner_elements_are_kept)
{
	// The default namespace is declared on the root, the gsml prefix on the element
	// returned, and a third prefix on an element inside it.
	const QByteArray document =
			"<root xmlns=\"http://www.opengis.net/gml\">"
			"<g:shape xmlns:g=\"urn:cgi:xmlns:CGI:GeoSciML:2.0\">"
			"<posList>1 2</posList>"
			"<x:extra xmlns:x=\"urn:extra\">text</x:extra>"
			"</g:shape>"
			"</root>";

	const std::vector<QByteArray> shapes = find_elements(document, "//gsml:shape");
	ASSERT_EQ(1u, shapes.size());
	const QByteArray &shape = shapes[0];

	EXPECT_TRUE(shape.startsWith("<g:shape "));
	EXPECT_TRUE(shape.contains("xmlns=\"http://www.opengis.net/gml\""));
	EXPECT_TRUE(shape.contains("<posList>1 2</posList>"));
	EXPECT_TRUE(shape.contains("<x:extra xmlns:x=\"urn:extra\">text</x:extra>"));

	const ParsedRoot root = parse_root(shape);
	EXPECT_TRUE(root.well_formed);
	EXPECT_EQ(GSML_NS, root.namespace_uri);
	EXPECT_EQ("g:shape", root.qualified_name);

	// The un-prefixed child is still in the default namespace.
	EXPECT_EQ(1u, find_elements(shape, "/gsml:shape/gml:posList").size());
}


TEST(GsmlXmlQueryTest, character_data_survives_the_copy)
{
	const QByteArray document =
			"<gml:a xmlns:gml=\"http://www.opengis.net/gml\">"
			"<gml:b>less &lt; more &amp; <![CDATA[<raw>]]> done</gml:b>"
			"<!-- a comment --><gml:c/>"
			"</gml:a>";

	const std::vector<QByteArray> hits = find_elements(document, "//gml:b");
	ASSERT_EQ(1u, hits.size());

	QXmlStreamReader reader(hits[0]);
	ASSERT_TRUE(reader.readNextStartElement());
	EXPECT_EQ("less < more & <raw> done", reader.readElementText());

	// An empty element and a comment do not upset the enclosing copy.
	EXPECT_TRUE(parse_root(find_elements(document, "//gml:a").at(0)).well_formed);
}


TEST(GsmlXmlQueryTest, find_attribute_values_in_document_order)
{
	const QByteArray document =
			"<gml:a xmlns:gml=\"http://www.opengis.net/gml\" srsName=\"EPSG:4326\">"
			"<gml:b srsName=\"EPSG:4283\" gml:id=\"b\"/>"
			"<gml:c/>"
			"</gml:a>";

	const QStringList values = find_attribute_values(document, "srsName");
	ASSERT_EQ(2, values.size());
	EXPECT_EQ("EPSG:4326", values[0]);
	EXPECT_EQ("EPSG:4283", values[1]);

	EXPECT_TRUE(find_attribute_values(document, "srsDimension").isEmpty());
}


TEST(GsmlXmlQueryTest, wrap_element_declares_the_wrapper_prefix)
{
	const QByteArray point =
			"<gml:Point xmlns:gml=\"http://www.opengis.net/gml\">"
			"<gml:pos>1 2</gml:pos></gml:Point>";

	const QByteArray wrapped = wrap_element(point, "gpml:position");
	EXPECT_TRUE(wrapped.startsWith(
			"<gpml:position xmlns:gpml=\"http://www.gplates.org/gplates\">"));
	EXPECT_TRUE(wrapped.endsWith("</gpml:position>"));
	EXPECT_EQ("gpml:position", parse_root(wrapped).qualified_name);
	EXPECT_EQ(1u, find_elements(wrapped, "/gpml:position/gml:Point").size());

	// Wrapping wraps (the polygon handler nests three).
	const QByteArray twice = wrap_element(wrapped, "gml:exterior");
	EXPECT_TRUE(parse_root(twice).well_formed);
	EXPECT_EQ(1u, find_elements(twice, "/gml:exterior/gpml:position/gml:Point").size());
}


TEST(GsmlXmlQueryTest, malformed_xml_returns_the_elements_before_the_error)
{
	// The second gml:b is never closed.
	const QByteArray truncated =
			"<gml:a xmlns:gml=\"http://www.opengis.net/gml\"><gml:b>1</gml:b><gml:b>2";

	const std::vector<QByteArray> hits = find_elements(truncated, "//gml:b");
	ASSERT_EQ(1u, hits.size());
	EXPECT_TRUE(hits[0].contains(">1<"));

	EXPECT_TRUE(find_elements("not xml at all", "//gml:b").empty());
	EXPECT_TRUE(find_attribute_values("<gml:a", "srsName").isEmpty());
}


TEST(GsmlXmlQueryTest, unknown_prefix_is_a_programming_error)
{
	EXPECT_THROW(find_elements(DOCUMENT, "//bogus:name"), GPlatesGlobal::AssertionFailureException);
	EXPECT_THROW(find_elements(DOCUMENT, "gml:name"), GPlatesGlobal::AssertionFailureException);
	EXPECT_THROW(wrap_element(DOCUMENT, "position"), GPlatesGlobal::AssertionFailureException);
}
