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

#include "GsmlFeatureHandlers.h"
#include "GsmlPropertyHandlers.h"
#include "GsmlNodeProcessorFactory.h"

using namespace GPlatesFileIO;

const QString mapped_feature_query = 
		wfs_ns + gml_ns + gsml_ns + 
		"declare variable $idx external; " +
		"doc($data_source)/wfs:FeatureCollection/gml:featureMember[$idx]/gsml:MappedFeature";
const QString geometry_property_query = 
		gsml_ns + 
		"declare variable $idx external; " +
		"doc($data_source)/gsml:MappedFeature/gsml:shape[$idx]";

#define REGISTER_FEATURE_PROCESSOR(name,container)                     \
container.push_back(                                                   \
		boost::shared_ptr<GPlatesFileIO::GsmlNodeProcessor>(           \
				new GsmlNodeProcessor(                                 \
						name ## _query,                                \
						boost::bind(                                   \
								&GsmlFeatureHandlers::handle_ ## name, \
								GsmlFeatureHandlers::instance(),       \
								_1))));

#define REGISTER_PROPERTY_PROCESSOR(name,container)                    \
container.push_back(                                                   \
		boost::shared_ptr<GPlatesFileIO::GsmlNodeProcessor>(           \
				new GsmlNodeProcessor(                                 \
						name ## _query,                                \
						boost::bind(                                   \
								&GsmlPropertyHandlers::handle_ ## name,\
								GsmlPropertyHandlers::instance(),      \
								_1))));

std::vector<boost::shared_ptr<GPlatesFileIO::GsmlNodeProcessor> >
GPlatesFileIO::GsmlNodeProcessorFactory::create_feature_processors()
{
	std::vector<boost::shared_ptr<GPlatesFileIO::GsmlNodeProcessor> > ret;
	REGISTER_FEATURE_PROCESSOR(mapped_feature, ret);
	return ret;
}


std::vector<boost::shared_ptr<GPlatesFileIO::GsmlNodeProcessor> >
GPlatesFileIO::GsmlNodeProcessorFactory::create_property_processors_for_mapped_feature()
{
	std::vector<boost::shared_ptr<GPlatesFileIO::GsmlNodeProcessor> > ret;
	REGISTER_PROPERTY_PROCESSOR(geometry_property,ret);
	return ret;
}

std::vector<boost::shared_ptr<GPlatesFileIO::GsmlNodeProcessor> >
GPlatesFileIO::GsmlNodeProcessorFactory::get_feature_processors()
{
	static std::vector<boost::shared_ptr<GPlatesFileIO::GsmlNodeProcessor> > ps = 
		create_feature_processors(); //initial once.
	
	return ps;
}

std::vector<boost::shared_ptr<GPlatesFileIO::GsmlNodeProcessor> >
GPlatesFileIO::GsmlNodeProcessorFactory::get_property_processors_for_mapped_feature()
{
	static std::vector<boost::shared_ptr<GPlatesFileIO::GsmlNodeProcessor> > ps = 
		create_property_processors_for_mapped_feature(); //initial once.
	
	return ps;
}




