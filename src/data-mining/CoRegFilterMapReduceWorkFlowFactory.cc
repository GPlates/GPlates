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

#include "Filter.h"
#include "CoRegFilterMapReduceWorkFlowFactory.h"
#include "CoRegConfigurationTable.h"
#include "ReducerTypes.h"
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

#include "utils/GenericFilter.h"
#include "utils/GenericMapper.h"
#include "utils/GenericReducer.h"

using namespace GPlatesUtils;

boost::shared_ptr< GPlatesDataMining::CoRegFilterMapReduceWorkFlow >
GPlatesDataMining::FilterMapReduceWorkFlowFactory::create(
		const ConfigurationTableRow& row,
		const std::vector<const GPlatesAppLogic::ReconstructedFeatureGeometry*>& seed_geos)
{
	boost::shared_ptr<CoRegFilter> filter;
	boost::shared_ptr<CoRegMaper> mapper;
	boost::shared_ptr<CoRegReducer> reducer;

	//filter
	switch(row.filter_type)
	{
		case REGION_OF_INTEREST:
			filter.reset(
					new GenericFilter< InputSequence::const_iterator, InputSequence::iterator, RegionOfInterestFilter> (
							RegionOfInterestFilter(
									seed_geos, 
									row.filter_cfg.d_ROI_range)));
			break;
		case SEED_ITSELF:
			filter.reset(
					new GenericFilter< InputSequence::const_iterator, InputSequence::iterator, SeedSelfFilter> (
							SeedSelfFilter(seed_geos)));
			break;
		case FEATURE_ID_LIST:
			/*
			* TODO:
			*/
			break;
		default:
			break;
	}

	//mapper
	switch(row.attr_type)
	{
	case CO_REGISTRATION_ATTRIBUTE:
		mapper.reset(
				new GenericMapper< CoRegMaper::InputIteratorType, CoRegMaper::OutputIteratorType, RFGToPropertyValueMapper> (
							RFGToPropertyValueMapper(row.attr_name, seed_geos)));
		break;

	case DISTANCE_ATTRIBUTE:
	case PRESENCE_ATTRIBUTE:
	case NUMBER_OF_PRESENCE_ATTRIBUTE:
		mapper.reset(
				new GenericMapper< CoRegMaper::InputIteratorType, CoRegMaper::OutputIteratorType, RFGToRelationalPropertyMapper> (
							RFGToRelationalPropertyMapper(row.attr_type, seed_geos)));
		break;
	
	//This case should be removed once the shape file attributes are treated the same with other attributes.
	case SHAPE_FILE_ATTRIBUTE: 
		mapper.reset(
				new GenericMapper< CoRegMaper::InputIteratorType, CoRegMaper::OutputIteratorType, RFGToPropertyValueMapper> (
							RFGToPropertyValueMapper(row.attr_name, seed_geos,true)));
		break;
	default:
		break;
	}

	#define SET_REDUCER(p)\
		reducer.reset( \
				new GenericReducer< CoRegReducer::InputIteratorType, CoRegReducer::OutputValueType, p> ( \
						p())); 
	//reducer
	switch(row.reducer_type)
	{
	case REDUCER_MIN:
		SET_REDUCER(MinReducer);
		break;
	
	case REDUCER_MAX:
		SET_REDUCER(MaxReducer);
		break;

	case REDUCER_MEAN:
		SET_REDUCER(MeanReducer);
		break;
	
	case REDUCER_VOTE:
		SET_REDUCER(VoteReducer);
		break;

	case REDUCER_WEIGHTED_MEAN:
		SET_REDUCER(WeightedMeanReducer);
		break;

	case REDUCER_MEDIAN:
		SET_REDUCER(MedianReducer);
		break;

	case REDUCER_PERCENTILE:
		SET_REDUCER(PercentileReducer);
		break;
	
	case REDUCER_LOOKUP:
	default:
		SET_REDUCER(LookupReducer);
		break;
	}

	if(filter && mapper && reducer)
	{
		return boost::shared_ptr<CoRegFilterMapReduceWorkFlow>
			(new CoRegFilterMapReduceWorkFlow( filter, mapper, reducer));
	}
	else
	{
		return boost::shared_ptr<CoRegFilterMapReduceWorkFlow>();
	}
}

