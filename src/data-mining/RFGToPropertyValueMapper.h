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


#include "CoRegMapper.h"
#include "DataTable.h"
#include "DataMiningUtils.h"
#include "GetValueFromPropertyVisitor.h"

#include "app-logic/ReconstructedFeatureGeometry.h"

#include <boost/foreach.hpp>
namespace GPlatesDataMining
{
	
	class RFGToPropertyValueMapper : public CoRegMapper
	{
	public:
		explicit
		RFGToPropertyValueMapper(
				const QString& attr_name,
				bool is_shapefile_attr = false):
			d_attr_name(attr_name),
			d_is_shapefile_attr(is_shapefile_attr)
			{	}

		void 
		process(
				CoRegMapper::MapperInDataset::const_iterator input_begin,
				CoRegMapper::MapperInDataset::const_iterator input_end,
				CoRegMapper::MapperOutDataset &output)
		{
			for(; input_begin != input_end; input_begin++)
			{
				const GPlatesAppLogic::ReconstructContext::ReconstructedFeature &reconstructed_target_feature =
						*input_begin;

				if(!d_is_shapefile_attr)
				{
					output.push_back(boost::make_tuple(
							DataMiningUtils::get_property_value_by_name(
									reconstructed_target_feature.get_feature(), 
									d_attr_name),
							reconstructed_target_feature));
				}
				else
				{
					output.push_back(boost::make_tuple(
							DataMiningUtils::get_shape_file_value_by_name(
									reconstructed_target_feature.get_feature(), 
									d_attr_name),
							reconstructed_target_feature));
				}
			}
			return;
		}

		virtual
		~RFGToPropertyValueMapper(){}			


	protected:
		const QString d_attr_name;
		bool d_is_shapefile_attr;
	};
}
#endif //GPLATESDATAMINING_RFGTOPROPERTYVALUEMAPPER_H





