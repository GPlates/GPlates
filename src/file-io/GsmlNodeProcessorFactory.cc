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
#include <boost/bind.hpp>
#include <boost/foreach.hpp>

#include "GsmlNodeProcessor.h"
#include "GsmlNodeProcessorFactory.h"
#include "GsmlFeaturesDef.h"

GPlatesFileIO::GsmlNodeProcessorFactory::GsmlNodeProcessorFactory(
		GPlatesModel::FeatureHandle::weak_ref f):
	d_property_handler(new GsmlPropertyHandlers(f))
{ }

void
GPlatesFileIO::GsmlNodeProcessorFactory::process_with_property_processors(
		const QString& feature_type,
		QBuffer& buf)
{
	std::vector<boost::shared_ptr<GsmlNodeProcessor> > processors = 
		create_property_processors(feature_type);

	BOOST_FOREACH(boost::shared_ptr<GsmlNodeProcessor> p, processors)
	{
		p->execute(buf);
	}
}


std::vector<boost::shared_ptr<GPlatesFileIO::GsmlNodeProcessor> >
GPlatesFileIO::GsmlNodeProcessorFactory::create_property_processors(
		const QString& feature_name)
{

//qDebug() << "GPlatesFileIO::GsmlNodeProcessorFactory::create_property_processors() feature_name=" << feature_name;
	std::vector<boost::shared_ptr<GsmlNodeProcessor> > processors;
	const FeatureInfo* feature = NULL; 
	for(unsigned i=0; i<sizeof(AllFeatures)/sizeof(FeatureInfo); i++)
	{
		// Search for exact match on feature type name
		if(feature_name == AllFeatures[i].name)
		{
			feature = &AllFeatures[i];
			break;
		}

		// NOTE: this is simple way to get all of RockUnit_* features to process 
		// search for common prefix on feature type name 
		if( feature_name.section('_',0,0) == AllFeatures[i].name )
		{
			feature = &AllFeatures[i];
			break;
		}
	}

	if(!feature)
	{
		qWarning() << "Cannot find property processors for " + feature_name + ".";
		return std::vector<boost::shared_ptr<GsmlNodeProcessor> >();
	}
	else
	{
		for(unsigned j=0; j<feature->property_num; j++)
		{
			processors.push_back(
				boost::shared_ptr<GPlatesFileIO::GsmlNodeProcessor>(           
					new GsmlNodeProcessor(                                 
						feature->properties[j]->query,                                
						boost::bind(                                   
							feature->properties[j]->handler, 
							d_property_handler,      
							_1)
					)
				)
			);
		}
	}
	return processors;
}


void
GPlatesFileIO::GsmlNodeProcessorFactory::process_with_property_processors(
		const QString& feature_type,
		QByteArray& data)
{
	QBuffer buffer(&data);
	buffer.open(QIODevice::ReadWrite | QIODevice::Text);
	if(!buffer.isOpen())
	{
		qWarning() << QString("Cannot open buffer for reading xml data.");
		return;
	}
	process_with_property_processors(feature_type,buffer);
	buffer.close();
	return;
}







