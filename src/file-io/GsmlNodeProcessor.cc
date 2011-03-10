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

#include <QByteArray>
#include <QBuffer>
#include <QDebug>
#include <QXmlQuery>
#include <QXmlSerializer>

#include "GsmlNodeProcessor.h"

void
GPlatesFileIO::GsmlNodeProcessor::execute(
		QBuffer& xml_data)
{
	for(int i=1; ;i++)
	{
		xml_data.reset();
		d_query.bindVariable("data_source",&xml_data);
		d_query.bindVariable("idx", QVariant(i));
		d_query.setQuery(d_query_str);

		if (!d_query.isValid())
		{
			qWarning() << "The query is not valid.";
			return;
		}
		QByteArray array;
		QBuffer buffer(&array);
		buffer.open(QIODevice::ReadWrite | QIODevice::Text);
		if(!buffer.isOpen())
		{
			qWarning() << "Cannot open buffer for output.";
		}

		QXmlSerializer serializer(d_query, &buffer);
		if(d_query.evaluateTo(&serializer))
		{
			if(array.size() == 0)
			{
				qDebug() << "No more data.";
				break;
			}
			buffer.reset();
			try
			{
				d_handler(buffer);
			}
			catch(...)
			{
				buffer.close();
				qDebug()<< "opps, something is wrong.";
				throw;
			}
		}
		else
		{
			qWarning() << "Failed to evaluate data.";
			break;
		}
		buffer.close();
	}
}

