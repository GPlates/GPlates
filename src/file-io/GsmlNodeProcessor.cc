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
#include <QByteArray>
#include <QBuffer>
#include <QDebug>
#include <QXmlQuery>
#include <QXmlSerializer>

#include "GsmlNodeProcessor.h"
#include "utils/XQueryUtils.h"

#include <boost/foreach.hpp>

void
GPlatesFileIO::GsmlNodeProcessor::execute(
		QBuffer& xml_data)
{
	std::vector<QByteArray> results = 
		GPlatesUtils::XQuery::evaluate_query(
				xml_data,
				d_query_str);

	BOOST_FOREACH(QByteArray& data, results)
	{
		QBuffer buffer(&data);
		buffer.open(QIODevice::ReadWrite | QIODevice::Text);
		if(!buffer.isOpen())
		{
			qWarning() << "Cannot open buffer for output.";
			continue;
		}
		d_handler(buffer);
		buffer.close();
	}
}
