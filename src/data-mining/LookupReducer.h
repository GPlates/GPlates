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
#ifndef GPLATESDATAMINING_LOOKUPREDUCER_H
#define GPLATESDATAMINING_LOOKUPREDUCER_H

#include <vector>
#include <QDebug>
#include <boost/foreach.hpp>
#include <boost/tuple/tuple.hpp>

#include <maths/MathsUtils.h>

#include "DataMiningUtils.h"

namespace GPlatesDataMining
{
	using namespace GPlatesUtils;
	
	typedef std::vector<OpaqueData> ReducerInputSequence;

	/*	
	*	TODO:
	*	Comments....
	*/
	class LookupReducer
	{
	public:

		/*
		* TODO: comments....
		*/
		inline
		OpaqueData
		operator()(
				ReducerInputSequence::const_iterator input_begin,
				ReducerInputSequence::const_iterator input_end) 
		{
			size_t size = std::distance(input_begin,input_end);
			if(size == 1)
			{
				return *input_begin;
			}
			else if(size == 0)
			{
				return EmptyData;
			}
			else
			{
				if(DataMiningUtils::RFG_INDEX_VECTOR.size() != size)
				{
					qWarning() << "The RFG_INDEX_VECTOR and input data has different size.";
					size = std::min(DataMiningUtils::RFG_INDEX_VECTOR.size(),size);
				}
				std::vector<const GPlatesAppLogic::ReconstructedFeatureGeometry*> seed_geos;
				const GPlatesAppLogic::ReconstructedFeatureGeometry* target_geo;
				double min_distance = -1;
				size_t idx = 0;

				for(size_t i=0; i<size; i++)
				{
					boost::tie(seed_geos, target_geo) = DataMiningUtils::RFG_INDEX_VECTOR.at(i);
					double d = DataMiningUtils::shortest_distance(seed_geos, target_geo);
					if(GPlatesMaths::are_slightly_more_strictly_equal(d,0))
					{
						std::advance(input_begin,i);
						return *input_begin;
					}
					if(min_distance < 0 || min_distance > d)
					{
						min_distance = d;
						idx = i;
					}
				}
				std::advance(input_begin,idx);
				return *input_begin;
			}
		}
	};
}
#endif





