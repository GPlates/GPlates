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
#include <QBuffer>
#include <QDebug>
#include <QString>
#include <QDomDocument>
#include <QXmlQuery>
#include <QXmlResultItems>
#include <QXmlSerializer>
#include <boost/bind/bind.hpp>
#include <boost/foreach.hpp>

#include "GeoscimlProfile.h"
#include "GsmlFeatureHandlers.h"
#include "GsmlPropertyHandlers.h"
#include "GsmlNodeProcessorFactory.h"

#include "utils/XQueryUtils.h"


GPlatesFileIO::GeoscimlProfile::progress_reporter_factory_type
GPlatesFileIO::GeoscimlProfile::s_progress_reporter_factory;


void
GPlatesFileIO::GeoscimlProfile::populate(
		File::Reference& file_ref)
{
	QString filename = file_ref.get_file_info().get_display_name(true);
	QFile source(filename);
	source.open(QFile::ReadOnly | QFile::Text);
	if(!source.isOpen())
	{
		qWarning() << QString("Cannot open xml file: %1.").arg(filename);
		return;
	}

	QByteArray array = source.readAll();
	populate(array,file_ref.get_feature_collection());
	source.close();
	return;
}

using namespace GPlatesUtils;

void
GPlatesFileIO::GeoscimlProfile::populate(
		QByteArray& xml_data,
		GPlatesModel::FeatureCollectionHandle::weak_ref fch)
{
// qDebug() << "GPlatesFileIO::GeoscimlProfile::populate:";

	// Report progress (and allow cancellation) if GPlates has registered a reporter.
	boost::shared_ptr<ProgressReporter> progress_reporter;
	if (s_progress_reporter_factory)
	{
		progress_reporter = s_progress_reporter_factory();
	}

	try
	{
		// evaluate for features
		std::vector<QByteArray> results = 
			XQuery::evaluate_features(
					xml_data,
					"/wfs:FeatureCollection/gml:featureMember");

		int count = results.size();
		int i = 1;
// qDebug() << "GPlatesFileIO::GeoscimlProfile::populate: count =" << count;

		if (progress_reporter)
		{
			progress_reporter->set_count(count);
		}

		if( results.size() == 0)
		{
			//This case covers GeoSciML data which has not been wrapped in wfs:FeatureCollection.
			GsmlFeatureHandlerFactory::get_instance()->handle_feature_member(fch, xml_data);
		}
		else
		{
			BOOST_FOREACH(QByteArray& array, results)
			{
				if (progress_reporter &&
					!progress_reporter->update(i))
				{
					break; // out of the loop
				}

				GsmlFeatureHandlerFactory::get_instance()->handle_feature_member(fch, array);

				++i;
			}
		}
	}
	catch(const std::exception& ex)
	{
		qWarning() << ex.what();
	}

	return;
}


void
GPlatesFileIO::GeoscimlProfile::set_progress_reporter_factory(
		const progress_reporter_factory_type &progress_reporter_factory)
{
	s_progress_reporter_factory = progress_reporter_factory;
}


int 
GPlatesFileIO::GeoscimlProfile::count_features(
		QByteArray& xml_data)
{
// qDebug() << "GPlatesFileIO::GeoscimlProfile::count_features:";
	std::vector<QByteArray> results;
	try
	{
		results = XQuery::evaluate_features(
					xml_data,
					"/wfs:FeatureCollection/gml:featureMember");
// qDebug() << "GPlatesFileIO::GeoscimlProfile::count_features: results=" << results.size();
		return results.size();
	}
	catch(const std::exception& ex)
	{
		qWarning() << ex.what();
	}
	return 0;
}
