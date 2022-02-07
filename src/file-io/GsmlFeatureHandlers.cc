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
#include <boost/bind.hpp>
#include <QBuffer>

#include "GsmlFeatureHandlers.h"
#include "GsmlPropertyHandlers.h"
#include "GsmlNodeProcessorFactory.h"

#include "global/LogException.h"
#include "utils/XQueryUtils.h"


using namespace GPlatesUtils;
using namespace GPlatesModel;

void
GPlatesFileIO::GsmlFeatureHandler::handle_gsml_feature(
		const QString& feature_type_str,
		FeatureCollectionHandle::weak_ref fc,
		QBuffer& xml_data)
{
	if ( feature_type_str == "UnclassifiedFeature")
	{
		FeatureHandle::weak_ref feature = FeatureHandle::create(
			fc,
			FeatureType(PropertyName::create_gpml( feature_type_str )));

		GsmlNodeProcessorFactory(feature).process_with_property_processors(
			feature_type_str, 
			xml_data);
	}
	else if ( feature_type_str.startsWith("RockUnit_") )
	{
		FeatureHandle::weak_ref feature = FeatureHandle::create(
			fc,
			FeatureType(PropertyName::create_gpml( feature_type_str )));

		GsmlNodeProcessorFactory(feature).process_with_property_processors(
			feature_type_str, 
			xml_data);
	}
	else if ( feature_type_str.startsWith("FossilCollection_") )
	{
		FeatureHandle::weak_ref feature = FeatureHandle::create(
			fc,
			FeatureType(PropertyName::create_gpml( feature_type_str )));

		GsmlNodeProcessorFactory(feature).process_with_property_processors(
			feature_type_str, 
			xml_data);
	}
	else
	{
		FeatureHandle::weak_ref feature = FeatureHandle::create(
			fc,
			FeatureType(PropertyName::create_gml( feature_type_str )));

		GsmlNodeProcessorFactory(feature).process_with_property_processors(
			feature_type_str, 
			xml_data);
	}

	return;
}

void
GPlatesFileIO::GsmlFeatureHandler::handle_feature_member(
		FeatureCollectionHandle::weak_ref fc,
		QByteArray& xml_data)
{
	QBuffer buffer(&xml_data);
	buffer.open(QIODevice::ReadOnly | QIODevice::Text);
	if(!buffer.isOpen())
	{
		throw GPlatesGlobal::LogException(
			GPLATES_EXCEPTION_SOURCE,	
			"Unable to open buffer.");
	}
	QXmlStreamReader reader(&buffer);

	//gml:featureMember
	XQuery::next_start_element(reader);

	//will give: 'gsml:MappedFeature', 'gpml:RockUnit_siliciclastic', etc.
	XQuery::next_start_element(reader);

	QString feature_type = reader.name().toString();

#if 0
qDebug() << "GsmlFeatureHandler::handle_feature_member(): feature_type" << feature_type;
qDebug() << "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
qDebug() << xml_data;
qDebug() << "XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX";
#endif

	std::vector<QByteArray> results;

	if ( feature_type.startsWith("UnclassifiedFeature") )
	{
		results = XQuery::evaluate_query(
				xml_data,
				"//gsml:" + feature_type);
	}
	else if ( feature_type.startsWith("RockUnit_") )
	{
		results = XQuery::evaluate_query(
				xml_data,
				"//gpml:" + feature_type);
	}
	else if ( feature_type.startsWith("FossilCollection_") )
	{
		results = XQuery::evaluate_query(
				xml_data,
				"//gpml:" + feature_type);
	}

	if(results.size() != 1)
	{
		throw GPlatesGlobal::LogException(
			GPLATES_EXCEPTION_SOURCE,	
			"The number of feature is not 1. We are expecting one and only one feature here.");
	}

	QBuffer buf(&results[0]);
	buf.open(QIODevice::ReadOnly | QIODevice::Text);
	if(!buf.isOpen())
	{
		throw GPlatesGlobal::LogException(
			GPLATES_EXCEPTION_SOURCE,	
			"Unable to open buffer.");
	}

	handle_gsml_feature( feature_type, fc, buf);
}

