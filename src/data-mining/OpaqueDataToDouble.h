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

#ifndef GPLATESDATAMINING_OPAQUEDATATODOUBLE_H
#define GPLATESDATAMINING_OPAQUEDATATODOUBLE_H

#include <boost/variant.hpp>
#include <boost/optional.hpp>

#include <QString>
#include <QDebug>

namespace GPlatesDataMining
{
	class ConvertOpaqueDataToDouble
		: public boost::static_visitor<boost::optional<double> >
	{
	public:
		template<class type>
		boost::optional<double> 
		operator()(
				const type) const
		{
			return  boost::none;
		}

		inline
		boost::optional<double> 
		operator()(
				const int data) const
		{
			return data;
		}

		inline
		boost::optional<double> 
		operator()(
				const double data) const
		{
			return data;
		}

		inline
		boost::optional<double> 
		operator()(
				const float data) const
		{
			return data;
		}

		inline
		boost::optional<double> 
		operator()(
				const unsigned data) const
		{
			return data;
		}
	};
}
#endif







