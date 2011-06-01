/* $Id: CoRegFilterMapReduceWorkFlowFactory.h 10236 2010-11-17 01:53:09Z mchin $ */

/**
 * \file 
 * $Revision: 10236 $
 * $Date: 2010-11-17 12:53:09 +1100 (Wed, 17 Nov 2010) $
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
#ifndef GPLATESDATAMINING_COREGFILTERMAPREDUCEFACTORY_H
#define GPLATESDATAMINING_COREGFILTERMAPREDUCEFACTORY_H

#include <vector>
#include <boost/shared_ptr.hpp>

#include "CoRegFilter.h"
#include "CoRegMapper.h"
#include "CoRegReducer.h"

namespace GPlatesAppLogic
{
	class ReconstructedFeatureGeometry;
}

namespace GPlatesDataMining
{
	struct ConfigurationTableRow;
	class CoRegFilter;
	class CoRegMapper;
	class CoRegReducer;
	
	class CoRegFilterFactory
	{
	public:
		static
		CoRegFilter*
		create(
				const ConfigurationTableRow&,
				const GPlatesDataMining::CoRegFilter::RFGVector&);
	};

	class CoRegMapperFactory
	{
	public:
		static
		CoRegMapper*
		create(
				const ConfigurationTableRow& row,
				const GPlatesDataMining::CoRegFilter::RFGVector&);
	};

	class CoRegReducerFactory
	{
	public:
		static
		CoRegReducer*
		create(
				const ConfigurationTableRow& row,
				const GPlatesDataMining::CoRegFilter::RFGVector&);
	};

	inline
	boost::tuple<
			boost::shared_ptr<CoRegFilter>, 
			boost::shared_ptr<CoRegMapper>, 
			boost::shared_ptr<CoRegReducer> 
				>
	create_filter_map_reduce(
			const ConfigurationTableRow& r,
			const GPlatesDataMining::CoRegFilter::RFGVector& s)
	{
		return boost::make_tuple(
			boost::shared_ptr<CoRegFilter>(CoRegFilterFactory::create(r,s)),
			boost::shared_ptr<CoRegMapper>(CoRegMapperFactory::create(r,s)),
			boost::shared_ptr<CoRegReducer>(CoRegReducerFactory::create(r,s)));
	}
}

#endif



