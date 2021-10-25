/* $Id:  $ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date: 2010-09-06 04:45:52 +1000 (Mon, 06 Sep 2010) $
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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
#include <QBuffer>
#include <QIODevice>
#include <QTextStream>
#include "Metadata.h"

const QString GPlatesModel::FeatureCollectionMetadata::DC_NAMESPACE = "http://purl.org/dc/elements/1.1/";
const QString GPlatesModel::FeatureCollectionMetadata::GPML_NAMESPACE = "http://www.gplates.org/gplates";

GPlatesModel::FeatureCollectionMetadata::FeatureCollectionMetadata(
		const GPlatesModel::XmlElementNode::non_null_ptr_type elem)
{
	init();
	QString buf;
	QXmlStreamWriter writer(&buf);
	elem->write_to(writer);
	QXmlStreamReader reader(buf);

	GPlatesUtils::XQuery::next_start_element(reader);

	if(qualified_name(reader) != "gpml:metadata")
	{
		qWarning() << QString("Expecting xml element gpml:metadata, but got %1.").arg(
			qualified_name(reader));
		return;
	}
	
	GPlatesUtils::XQuery::next_start_element(reader);
	if(qualified_name(reader) != "gpml:GpmlMetadata")
	{
		qWarning() << QString("Expecting xml element gpml:GpmlMetadata, but got %1.").arg(
			qualified_name(reader));
	}else
	{
		process_complex_xml_element(reader);
	}
	return;
}


void
GPlatesModel::FeatureCollectionMetadata::process_complex_xml_element(
		QXmlStreamReader& reader)
{
	GPlatesUtils::XQuery::next_start_element(reader);
	while(!reader.atEnd())
	{
		if(!reader.isStartElement())
		{
			GPlatesUtils::XQuery::next_start_element(reader);
			continue;
		}
		//qWarning() << "processing " << qualified_name(reader);
		XMLFuncMap::iterator it = d_xml_func_map.find(qualified_name(reader));
		if(it != d_xml_func_map.end())
		{
			(this->*(it->second))(reader);
		}
		else
		{
			qWarning() <<  "Unrecognised element: " <<  qualified_name(reader);
			GPlatesUtils::XQuery::next_start_element(reader);
			continue;
		}
	}
}


void
GPlatesModel::FeatureCollectionMetadata::process_dc_creator(
		QXmlStreamReader& reader)
{
	set_dc_creator(reader.readElementText());
	GPlatesUtils::XQuery::next_start_element(reader);
}


void
GPlatesModel::FeatureCollectionMetadata::process_dc_rights(
		QXmlStreamReader& reader)
{
	GPlatesUtils::XQuery::next_start_element(reader);
	set_data("dc:license", reader, &FeatureCollectionMetadata::set_dc_rights_license);
	GPlatesUtils::XQuery::next_start_element(reader);
	set_data("dc:url", reader, &FeatureCollectionMetadata::set_dc_rights_url);
	while(!GPlatesUtils::XQuery::next_start_element(reader)){}
}


void
GPlatesModel::FeatureCollectionMetadata::process_dc_date(
		QXmlStreamReader& reader)
{
	GPlatesUtils::XQuery::next_start_element(reader);
	set_data("dc:created", reader, &FeatureCollectionMetadata::set_dc_date_created);
	while(GPlatesUtils::XQuery::next_start_element(reader) && (qualified_name(reader) == "dc:modified"))
	{
		set_data("dc:modified", reader, &FeatureCollectionMetadata::set_dc_date_modified);
	}
	while(!GPlatesUtils::XQuery::next_start_element(reader)){}
}


void
GPlatesModel::FeatureCollectionMetadata::process_gpml_meta(
		QXmlStreamReader& reader)
{
	QXmlStreamAttributes attr =	reader.attributes(); 
	QStringRef name =attr.value("name");
	
	FuncMap::iterator it = d_meta_func.find(name.toString());
	if(it != d_meta_func.end())
	{
		(this->*(it->second))(reader.readElementText());
	}
	else
	{
		qWarning() << "Unexpected attribute name: " << name.toString();
	}
	GPlatesUtils::XQuery::next_start_element(reader);
}


void
GPlatesModel::FeatureCollectionMetadata::process_dc_coverage(
		QXmlStreamReader& reader)
{
	GPlatesUtils::XQuery::next_start_element(reader);
	set_data("dc:temporal", reader, &FeatureCollectionMetadata::set_dc_coverage_temporal);
	while(!GPlatesUtils::XQuery::next_start_element(reader)){}
}


GPlatesModel::FeatureCollectionMetadata::FeatureCollectionMetadata()
{
	init();
}


void
GPlatesModel::FeatureCollectionMetadata::init()
{
	d_meta_func["GPLATESROTATIONFILE:version"] = &FeatureCollectionMetadata::set_version;
	d_meta_func["GPLATESROTATIONFILE:documentation"] = &FeatureCollectionMetadata::set_documentation;
	d_meta_func["DC:namespace"] = &FeatureCollectionMetadata::set_dc_namespace;
	d_meta_func["DC:title"] = &FeatureCollectionMetadata::set_dc_title;
	d_meta_func["DC:creator"] = &FeatureCollectionMetadata::set_dc_creator;
	d_meta_func["DC:rights:license"] = &FeatureCollectionMetadata::set_dc_rights_license;
	d_meta_func["DC:rights:url"] = &FeatureCollectionMetadata::set_dc_rights_url;
	d_meta_func["DC:date:created"] = &FeatureCollectionMetadata::set_dc_date_created;
	d_meta_func["DC:date:modified"] = &FeatureCollectionMetadata::set_dc_date_modified;
	d_meta_func["DC:coverage:temporal"] = &FeatureCollectionMetadata::set_dc_coverage_temporal;
	d_meta_func["DC:bibliographicCitation"] = &FeatureCollectionMetadata::set_dc_bibliographicCitation;
	d_meta_func["DC:description"] = &FeatureCollectionMetadata::set_dc_description;
	d_meta_func["DC:contributor"] = &FeatureCollectionMetadata::set_dc_contributor;
	d_meta_func["BIBINFO:bibfile"] =  &FeatureCollectionMetadata::set_dc_bibinfo_bibfile;
	d_meta_func["BIBINFO:doibase"]= &FeatureCollectionMetadata::set_dc_bibinfo_doibase ;
	d_meta_func["GPML:namespace"] = &FeatureCollectionMetadata::set_gpml_namespace;
	d_meta_func["GEOTIMESCALE"] = &FeatureCollectionMetadata::set_geotimescale;
	d_meta_func["REVISIONHIST"] = &FeatureCollectionMetadata::set_dc_revision_history;

	d_xml_func_map["gpml:dublinCoreMeta"] = &FeatureCollectionMetadata::process_complex_xml_element;
	d_xml_func_map["gpml:meta"] = &FeatureCollectionMetadata::process_gpml_meta;
	d_xml_func_map["dc:creator"] = &FeatureCollectionMetadata::process_dc_creator;
	d_xml_func_map["dc:rights"] = &FeatureCollectionMetadata::process_dc_rights;
	d_xml_func_map["dc:date"] = &FeatureCollectionMetadata::process_dc_date;
	d_xml_func_map["dc:coverage"] = &FeatureCollectionMetadata::process_dc_coverage;
	d_xml_func_map["dc:namespace"] = &FeatureCollectionMetadata::process_dc_namespace;
	d_xml_func_map["dc:title"] = &FeatureCollectionMetadata::process_dc_title;
	d_xml_func_map["dc:bibliographicCitation"] = &FeatureCollectionMetadata::process_dc_bibliographicCitation;
	d_xml_func_map["dc:description"] = &FeatureCollectionMetadata::process_dc_description;
	d_xml_func_map["dc:contributor"] = &FeatureCollectionMetadata::process_dc_contributor;

	d_recurring_data.insert("DC:creator:affiliation");
	d_recurring_data.insert("GEOTIMESCALE");
	d_recurring_data.insert("DC:contributor");
}


bool
GPlatesModel::FeatureCollectionMetadata::set_metadata(
		const QString& name,
		const QString& value)
{
	QString simplied_name(name.simplified()); 
	if(is_fc_metadata(simplied_name))
	{
		(this->*d_meta_func[simplied_name])(value);
		return true;
	}
	else
	{
		return false;
	}
}


std::multimap<QString, QString>
GPlatesModel::FeatureCollectionMetadata::get_metadata_as_map() const 
{
	std::multimap<QString, QString> ret;
	ret.insert(std::pair<QString, QString>("GPLATESROTATIONFILE:version",HeaderMetadata.GPLATESROTATIONFILE_version));
	ret.insert(std::pair<QString, QString>("GPLATESROTATIONFILE:documentation",HeaderMetadata.GPLATESROTATIONFILE_documentation));
	ret.insert(std::pair<QString, QString>("DC:namespace", DC.dc_namespace));
	ret.insert(std::pair<QString, QString>("DC:title", DC.title));
	
	BOOST_FOREACH(const DublinCoreMetadata::Creator& creator, DC.creators)
	{
		ret.insert(std::pair<QString, QString>("DC:creator", creator.to_string()));
	}
	ret.insert(std::pair<QString, QString>("DC:rights:license", DC.rights.license));
	ret.insert(std::pair<QString, QString>("DC:rights:url", DC.rights.url));
	ret.insert(std::pair<QString, QString>("DC:date:created", DC.date.created));
	typedef boost::shared_ptr<QString> qstring_shared_ptr;
	BOOST_FOREACH(qstring_shared_ptr date_m, DC.date.modified)
	{
		ret.insert(std::pair<QString, QString>("DC:date:modified", *date_m));
	}
	ret.insert(std::pair<QString, QString>("DC:coverage:temporal", DC.coverage.temporal));
	ret.insert(std::pair<QString, QString>("DC:bibliographicCitation", DC.bibliographicCitation));
	ret.insert(std::pair<QString, QString>("DC:description", DC.description));
	BOOST_FOREACH(const DublinCoreMetadata::Contributor& contri, DC.contributors)
	{
		ret.insert(std::pair<QString, QString>("DC:contributor", contri.to_string()));;
	}
	ret.insert(std::pair<QString, QString>("BIBINFO:bibfile", BIBINFO.bibfile));
	ret.insert(std::pair<QString, QString>("BIBINFO:doibase", BIBINFO.doibase)) ;
	ret.insert(std::pair<QString, QString>("GPML:namespace", HeaderMetadata.GPML_namespace));
	BOOST_FOREACH(const GeoTimeScale& time_scale, GEOTIMESCALE)
	{
		ret.insert(std::pair<QString, QString>("GEOTIMESCALE", time_scale.to_string()));
	}
	BOOST_FOREACH(qstring_shared_ptr his, HeaderMetadata.REVISIONHIST)
	{
		ret.insert(std::pair<QString, QString>("REVISIONHIST", *his));
	}
	return ret;
}


QString
GPlatesModel::FeatureCollectionMetadata::to_xml() const
{
	QByteArray byte_array;
	QBuffer buffer(&byte_array);
	GPlatesFileIO::XmlWriter writer(&buffer);
	serialize(writer);
	return QString(byte_array);
}

void
GPlatesModel::FeatureCollectionMetadata::serialize(
		GPlatesFileIO::XmlWriter& writer) const
{
	typedef boost::shared_ptr<QString> qstring_shared_ptr;
	QXmlStreamWriter& q_writer = writer.get_writer();
	q_writer.writeStartElement("gpml:GpmlMetadata");
	q_writer.writeNamespace("http://purl.org/dc/elements/1.1/", "dc");

	q_writer.writeStartElement("gpml:dublinCoreMeta");
	q_writer.writeStartElement("dc:namespace");
	q_writer.writeCharacters(DC.dc_namespace);
	q_writer.writeEndElement();
	q_writer.writeTextElement("dc:title", DC.title);

	BOOST_FOREACH(const DublinCoreMetadata::Creator& creator, DC.creators)
	{
		q_writer.writeTextElement("dc:creator", creator.to_string());
	}
	
	q_writer.writeStartElement("dc:rights");
	q_writer.writeTextElement("dc:license", DC.rights.license);
	q_writer.writeTextElement("dc:url", DC.rights.url);
	q_writer.writeEndElement();
	q_writer.writeStartElement("dc:date");
	q_writer.writeTextElement("dc:created", DC.date.created);
	BOOST_FOREACH(qstring_shared_ptr date_m, DC.date.modified)
	{
		q_writer.writeTextElement("dc:modified", *date_m); 
	}
	q_writer.writeEndElement();
	q_writer.writeStartElement("dc:coverage");
	q_writer.writeTextElement("dc:temporal", DC.coverage.temporal);
	q_writer.writeEndElement();
	q_writer.writeTextElement("dc:bibliographicCitation", DC.bibliographicCitation);
	q_writer.writeTextElement("dc:description", DC.description);
	BOOST_FOREACH(const DublinCoreMetadata::Contributor& contri, DC.contributors)
	{
		q_writer.writeTextElement("dc:contributor", contri.to_string()); 
	}
	q_writer.writeEndElement();

	BOOST_FOREACH(qstring_shared_ptr his,HeaderMetadata.REVISIONHIST)
	{
		q_writer.writeStartElement("gpml:meta");
		q_writer.writeAttribute("name","REVISIONHIST");
		q_writer.writeCharacters(*his);
		q_writer.writeEndElement();
	}
	q_writer.writeStartElement("gpml:meta");
	q_writer.writeAttribute("name","GPLATESROTATIONFILE:version");
	q_writer.writeCharacters(HeaderMetadata.GPLATESROTATIONFILE_version);
	q_writer.writeEndElement();
	q_writer.writeStartElement("gpml:meta");
	q_writer.writeAttribute("name","GPLATESROTATIONFILE:documentation");
	q_writer.writeCharacters(HeaderMetadata.GPLATESROTATIONFILE_documentation);
	q_writer.writeEndElement();
	q_writer.writeStartElement("gpml:meta");
	q_writer.writeAttribute("name","BIBINFO:bibfile");
	q_writer.writeCharacters(BIBINFO.bibfile);
	q_writer.writeEndElement();
	q_writer.writeStartElement("gpml:meta");
	q_writer.writeAttribute("name","BIBINFO:doibase");
	q_writer.writeCharacters(BIBINFO.doibase);
	q_writer.writeEndElement();
	q_writer.writeStartElement("gpml:meta");
	q_writer.writeAttribute("name", "GPML:namespace");
	q_writer.writeCharacters(HeaderMetadata.GPML_namespace);
	q_writer.writeEndElement();
	BOOST_FOREACH(const GeoTimeScale& time_scale, GEOTIMESCALE)
	{
		q_writer.writeStartElement("gpml:meta");
		q_writer.writeAttribute("name","GEOTIMESCALE");
		q_writer.writeCharacters(time_scale.to_string());
		q_writer.writeEndElement();
	}
	q_writer.writeEndElement();
}


inline
const QString
create_attr_str(
		const QString& name,
		const QString& val)
{
	const QString prefix_at("@");
	QString sep = "\"";
	if(val.contains("\n"))
	{
		sep = "\"\"\"";
	}
	return prefix_at + name + sep + val + sep + "\n";
}

void
GPlatesModel::FeatureCollectionMetadata::serialize(
		QString& buffer) const
{
	typedef boost::shared_ptr<QString> qstring_shared_ptr;
	BOOST_FOREACH(qstring_shared_ptr his, HeaderMetadata.REVISIONHIST)
	{
		buffer += create_attr_str("REVISIONHIST", *his);
	}
	buffer += create_attr_str("GPLATESROTATIONFILE:version", 
		HeaderMetadata.GPLATESROTATIONFILE_version);
	buffer += create_attr_str("GPLATESROTATIONFILE:documentation", 
		HeaderMetadata.GPLATESROTATIONFILE_documentation);
	buffer += create_attr_str("GPML:namespace", HeaderMetadata.GPML_namespace);

	buffer += create_attr_str("DC:namespace", DC.dc_namespace);
	buffer += create_attr_str("DC:title", DC.title);
	
	BOOST_FOREACH(const DublinCoreMetadata::Creator& creator, DC.creators)
	{
		buffer += create_attr_str("DC:creator", creator.to_string());
	}

	buffer += create_attr_str("DC:rights:license", DC.rights.license);
	buffer += create_attr_str("DC:rights:url", DC.rights.url);

	buffer += create_attr_str("DC:date:created", DC.date.created);
	BOOST_FOREACH(qstring_shared_ptr date_m, DC.date.modified)
	{
		buffer += create_attr_str("DC:date:modified", *date_m);
	}
	
	buffer += create_attr_str("DC:coverage:temporal", DC.coverage.temporal);
	
	buffer += create_attr_str("DC:bibliographicCitation", DC.bibliographicCitation);
	buffer += create_attr_str("DC:description", DC.description);

	BOOST_FOREACH(const DublinCoreMetadata::Contributor& contri, DC.contributors)
	{
		buffer += create_attr_str("DC:contributor", contri.to_string());
	}
	
	buffer += create_attr_str("BIBINFO:bibfile", BIBINFO.bibfile);
	buffer += create_attr_str("BIBINFO:doibase", BIBINFO.doibase);
	

	
	BOOST_FOREACH(const GeoTimeScale& time_scale, GEOTIMESCALE)
	{
		buffer += create_attr_str("GEOTIMESCALE", time_scale.to_string());
	}
}





