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
#ifndef GPLATESDATAMINING_RFGTOPROPERTYVALUEMAPPER_H
#define GPLATESDATAMINING_RFGTOPROPERTYVALUEMAPPER_H

#include <vector>
#include <QDebug>

#include <boost/foreach.hpp>

#include "DataTable.h"
#include "DataMiningUtils.h"
#include "GetValueFromPropertyVisitor.h"

#include "app-logic/ReconstructedFeatureGeometry.h"

#include "utils/FilterMapOutputHandler.h"

namespace GPlatesDataMining
{
	using namespace GPlatesUtils;
	
	typedef std::vector<const GPlatesAppLogic::ReconstructedFeatureGeometry*> InputSequence;
	typedef std::vector<OpaqueData> OutputSequence;

	/*	
	*	TODO:
	*	Comments....
	*/
	class RFGToPropertyValueMapper
	{
	public:

		explicit
		RFGToPropertyValueMapper(
				const QString& attr_name,
				const std::vector<const GPlatesAppLogic::ReconstructedFeatureGeometry*>& seed_geos,
				bool is_shapefile_attr = false):
			d_attr_name(attr_name),
			d_seed_geos(seed_geos),
			d_is_shapefile_attr(is_shapefile_attr)
			{	}

		/*
		* TODO: comments....
		*/
		template< class OutputType, class OutputMode>
		int
		operator()(
				InputSequence::const_iterator input_begin,
				InputSequence::const_iterator input_end,
				FilterMapOutputHandler<OutputType, OutputMode> &handler)
		{
			int count = 0;
			DataMiningUtils::RFG_INDEX_VECTOR.clear();
			for(; input_begin != input_end; input_begin++)
			{
				if(!d_is_shapefile_attr)
				{
					handler.insert(
							DataMiningUtils::get_property_value_by_name(
									(*input_begin)->get_feature_ref(), 
									d_attr_name));
				}
				else
				{
					handler.insert(
							DataMiningUtils::get_shape_file_value_by_name(
									(*input_begin)->get_feature_ref(), 
									d_attr_name));
				}
				DataMiningUtils::RFG_INDEX_VECTOR.push_back(
						boost::tie(
								d_seed_geos,
								*input_begin));
				count++;
			}
			return count;
		}

		virtual
		~RFGToPropertyValueMapper(){}			


	protected:
		
		const QString d_attr_name;
		std::vector<const GPlatesAppLogic::ReconstructedFeatureGeometry*> d_seed_geos;
		bool d_is_shapefile_attr;
	};
}
#endif //GPLATESDATAMINING_RFGTOPROPERTYVALUEMAPPER_H





