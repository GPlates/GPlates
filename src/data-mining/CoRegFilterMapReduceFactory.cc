/* $Id: CoRegFilterMapReduceWorkFlowFactory.cc 11569 2011-05-17 03:19:27Z mchin $ */

/**
 * \file 
 * $Revision: 11569 $
 * $Date: 2011-05-17 13:19:27 +1000 (Tue, 17 May 2011) $
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
#include "CoRegFilterMapReduceFactory.h"
#include "CoRegConfigurationTable.h"
#include "Types.h"
#include "LookupReducer.h"
#include "MinReducer.h"
#include "MaxReducer.h"
#include "MeanReducer.h"
#include "MedianReducer.h"
#include "PercentileReducer.h"
#include "RegionOfInterestFilter.h"
#include "RFGToRelationalPropertyMapper.h"
#include "RFGToPropertyValueMapper.h"
#include "SeedSelfFilter.h"
#include "VoteReducer.h"
#include "WeightedMeanReducer.h"

using namespace GPlatesUtils;

GPlatesDataMining::CoRegFilter*
GPlatesDataMining::CoRegFilterFactory::create(
		const ConfigurationTableRow& row, 
		const GPlatesDataMining::CoRegFilter::RFGVector& seeds)
{
	return row.filter_cfg->create_filter(seeds);
}

GPlatesDataMining::CoRegMapper*
GPlatesDataMining::CoRegMapperFactory::create(
		const ConfigurationTableRow& row,
		const GPlatesDataMining::CoRegFilter::RFGVector& seeds)
{
	switch(row.attr_type)
	{
		case CO_REGISTRATION_ATTRIBUTE:
			return new	RFGToPropertyValueMapper(row.attr_name);
		case DISTANCE_ATTRIBUTE:
		case PRESENCE_ATTRIBUTE:
		case NUMBER_OF_PRESENCE_ATTRIBUTE:
			return new	RFGToRelationalPropertyMapper(row.attr_type,seeds);
	
			//This case should be removed once the shape file attributes are treated the same with other attributes.
		case SHAPE_FILE_ATTRIBUTE: 
			return new	RFGToPropertyValueMapper(row.attr_name,true);
		default:
			break;
	}
	return new DummyMapper();
}


GPlatesDataMining::CoRegReducer*
GPlatesDataMining::CoRegReducerFactory::create(
		const ConfigurationTableRow& row,
		const GPlatesDataMining::CoRegFilter::RFGVector& seeds)
{
	switch(row.reducer_type)
	{
		case REDUCER_MIN:
			return new MinReducer();

		case REDUCER_MAX:
			return new MaxReducer();

		case REDUCER_MEAN:
			return new MeanReducer();

		case REDUCER_VOTE:
			return new VoteReducer();

		case REDUCER_WEIGHTED_MEAN:
			return new WeightedMeanReducer();

		case REDUCER_MEDIAN:
			return new MedianReducer();

		case REDUCER_PERCENTILE:
			return new PercentileReducer();

		case REDUCER_LOOKUP:
			return new LookupReducer(seeds);
		
		default:
			break;
	}
	return new DummyReducer();
}





