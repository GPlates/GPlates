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

#include <QBuffer>

#include "GsmlFeatureHandlers.h"
#include "GsmlPropertyHandlers.h"
#include "GsmlNodeProcessorFactory.h"

void
GPlatesFileIO::GsmlFeatureHandlers::handle_mapped_feature(
		QBuffer& xml_data)
{
	GPlatesModel::FeatureType feature_type(GPlatesModel::PropertyName::create_gml("MappedFeature"));
	GPlatesModel::FeatureHandle::weak_ref feature = GPlatesModel::FeatureHandle::create(
			d_feature_collection,
			feature_type);

	GsmlPropertyHandlers::instance()->set_feature(feature);
	std::vector<boost::shared_ptr<GsmlNodeProcessor> > processors = 
		GsmlNodeProcessorFactory::get_property_processors_for_mapped_feature();
	BOOST_FOREACH(boost::shared_ptr<GsmlNodeProcessor>& p, processors)
	{
		p->execute(xml_data);
	}
	return;
}


