/* $Id$ */

/**
 * \file .
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
#include <map>
#include <algorithm>
#include <QCoreApplication>

#include "global/CompilerWarnings.h"
#include <boost/assign.hpp>
#include <boost/foreach.hpp>

#include "CoRegFilterMapReduceWorkFlowFactory.h"
#include "DataSelector.h"
#include "DataMiningUtils.h"
#include "RegionOfInterestFilter.h"

#include "maths/Real.h"
#include "maths/SphericalArea.h"

#include "utils/Profile.h"

using namespace GPlatesDataMining;

#if 0
//#define GPLATES_MULTI_THREADS
//#ifdef GPLATES_MULTI_THREADS

void
GPlatesDataMining::DataSelector::select(
		const GPlatesModel::FeatureCollectionHandle::const_weak_ref& seed_collection,	
		boost::optional< GPlatesAppLogic::Reconstruction& > reconstruction_,						
		DataTable& ret)
{
	std::vector< GPlatesModel::FeatureCollectionHandle::const_weak_ref > 
		target_collection;
	
	get_target_collections_from_input_table(target_collection);
	construct_geometry_map(
			seed_collection,
			target_collection,
			reconstruction_);

	GPlatesModel::FeatureCollectionHandle::const_iterator it = seed_collection->begin();
	GPlatesModel::FeatureCollectionHandle::const_iterator it_end = seed_collection->end();

	TableDesc table_desc;
	get_table_desc_from_input_table(table_desc);
	ret.set_table_desc(table_desc);

	TaskQueue q;q.init();
	for(; it != it_end; it++)//for each seed feature
	{
		qDebug() << "select data from seed feature.";
		SubDataSelector* obj = new SubDataSelector(
				d_configuration_table,
				(*it)->reference(),
				d_seed_geometry_map,
				d_target_geometry_map);
		q.add(obj);
	}
	q.shutdown();
	qDebug() << "data selector returned!";
	return;
}
#endif 


// The BOOST_FOREACH macro in versions of boost before 1.37 uses the same local
// variable name in each instantiation. Nested BOOST_FOREACH macros therefore
// cause GCC to warn about shadowed declarations.
DISABLE_GCC_WARNING("-Wshadow")

void
DataSelector::select(
		const GPlatesModel::FeatureCollectionHandle::const_weak_ref& seed_collection,	
		const GPlatesAppLogic::Reconstruction* reconstruction_,						
		DataTable& ret)
{
	PROFILE_FUNC();
	std::vector< GPlatesModel::FeatureCollectionHandle::const_weak_ref > 
		target_collection;
	
	//do the reconstruction
	get_target_collections_from_input_table(target_collection);
	construct_geometry_map( seed_collection, target_collection, reconstruction_);

	//populate the table description
	TableDesc table_desc;
	get_table_desc_from_input_table(table_desc);
	ret.set_table_desc(table_desc);

	//for each seed feature
	BOOST_FOREACH(const GPlatesModel::FeatureHandle::non_null_ptr_to_const_type &seed_feature, *seed_collection)
	{
		DataRowSharedPtr row = DataRowSharedPtr(new DataRow); 
		d_associated_data_cache.clear();

		//for each row in input table
		BOOST_FOREACH(const ConfigurationTableRow &co_reg_info, d_configuration_table)
		{
			//firstly, try to get data from cache
			boost::shared_ptr< const AssociationOperator::AssociatedCollection > 
				associated_data_ptr = 
					retrieve_associated_data_from_cache(
							co_reg_info.association_parameters,
							co_reg_info.target_feature_collection_handle);

			if(!associated_data_ptr)
			{
				//if unfortunately we did not find data in cache
				boost::scoped_ptr< AssociationOperator > association_operator( 
						AssociationOperatorFactory::create(
								co_reg_info.association_operator_type,
								co_reg_info.association_parameters));

				association_operator->execute(
						seed_feature->reference(),
						co_reg_info.target_feature_collection_handle,
						d_seed_geometry_map,
						d_target_geometry_map);

				associated_data_ptr = association_operator->get_associated_collection_ptr();
				insert_associated_data_into_cache(
						associated_data_ptr,
						co_reg_info.target_feature_collection_handle);
			}

			boost::scoped_ptr< DataOperator > data_operator(
					DataOperatorFactory::create(
							co_reg_info.data_operator_type,
							co_reg_info.data_operator_parameters));
			
			data_operator->get_data(
					*associated_data_ptr,
					co_reg_info.attribute_name,
					*row);
		}//end of second loop (for each row in matrix)
		QCoreApplication::processEvents();
		ret.push_back( row );
	}//end of first loop (for each seed)
}//end of the function

// See above
ENABLE_GCC_WARNING("-Wshadow")


namespace
{
	void
	get_feature_handle_vector(
			GPlatesModel::FeatureCollectionHandle::const_weak_ref feature_collection,
			std::vector<const GPlatesModel::FeatureHandle*>& ret)
	{
		BOOST_FOREACH(const GPlatesModel::FeatureHandle::non_null_ptr_to_const_type& feature, *feature_collection)
		{
			ret.push_back(feature.get());
		}
	}
}


namespace
{
	using namespace GPlatesAppLogic;

	void
	convert_to_reconstructed_feature_geometry_vector(
			const std::vector<ReconstructionGeometryCollection::non_null_ptr_to_const_type>& input,
			std::vector<const ReconstructedFeatureGeometry*>& output)
	{
		BOOST_FOREACH(const GPlatesAppLogic::ReconstructionGeometryCollection::non_null_ptr_to_const_type& rgc, input)
		{
			GPlatesAppLogic::ReconstructionGeometryCollection::const_iterator it = rgc->begin();
			GPlatesAppLogic::ReconstructionGeometryCollection::const_iterator it_end = rgc->end();
			for(; it != it_end; it++)
			{
				//use dynamic case here since we are expecting ReconstructedFeatureGeometry
				//it would be faster than double dispatch visitor
				//the performance would not be a problem because the heritage structure is shallow. 
				const GPlatesAppLogic::ReconstructedFeatureGeometry* rfg = 
					dynamic_cast<const GPlatesAppLogic::ReconstructedFeatureGeometry*>((*it).get());
				if(!rfg)
				{
					qWarning() << "Failed to cast from ReconstructionGeometry* to ReconstructedFeatureGeometry*";
					continue; //We are expecting ReconstructedFeatureGeometry, the dynamic cast should not fail
				}
				else
				{
					output.push_back(rfg);
				}
			}
		}
	}

	bool
	is_feature_collection_contains_this_feature(
			const GPlatesModel::FeatureCollectionHandle* feature_collection,
			const GPlatesModel::FeatureHandle* feature)
	{
		BOOST_FOREACH(const GPlatesModel::FeatureHandle::non_null_ptr_to_const_type& fh, *feature_collection)
		{
			if(fh.get() == feature)
			{
				return true;
			}
		}
		return false;
	}

	bool
	less_area(
			const GPlatesAppLogic::ReconstructedFeatureGeometry* rfg1,
			const GPlatesAppLogic::ReconstructedFeatureGeometry* rfg2)
	{
		using namespace GPlatesMaths::SphericalArea;
		const PolygonOnSphere* p1 = dynamic_cast<const PolygonOnSphere*>(rfg1->geometry().get());
		const PolygonOnSphere* p2 = dynamic_cast<const PolygonOnSphere*>(rfg2->geometry().get());
		if(p1 && p2)
		{
			return calculate_polygon_area(*p1) < calculate_polygon_area(*p2);
		}
		if(p1)
		{
			return false;
		}
		if(p2)
		{
			return true;
		}
		return false;
	}

	std::vector< const GPlatesAppLogic::ReconstructedFeatureGeometry* >
	convert_reconstruction_geometry_collection_to_reconstructed_feature_geometry_vector(
		GPlatesAppLogic::ReconstructionGeometryCollection::const_iterator begin,
		GPlatesAppLogic::ReconstructionGeometryCollection::const_iterator end)
	{
		std::vector< const GPlatesAppLogic::ReconstructedFeatureGeometry* > ret;
		for(; begin != end; begin++)
		{
			//use dynamic case here since we are expecting ReconstructedFeatureGeometry
			//it would be faster than double dispatch visitor
			//the performance would not be a problem because the heritage structure is shallow. 
			const GPlatesAppLogic::ReconstructedFeatureGeometry* rfg = 
				dynamic_cast<const GPlatesAppLogic::ReconstructedFeatureGeometry*>((*begin).get());
			if(!rfg)
			{
				continue;
			}
			else
			{
				ret.push_back(rfg);
			}
		}
		std::sort(ret.begin(),ret.end(),less_area);
		return ret;
	}


// The BOOST_FOREACH macro in versions of boost before 1.37 uses the same local
// variable name in each instantiation. Nested BOOST_FOREACH macros therefore
// cause GCC to warn about shadowed declarations.
DISABLE_GCC_WARNING("-Wshadow")

	void
	construct_feature_collection_handle_and_reconstructed_feature_geometries_map(
			const std::vector<ReconstructionGeometryCollection::non_null_ptr_to_const_type>& input,
			std::vector< GPlatesModel::FeatureCollectionHandle::const_weak_ref > feature_collections,
			std::map< const GPlatesModel::FeatureCollectionHandle*,
					  std::vector<const ReconstructedFeatureGeometry*> >& output)
	{
		BOOST_FOREACH(const GPlatesModel::FeatureCollectionHandle::const_weak_ref& fc, feature_collections)
		{
			std::vector<const ReconstructedFeatureGeometry*> empty_vector;
			output.insert(std::make_pair(fc.handle_ptr(),empty_vector));
		}

		BOOST_FOREACH(const GPlatesAppLogic::ReconstructionGeometryCollection::non_null_ptr_to_const_type& rgc, input)
		{
			GPlatesAppLogic::ReconstructionGeometryCollection::const_iterator it = rgc->begin();
			GPlatesAppLogic::ReconstructionGeometryCollection::const_iterator it_end = rgc->end();
			if(it == it_end)//empty ReconstructionGeometryCollection
			{
				continue;
			}

			//use dynamic case here since we are expecting ReconstructedFeatureGeometry
			//it would be faster than double dispatch visitor
			//the performance should not be a problem because the heritage structure is flat. 
			const GPlatesAppLogic::ReconstructedFeatureGeometry* rfg = 
				dynamic_cast<const GPlatesAppLogic::ReconstructedFeatureGeometry*>((*it).get());
			
			if(!rfg)
			{
				continue; //We are expecting ReconstructedFeatureGeometry, the dynamic cast should not fail
			}

			BOOST_FOREACH(const GPlatesModel::FeatureCollectionHandle::const_weak_ref& fc, feature_collections)
			{
				if(is_feature_collection_contains_this_feature(fc.handle_ptr(),rfg->feature_handle_ptr()))
				{
					output[fc.handle_ptr()] = 
						convert_reconstruction_geometry_collection_to_reconstructed_feature_geometry_vector(it, it_end);
				}
			}

		}
	}

// See above
ENABLE_GCC_WARNING("-Wshadow")


	void
	construct_feature_and_reconstructed_feature_geometries_map(
			const std::vector<GPlatesAppLogic::ReconstructionGeometryCollection::non_null_ptr_to_const_type>& rgcs,
			std::map< const GPlatesModel::FeatureHandle*,
					std::vector<const ReconstructedFeatureGeometry*> >& output)
	{
		BOOST_FOREACH(const GPlatesAppLogic::ReconstructionGeometryCollection::non_null_ptr_to_const_type& rgc, rgcs)
		{
			GPlatesAppLogic::ReconstructionGeometryCollection::const_iterator it = rgc->begin();
			GPlatesAppLogic::ReconstructionGeometryCollection::const_iterator it_end = rgc->end();
			for(; it != it_end; it++)
			{
				const GPlatesAppLogic::ReconstructedFeatureGeometry* rfg = 
					dynamic_cast<const GPlatesAppLogic::ReconstructedFeatureGeometry*>((*it).get());
				if(!rfg)
				{
					continue;
				}
				else
				{
					std::map< 
							const GPlatesModel::FeatureHandle*,
							std::vector<const ReconstructedFeatureGeometry*> >::iterator map_it = 
								output.find( rfg->feature_handle_ptr());
					if( map_it != output.end() )
					{
						map_it->second.push_back(rfg);
					}
					else
					{
						//create a vector of length 1 which is initialized with rfg.
						output[rfg->feature_handle_ptr()] = std::vector< const ReconstructedFeatureGeometry* > (1, rfg);
					}
				}
			}
		}
	}
}


// The BOOST_FOREACH macro in versions of boost before 1.37 uses the same local
// variable name in each instantiation. Nested BOOST_FOREACH macros therefore
// cause GCC to warn about shadowed declarations.
DISABLE_GCC_WARNING("-Wshadow")

void
DataSelector::select(
		const std::vector<GPlatesAppLogic::ReconstructionGeometryCollection::non_null_ptr_to_const_type>
			&seed_collection,	
		const std::vector<GPlatesAppLogic::ReconstructionGeometryCollection::non_null_ptr_to_const_type>
			&co_reg_collection,							
		DataTable& data_table)
{
	using namespace GPlatesAppLogic;

	//prepare the input data from layers
	FeatureCollectionRFGMap target_feature_collection_rfg_map;
	FeatureRFGMap seed_feature_rfg_map;
		
	std::vector< GPlatesModel::FeatureCollectionHandle::const_weak_ref > target_collections;
	get_target_collections_from_input_table(target_collections);
	construct_feature_and_reconstructed_feature_geometries_map(seed_collection, seed_feature_rfg_map);
	construct_feature_collection_handle_and_reconstructed_feature_geometries_map(
			co_reg_collection,
			target_collections,
			target_feature_collection_rfg_map);

	if(!is_cfg_table_valid())
	{
		qWarning() << "The configuration table contains invalid data.";
		d_configuration_table.clear();
		return;
	}
	//the main loop
	//for each seed feature
	BOOST_FOREACH(const FeatureRFGMap::value_type& seed_feature_and_rfg_pair, seed_feature_rfg_map)
	{
		DataRowSharedPtr row = DataRowSharedPtr(new DataRow); 
		row->set_seed_rfgs(seed_feature_and_rfg_pair.second);

		//for each row in input table
		BOOST_FOREACH(const ConfigurationTableRow &config_row, d_configuration_table)
		{
			OpaqueData cell;
			const GPlatesModel::FeatureCollectionHandle* fh = 
					config_row.target_feature_collection_handle.handle_ptr();
			const ConfigurationTableRow optimised_cfg_row = 
					optimize_cfg_table_row(
							config_row,
							seed_feature_and_rfg_pair.first,
							fh);
			
			boost::shared_ptr< CoRegFilterMapReduceWorkFlow > workflow =
				FilterMapReduceWorkFlowFactory::create(optimised_cfg_row, seed_feature_and_rfg_pair.second);

			if(!workflow)
			{
				qWarning() << "Failed to create CoRegFilterMapReduceWorkFlow.";
				continue;
			}

			workflow->execute(
					target_feature_collection_rfg_map[fh].begin(),
					target_feature_collection_rfg_map[fh].end(),
					cell);
			row->append_cell(cell);
		}
		data_table.push_back(row);
	}
	return;
}

// See above
ENABLE_GCC_WARNING("-Wshadow")


void
GPlatesDataMining::DataSelector::get_target_collections_from_input_table(
		std::vector< GPlatesModel::FeatureCollectionHandle::const_weak_ref >&  target_collection)
{
	CoRegConfigurationTable::const_iterator it = d_configuration_table.begin();
	CoRegConfigurationTable::const_iterator it_end = d_configuration_table.end();
	for(; it != it_end; it++)
	{
		if(	std::find(
					target_collection.begin(),
					target_collection.end(),
					it->target_feature_collection_handle) ==
			target_collection.end() )
		{
			target_collection.push_back(it->target_feature_collection_handle);
		}
	}
}


void
GPlatesDataMining::DataSelector::get_table_desc_from_input_table(
		TableDesc& table_desc)			
{
	CoRegConfigurationTable::const_iterator it = d_configuration_table.begin();
	CoRegConfigurationTable::const_iterator it_end = d_configuration_table.end();
	for(; it != it_end; it++)
	{
		table_desc.push_back( it->attribute_name );
	}
	return;
}


void
GPlatesDataMining::DataSelector::get_reconstructed_geometries(
		GPlatesModel::FeatureHandle::non_null_ptr_to_const_type feature,
		std::vector< GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type	>& geometries)
{
	using namespace GPlatesFeatureVisitors;
	GeometryFinder geo_finder;
	geo_finder.visit_feature( feature->reference() );

	geometries.insert(
			geometries.end(),
			geo_finder.found_geometries_begin(),
			geo_finder.found_geometries_end());
}


void
GPlatesDataMining::DataSelector::get_reconstructed_geometries(
		GPlatesModel::FeatureHandle::non_null_ptr_to_const_type feature,
		std::vector< GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type	>& geometries,
		const GPlatesAppLogic::Reconstruction& rec)
{
	using namespace GPlatesFeatureVisitors;
	GeometryFinder geo_finder;
	geo_finder.visit_feature( feature->reference() );
	
	GeometryFinder::geometry_container_const_iterator it = 
		geo_finder.found_geometries_begin();
	GeometryFinder::geometry_container_const_iterator it_end = 
		geo_finder.found_geometries_end();

	GPlatesAppLogic::ReconstructionFeatureProperties properties(
			rec.get_reconstruction_time());
	properties.visit_feature(feature->reference());
	
	if(!properties.is_feature_defined_at_recon_time())
		return;

	for(; it != it_end; it++)
	{
		if(properties.get_recon_plate_id())
		{
			GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry =
				GPlatesAppLogic::ReconstructUtils::reconstruct(
						*it,
						*properties.get_recon_plate_id(),
						*rec.get_default_reconstruction_tree());
			geometries.push_back(geometry);
		}
		else
		{
			geometries.push_back(*it);
		}
	}

}


void
GPlatesDataMining::DataSelector::construct_geometry_map(
		const std::vector<GPlatesAppLogic::ReconstructionGeometryCollection::non_null_ptr_to_const_type>& RGCs,
		FeatureGeometryMap& the_map)
{
	BOOST_FOREACH(const GPlatesAppLogic::ReconstructionGeometryCollection::non_null_ptr_to_const_type& RGC, RGCs)
	{
		GPlatesAppLogic::ReconstructionGeometryCollection::const_iterator it = RGC->begin();
		GPlatesAppLogic::ReconstructionGeometryCollection::const_iterator it_end = RGC->end();
		for(; it != it_end; it++)
		{
			const GPlatesAppLogic::ReconstructedFeatureGeometry* RFG = 
				dynamic_cast<const GPlatesAppLogic::ReconstructedFeatureGeometry*>((*it).get());
			if(!RFG)
			{
				continue;
			}
			else
			{
				FeatureGeometryMap::iterator map_it = 
						the_map.find( RFG->feature_handle_ptr());
				if( map_it != the_map.end() )
				{
					map_it->second.push_back(RFG->geometry());
				}
				else
				{
					std::vector< GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type > tmp(1,RFG->geometry());
					the_map[RFG->feature_handle_ptr()] = tmp;
				}
			}
		}
	}
}


void
GPlatesDataMining::DataSelector::construct_geometry_map(
		const GPlatesModel::FeatureCollectionHandle::const_weak_ref& feature_collection,
		FeatureGeometryMap& the_map,
		const GPlatesAppLogic::Reconstruction* reconstruction)

{
	GPlatesModel::FeatureCollectionHandle::const_iterator it = feature_collection->begin();
	GPlatesModel::FeatureCollectionHandle::const_iterator it_end = feature_collection->end();

	for(; it != it_end; it++)
	{
		std::vector<
			GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type 
		> tmp;

		FeatureGeometryMap::iterator map_it = 
			the_map.find( (*it).get());

		if( map_it != the_map.end() )
		{
			//There is already a entry in the map, something is wrong
			return;
		}
		else
		{
			if(reconstruction)
			{
				get_reconstructed_geometries( *it, tmp, *reconstruction);
			}
			else
			{
				get_reconstructed_geometries( *it, tmp );
			}
			the_map[(*it).get()] = tmp;
		}
	}
}


void
GPlatesDataMining::DataSelector::construct_geometry_map(
		const GPlatesModel::FeatureCollectionHandle::const_weak_ref& seed_collection,	
		const std::vector< GPlatesModel::FeatureCollectionHandle::const_weak_ref >& target_collection,							/*In*/
		const GPlatesAppLogic::Reconstruction* reconstruction)													/*In*/
{
	d_seed_geometry_map.clear();
	d_target_geometry_map.clear();

	construct_geometry_map(
			seed_collection,
			d_seed_geometry_map,
			reconstruction);

	std::vector < GPlatesModel::FeatureCollectionHandle::const_weak_ref >::const_iterator
		it = target_collection.begin();
	std::vector < GPlatesModel::FeatureCollectionHandle::const_weak_ref >::const_iterator 
		it_end = target_collection.end();

	for(; it != it_end; it++)
	{
		construct_geometry_map(
				(*it),
				d_target_geometry_map,
				reconstruction);
	}
}


