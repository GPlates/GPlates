/* $Id: XmlNamespaces.cc 10554 2010-12-13 05:57:08Z mchin $ */

/**
 * \file
 *
 * Most recent change:
 *   $Date: 2010-12-13 16:57:08 +1100 (Mon, 13 Dec 2010) $
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

#include <QBuffer>
#include <QXmlQuery>
#include <QXmlResultItems>
#include <QXmlSerializer>

#include "XQueryUtils.h"
#include "file-io/GsmlConst.h"

std::vector<QByteArray>
GPlatesUtils::XQuery::evaluate(
		QByteArray& xml_data,
		const QString& query_str,
		IsEmptyFun is_empty)
{
	std::vector<QByteArray> ret;
	using namespace GPlatesFileIO::GsmlConst;
	QBuffer buffer(&xml_data);
	buffer.open(QIODevice::ReadOnly | QIODevice::Text);
	if(!buffer.isOpen())
	{
		qWarning() << "Cannot open input buffer.";
		return ret;
	}

	QXmlQuery query;
	for(int i=1; ;i++)
	{
		buffer.reset();
		query.bindVariable("idx", QVariant(i));
		query.bindVariable("data_source", &buffer);
		query.setQuery(
				all_namespaces() + 
				declare_idx + 
				"doc($data_source)"+ 
				query_str + "[$idx]");
		
		QByteArray array;
		QBuffer out_buf(&array);
		out_buf.open(QIODevice::ReadWrite | QIODevice::Text);
		if(!out_buf.isOpen())
		{
			qWarning() << "Cannot open temporary buffer.";
			break;
		}

		QXmlSerializer serializer(query, &out_buf);
		if(query.evaluateTo(&serializer))
		{
			if(is_empty(out_buf))
			{
				qDebug() << "No more data.";
				break;
			}
			ret.push_back(array);
		}
		out_buf.close();
	}
	return ret;
}


std::vector<QVariant>
GPlatesUtils::XQuery::evaluate_attribute(
		QByteArray& xml_data,
		const QString& attr_name)
{
	std::vector<QVariant> ret;

	QBuffer buffer(&xml_data);
	buffer.open(QIODevice::ReadOnly | QIODevice::Text);
	if(!buffer.isOpen())
	{
		qWarning() << "Cannot open input buffer in evaluate_string_attr().";
		return ret;
	}

	using namespace GPlatesFileIO::GsmlConst;
	QXmlQuery query;
	QXmlResultItems results;
	query.bindVariable("data_source", &buffer);
	query.setQuery(all_namespaces() + "data(doc($data_source)//@" + attr_name + ")" );
	query.evaluateTo(&results);
	QXmlItem item(results.next());
	
	while(!item.isNull() && item.isAtomicValue())
	{
		ret.push_back(item.toAtomicValue());
		item = results.next();
	}
	
	buffer.close();
	return ret;
}

void
GPlatesUtils::XQuery::wrap_xml_data(
		QByteArray& xml_data,
		const QString& wrapper)
{
	QBuffer buffer(&xml_data);
	buffer.open(QIODevice::ReadOnly | QIODevice::Text);
	if(!buffer.isOpen())
	{
		qWarning() << "Cannot open input buffer.";
		return;
	}

	using namespace GPlatesFileIO::GsmlConst;
	QXmlQuery query;
	query.bindVariable("data_source", &buffer);
	query.setQuery(
			all_namespaces() + "<" + wrapper+">"+
			"{doc($data_source)}</" + wrapper + ">");
	QByteArray array;
	QBuffer out_buf(&array);
	out_buf.open(QIODevice::ReadWrite | QIODevice::Text);
	if(!out_buf.isOpen())
	{
		qWarning() << "Cannot open temporary buffer.";
		return;
	}

	QXmlSerializer serializer(query, &out_buf);
	if(query.evaluateTo(&serializer))
	{
		xml_data = array;
	}
	out_buf.close();
}

bool 
GPlatesUtils::XQuery::next_start_element(
		QXmlStreamReader& reader)
{
	while (reader.readNext() != QXmlStreamReader::Invalid) {
		if (reader.isEndElement())
			return false;
		else if (reader.isStartElement())
			return true;
	}
	return false;
}















