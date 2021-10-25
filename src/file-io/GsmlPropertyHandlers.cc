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

#include <boost/foreach.hpp>
#include <boost/optional.hpp>
#include <QDebug>
#include <QXmlQuery>
#include <QXmlSerializer>
#include <QXmlStreamReader>
#include <QXmlResultItems>

#include "ArbitraryXmlReader.h"
#include "GsmlConst.h"
#include "GsmlNodeProcessorFactory.h"
#include "GsmlPropertyHandlers.h"
#include "GpmlPropertyStructuralTypeReaderUtils.h"

#include "global/LogException.h"

#include "model/Gpgim.h"
#include "model/ModelUtils.h"
#include "model/PropertyValue.h"
#include "model/XmlNode.h"

#include "property-values/CoordinateTransformation.h"
#include "property-values/GmlLineString.h"
#include "property-values/SpatialReferenceSystem.h"
#include "property-values/TextContent.h"
#include "property-values/UninterpretedPropertyValue.h"
#include "property-values/XsDouble.h"
#include "property-values/XsString.h"

#include "utils/XQueryUtils.h"


using namespace GPlatesFileIO::GsmlConst;
using namespace GPlatesPropertyValues;
using namespace GPlatesModel;
using namespace GPlatesUtils;

namespace
{
	const QString
	get_element_text(
			QBuffer& xml_data)
	{
		QXmlStreamReader reader(&xml_data);
		if(XQuery::next_start_element(reader))
		{
			return reader.readElementText();
		}
		return QString();
	}
	
	/*
	* Since EPSG:4326 coordinates system put longitude before latitude, which is opposite in gpml.
	* This function changes the order of lat-lon.
	* The output will replace the input buffer.
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

			if(idx_end == -1 || idx_begin == -1)
			{
				qWarning() << "The input data is not well-formed.";
				break;
			}

			QByteArray tail = buf.right(buf.size()-idx_end);
			QList<QByteArray> list = buf.mid(idx_begin, idx_end-idx_begin).simplified().split(' ');
			buf.chop(buf.size()-idx_begin);
			
			//swap the lat-lon pair.
			for(int i=0; i<list.size();i+=2)
			{
				buf.append(" ");
				buf.append(list[i+1]);
				buf.append(" ");
				buf.append(list[i]);
			}
			//append the rest of data.
			idx_end = buf.length();
			buf.append(tail);
			
			//move to next "<gml:posList>  </gml:posList>" block.
			idx_end = buf.indexOf(posList_end,idx_end) + posList_end.length();
			idx = buf.indexOf(posList_begin.toUtf8(),idx_end);
		}
	}

	/*
	* Get the Spatial Reference System name from xml data.
	*/
	const QString
	get_srs_name(
			QByteArray& array_buf)
	{
		std::vector<QVariant> results = 
			GPlatesUtils::XQuery::evaluate_attribute(array_buf,"srsName");

		std::size_t s = results.size();
		if( s >= 1)
		{
			if(s > 1)
			{
				qWarning() << "More than one srsName attributes have been found.";
				qWarning() << "Only the first one will be returned.";
			}
			return results[0].toString();
		}
		return QString();
	}

	/*
	* Check the input name and determine if it is EPSG_4326.
	*/
	inline
	bool
	is_epsg_4326(
			QString& name)
	{
		return (name.contains("EPSG",Qt::CaseInsensitive) && name.contains("4326"));
	}

	/*
	* Find the dimension of Spatial Reference System from xml data buffer.
	*/
	unsigned int
	find_srs_dimension(
			const QByteArray& buf)
	{
		QXmlStreamReader reader(buf);
		QXmlStreamAttributes attrs;
		while(XQuery::next_start_element(reader))
		{
			if(reader.name() == "posList")
			{
				attrs = reader.attributes();
				break;
			}
		}

		BOOST_FOREACH(const QXmlStreamAttribute& attr, attrs)
		{
			if(
				attr.name().toString() == "srsDimension" && 
				attr.value().toString() == "3")
				{
					return 3;
				}
		}
		return 2;
	}

