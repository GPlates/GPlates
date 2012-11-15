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
#ifndef GPLATESDATAMINING_VOTEREDUCER_H
#define GPLATESDATAMINING_VOTEREDUCER_H

#include <map>
#include <vector>
#include <QDebug>

#include "CoRegReducer.h"
#include "OpaqueDataToQString.h"

#include <boost/foreach.hpp>
namespace GPlatesDataMining
{
	class VoteReducer : public CoRegReducer
	{
	protected:
		OpaqueData
		exec(
				ReducerInDataset::const_iterator input_begin,
				ReducerInDataset::const_iterator input_end) 
		{
			std::vector<OpaqueData> data;
			extract_opaque_data(input_begin, input_end, data);

			std::pair<QString, unsigned> tmp("N/A",0);
			std::vector<QString> tmp_vector;
			BOOST_FOREACH(OpaqueData& d, data)
			{
				tmp_vector.push_back(
						boost::apply_visitor(
								ConvertOpaqueDataToString(),
								d));
			}
			
			std::sort(tmp_vector.begin(),tmp_vector.end());
			
			unsigned count = 0;
			std::vector<QString>::iterator 
				begin = tmp_vector.begin(),
			    end = tmp_vector.end();
			
			for(; begin != end; begin+=count)
			{
				count = std::count(begin,end,*begin);
				if(count > tmp.second)
				{
					tmp.first = *begin;
					tmp.second = count;
				}
			}
			return OpaqueData(tmp.first);
		}
	};
}
#endif





