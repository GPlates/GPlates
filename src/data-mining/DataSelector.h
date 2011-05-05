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

#include "CoRegConfigurationTable.h"
#include "AssociationOperator.h"
#include "AssociationOperatorFactory.h"
#include "DataTable.h"
#include "DataOperator.h"
#include "DataOperatorFactory.h"

#include "app-logic/ReconstructUtils.h"
#include "app-logic/ReconstructedFeatureGeometryPopulator.h"
#include "app-logic/AppLogicUtils.h"
#include "app-logic/ReconstructionGeometryUtils.h"

#include "feature-visitors/GeometryFinder.h"

#include "model/FeatureHandle.h"
#include "app-logic/Reconstruction.h"
#include "app-logic/ReconstructedFeatureGeometry.h"
#include "app-logic/ReconstructionGeometryCollection.h"

#include "utils/UnicodeStringUtils.h"

namespace GPlatesDataMining
{
	/*
	* 
	*/
	class DataSelector
	{
	public:

		typedef std::map< const GPlatesModel::FeatureHandle*,
				std::vector<const GPlatesAppLogic::ReconstructedFeatureGeometry*> > FeatureRFGMap;

		typedef std::map< 
				const GPlatesModel::FeatureCollectionHandle*,
				std::vector<const GPlatesAppLogic::ReconstructedFeatureGeometry*> > FeatureCollectionRFGMap;

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
				const std::vector<GPlatesAppLogic::ReconstructionGeometryCollection::non_null_ptr_to_const_type>& seed_collection,	
				const std::vector<GPlatesAppLogic::ReconstructionGeometryCollection::non_null_ptr_to_const_type>& co_reg_collection,								
				DataTable& ret														
				);

		typedef 
		std::multimap< 
				GPlatesModel::FeatureCollectionHandle::const_weak_ref, 
				boost::shared_ptr< const AssociationOperator::AssociatedCollection > 
					> CacheMap;

		/*
		*
		*/
		static
		void
		insert_associated_data_into_cache(
				boost::shared_ptr< const AssociationOperator::AssociatedCollection >,
				GPlatesModel::FeatureCollectionHandle::const_weak_ref  target_feature_collection,
				CacheMap& cache_map);

		/*
		*
		*/
		static
		boost::shared_ptr< const AssociationOperator::AssociatedCollection > 
		retrieve_associated_data_from_cache(
				AssociationOperatorParameters,
				GPlatesModel::FeatureCollectionHandle::const_weak_ref target_feature_collection,
				const CacheMap& cache_map);

		static
		inline
		void
		set_data_table(
				const DataTable& table)
		{
			d_data_table = table;
		}

		static
		inline
		const DataTable&
		get_data_table()
		{
			return d_data_table;
		}

		~DataSelector()
		{ }
		
	protected:
		/*
		*
		*/
		void
		get_target_collections_from_input_table(
				std::vector< GPlatesModel::FeatureCollectionHandle::const_weak_ref >& );
		/*
		*
		*/
		void
		get_table_desc_from_input_table(
				TableDesc& table_desc);
		
		/*
		*
		*/
		void
		get_reconstructed_geometries(
				GPlatesModel::FeatureHandle::non_null_ptr_to_const_type feature,
				std::vector< GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type	>& geometries
				);
		
		/*
		*
		*/
		void
		get_reconstructed_geometries(
				GPlatesModel::FeatureHandle::non_null_ptr_to_const_type feature,
				std::vector< GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type	>& geometries,
				const GPlatesAppLogic::Reconstruction& rec);

		/*
		*
		*/
		void
		construct_geometry_map(
				const GPlatesModel::FeatureCollectionHandle::const_weak_ref& feature_collection,
				FeatureGeometryMap& the_map,
				const GPlatesAppLogic::Reconstruction* reconstruction);
		
		/*
		*
		*/
		void
		construct_geometry_map(
				const GPlatesModel::FeatureCollectionHandle::const_weak_ref& seed_collection,	/*In*/
				const std::vector< GPlatesModel::FeatureCollectionHandle::const_weak_ref >& target_collection,							/*In*/
				const GPlatesAppLogic::Reconstruction* reconstruction);

		/*
		*
		*/
		void
		construct_geometry_map(
				const std::vector<GPlatesAppLogic::ReconstructionGeometryCollection::non_null_ptr_to_const_type>&,
				FeatureGeometryMap& the_map);
		/*
		*
		*/
		boost::shared_ptr< const AssociationOperator::AssociatedCollection > 
		retrieve_associated_data_from_cache(
				AssociationOperatorParameters,
				GPlatesModel::FeatureCollectionHandle::const_weak_ref target_feature_collection);
		/*
		*
		*/
		void
		insert_associated_data_into_cache(
				boost::shared_ptr< const AssociationOperator::AssociatedCollection >,
				GPlatesModel::FeatureCollectionHandle::const_weak_ref  target_feature_collection);

		/*
		* Modify the @a cfg_row to optimize performance.
		*/
		const ConfigurationTableRow
		optimize_cfg_table_row(
				const ConfigurationTableRow& cfg_row,
				const GPlatesModel::FeatureHandle* seed_feature,
				const GPlatesModel::FeatureCollectionHandle* target_feature_collection);

		/*
		* Check if the configuration table is still valid.
		*/
		bool
		is_cfg_table_valid();
		
		//default constructor
		DataSelector();

		//copy constructor
		DataSelector(
				const DataSelector&);
		
		//assignment 
		const DataSelector&
		operator=(
				const DataSelector&);

		DataSelector(
				const CoRegConfigurationTable &table) 
			: d_configuration_table(table)
		{ }

		CoRegConfigurationTable d_configuration_table;

		FeatureGeometryMap d_seed_geometry_map;
		FeatureGeometryMap d_target_geometry_map;
		
		CacheMap d_associated_data_cache;

		static DataTable d_data_table;

	};
}

#endif