	/*
	* Transform data into EPSG 4326.
	* Output data will replace input buffer.
	*/
	void
	convert_to_epsg_4326(
			QByteArray& buf)
	{
		QString srs_name = get_srs_name(buf);
		
		if(srs_name.size() == 0)
		{
			// qWarning() << "No Spatial Reference System name found. Use default EPSG:4326.";
			srs_name = "EPSG:4326";
		}
		else if(is_epsg_4326(srs_name))
		{
			// if it is already EPSG:4326, do nothing.
			// qDebug() << "The Spatial Reference System is already EPSG:4326. Do nothing and return";
			return;
		}

		static const QString posList_begin = "<gml:posList";
		static const QString posList_end = "</gml:posList>";
		
		int idx = 0, idx_begin = 0, idx_end = 0;
		unsigned int srs_dimension = 2; //by default, 2D

		idx = buf.indexOf(posList_begin.toUtf8());
		while(idx != -1)
		{
			idx_begin = buf.indexOf(">", idx) + 1;
			srs_dimension = find_srs_dimension(buf.mid(idx, idx_begin-idx));
			idx_end = buf.indexOf(posList_end.toUtf8(), idx_begin);

			if(idx_end == -1)
			{
				qWarning() << "The XML data is not well-formedin convert_to_EPSG_4326().";
				break;
			}

			QByteArray tail = buf.right(buf.size()-idx_end);
			QByteArray head = buf.left(idx_begin);
			QList<QByteArray> list = buf.mid(idx_begin, idx_end-idx_begin).simplified().split(' ');
			buf.clear();buf.append(head);

			std::vector<GPlatesPropertyValues::CoordinateTransformation::Coord> coordinates;
			QList<QByteArray>::const_iterator it = list.begin();
			QList<QByteArray>::const_iterator it_end = list.end();
			for(;it != it_end;)
			{
				//TODO:
				//validate the QList
				//check the result flag of toDouble()
				//bool f; toDouble(&f)
				const double x = (*it).toDouble();
				++it;
				const double y = (*it).toDouble();
				++it;
				boost::optional<double> z;
				if (srs_dimension == 3)
				{
					z = (*it).toDouble();
					++it;
				}
				GPlatesPropertyValues::CoordinateTransformation::Coord coord(x, y, z);
				coordinates.push_back(coord);
			}

#if 0
			OGRSpatialReference from_srs;
			from_srs.SetWellKnownGeogCS(srs_name.toStdString().c_str());
			boost::optional<GPlatesPropertyValues::CoordinateTransformation::non_null_ptr_type>
					coordinate_transformation = GPlatesPropertyValues::CoordinateTransformation::create(
							GPlatesPropertyValues::SpatialReferenceSystem::create(from_srs));
			if (coordinate_transformation)
			{
				if (!coordinate_transformation.get()->transform_in_place(coordinates))
				{
					//TODO:
					//throw exception
					qWarning() << "Error occurred during Transform.";
				}
			}
			else
			{
				//TODO:
				//throw exception
				qWarning() << "cannot create transform.";
			}
#endif

			list.clear();
			std::vector<GPlatesPropertyValues::CoordinateTransformation::Coord>::const_iterator c_it = coordinates.begin();
			std::vector<GPlatesPropertyValues::CoordinateTransformation::Coord>::const_iterator c_it_end = coordinates.end();
			for(;c_it != c_it_end; ++c_it)
			{
				buf.append(" ");
				buf.append(QByteArray().setNum(c_it->x));
				buf.append(" ");
				buf.append(QByteArray().setNum(c_it->y));
			}

			idx_end = buf.length();
			buf.append(tail);
		
			idx_end = buf.indexOf(posList_end,idx_end) + posList_end.length();
			idx = buf.indexOf(posList_begin.toUtf8(),idx_end);
		}
	}