boost::shared_ptr< const AssociationOperator::AssociatedCollection > 
DataSelector::retrieve_associated_data_from_cache(
		AssociationOperatorParameters cfg,
		GPlatesModel::FeatureCollectionHandle::const_weak_ref target_feature_collection,
		const CacheMap& cache_map)
{
	using namespace GPlatesDataMining;
	std::pair< CacheMap::const_iterator, CacheMap::const_iterator > ret =
	cache_map.equal_range(target_feature_collection);
	CacheMap::const_iterator it;

	for (it = ret.first; it != ret.second; ++it)
	{
		if(cfg.d_associator_type != it->second->d_associator_cfg.d_associator_type)
		{
			continue;
		}
		if(REGION_OF_INTEREST == cfg.d_associator_type)
		{
			if(0.0 == GPlatesMaths::Real(cfg.d_ROI_range - it->second->d_associator_cfg.d_ROI_range))
			{
				return it->second;
			}
			if(cfg.d_ROI_range < it->second->d_associator_cfg.d_ROI_range)
			{
				boost::shared_ptr< AssociationOperator::AssociatedCollection > result(
						new AssociationOperator::AssociatedCollection);
				result->d_reconstruction_time = it->second->d_reconstruction_time;
				result->d_associator_cfg =  it->second->d_associator_cfg;
				result->d_seed = it->second->d_seed;
				AssociationOperator::AssociatedCollection::FeatureDistanceMap::const_iterator 
					inner_it =  it->second->d_associated_features.begin();
				AssociationOperator::AssociatedCollection::FeatureDistanceMap::const_iterator 
					inner_it_end =  it->second->d_associated_features.end();
				for(; inner_it != inner_it_end; ++inner_it)
				{
					if(DataMiningUtils::minimum(inner_it->second) <= cfg.d_ROI_range)
					{
						result->d_associated_features.insert(*inner_it);
					}
				}
#ifdef _DEBUG
				qDebug() << "Associated data cache hit!";
#endif
				return result;
			}
		}
	}
	return boost::shared_ptr< const AssociationOperator::AssociatedCollection >();
}


