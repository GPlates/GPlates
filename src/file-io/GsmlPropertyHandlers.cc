/* $Id$ */

/**
 * @file 
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
#include <boost/optional.hpp>
#include <QDebug>
#include <QXmlQuery>
#include <QXmlSerializer>
#include <QXmlStreamReader>
#include <QXmlResultItems>

#include "GsmlPropertyHandlers.h"
#include "GsmlNodeProcessorFactory.h"
#include "PropertyCreationUtils.h"

#include <model/XmlNode.h>
#include <model/PropertyValue.h>
#include <model/ModelUtils.h>

#include <property-values/GmlLineString.h>

#include "ogr_spatialref.h"

using namespace GPlatesFileIO;

const QString line_string_query = gsml_ns + gml_ns + 
	"declare variable $idx external; " 
	"<gml:baseCurve>{doc($data_source)/gsml:shape/gml:LineString[$idx]}</gml:baseCurve>";

const QString point_query = gsml_ns + gml_ns + 
	"declare variable $idx external; " 
	"<gml:baseCurve>{doc($data_source)/gsml:shape/gml:point[$idx]}</gml:baseCurve>";

const QString polygon_query = gsml_ns + gml_ns + 
	"declare variable $idx external; " 
	"<gml:baseCurve>{doc($data_source)/gsml:shape/gml:Polygon[$idx]}</gml:baseCurve>";

namespace
{
	/*
	* Since EPSG:4326 coordinates system put longitude before latitude, which is opposite from gpml,
	* change the order using this function.
	*/
	void
	normalize_geometry_coord(
			QByteArray& buf)
	{
		static const QString posList_begin = "<gml:posList";
		static const QString posList_end = "</gml:posList>";
		
		int idx = 0, idx_begin = 0, idx_end = 0;

		idx = buf.indexOf(posList_begin.toUtf8());
		while(idx != -1)
		{
			idx_begin = buf.indexOf(">", idx)+1;
			idx_end = buf.indexOf(posList_end.toUtf8(), idx_begin);

			QByteArray tail = buf.right(buf.size()-idx_end);
			QList<QByteArray> list = buf.mid(idx_begin, idx_end-idx_begin).simplified().split(' ');
			buf.chop(buf.size()-idx_begin);
			for(int i=0; i<list.size();i+=2)
			{
				buf.append(" ");
				buf.append(list[i+1]);
				buf.append(" ");
				buf.append(list[i]);
			}
			idx_end = buf.length();
			buf.append(tail);
			idx_end = buf.indexOf(posList_end,idx_end) + posList_end.length();
			if(idx_end < posList_end.length())
			{
				qWarning() << "Cannot find </gml:posList>. The data might not be well-formed.";
				//TODO: throw exception.
				break;
			}
			idx = buf.indexOf(posList_begin.toUtf8(),idx_end);
		}
	}

	QString
	get_srs_name(
			QByteArray& array_buf)
	{
		QBuffer buffer(&array_buf);
		buffer.open(QIODevice::ReadWrite | QIODevice::Text);
		if(!buffer.isOpen())
		{
			qWarning() << "Cannot open buffer for output.";
			return QString();
		}

		QXmlQuery query;
		QXmlResultItems srs_name;
		query.bindVariable("data_source", &buffer);
		query.setQuery(gml_ns + "data(doc($data_source)//@srsName)");
		query.evaluateTo(&srs_name);
		QXmlItem item(srs_name.next());
		if(!item.isNull() && item.isAtomicValue())
		{
			return item.toAtomicValue().toString();
		}
		else
		{
			return QString();
		}
		buffer.close();
	}

	bool
	is_EPSG_4326(
			QString& name)
	{
		if(name.contains("EPSG",Qt::CaseInsensitive) && name.contains("4326"))
		{
			return true;
		}
		else
		{
			return false;
		}
	}


	unsigned
	find_srs_dimension(
			QByteArray& buf)
	{
		static const QString DIMENSION_TAG = "srsDimension";
		QByteArray tmp = buf.trimmed();
		int idx = tmp.indexOf(DIMENSION_TAG);
		if(-1 == idx)
		{
			//by default, it is two dimension coordinates
			return 2;
		}
		idx += DIMENSION_TAG.length();
		idx++;idx++;//skip "=" and "\""
		if('2' == tmp[idx])
		{
			return 2;
		}else if('3' == tmp[idx])
		{
			return 3;
		}else
		{
			qWarning() << "Unrecognized srs dimension. Default value 2 returned.";
			return 2;
		}
	}


	void
	srs_transform(
			const QString& from,
			const QString& to,
			unsigned dimension,
			QList<QByteArray>& points)
	{
		if(3 != dimension && 2 != dimension)
		{
			qWarning() << "Invalid dimension of points.";
		}
		if(points.size()%dimension != 0)
		{
			qWarning() << "The number of points is not correct. do nothing and return.";
			return;
		}
		unsigned p_num = points.size()/dimension;

		boost::scoped_ptr<double> x(new double[p_num]);
		boost::scoped_ptr<double> y(new double[p_num]);
		boost::scoped_ptr<double> z;
		if(3 == dimension)
		{
			z.reset(new double[p_num]);
		}

		for(int i=0; i<points.size(); i+=dimension)
		{
			bool x_f,y_f,z_f=true;
			x.get()[i] = points[i].toDouble(&x_f);
			y.get()[i] = points[i+1].toDouble(&y_f);
			if(3 == dimension)
			{
				z.get()[i] = points[i+2].toDouble(&z_f);
			}
			if(x_f && y_f && z_f)
			{
				continue;
			}else
			{
				throw QString("Error occurred during srs transormation.");
			}
		}

		OGRSpatialReference from_srs, to_srs;
	    OGRCoordinateTransformation *transform;
    
		from_srs.SetWellKnownGeogCS( from.toStdString().c_str() );
		to_srs.SetWellKnownGeogCS( to.toStdString().c_str());
		transform = OGRCreateCoordinateTransformation(&from_srs, &to_srs);
		if( transform == NULL )
		{
		   qDebug()<<"cannot create transform.";
		   return;
		}

		if( !transform->Transform( p_num, x.get(), y.get(), z.get() ) )
		{
			qDebug() << "Error occurred during Transform.";
			return;
		}
		points.clear();
		for(unsigned j=0; j<p_num; j++)
		{
			points.push_back(QString().setNum(x.get()[j]).toUtf8());
			points.push_back(QString().setNum(y.get()[j]).toUtf8());
			if(3 == dimension)
			{
				points.push_back(QString().setNum(z.get()[j]).toUtf8());
			}
		}
	}


	void
	convert_to_EPSG_4326(
			QByteArray& buf)
	{
		QString srs_name = get_srs_name(buf);
		if(is_EPSG_4326(srs_name))
		{
			//if it is already EPSG:4326, do nothing.
			return;
		}

		static const QString posList_begin = "<gml:posList";
		static const QString posList_end = "</gml:posList>";
		
		int idx = 0, idx_begin = 0, idx_end = 0;
		unsigned srs_dimension = 2;

		idx = buf.indexOf(posList_begin.toUtf8());
		while(idx != -1)
		{
			idx_begin = buf.indexOf(">", idx)+1;
			srs_dimension = find_srs_dimension(buf.mid(idx, idx_begin-idx));
			idx_end = buf.indexOf(posList_end.toUtf8(), idx_begin);

			QByteArray tail = buf.right(buf.size()-idx_end);
			QByteArray head = buf.left(idx_begin);
			QList<QByteArray> list = buf.mid(idx_begin, idx_end-idx_begin).simplified().split(' ');
			buf.clear();buf.append(head);

			srs_transform("EPSG:4326", srs_name, srs_dimension, list);

			for(int i=0; i<list.size();i++)
			{
				buf.append(" ");
				buf.append(list[i]);
			}

			idx_end = buf.length();
			buf.append(tail);
			idx_end = buf.indexOf(posList_end,idx_end) + posList_end.length();
			if(idx_end < posList_end.length())
			{
				qWarning() << "Cannot find </gml:posList>. The data might not be well-formed.";
				//TODO: throw exception.
				break;
			}
			idx = buf.indexOf(posList_begin.toUtf8(),idx_end);
		}
	}
}


