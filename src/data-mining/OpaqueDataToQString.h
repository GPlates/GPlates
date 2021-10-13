/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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

#ifndef GPLATESDATAMINING_OPAQUEDATAVISITORS_H
#define GPLATESDATAMINING_OPAQUEDATAVISITORS_H

#include <boost/variant.hpp>
#include <QString>
#include <QDebug>

#include "GetValueFromPropertyVisitor.h"

namespace GPlatesDataMining
{
	class ConvertOpaqueDataToString
		: public boost::static_visitor<QString>
	{
	public:

		inline
		QString 
		operator()(
				const empty_data_type) const
		{
			return  QString("N/A");
		}

		inline
		QString 
		operator()(
				const bool b) const
		{
			if(b)
			{
				return "true";
			}
			else
			{
				return "false";
			}
		}

		template< class Type >
		inline
		QString 
		operator()(
				const Type data) const
		{
			return QString::number(data);
		}

		inline
		QString 
		operator()(
				const QString& str) const
		{
			return str;
		}
	
		inline
		QString 
		operator()(
				const char c) const
		{
			return QString(c);
		}
	};
}
#endif