	/*
	* Create XmlElementNode from QBuffer
	*/
	GPlatesModel::XmlElementNode::non_null_ptr_type
	create_xml_node(
			QBuffer& buf)
	{
		QXmlStreamReader reader(&buf);
		XQuery::next_start_element(reader);

		boost::shared_ptr<XmlElementNode::AliasToNamespaceMap> alias_map(
			new XmlElementNode::AliasToNamespaceMap());

		return XmlElementNode::create(reader,alias_map);
	}

	/*
	* Create XmlElementNode from QByteArray
	*/
	GPlatesModel::XmlElementNode::non_null_ptr_type
	create_xml_node(
			QByteArray& array)
	{
		QBuffer buffer(&array);
		buffer.open(QIODevice::ReadOnly | QIODevice::Text);
		if(!buffer.isOpen())
		{
			throw GPlatesGlobal::LogException(
					GPLATES_EXCEPTION_SOURCE,	
					"Unable to open buffer.");
		}
		return create_xml_node(buffer);
	}
}


GPlatesFileIO::GsmlPropertyHandlers::GsmlPropertyHandlers(
		GPlatesModel::FeatureHandle::weak_ref fh):
	d_feature(fh)
{
	d_gpgim = ArbitraryXmlReader::instance()->get_gpgim();
	d_read_errors = ArbitraryXmlReader::instance()->get_read_error_accumulation();
}


void
GPlatesFileIO::GsmlPropertyHandlers::process_geometries(
		QBuffer& xml_data,
		const QString& query_str)
{
	QByteArray buf_array = xml_data.data();

#if 0
qDebug() << "======================================================================";
qDebug() << "GPlatesFileIO::GsmlPropertyHandlers::process_geometries() buf_array = " << buf_array;
qDebug() << "======================================================================";
#endif

	std::vector<QByteArray> results = 
		XQuery::evaluate_query(
				buf_array,
				query_str);
	
// qDebug() << "GPlatesFileIO::GsmlPropertyHandlers::process_geometries() results.size() = " << results.size();

	BOOST_FOREACH(QByteArray& array, results)
	{
		// GPlates doesn't support gml:outerBoundaryIs and gml:innerBoundaryIs
		// replace them with gml:exterior and gml:interior
		array.replace("outerBoundaryIs", "exterior");
		array.replace("innerBoundaryIs", "interior");

		boost::optional<PropertyValue::non_null_ptr_type> geometry_property = boost::none;

		if(query_str.indexOf("Point") != -1)
		{
			XQuery::wrap_xml_data(array,"gpml:position");

			convert_to_epsg_4326(array);
			normalize_geometry_coord(array);

			XmlElementNode::non_null_ptr_type xml_node = create_xml_node(array);

			geometry_property = 
				GpmlPropertyStructuralTypeReaderUtils::create_gml_point(
					xml_node,
					// Read using current GPGIM version (it's not GPML so it won't change format anyway)...
					d_gpgim->get_version(),
					*d_read_errors);

			ModelUtils::add_property(
				d_feature,
				PropertyName::create_gpml("position"),
				*geometry_property);
		}
		else if(query_str.indexOf("LineString") != -1)
		{
			// need to reorder the xml nesting 
			XQuery::wrap_xml_data(array,"gml:baseCurve");

			convert_to_epsg_4326(array);
			normalize_geometry_coord(array);

			XmlElementNode::non_null_ptr_type xml_node = create_xml_node(array);

			GPlatesPropertyValues::GmlLineString::non_null_ptr_type gml_line_string =
				GpmlPropertyStructuralTypeReaderUtils::create_gml_line_string(
					xml_node,
					// Read using current GPGIM version (it's not GPML so it won't change format anyway)...
					d_gpgim->get_version(),
					*d_read_errors);

            GPlatesPropertyValues::GmlOrientableCurve::non_null_ptr_type gml_orientable_curve =
                    GPlatesModel::ModelUtils::create_gml_orientable_curve(gml_line_string);

            GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type property_value =
                    GPlatesModel::ModelUtils::create_gpml_constant_value(gml_orientable_curve);

			d_feature->add(
				GPlatesModel::TopLevelPropertyInline::create(
				PropertyName::create_gpml("centerLineOf"),
				property_value)
			);
		}
		else if(query_str.indexOf("Polygon") != -1)
		{
			// need to reorder the xml nesting to match gpml
			array.replace("Polygon", "LinearRing");
			XQuery::wrap_xml_data(array,"gml:exterior");
			XQuery::wrap_xml_data(array,"gml:Polygon");
			XQuery::wrap_xml_data(array,"gpml:ConstantValue");

			convert_to_epsg_4326(array);
			normalize_geometry_coord(array);
			
			XmlElementNode::non_null_ptr_type xml_node = create_xml_node(array);

			GPlatesPropertyValues::GmlPolygon::non_null_ptr_type gml_polygon = 
				GpmlPropertyStructuralTypeReaderUtils::create_gml_polygon(
					xml_node,
					// Read using current GPGIM version (it's not GPML so it won't change format anyway)...
					d_gpgim->get_version(),
					*d_read_errors);

            GPlatesPropertyValues::GpmlConstantValue::non_null_ptr_type property_value =
                    GPlatesModel::ModelUtils::create_gpml_constant_value(gml_polygon);

			d_feature->add(
				GPlatesModel::TopLevelPropertyInline::create(
				PropertyName::create_gpml("outlineOf"),
				property_value)
			);


		}
		else
		{
			qDebug() << "Unknown Geometry type?";
		}

#if 0
// FIXME: do we need to do this? 

		// Add the geom as a GsmlGeometry prop
		if(!geometry_property)
		{
			qWarning() << "Failed to create geometry property.";
		}
		else
		{
			ModelUtils::add_property(
					d_feature,
					PropertyName::create_gpml("GsmlGeometry"),
					*geometry_property);
		}
#endif

	} // end of loop over results
}


