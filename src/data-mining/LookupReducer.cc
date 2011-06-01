/* $Id: LookupReducer.h 11660 2011-05-27 07:12:20Z mchin $ */

/**
 * \file 
 * $Revision: 11660 $
 * $Date: 2011-05-27 17:12:20 +1000 (Fri, 27 May 2011) $
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
#include <boost/optional.hpp>

#include "app-logic/ReconstructedFeatureGeometry.h"
#include "maths/types.h"
#include "maths/PolygonOnSphere.h"

#include "DataMiningUtils.h"
#include "LookupReducer.h"
namespace
{
	/*
	* If the seed geometries are inside polygons, 
	* return the area of smallest polygon in which seed locates.
	*/
	boost::optional<GPlatesMaths::real_t>
	test_polygon_area(
			std::vector<const GPlatesAppLogic::ReconstructedFeatureGeometry*> seeds,
			std::vector<const GPlatesAppLogic::ReconstructedFeatureGeometry*> rfgs)
	{
		using namespace GPlatesMaths;
		boost::optional<GPlatesMaths::real_t> area = boost::none;
		BOOST_FOREACH(const GPlatesAppLogic::ReconstructedFeatureGeometry* rfg, rfgs)
		{
			double tmp_dist = 0.0;
			const PolygonOnSphere* p = dynamic_cast<const PolygonOnSphere*>(rfg->reconstructed_geometry().get());
			if(!p)// not a polygon
				continue;
			tmp_dist = GPlatesDataMining::DataMiningUtils::shortest_distance(seeds, rfg);
			if(!are_slightly_more_strictly_equal(tmp_dist, 0.0))//not inside this polygon
				continue;
			
			GPlatesMaths::real_t tmp_area = p->get_area();
			if(!area)
				area = tmp_area;
			else
				area = tmp_area < *area ? tmp_area : *area;
		}
		return area;
	}
}


GPlatesDataMining::OpaqueData
GPlatesDataMining::LookupReducer::exec(
		ReducerInDataset::const_iterator input_begin,
		ReducerInDataset::const_iterator input_end) 
{
	std::vector<OpaqueData> data;
	extract_opaque_data(input_begin, input_end, data);
	size_t size = data.size();
	if(size == 1)
	{
		return data[0];
	}
	else if(size == 0)
	{
		return EmptyData;
	}
	else
	{
		typedef std::multimap<
				double, 
				ReducerInDataset::value_type > ResultMap;

		ResultMap tmp_map;
		for(; input_begin != input_end; input_begin++)
		{
			std::vector<const GPlatesAppLogic::ReconstructedFeatureGeometry*> rfgs;
			boost::tie(boost::tuples::ignore,rfgs) = *input_begin;
			double t = DataMiningUtils::shortest_distance(rfgs,d_seeds);
			tmp_map.insert(std::make_pair(t, *input_begin));
		}

		std::pair<ResultMap::iterator, ResultMap::iterator> the_pair;
		the_pair = tmp_map.equal_range(tmp_map.begin()->first);
		OpaqueData ret = EmptyData;
		boost::tie(ret, boost::tuples::ignore) = the_pair.first->second;

		if(std::distance(the_pair.first,the_pair.second) != 1 && 
			GPlatesMaths::are_slightly_more_strictly_equal(the_pair.first->first, 0.0))
		{
			//This is tricky. We need to handle the situation in which seed geometries locate inside several target polygons.
			//we need to return the data from smallest target polygon.
			ResultMap::iterator s = the_pair.first, e = the_pair.second;
			boost::optional<GPlatesMaths::real_t> area = boost::none;
			boost::optional<ReducerInDataset::value_type> val = boost::none;
			for(; s != e; ++s)
			{
				std::vector<const GPlatesAppLogic::ReconstructedFeatureGeometry*> rfgs;
				boost::tie(boost::tuples::ignore,rfgs) = s->second;
				boost::optional<GPlatesMaths::real_t> tmp;
				tmp = test_polygon_area(d_seeds,rfgs);
				if(!tmp)
					continue;
				if(!area || *tmp < *area)
				{
					area = tmp;
					val = s->second;
				}
			}
			if(val)
			{
				boost::tie(ret, boost::tuples::ignore) = *val;
			}
		}
		else if(std::distance(the_pair.first,the_pair.second) != 1)
		{
			qWarning() << "Lookup reducer found multiple values and cannot determine which one should be return.";
			qWarning() << "So, just return the first one in the list";
		}
		return ret;
	}
}
	