void
GsmlPropertyHandlers::process_geometries(
		QBuffer& xml_data,
		const QString& query_str)
{
	QXmlQuery query;
	for(int i = 1; ;i++)
	{
		xml_data.reset();
		query.bindVariable("idx", QVariant(i));
		query.bindVariable("data_source", &xml_data);
		query.setQuery(query_str);
		QByteArray array;
		QBuffer buffer(&array);
		buffer.open(QIODevice::ReadWrite | QIODevice::Text);
		if(!buffer.isOpen())
		{
			qWarning() << "Cannot open buffer for output.";
			continue;
		}
		QXmlSerializer serializer(query, &buffer);
		if(query.evaluateTo(&serializer))
		{
			if(array.indexOf(QString("pos").toUtf8()) == -1)
			{
				qDebug() << "No more data.";
				break;
			}
			
			convert_to_EPSG_4326(array);
			normalize_geometry_coord(array);
			buffer.reset();
			
			//gplates doesn't support gml:outerBoundaryIs and gml:innerBoundaryIs
			//replace them with gml:exterior and gml:interior
			array.replace("outerBoundaryIs", "exterior");
			array.replace("innerBoundaryIs", "interior");

			QXmlStreamReader reader(&buffer);
			QXmlStreamReader::TokenType token_type = reader.readNext();
			while(QXmlStreamReader::StartElement != token_type)
			{
				token_type = reader.readNext();
			}
			
			boost::shared_ptr<GPlatesModel::XmlElementNode::AliasToNamespaceMap> alias_map(
					new GPlatesModel::XmlElementNode::AliasToNamespaceMap);

			boost::optional<GPlatesModel::PropertyValue::non_null_ptr_type> geometry_property = boost::none;

			if(query_str.indexOf("LineString") != -1)
			{
				geometry_property = 
						PropertyCreationUtils::create_line_string(
								GPlatesModel::XmlElementNode::create(reader,alias_map));
			}
			else if(query_str.indexOf("Polygon") != -1)
			{
				geometry_property = 
						PropertyCreationUtils::create_gml_polygon(
								GPlatesModel::XmlElementNode::create(reader,alias_map));
			}
			else if(query_str.indexOf("Point") != -1)
			{
				geometry_property = 
					PropertyCreationUtils::create_point(
								GPlatesModel::XmlElementNode::create(reader,alias_map));
			}

			if(!geometry_property)
			{
				qWarning() << "The geometry pointer is NULL.";
				continue;
			}
			
			GPlatesModel::TopLevelPropertyInline::non_null_ptr_type top_property = 
				GPlatesModel::TopLevelPropertyInline::create(
						GPlatesModel::PropertyName::create_gpml("GsmlGeometry"),
						*geometry_property);
			d_feature->add(top_property);
		}
		else
		{
			qWarning() << "Failed to evaluate data.";
			break;
		}
	}
}

void
GsmlPropertyHandlers::handle_geometry_property(
		QBuffer& xml_data)
{
	process_geometries(xml_data, line_string_query);
	process_geometries(xml_data, point_query);
	process_geometries(xml_data, polygon_query);
}

