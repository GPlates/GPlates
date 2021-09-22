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
#include <QStringList>
#include <QXmlResultItems>

#include "XQueryUtils.h"
#include "file-io/GsmlConst.h"




std::vector<QByteArray>
GPlatesUtils::XQuery::evaluate_features(
		QByteArray& xml_data,
		const QString& query_str)
{
//qDebug() << "GPlatesUtils::XQuery::evaluate_features()";
//qDebug() << "GPlatesUtils::XQuery::evaluate_features(): query_str=" << query_str;

	std::vector<QByteArray> ret;

	// using namespace GPlatesFileIO::GsmlConst;

	QBuffer buffer(&xml_data);
	buffer.open(QIODevice::ReadOnly | QIODevice::Text);
	if(!buffer.isOpen())
	{
		qWarning() << "Cannot open input buffer.";
		return ret;
	}
	buffer.reset();

	// Create the query
	QXmlQuery query;

	QString q_s = "";
	q_s.append( 
		GPlatesFileIO::GsmlConst::all_namespaces() + 
		"doc($data_source)" + 
		"//gml:featureMember");

	query.bindVariable("data_source", &buffer);

	query.setQuery( q_s );

	QByteArray out_array;
	QBuffer out_buf(&out_array);
	out_buf.open(QIODevice::ReadWrite | QIODevice::Text);

	if(!out_buf.isOpen())
	{
		qWarning() << "Cannot open temporary buffer.";
	}


	// Evaluate query to QXmlSerializer
	QXmlSerializer serializer(query, &out_buf);
	if(query.evaluateTo(&serializer))
	{
		//qDebug() << "GPlatesUtils::XQuery::evaluate_features(): out_array.size() =" << out_array.size();
		if (out_array.size() == 0)
		{
			//qDebug() << "WARN: GPlatesUtils::XQuery::evaluate_features(): out_array.size()=" << out_array.size();
			// query returned no elements to out_array
			return ret;
		}

		// split the out_array by gml:featureMember tags
		out_array.replace(
			"</gml:featureMember><gml:featureMember",
			"</gml:featureMember>;<gml:featureMember");
		QList<QByteArray> list = out_array.split(';');
		QList<QByteArray>::iterator itr = list.begin();
//qDebug() << "GPlatesUtils::XQuery::evaluate_features(): list.size() =" << list.size();
		for ( ; itr != list.end(); ++itr )
		{
			itr->replace(';', "");
			ret.push_back( *itr );
		}
	}
	else
	{
		qDebug() << "GPlatesUtils::XQuery::evaluate_features(): query.evaluateTo(serializer) false!";
	}

// FIXME: if this the way to do it ...
#if 0
	out_array.clear();

	// evaulate to results
	QXmlResultItems items;
	query.evaluateTo(&items);

	// double check for errors
	if (items.hasError())
	{
		qDebug() << "GPlatesUtils::XQuery::evaluate_features(): items.hasError()";
		return ret;
	}

	// Process results 
	QXmlItem item(items.next());
	int i = 1;
	while (!item.isNull()) 
	{
		qDebug() << "GPlatesUtils::XQuery::evaluate_features(): item number i =" << i;
		if (item.isAtomicValue()) 
		{
			qDebug() << "GPlatesUtils::XQuery::evaluate_features(): item.isAtomicValue()";
			QVariant v = item.toAtomicValue();
			switch (v.type()) 
			{
                 case QVariant::LongLong:
                     // xs:integer
                     break;
                 case QVariant::String:
                     // xs:string
                     break;
                 default:
                     // error
                     break;
			}
		}
		else if (item.isNode()) 
		{
			// QXmlNodeModelIndex i = item.toNodeModelIndex();
			qDebug() << "GPlatesUtils::XQuery::evaluate_features(): item.isNode()";

			// FIXME: how to transform the QXmlItem item 
			// into somthing to append to std::vector<QByteArray> ret??

		}
		item = items.next();
	}
#endif

//qDebug() << "GPlatesUtils::XQuery::evaluate_features(): ret.size() =" << ret.size();
	return ret;
}


std::vector<QByteArray>
GPlatesUtils::XQuery::evaluate_query(
		QByteArray& xml_data,
		const QString& query_str)
{
#if 0
qDebug() << "GPlatesUtils::XQuery::evaluate_query()";
qDebug() << "GPlatesUtils::XQuery::evaluate_query(): query_str=" << query_str;
#endif

	std::vector<QByteArray> ret;

	// fill input buffer with xml data to query
	QBuffer in_buf(&xml_data);
	in_buf.open(QIODevice::ReadOnly | QIODevice::Text);

	if(!in_buf.isOpen())
	{
		qWarning() << "Cannot open input buffer.";
		return ret;
	}
	in_buf.reset();

	// Create the query
	QXmlQuery query;

	QString q_s = "";
	q_s.append( 
		GPlatesFileIO::GsmlConst::all_namespaces() + 
		"doc($data_source)" + 
		query_str);

	query.bindVariable("data_source", &in_buf);
	query.setQuery( q_s );

	QByteArray out_array;
	QBuffer out_buf(&out_array);
	out_buf.open(QIODevice::ReadWrite | QIODevice::Text);

	if(!out_buf.isOpen())
	{
		qWarning() << "ERROR Cannot open temporary output buffer.";
		return ret;
	}

	// Evaluate query to QXmlSerializer
	QXmlSerializer serializer(query, &out_buf);

	if(query.evaluateTo(&serializer))
	{
		//qDebug() << "GPlatesUtils::XQuery::evaluate_query(): out_array.size()=" << out_array.size();
		if (out_array.size() == 0)
		{
			// query returned no elements to out_array
			// qDebug() << "WARN: GPlatesUtils::XQuery::evaluate_query(): out_array.size()=" << out_array.size();
			return ret;
		}

		// else, there is some data to process 

		// build the tag pairs like:				"</gml:featureMember><gml:featureMember"
        // and insert a ^ chars to split on:		"</gml:featureMember>^<gml:featureMember"

		// extract the tag_name string from query_str like 
		// "//gpml:RockUnit_siliciclastic" or "/gsml:shape/gml:Point" 

		QByteArray tag_name = query_str.mid( query_str.lastIndexOf("/") ).toUtf8();

		QByteArray tag_i; tag_i.append("</" + tag_name + "><" + tag_name);
		QByteArray tag_o; tag_o.append("</" + tag_name + ">^<" + tag_name);

		out_array.replace( tag_i, tag_o );

		// split the out_array by input tags

		QList<QByteArray> list = out_array.split('^');
// qDebug() << "GPlatesUtils::XQuery::evaluate_query(): list.size() =" << list.size();
		QList<QByteArray>::iterator itr = list.begin();
		for ( ; itr != list.end(); ++itr )
		{
			itr->replace('^', "");
			ret.push_back( *itr );
		}
	}
	else
	{
		qDebug() << "GPlatesUtils::XQuery::evaluate_query(): query.evaluateTo(serializer) false!";
		return ret;
	}

// qDebug() << "GPlatesUtils::XQuery::evaluate_query(): ret.size() =" << ret.size();
	return ret;
}



#if 0
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

	int i = 1;
	for( ; ; i++)
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
#endif 


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















