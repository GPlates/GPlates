/* $Id$ */

/**
 * \file 
 * File specific comments.
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

#ifndef GPLATES_FILEIO_GSMLNODEPROCESSOR_H
#define GPLATES_FILEIO_GSMLNODEPROCESSOR_H

#include <vector>
#include <boost/function.hpp>
#include <boost/shared_ptr.hpp>
#include <QBuffer>
#include <QFile>
#include <QString>
#include <QXmlQuery>

namespace GPlatesFileIO
{
	class GsmlNodeProcessor
	{
	public:

		typedef boost::function<void(QBuffer&)> Handler;

		explicit
		GsmlNodeProcessor(
				const QString& query_str,
				Handler handler = 0):
			d_query_str(query_str),
			d_handler(handler)
		{
		}

		void
		execute(
				QBuffer& xml_data);

		inline
		void
		set_handler(
				Handler h)
		{
			d_handler = h;
		}

		inline
		void
		bind(
				QIODevice* f)
		{
			d_query.bindVariable("data_source",f);
			d_device = f;
			if(!d_query.isValid())
			{
				d_query.setQuery(d_query_str);
			}
		}

	protected:
		QXmlQuery d_query;
		QString d_query_str;
		Handler d_handler;
		QIODevice* d_device;
	};
	
}

#endif  // GPLATES_FILEIO_GSMLNODEPROCESSOR_H
