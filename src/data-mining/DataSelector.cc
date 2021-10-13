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

#include "app-logic/ReconstructionFeatureProperties.h"

#include "feature-visitors/TotalReconstructionSequenceTimePeriodFinder.h"

#include "maths/Real.h"
#include "maths/SphericalArea.h"

#include "utils/Profile.h"

GPlatesDataMining::DataTable GPlatesDataMining::DataSelector::d_data_table;

using namespace GPlatesModel;
using namespace GPlatesAppLogic;

namespace
{
	using namespace GPlatesDataMining;

	bool
	is_feature_collection_contains_this_feature(
			const FeatureCollectionHandle* feature_collection,
			const FeatureHandle* feature)
	{
		if(!(feature_collection && feature))
			return false;
		
		BOOST_FOREACH(const FeatureHandle::non_null_ptr_to_const_type& fh, *feature_collection)
		{
			if(fh.get() == feature)
				return true;
		}
		return false;
	}

	bool
	less_area(
			const ReconstructedFeatureGeometry* rfg1,
			const ReconstructedFeatureGeometry* rfg2)
	{
		using namespace GPlatesMaths;
		const PolygonOnSphere* p1 = dynamic_cast<const PolygonOnSphere*>(rfg1->reconstructed_geometry().get());
		const PolygonOnSphere* p2 = dynamic_cast<const PolygonOnSphere*>(rfg2->reconstructed_geometry().get());
		
		// FIXME: If there are non-polygons mixed with polygon then it might be possible to
		// get islands of polygons (separated by non-polygons) that aren't sorted correctly.
		if(p1 && p2)
		{
			return p1->get_area() < p2->get_area();
		}
		if(p1)
			return false;
		
		if(p2)
			return true;
		
		return false;
	}


// The BOOST_FOREACH macro in versions of boost before 1.37 uses the same local
// variable name in each instantiation. Nested BOOST_FOREACH macros therefore
// cause GCC to warn about shadowed declarations.
DISABLE_GCC_WARNING("-Wshadow")

	void
	construct_feature_collection_handle_and_reconstructed_feature_geometries_map(
			const std::vector<ReconstructedFeatureGeometry::non_null_ptr_type>& input_rfgs,
			std::vector< FeatureCollectionHandle::const_weak_ref > feature_collections,
			DataSelector::FeatureCollectionRFGMap& output)
	{
		// Iterate over the feature collections.
		BOOST_FOREACH(const FeatureCollectionHandle::const_weak_ref& fc, feature_collections)
		{
			typedef std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> input_rfg_seq_type;
			typedef std::vector< const ReconstructedFeatureGeometry* > output_rfg_seq_type;

			// Iterate over the input RFGs and associate each one with the current feature collection
			// *if* they came from there.
			output_rfg_seq_type output_rfgs;
			for (input_rfg_seq_type::const_iterator input_rfg_iter = input_rfgs.begin();
				input_rfg_iter != input_rfgs.end();
				++input_rfg_iter)
			{
				const ReconstructedFeatureGeometry::non_null_ptr_type &input_rfg = *input_rfg_iter;

				if(is_feature_collection_contains_this_feature(fc.handle_ptr(), input_rfg->feature_handle_ptr()))
				{
					output_rfgs.push_back(input_rfg.get());
				}
			}

			// Sort the RFGs according to area if they are polygons.
			std::sort(output_rfgs.begin(), output_rfgs.end(), less_area);

			// Assign the vector of RFGs to their feature collection.
			output[fc.handle_ptr()] = output_rfgs;
		}
	}

// See above
ENABLE_GCC_WARNING("-Wshadow")

