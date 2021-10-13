/* $Id: DistanceDataOperator.cc 10236 2010-11-17 01:53:09Z mchin $ */

/**
 * \file .
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
#include <map> 
#include <sstream>
#include <algorithm>
#include <iostream>

#include "DistanceDataOperator.h"
#include "DataMiningUtils.h"

#include "GetValueFromPropertyVisitor.h"

#include "model/TopLevelProperty.h"

using namespace GPlatesDataMining;

void
DistanceDataOperator::get_data(
		const AssociationOperator::AssociatedCollection& input,
		const QString& attr_name,
		DataRow& data_row)
{
	AssociationOperator::AssociatedCollection::FeatureDistanceMap::const_iterator
		it = input.d_associated_features.begin();
	AssociationOperator::AssociatedCollection::FeatureDistanceMap::const_iterator
		it_end = input.d_associated_features.end();
	
	std::vector< double > tmp_vec;
	for(; it != it_end; it++)
	{
		boost::optional< double > tmp_min = 
			DataMiningUtils::minimum( it->second );
		if(tmp_min)
		{
			tmp_vec.push_back(*tmp_min);
		}
		else 
		{
			continue;
		}
	}
	if(tmp_vec.size() > 0)
	{
		data_row.append_cell(OpaqueData( *DataMiningUtils::minimum( tmp_vec ) ) );
	}
	else
	{
		data_row.append_cell(OpaqueData( EmptyData ));
	}
}
