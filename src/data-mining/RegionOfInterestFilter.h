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
#ifndef GPLATESDATAMINING_REGIONOFINTERESTFILTER_H
#define GPLATESDATAMINING_REGIONOFINTERESTFILTER_H

#include <vector>
#include <QDebug>
#include <boost/foreach.hpp>

#include "CoRegFilterMapReduceWorkFlow.h"
#include "IsCloseEnoughChecker.h"

#include "app-logic/ReconstructedFeatureGeometry.h"

#include "utils/FilterMapOutputHandler.h"

namespace GPlatesDataMining
{
	using namespace GPlatesUtils;
	using namespace GPlatesMaths;
	
	typedef FilterInputSequence InputSequence;
	typedef std::vector<const GPlatesAppLogic::ReconstructedFeatureGeometry*> SeedType;

	/*	
	*	TODO:
	*	Comments....
	*/
	class RegionOfInterestFilter
	{
	public:
		explicit
		RegionOfInterestFilter(
				const SeedType& seed, 
				const double range):
			d_seed(seed),
			d_range(range)
			{	}


		/*
		* TODO: comments....
		*/
		template< class OutputType, class OutputMode>
		inline
		int
		operator()(
				InputSequence::const_iterator input_begin,
				InputSequence::const_iterator input_end,
				FilterMapOutputHandler<OutputType, OutputMode> &handler) 
		{
			int count = 0;
			for(; input_begin != input_end; input_begin++)
			{
				if(is_in_region(*input_begin))
				{
					handler.insert(*input_begin);
					count++;
				}
			}
			return count;

		}

	protected:
		inline
		bool
		is_in_region(
				InputSequence::value_type geo)
		{
			BOOST_FOREACH(const SeedType::value_type& seed_geo, d_seed )
			{
				if(is_close_enough(*seed_geo->reconstructed_geometry(), *geo->reconstructed_geometry(), d_range))
				{
					return true;
				}
			}
			return false;
		}
		const SeedType& d_seed;
		double d_range;
	};
}
#endif





