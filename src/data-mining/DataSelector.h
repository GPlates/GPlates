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

#ifndef GPLATESDATAMINING_DATASELECTOR_H
#define GPLATESDATAMINING_DATASELECTOR_H

#include <vector>
#include <string>
#include <QString>
#include <map>
#include <boost/unordered_map.hpp>

#include "CoRegConfigurationTable.h"
#include "CoRegFilterCache.h"
#include "DataTable.h"

#include "app-logic/ReconstructedFeatureGeometry.h"
#include "app-logic/ReconstructUtils.h"
#include "app-logic/AppLogicUtils.h"
#include "app-logic/ReconstructionGeometryUtils.h"

#include "feature-visitors/GeometryFinder.h"

#include "model/FeatureHandle.h"
#include "app-logic/Reconstruction.h"
#include "app-logic/ReconstructedFeatureGeometry.h"

#include "utils/UnicodeStringUtils.h"

namespace GPlatesDataMining
{
	class DataSelector
	{
	public:
		typedef boost::unordered_map< 
				const GPlatesModel::FeatureHandle*,
				std::vector<const GPlatesAppLogic::ReconstructedFeatureGeometry*> 
									> FeatureRFGMap;

		typedef boost::unordered_map< 
				const GPlatesModel::FeatureCollectionHandle*,
				std::vector<const GPlatesAppLogic::ReconstructedFeatureGeometry*> 
									> FeatureCollectionRFGMap;

		static
		boost::shared_ptr<DataSelector> 
		create(
				const CoRegConfigurationTable& table)
		{
			return boost::shared_ptr<DataSelector>(new DataSelector(table));
		}

		
		/*
		* Given the seed and target, select() will return the associated data in DataTable.
		*/
		void
		select(
				const std::vector<GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_type>& seed_collection,	
				const std::vector<GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_type>& co_reg_collection,								
				DataTable& ret														
				);

		static
		void
		set_data_table(
				const DataTable& table)
		{
			d_data_table = table;
		}

		static
		const DataTable&
		get_data_table() 
		{
			return d_data_table;
		}

		void
		populate_table_header(DataTable& data_table) const;

		~DataSelector()
		{ }
		
	protected:
		/*
		* Get target feature collections from input table.
		*/
		void
		get_target_collections(
				std::vector<const GPlatesModel::FeatureCollectionHandle*>& ) const;				
		/*
		* Check if the configuration table is still valid.
		*/
		bool
		is_cfg_table_valid() const;

		void
		fill_seed_info(
				const GPlatesModel::FeatureHandle*,
				DataRowSharedPtr);
		
		//default constructor
		DataSelector();

		//copy constructor
		DataSelector(const DataSelector&);
		
		//assignment 
		const DataSelector&
		operator=(const DataSelector&);

		DataSelector(const CoRegConfigurationTable &table) 
			: d_cfg_table(table)
		{
			if(!d_cfg_table.is_optimized())
				d_cfg_table.optimize();
		}

		CoRegConfigurationTable d_cfg_table;

		FeatureCollectionRFGMap d_target_fc_rfg_map;
		FeatureRFGMap d_seed_feature_rfg_map;

		/*
		* TODO:
		* Need to remove the "static" in the future.
		* Find a way to move result data more gracefully.
		*/
		static DataTable d_data_table;
	};
}

#endif


