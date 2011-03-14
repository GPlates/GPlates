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
#include <boost/foreach.hpp>

#include "GeoscimlProfile.h"
#include "GsmlFeatureHandlers.h"
#include "GsmlPropertyHandlers.h"
#include "GsmlNodeProcessorFactory.h"

const QString root_query = "doc($data_source)";

void
GPlatesFileIO::GeoscimlProfile::populate(
		const File::Reference& file_ref,
		GPlatesModel::FeatureCollectionHandle::weak_ref fch)
{
	QString filename = file_ref.get_file_info().get_display_name(true);
	QFile source(filename);
	source.open(QFile::ReadOnly | QFile::Text);
	if(!source.isOpen())
	{
		qWarning() << QString("cannot open xml file: %1.").arg(filename);
		return;
	}

	QByteArray array = source.readAll();
	QBuffer buffer(&array);
	buffer.open(QIODevice::ReadWrite | QIODevice::Text);
	if(!buffer.isOpen())
	{
		qWarning() << QString("cannot open buffer for reading xml file.");
		return;
	}

	try
	{
		GsmlFeatureHandlers::instance()->set_feature_collection(fch);
		std::vector<boost::shared_ptr<GsmlNodeProcessor> > processors = 
			GsmlNodeProcessorFactory::get_feature_processors();

		BOOST_FOREACH(boost::shared_ptr<GsmlNodeProcessor>& p, processors)
		{
			p->execute(buffer);
		}
		
	}
	catch(const QString& err)
	{
		qWarning() << err;
	}
	buffer.close();
	source.close();
	return;
}


void
GPlatesFileIO::GeoscimlProfile::populate(
		QByteArray& xml_data,
		GPlatesModel::FeatureCollectionHandle::weak_ref fch)
{
	QBuffer buffer(&xml_data);
	buffer.open(QIODevice::ReadWrite | QIODevice::Text);
	if(!buffer.isOpen())
	{
		qWarning() << QString("cannot open buffer for reading xml data.");
		return;
	}

	try
	{
		GsmlFeatureHandlers::instance()->set_feature_collection(fch);
		std::vector<boost::shared_ptr<GsmlNodeProcessor> > processors = 
			GsmlNodeProcessorFactory::get_feature_processors();

		BOOST_FOREACH(boost::shared_ptr<GsmlNodeProcessor>& p, processors)
		{
			p->execute(buffer);
		}
		
	}
	catch(const QString& err)
	{
		qWarning() << err;
	}
	buffer.close();
	return;
}


