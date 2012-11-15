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
#ifndef GPLATESDATAMINING_MEDIANREDUCER_H
#define GPLATESDATAMINING_MEDIANREDUCER_H

#include <algorithm>
#include <vector>
#include <QDebug>

#include "CoRegReducer.h"

#include <boost/foreach.hpp>
namespace GPlatesDataMining
{
	class MedianReducer : public CoRegReducer
	{
	protected:
		OpaqueData
		exec(
				ReducerInDataset::const_iterator in_first,
				ReducerInDataset::const_iterator in_last) 
		{
			std::vector<OpaqueData> data;
			extract_opaque_data(in_first, in_last, data);
			std::vector<double> buf;
			DataMiningUtils::convert_to_double_vector(data.begin() ,data.end(), buf);
			/* 
			* For even length vector, this function will return "upper" median. 
			* If we want the traditional median of even length vector, 
			* we need to run nth_element twice.
			*/
			std::vector<double>::iterator 
				first = buf.begin(), 
				last  = buf.end(), 
				middle = first + (last - first) / 2;
			
			std::nth_element(first, middle, last); 
			return OpaqueData(*middle);
		}
	};
}
#endif





