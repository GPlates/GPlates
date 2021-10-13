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
#include <algorithm>

#include <boost/foreach.hpp>

#include "CoRegConfigurationTable.h"

#include "global/GPlatesAssert.h"

namespace
{
	using namespace GPlatesDataMining;
	bool
	compare_cfg_table_rows(
			ConfigurationTableRow row_1,
			ConfigurationTableRow row_2)
	{
		const GPlatesModel::FeatureCollectionHandle *fch_1 = row_1.target_feature_collection_handle.handle_ptr();
		const GPlatesModel::FeatureCollectionHandle *fch_2 = row_2.target_feature_collection_handle.handle_ptr();
		if(fch_1 > fch_2)
		{
			return true;
		}
		else if(fch_1 == fch_2)
		{
			if( row_1.association_operator_type > row_2.association_operator_type)
			{
				return true;
			}
			else if(
					(row_1.association_operator_type == REGION_OF_INTEREST) 
					 && 
					(row_2.association_operator_type == REGION_OF_INTEREST)
					)
			{
				if( row_1.association_parameters.d_ROI_range > 
					row_2.association_parameters.d_ROI_range )
				{
					return true;
				}
			}
		}
		return false;
	}
}


void
GPlatesDataMining::CoRegConfigurationTable::optimize()
{
	//sort the cfg table so that co-registration cache can be used to optimize performance.
	std::sort(begin(), end(), compare_cfg_table_rows);

	std::vector<ConfigurationTableRow>* table = this;
	//check the if the input data set is seed data.
	//if so, a special filter will be used to optimize performance.
	BOOST_FOREACH(ConfigurationTableRow& row, *table)
	{
		if(is_seed_feature_collection(row))
		{
			if(0.0 == GPlatesMaths::Real(row.association_parameters.d_ROI_range))
			{
				row.association_operator_type = SEED_ITSELF;
			}
		}
	}

}


bool
GPlatesDataMining::CoRegConfigurationTable::is_seed_feature_collection(
		const ConfigurationTableRow& input_row)
{
	using namespace GPlatesAppLogic;
	BOOST_FOREACH(const FeatureCollectionFileState::file_reference &file_ref, d_seed_files)
	{
		if( file_ref.get_file().get_feature_collection().handle_ptr() == 
			input_row.target_feature_collection_handle.handle_ptr() )
		{
			return true;
		}
	}
	return false;
}