// used to process GeometryProperty
void
GPlatesFileIO::GsmlPropertyHandlers::handle_geometry_property(
		QBuffer& xml_data)
{
	// keep the query strings locally for better readability.
	process_geometries(xml_data, "/gsml:shape/gml:Point");
	process_geometries(xml_data, "/gsml:shape/gml:LineString");
	process_geometries(xml_data, "/gsml:shape/gml:Polygon");
}


void
GPlatesFileIO::GsmlPropertyHandlers::handle_observation_method(
		QBuffer& xml_data)
{
	ModelUtils::add_property(
			d_feature,
			PropertyName::create_gpml("ObservationMethod"),
			UninterpretedPropertyValue::create(create_xml_node(xml_data)));
}


void
GPlatesFileIO::GsmlPropertyHandlers::handle_gml_name(
		QBuffer& xml_data)
{
	ModelUtils::add_property(
			d_feature,
			PropertyName::create_gml("name"),
			XsString::create(UnicodeString(get_element_text(xml_data))));
}


void
GPlatesFileIO::GsmlPropertyHandlers::handle_gml_desc(
		QBuffer& xml_data)
{
	ModelUtils::add_property(
			d_feature,
			PropertyName::create_gml("description"),
			XsString::create(UnicodeString(get_element_text(xml_data))));
}


void
GPlatesFileIO::GsmlPropertyHandlers::handle_occurrence_property(
		QBuffer& xml_data)
{
#if 0
	std::vector<QByteArray> results = 
		XQuery::evaluate(
				xml_data,
				"/gsml:occurrence/gsml:MappedFeature/gsml:shape",
				boost::bind(&XQuery::is_empty,_1));
#endif

	std::vector<QByteArray> results = 
		XQuery::evaluate_query(
				xml_data,
				"/gsml:occurrence/gsml:MappedFeature/gsml:shape");

	BOOST_FOREACH(QByteArray& array, results)
	{
		QBuffer buffer(&array);
		buffer.open(QIODevice::ReadOnly | QIODevice::Text);
		if(!buffer.isOpen())
		{
			throw GPlatesGlobal::LogException(
				GPLATES_EXCEPTION_SOURCE,	
				"Unable to open buffer.");
		}
		handle_geometry_property(buffer);
	}
}