	void
	construct_feature_and_reconstructed_feature_geometries_map(
			const std::vector<ReconstructedFeatureGeometry::non_null_ptr_type>& rgcs,
			DataSelector::FeatureRFGMap& output)
	{
		typedef std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> rfg_seq_type;

		rfg_seq_type::const_iterator it = rgcs.begin(), it_end = rgcs.end();
		for(; it != it_end; it++)
		{
			const ReconstructedFeatureGeometry::non_null_ptr_type &rfg = *it;

			DataSelector::FeatureRFGMap::iterator map_it = output.find( rfg->feature_handle_ptr());
		
			if( map_it != output.end() )
			{
				map_it->second.push_back(rfg.get());
			}
			else
			{
				//create a vector of length 1 which is initialized with rfg.
				output[rfg->feature_handle_ptr()] = std::vector< const ReconstructedFeatureGeometry* > (1, rfg.get());
			}
		}
	}
}


// The BOOST_FOREACH macro in versions of boost before 1.37 uses the same local
// variable name in each instantiation. Nested BOOST_FOREACH macros therefore
// cause GCC to warn about shadowed declarations.
DISABLE_GCC_WARNING("-Wshadow")

void
GPlatesDataMining::DataSelector::select(
		const std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> &seed_collection,	
		const std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> &co_reg_collection,							
		GPlatesDataMining::DataTable& data_table)
{
	//prepare the input data from layers
	FeatureCollectionRFGMap target_feature_collection_rfg_map;
	FeatureRFGMap seed_feature_rfg_map;
		
	std::vector< FeatureCollectionHandle::const_weak_ref > target_collections;
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

	//populate the table description
	TableDesc table_desc;
	table_desc.push_back("Seed Feature ID");
	table_desc.push_back("Seed Feature begin time");
	table_desc.push_back("Seed Feature end time");
	get_table_desc_from_input_table(table_desc);
	data_table.set_table_desc(table_desc);

	//for each seed feature
	BOOST_FOREACH(const FeatureRFGMap::value_type& seed_feature_and_rfg_pair, seed_feature_rfg_map)
	{
		DataRowSharedPtr row = DataRowSharedPtr(new DataRow); 
		row->set_seed_rfgs(seed_feature_and_rfg_pair.second);
		append_seed_info(seed_feature_and_rfg_pair.first,row);
		//for each row in input table
		BOOST_FOREACH(const ConfigurationTableRow &config_row, d_configuration_table)
		{
			OpaqueData cell;
			const FeatureCollectionHandle* fh = 
					config_row.target_fc.handle_ptr();
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
		std::vector< FeatureCollectionHandle::const_weak_ref >&  target_collection)
{
	CoRegConfigurationTable::const_iterator 
		it = d_configuration_table.begin(), it_end = d_configuration_table.end();
	
	for(; it != it_end; it++)
	{
		if(	std::find(
					target_collection.begin(),
					target_collection.end(),
					it->target_fc) == target_collection.end() )
		{
			target_collection.push_back(it->target_fc);
		}
	}
}


void
GPlatesDataMining::DataSelector::get_table_desc_from_input_table(
		TableDesc& table_desc)			
{
	CoRegConfigurationTable::const_iterator 
		it = d_configuration_table.begin(), it_end = d_configuration_table.end();

	for(; it != it_end; it++)
	{
		table_desc.push_back( it->attr_name );
	}
	return;
}


void
GPlatesDataMining::DataSelector::get_reconstructed_geometries(
		FeatureHandle::non_null_ptr_to_const_type feature,
		std::vector< GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type	>& geometries)
{
	GPlatesFeatureVisitors::GeometryFinder geo_finder;
	geo_finder.visit_feature( feature->reference() );

	geometries.insert(
			geometries.end(),
			geo_finder.found_geometries_begin(),
			geo_finder.found_geometries_end());
}


void
GPlatesDataMining::DataSelector::get_reconstructed_geometries(
		FeatureHandle::non_null_ptr_to_const_type feature,
		std::vector< GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type	>& geometries,
		const GPlatesAppLogic::Reconstruction& rec)
{
	using namespace GPlatesFeatureVisitors;
	GeometryFinder geo_finder;
	geo_finder.visit_feature( feature->reference() );
	
	GeometryFinder::geometry_container_const_iterator 
		it = geo_finder.found_geometries_begin(),
		it_end = geo_finder.found_geometries_end();

	ReconstructionFeatureProperties properties(rec.get_reconstruction_time());
	properties.visit_feature(feature->reference());
	
	if(!properties.is_feature_defined_at_recon_time())
		return;

	for(; it != it_end; it++)
	{
		if(properties.get_recon_plate_id())
		{
			GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry =
				ReconstructUtils::reconstruct(
						*it,
						*properties.get_recon_plate_id(),
						*rec.get_default_reconstruction_layer_output()->get_reconstruction_tree());
			geometries.push_back(geometry);
		}
		else
			geometries.push_back(*it);
	}

}


void
GPlatesDataMining::DataSelector::construct_geometry_map(
		const std::vector<GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_type>& RGCs,
		FeatureGeometryMap& the_map)
{
	typedef std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> rfg_seq_type;

	rfg_seq_type::const_iterator it = RGCs.begin(), it_end = RGCs.end();
	for( ; it != it_end; it++)
	{
		const ReconstructedFeatureGeometry* RFG = it->get();

		FeatureGeometryMap::iterator map_it = the_map.find( RFG->feature_handle_ptr());
		if( map_it != the_map.end() )
			map_it->second.push_back(RFG->reconstructed_geometry());
		else
		{
			std::vector< GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type > tmp(1,RFG->reconstructed_geometry());
			the_map[RFG->feature_handle_ptr()] = tmp;
		}
	}
}


void
GPlatesDataMining::DataSelector::construct_geometry_map(
		const FeatureCollectionHandle::const_weak_ref& feature_collection,
		FeatureGeometryMap& the_map,
		const Reconstruction* reconstruction)

{
	FeatureCollectionHandle::const_iterator 
		it = feature_collection->begin(), it_end = feature_collection->end();

	for(; it != it_end; it++)
	{
		std::vector<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> tmp;

		FeatureGeometryMap::iterator map_it = the_map.find( (*it).get());

		if( map_it != the_map.end() )//The entry is already in the map. Do nothing and return.
			return;
		else
		{
			if(reconstruction)
				get_reconstructed_geometries( *it, tmp, *reconstruction);
			else
				get_reconstructed_geometries( *it, tmp );
			
			the_map[(*it).get()] = tmp;
		}
	}
}


void
GPlatesDataMining::DataSelector::construct_geometry_map(
		const FeatureCollectionHandle::const_weak_ref& seed_collection,	
		const std::vector< FeatureCollectionHandle::const_weak_ref >& target_collection,							/*In*/
		const Reconstruction* reconstruction)													/*In*/
{
	d_seed_geometry_map.clear();
	d_target_geometry_map.clear();

	construct_geometry_map(seed_collection, d_seed_geometry_map, reconstruction);

	std::vector < FeatureCollectionHandle::const_weak_ref >::const_iterator
		it = target_collection.begin(),
		it_end = target_collection.end();

	for(; it != it_end; it++)
		construct_geometry_map((*it), d_target_geometry_map, reconstruction);
}


boost::shared_ptr< const GPlatesDataMining::AssociationOperator::AssociatedCollection > 
GPlatesDataMining::DataSelector::retrieve_associated_data_from_cache(
		FilterCfg cfg,
		FeatureCollectionHandle::const_weak_ref target_feature_collection,
		const CacheMap& cache_map)
{
	std::pair< CacheMap::const_iterator, CacheMap::const_iterator > ret =
		cache_map.equal_range(target_feature_collection);
	CacheMap::const_iterator it = ret.first;

	for ( ; it != ret.second; ++it)
	{
		if(cfg.d_filter_type != it->second->d_associator_cfg.d_filter_type)
			continue;
		
		if(REGION_OF_INTEREST == cfg.d_filter_type)
		{
			if(0.0 == GPlatesMaths::Real(cfg.d_ROI_range - it->second->d_associator_cfg.d_ROI_range))
				return it->second;
			
			if(cfg.d_ROI_range < it->second->d_associator_cfg.d_ROI_range)
			{
				boost::shared_ptr< AssociationOperator::AssociatedCollection > result(
						new AssociationOperator::AssociatedCollection());
				result->d_reconstruction_time = it->second->d_reconstruction_time;
				result->d_associator_cfg =  it->second->d_associator_cfg;
				result->d_seed = it->second->d_seed;
				AssociationOperator::AssociatedCollection::FeatureDistanceMap::const_iterator 
					inner_it =  it->second->d_associated_features.begin(),
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
GPlatesDataMining::DataSelector::insert_associated_data_into_cache(
		boost::shared_ptr< const GPlatesDataMining::AssociationOperator::AssociatedCollection > data,
		FeatureCollectionHandle::const_weak_ref target_feature_collection,
		CacheMap& cache_map)
{
	cache_map.insert(std::make_pair(target_feature_collection, data));
}


void
GPlatesDataMining::DataSelector::insert_associated_data_into_cache(
		boost::shared_ptr< const GPlatesDataMining::AssociationOperator::AssociatedCollection > data,
		FeatureCollectionHandle::const_weak_ref  target_feature_collection)
{
	insert_associated_data_into_cache(
			data,
			target_feature_collection,
			d_associated_data_cache);
}


boost::shared_ptr< const GPlatesDataMining::AssociationOperator::AssociatedCollection > 
GPlatesDataMining::DataSelector::retrieve_associated_data_from_cache(
		GPlatesDataMining::FilterCfg cfg,
		FeatureCollectionHandle::const_weak_ref target_feature_collection)
{
	return retrieve_associated_data_from_cache(
			cfg,
			target_feature_collection,
			d_associated_data_cache);
}


const GPlatesDataMining::ConfigurationTableRow
GPlatesDataMining::DataSelector::optimize_cfg_table_row(
		const GPlatesDataMining::ConfigurationTableRow& cfg_row,
		const FeatureHandle* seed_feature,
		const FeatureCollectionHandle* target_feature_collection)
{
	ConfigurationTableRow modified_cfg_row = cfg_row;
	if(is_feature_collection_contains_this_feature(target_feature_collection,seed_feature))
	{
		if(0.0 == GPlatesMaths::Real(cfg_row.filter_cfg.d_ROI_range))
		{
			//The target feature collection contains the seed feature and 
			//the region of interest is zero, which implies the user wants
			//to extract data from seed feature.
			modified_cfg_row.filter_type = SEED_ITSELF;
		}
	}
	return modified_cfg_row;
}


bool
GPlatesDataMining::DataSelector::is_cfg_table_valid()
{
	BOOST_FOREACH(const ConfigurationTableRow &config_row, d_configuration_table)
	{
		if(!config_row.target_fc.is_valid())
			return false;
	}
	return true;
}


void
GPlatesDataMining::DataSelector::append_seed_info(
		const GPlatesModel::FeatureHandle* feature,
		DataRowSharedPtr row)
{
	//Write out feature id as the first column so that each data row can be correlated. 
	//This is a temporary solution and will be removed when layer framework is ready to handle this.
	QString feature_id = feature->feature_id().get().qstring();

	ReconstructionFeatureProperties visitor(false);
	visitor.visit_feature(WeakReference<const FeatureHandle>(*feature));

	boost::optional<GPlatesPropertyValues::GeoTimeInstant> 
		begin_time = visitor.get_time_of_appearance(), 
		end_time = visitor.get_time_of_dissappearance();

	row->append_cell(OpaqueData(feature_id));
	if(begin_time)
	{
		std::stringstream ss; ss << *begin_time ; 
		row->append_cell(OpaqueData(QString(ss.str().c_str()).remove(QRegExp("[()]"))));
	}
	else
	{
		row->append_cell(OpaqueData(EmptyData));
	}

	if(end_time)
	{
		std::stringstream ss; ss << *end_time ; 
		row->append_cell(OpaqueData(QString(ss.str().c_str()).remove(QRegExp("[()]"))));
	}
	else
	{
		row->append_cell(OpaqueData(EmptyData));
	}
}


// Suppress warning with boost::variant with Boost 1.34 and g++ 4.2.
// This is here at the end of the file because the problem resides in a template
// being instantiated at the end of the compilation unit.
DISABLE_GCC_WARNING("-Wshadow")

