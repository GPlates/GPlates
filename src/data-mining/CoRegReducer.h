/* $Id: Filter.h 10236 2010-11-17 01:53:09Z mchin $ */

/**
 * \file 
 * $Revision: 10236 $
 * $Date: 2010-11-17 12:53:09 +1100 (Wed, 17 Nov 2010) $
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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
#ifndef GPLATESDATAMINING_COREGREDUCER_H
#define GPLATESDATAMINING_COREGREDUCER_H

#include <vector>

#include "OpaqueData.h"

#include "app-logic/ReconstructContext.h"


namespace GPlatesAppLogic
{
	class ReconstructedFeatureGeometry;
}

namespace GPlatesDataMining
{
	class CoRegReducer
	{
	public:
		typedef std::vector<GPlatesAppLogic::ReconstructContext::ReconstructedFeature>
				reconstructed_feature_vector_type;

		typedef std::vector<
				boost::tuple<
						OpaqueData,
						GPlatesAppLogic::ReconstructContext::ReconstructedFeature> 
								> ReducerInDataset;


		class Config
		{
		public:
			virtual
			bool
			is_same_type(const Config* other) = 0;
			
			virtual
			~Config() {};
		};

		virtual
		~CoRegReducer(){ };

		OpaqueData
		process(
				ReducerInDataset::const_iterator first,
				ReducerInDataset::const_iterator last)
		{
			if(first == last)
				return EmptyData;
			return exec(first,last);
		}
	protected:
		virtual
		OpaqueData
		exec(
				ReducerInDataset::const_iterator first,
				ReducerInDataset::const_iterator last) = 0;

	};

	class DummyReducer : public CoRegReducer
	{
	public:
		class Config : public CoRegReducer::Config
		{
		public:
			bool
			is_same_type(const CoRegReducer::Config* other)
			{
				return dynamic_cast<const DummyReducer::Config*>(other);
			}

			~Config(){}
		};
	protected:
		OpaqueData
		exec(
				CoRegReducer::ReducerInDataset::const_iterator first,
				CoRegReducer::ReducerInDataset::const_iterator last)
		{ return EmptyData; }
		
		~DummyReducer(){ }
	};

	inline
	void
	extract_opaque_data(
			CoRegReducer::ReducerInDataset::const_iterator first,
			CoRegReducer::ReducerInDataset::const_iterator last,
			std::vector<OpaqueData>& output)
	{
		for(; first != last; first++)
		{
			OpaqueData data;
			boost::tie(data,boost::tuples::ignore) = *first;
			output.push_back(data);
		}
	}

}
#endif //GPLATESDATAMINING_COREGREDUCER_H





