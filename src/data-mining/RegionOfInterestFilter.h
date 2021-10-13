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

#include "CoRegFilter.h"
#include "IsCloseEnoughChecker.h"

#include "app-logic/ReconstructedFeatureGeometry.h"

namespace GPlatesDataMining
{
	class RegionOfInterestFilter : public CoRegFilter
	{
	public:
		RegionOfInterestFilter(
				const CoRegFilter::RFGVector& seed, 
				const double range):
			d_seed(seed),
			d_range(range)
			{	}

		class Config : public CoRegFilter::Config
		{
		public:
			explicit
			Config(double val) :
				d_range(val)
			{	}

			CoRegFilter*
			create_filter(const CoRegFilter::RFGVector& seed)
			{
				return new RegionOfInterestFilter(seed, d_range);
			}

			bool
			is_same_type(const CoRegFilter::Config* other)
			{
				return dynamic_cast<const RegionOfInterestFilter::Config*>(other);
			}

			const QString
			to_string()
			{
				return QString("Region of Interest(%1)").arg(d_range);
			}

			bool
			operator< (const CoRegFilter::Config& other)
			{
				if(!is_same_type(&other))
					throw GPlatesGlobal::LogException(GPLATES_EXCEPTION_SOURCE,"Try to compare different filter types.");
				return d_range < dynamic_cast<const RegionOfInterestFilter::Config&>(other).range();
			}

			bool
			operator==(const CoRegFilter::Config& other) 
			{
				if(!is_same_type(&other))
					throw GPlatesGlobal::LogException(GPLATES_EXCEPTION_SOURCE,"Try to compare different filter types.");
				return GPlatesMaths::are_slightly_more_strictly_equal(d_range,dynamic_cast<const RegionOfInterestFilter::Config&>(other).range());
			}

			const QString
			filter_name()
			{
				return "Region of Interest";
			}
			
			~Config(){ }

		private:
			double
			range() const
			{
				return d_range;
			}

			double d_range;
		};

		void
		process(
				CoRegFilter::RFGVector::const_iterator input_begin,
				CoRegFilter::RFGVector::const_iterator input_end,
				CoRegFilter::RFGVector& output) 
		{
			for(; input_begin != input_end; input_begin++)
			{
				if(is_in_region(*input_begin))
				{
					output.push_back(*input_begin);
				}
			}
			return;
		}

		~RegionOfInterestFilter(){ }

	protected:
		bool
		is_in_region(CoRegFilter::RFGVector::value_type geo)
		{
			BOOST_FOREACH(const CoRegFilter::RFGVector::value_type& seed_geo, d_seed )
			{
				if(is_close_enough(
						*seed_geo->reconstructed_geometry(), 
						*geo->reconstructed_geometry(), 
						d_range))
				{
					return true;
				}
			}
			return false;
		}
		const CoRegFilter::RFGVector& d_seed;
		double d_range;
	};
}
#endif





