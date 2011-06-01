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
#include <algorithm>

#include <boost/foreach.hpp>

#include "CoRegConfigurationTable.h"
#include "RegionOfInterestFilter.h"
#include "global/GPlatesAssert.h"

namespace
{
	using namespace GPlatesDataMining;
	
	bool
	compare_feature_collection(
			const ConfigurationTableRow& row_1,
			const ConfigurationTableRow& row_2)
	{
		return row_1.target_fc.handle_ptr() < row_2.target_fc.handle_ptr();
	}

	bool
	compare_filter_type(
			const ConfigurationTableRow& row_1,
			const ConfigurationTableRow& row_2)
	{
		return row_1.filter_cfg->filter_name() < row_2.filter_cfg->filter_name();
	}

	bool
	compare_filter(
			const ConfigurationTableRow& row_1,
			const ConfigurationTableRow& row_2)
	{
		return row_2.filter_cfg < row_1.filter_cfg ;
	}
}

void
GPlatesDataMining::CoRegConfigurationTable::optimize()
{
	group_and_sort();
	d_optimized = true;
}


void
GPlatesDataMining::CoRegConfigurationTable::group_and_sort()
{
	for(std::size_t i=0; i<d_rows.size(); i++)//keep the original index.
		d_rows[i].index = i;

	//group by feature collection
	std::sort(begin(), end(), compare_feature_collection);
	
	iterator it = begin(), it_end = end();
	while(it != it_end)
	{
		std::pair<iterator,iterator> bounds;
		bounds = std::equal_range(it, it_end, *it, compare_feature_collection);
		it = bounds.second;
		iterator inner_it = bounds.first, inner_it_end =--bounds.second;
		std::sort(inner_it, inner_it_end, compare_filter_type); // sort by filter type
		while(inner_it != inner_it_end)
		{
			std::pair<iterator,iterator> inner_bounds;
			inner_bounds = std::equal_range(inner_it, inner_it_end, *inner_it, compare_filter_type);
			inner_it = inner_bounds.second;
			if(dynamic_cast<RegionOfInterestFilter*> (inner_bounds.first->filter_cfg.get()))//if it is region of interest filter, sort by range.
			{
				std::sort(inner_bounds.first,inner_bounds.second,compare_filter);
			}
		}
	}
}