void
GPlatesFileIO::GsmlPropertyHandlers::handle_gml_valid_time(
		QBuffer& xml_data)
{
// FIXME: remove; used for reference :
#if 0
<gml:validTime>
<gml:TimePeriod>
<gml:begin>
<gml:TimeInstant>
<gml:timePosition gml:frame="http://gplates.org/TRS/flat">22
</gml:timePosition> 
</gml:TimeInstant> 
</gml:begin> 
<gml:end> 
<gml:TimeInstant> 
<gml:timePosition gml:frame="http://gplates.org/TRS/flat">0</gml:timePosition> 
</gml:TimeInstant> 
</gml:end> 
</gml:TimePeriod> 
</gml:validTime>
#endif

	QXmlStreamReader reader(&xml_data);

	QString b = "";
	QString e = "";
	bool get_begin = true;

	while(reader.readNext() != QXmlStreamReader::Invalid) 
	{
		if( reader.tokenString() == "StartElement")
		{
			if(reader.name() == "begin") { get_begin = true; }
			else if( reader.name() == "end") { get_begin = false; }
		}

		if (reader.name() == "timePosition")
		{
			reader.readNext();
			if( get_begin )
			{
				b.append( reader.text() );
			}
			else
			{
				e.append( reader.text() );
			}
		}
	}

	b = b.trimmed();
	e = e.trimmed();

	const double begin 	= b.toDouble();
	const double end 	= e.toDouble();
	
	GmlTimePeriod::non_null_ptr_type gml_valid_time =
		ModelUtils::create_gml_time_period( 
			GPlatesPropertyValues::GeoTimeInstant( begin ),
			GPlatesPropertyValues::GeoTimeInstant( end )
		);

	ModelUtils::add_property(
		d_feature,
		PropertyName::create_gml("validTime"),
		gml_valid_time);
}


// FIXME: remove?  
// this was as test to see how to pass data...maybe not be needed
void
GPlatesFileIO::GsmlPropertyHandlers::handle_gpml_valid_time_range(
		QBuffer& xml_data)
{
	QString range = get_element_text(xml_data);
	QStringList l = range.split(",");
	QString b = l[0];
	QString e = l[1];

	const double begin 	= b.toDouble();
	const double end 	= e.toDouble();
	
	GmlTimePeriod::non_null_ptr_type gml_valid_time =
		ModelUtils::create_gml_time_period( 
			GPlatesPropertyValues::GeoTimeInstant( begin ),
			GPlatesPropertyValues::GeoTimeInstant( end )
		);

	d_feature->add(
		TopLevelPropertyInline::create( 
			PropertyName::create_gml("validTime"), 
			gml_valid_time) );

	// above call is equivalent to: as used elsewhere
	//ModelUtils::add_property(
	//	d_feature,
	//	PropertyName::create_gml("validTime"),
	//	gml_valid_time);
}

void
GPlatesFileIO::GsmlPropertyHandlers::handle_gpml_rock_type(
		QBuffer& xml_data)
{
	ModelUtils::add_property(
			d_feature,
			PropertyName::create_gpml("rock_type"),
			XsString::create(UnicodeString(get_element_text(xml_data))));
}


void
GPlatesFileIO::GsmlPropertyHandlers::handle_gpml_rock_max_thick(
		QBuffer& xml_data)
{
	ModelUtils::add_property(
			d_feature,
			PropertyName::create_gpml("rock_min_thick"),
			XsDouble::create(get_element_text(xml_data).toDouble()) 
	);


}

void
GPlatesFileIO::GsmlPropertyHandlers::handle_gpml_rock_min_thick(
		QBuffer& xml_data)
{
	ModelUtils::add_property(
			d_feature,
			PropertyName::create_gpml("rock_min_thick"),
			XsDouble::create(get_element_text(xml_data).toDouble()) 
	);
}

void
GPlatesFileIO::GsmlPropertyHandlers::handle_gpml_fossil_diversity(
		QBuffer& xml_data)
{
	ModelUtils::add_property(
			d_feature,
			PropertyName::create_gpml("fossil_diversity"),
			XsDouble::create(get_element_text(xml_data).toDouble()) 
	);
}