void
DataSelector::insert_associated_data_into_cache(
		boost::shared_ptr< const AssociationOperator::AssociatedCollection > data,
		GPlatesModel::FeatureCollectionHandle::const_weak_ref target_feature_collection,
		CacheMap& cache_map)
{
	std::pair<	GPlatesModel::FeatureCollectionHandle::const_weak_ref, 
				boost::shared_ptr< const AssociationOperator::AssociatedCollection > >
				p( target_feature_collection, data );
		cache_map.insert(p);
}


void
DataSelector::insert_associated_data_into_cache(
		boost::shared_ptr< const AssociationOperator::AssociatedCollection > data,
		GPlatesModel::FeatureCollectionHandle::const_weak_ref  target_feature_collection)
{
	insert_associated_data_into_cache(
			data,
			target_feature_collection,
			d_associated_data_cache);
}


boost::shared_ptr< const AssociationOperator::AssociatedCollection > 
DataSelector::retrieve_associated_data_from_cache(
		AssociationOperatorParameters cfg,
		GPlatesModel::FeatureCollectionHandle::const_weak_ref target_feature_collection)
{
	return retrieve_associated_data_from_cache(
			cfg,
			target_feature_collection,
			d_associated_data_cache);
}


const ConfigurationTableRow
DataSelector::optimize_cfg_table_row(
		const ConfigurationTableRow& cfg_row,
		const GPlatesModel::FeatureHandle* seed_feature,
		const GPlatesModel::FeatureCollectionHandle* target_feature_collection)
{
	ConfigurationTableRow modified_cfg_row = cfg_row;
	if(is_feature_collection_contains_this_feature(target_feature_collection,seed_feature))
	{
		if(0.0 == GPlatesMaths::Real(cfg_row.association_parameters.d_ROI_range))
		{
			//The target feature collection contains the seed feature and 
			//the region of interest is zero, which implies the user wants
			//to extract data from seed feature.
			modified_cfg_row.association_operator_type = SEED_ITSELF;
		}
	}
	return modified_cfg_row;
}


bool
DataSelector::is_cfg_table_valid()
{
	BOOST_FOREACH(const ConfigurationTableRow &config_row, d_configuration_table)
	{
		if(!config_row.target_feature_collection_handle.is_valid())
		{
			return false;
		}
	}
	return true;
}


// Suppress warning with boost::variant with Boost 1.34 and g++ 4.2.
// This is here at the end of the file because the problem resides in a template
// being instantiated at the end of the compilation unit.
DISABLE_GCC_WARNING("-Wshadow")

