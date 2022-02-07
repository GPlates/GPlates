/* $Id: GenericMapper.h 10236 2010-11-17 01:53:09Z mchin $ */

/**
 * \file validate filename template.
 * $Revision: 10236 $
 * $Date: 2010-11-17 12:53:09 +1100 (Wed, 17 Nov 2010) $
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

#ifndef GPLATES_UTILS_XQUERYUTILS_H
#define GPLATES_UTILS_XQUERYUTILS_H

#include <vector>
#include <QDebug>
#include <QVariant>
#include <boost/function.hpp>

#include <QString>
#include <QBuffer>
#include <QByteArray>
#include <QVariant>
#include <QXmlStreamReader>

namespace GPlatesUtils
{
	namespace XQuery
	{
		typedef boost::function<bool (QBuffer&)> IsEmptyFun;

		std::vector<QByteArray>
		evaluate_query(
				QByteArray& xml_data,
				const QString& query_str);

		inline
		std::vector<QByteArray>
		evaluate_query(
				QBuffer& buf,
				const QString& query_str)
		{
			QByteArray data = buf.data();
			return evaluate_query(data, query_str);
		}

		// FIXME: could this test be replaced with above? 
		std::vector<QByteArray>
		evaluate_features(
				QByteArray& xml_data,
				const QString& query_str);

#if 0
		/*
		* Run the query_str on xml_data 
		* return the result in std::vector<QByteArray>
		*/
		std::vector<QByteArray>
		evaluate(
				QByteArray& xml_data,
				const QString& query_str,
				IsEmptyFun is_empty);

		inline
		std::vector<QByteArray>
		evaluate(
				QBuffer& buf,
				const QString& query_str,
				IsEmptyFun is_empty)
		{
			QByteArray data = buf.data();
			return evaluate(data, query_str, is_empty);
		}
#endif

		/*
		* retrieve the attribute value from xml_data as string.
		*/
		std::vector<QVariant>
		evaluate_attribute(
				QByteArray& xml_data,
				const QString& attr_name);

		void
		wrap_xml_data(
				QByteArray& xml_data,
				const QString& wrapper);

		inline
		bool
		is_empty(const QBuffer& data)
		{
			return (data.size() == 0);
		}

		/*
		* The same as QXmlStreamReader::readNextStartElement().
		*/
		bool
		next_start_element(
				QXmlStreamReader&);
	}
}
#endif //GPLATES_UTILS_XQUERYUTILS_H









