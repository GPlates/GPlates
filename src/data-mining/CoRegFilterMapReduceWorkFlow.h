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
#ifndef GPLATESDATAMINING_COREGFILTERMAPREDUCEWORKFLOW_H
#define GPLATESDATAMINING_COREGFILTERMAPREDUCEWORKFLOW_H

#include <vector>
#include "boost/shared_ptr.hpp"
#include "boost/any.hpp"
#include "boost/mpl/vector.hpp"

#include "OpaqueData.h"

#include "utils/Filter.h"
#include "utils/Mapper.h"
#include "utils/Reducer.h"
#include "utils/FilterMapReduceWorkFlow.h"

namespace GPlatesAppLogic
{
	class ReconstructedFeatureGeometry;
}

namespace GPlatesDataMining
{
	using namespace GPlatesUtils;

	typedef std::vector<
			const GPlatesAppLogic::ReconstructedFeatureGeometry*> FilterInputSequence;

	typedef FilterInputSequence FilterOutputSequence;

	typedef FilterOutputSequence MapperInputSequence;

	typedef std::vector<OpaqueData> MapperOutputSequence;

	typedef MapperOutputSequence ReducerInputSequence;

	typedef OpaqueData ReducerOutputType;

	typedef FilterInputSequence WorkFlowInputSequence;

	typedef ReducerOutputType WorkFlowOutputType;
	
	typedef Filter <  FilterInputSequence::const_iterator,
					  FilterOutputSequence::iterator 
					> CoRegFilter;
	
	typedef Mapper <  MapperInputSequence::const_iterator,
						   MapperOutputSequence::iterator
					> CoRegMaper;

	typedef Reducer < ReducerInputSequence::const_iterator,
					  ReducerOutputType
					> CoRegReducer;

	typedef  boost::mpl::vector<CoRegFilter, CoRegMaper>::type TypeList;

	/*
	* @a FilterMapReduceWorkFlowFactory creates @a CoRegFilterMapReduceWorkFlow instance.
	* Clients call @a execute() funtion of @a CoRegFilterMapReduceWorkFlow object.
	*/
	class CoRegFilterMapReduceWorkFlow
	{
	public:
		
		explicit
		CoRegFilterMapReduceWorkFlow(
				boost::shared_ptr<CoRegFilter> filter,
				boost::shared_ptr<CoRegMaper> maper,
				boost::shared_ptr<CoRegReducer> reducer):
			d_reducer(reducer)
		{
			work_units.push_back(filter);
			work_units.push_back(maper);
		}

		//execute the work flow and the output is in @a OpaqueData& data.
		inline
		void
		execute(
				WorkFlowInputSequence::const_iterator begin,
				WorkFlowInputSequence::const_iterator end,
				WorkFlowOutputType& data)
		{
			data = 
				FilterMapReduceWorkFlow<
						TypeList,
						CoRegReducer,
						WorkFlowInputSequence::const_iterator,
						WorkFlowOutputType
						>::exec(work_units, *d_reducer, begin, end);
		}

	protected:
			std::vector< boost::any > work_units;
			boost::shared_ptr<CoRegReducer> d_reducer;
	};
}
#endif




