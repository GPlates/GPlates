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
#include <QCoreApplication>

#include "global/CompilerWarnings.h"
#include <boost/assign.hpp>
#include <boost/foreach.hpp>

#include "CoRegFilterCache.h"
#include "CoRegFilterMapReduceFactory.h"
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
		
		BOOST_FOREACH(FeatureHandle::non_null_ptr_to_const_type fh, *feature_collection)
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
	gen_feature_collection_rfgs_map(
			const std::vector<ReconstructedFeatureGeometry*>& input_rfgs,
			const std::vector<const FeatureCollectionHandle*>& fcs,
			DataSelector::FeatureCollectionRFGMap& output)
	{	
		// Iterate over the feature collections.
		BOOST_FOREACH(const FeatureCollectionHandle* fc, fcs)
		{
			typedef std::vector< const ReconstructedFeatureGeometry* > rfg_seq_type;

			// Iterate over the input RFGs and associate each one with the current feature collection
			// *if* they came from there.
			rfg_seq_type output_rfgs;
			BOOST_FOREACH(const ReconstructedFeatureGeometry* rfg, input_rfgs)
			{
				if(is_feature_collection_contains_this_feature(fc, rfg->feature_handle_ptr()))
				{
					output_rfgs.push_back(rfg);
				}
			}
			// Assign the vector of RFGs to their feature collection.
			output[fc] = output_rfgs;
		}
	}

	void
	gen_feature_collection_rfgs_map(
			const std::vector<ReconstructedFeatureGeometry::non_null_ptr_type>& input_rfgs,
			const std::vector<const FeatureCollectionHandle*>& feature_collections,
			DataSelector::FeatureCollectionRFGMap& output)
{
	std::vector<ReconstructedFeatureGeometry*> rfgs;
	BOOST_FOREACH(const ReconstructedFeatureGeometry::non_null_ptr_type r, input_rfgs)
	{
		rfgs.push_back(r.get());
	}
	return gen_feature_collection_rfgs_map(rfgs, feature_collections, output);
}

	void
	gen_feature_collection_rfgs_map(
			const std::vector<ReconstructedFeatureGeometry::non_null_ptr_type>& input_rfgs,
			const std::vector<FeatureCollectionHandle::const_weak_ref>& feature_collections,
			DataSelector::FeatureCollectionRFGMap& output)
	{
		std::vector<const FeatureCollectionHandle*> fcs;
		BOOST_FOREACH(const FeatureCollectionHandle::const_weak_ref fh, feature_collections)
		{
			fcs.push_back(fh.handle_ptr());
		}
		return gen_feature_collection_rfgs_map(input_rfgs, fcs, output);
	}

// See above
ENABLE_GCC_WARNING("-Wshadow")

	void
	gen_feature_rfg_map(
			const std::vector<const ReconstructedFeatureGeometry*>& rfgs,
			DataSelector::FeatureRFGMap& output)
	{
		BOOST_FOREACH(const ReconstructedFeatureGeometry* r, rfgs)
		{
			GPlatesModel::FeatureHandle* f = r->feature_handle_ptr();
			DataSelector::FeatureRFGMap::iterator map_it = output.find(f);

			if( map_it != output.end() )//found it.
			{
				map_it->second.push_back(r);
			}
			else
			{
				//create a vector of length 1 which is initialized with rfg.
				output[f] = std::vector<const ReconstructedFeatureGeometry*>(1, r);
			}
		}
	}

	void
	gen_feature_rfg_map(
			const std::vector<ReconstructedFeatureGeometry::non_null_ptr_type>& rgcs,
			DataSelector::FeatureRFGMap& output)
	{
		std::vector<const ReconstructedFeatureGeometry*> rfgs;
		BOOST_FOREACH(const ReconstructedFeatureGeometry::non_null_ptr_type r, rgcs)
		{
			rfgs.push_back(r.get());
		}
		gen_feature_rfg_map(rfgs,output);
	}
}


// The BOOST_FOREACH macro in versions of boost before 1.37 uses the same local
// variable name in each instantiation. Nested BOOST_FOREACH macros therefore
// cause GCC to warn about shadowed declarations.
DISABLE_GCC_WARNING("-Wshadow")

void
GPlatesDataMining::DataSelector::select(
		const std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> &seed_rfgs,	
		const std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> &target_rfgs,							
		GPlatesDataMining::DataTable& data_table)
{
	if(!is_cfg_table_valid())
	{
		qWarning() << "The configuration table contains invalid data. Do nothing and return.";
		return;
	}
		
	std::vector<const FeatureCollectionHandle*> target_fcs;
	get_target_collections(target_fcs);

	//The data from layer framework cannot be used directly.
	//convert the input data 
	d_seed_feature_rfg_map.clear();
	d_target_fc_rfg_map.clear();
	gen_feature_rfg_map(seed_rfgs, d_seed_feature_rfg_map);
	gen_feature_collection_rfgs_map(target_rfgs, target_fcs, d_target_fc_rfg_map);

	populate_table_header(data_table);
	//for each seed feature
	BOOST_FOREACH(const FeatureRFGMap::value_type& seed_feature_rfg_pair, d_seed_feature_rfg_map)
	{
		DataRowSharedPtr row = DataRowSharedPtr(new DataRow); 
		fill_seed_info(seed_feature_rfg_pair.first,row);
		//append placeholder for real data below.
		row->append(d_cfg_table.size(),EmptyData);

		CoRegFilterCache filter_cache;
		//for each row in cfg table
		const CoRegConfigurationTable& const_table = d_cfg_table;//must use the const table.
		BOOST_FOREACH(const ConfigurationTableRow &config_row, const_table)
		{
			boost::shared_ptr< CoRegFilter > filter;
			boost::shared_ptr< CoRegMapper > mapper;
			boost::shared_ptr<CoRegReducer> reduer;
			boost::tie(filter,mapper,reduer)=
					create_filter_map_reduce(config_row, seed_feature_rfg_pair.second);
			//filter
			CoRegFilter::RFGVector filter_result;
			if(!filter_cache.find(config_row, filter_result))
			{
				const FeatureCollectionHandle* fh =	config_row.target_fc.handle_ptr();
				CoRegFilter::RFGVector::const_iterator it_s = d_target_fc_rfg_map[fh].begin(), it_e = d_target_fc_rfg_map[fh].end();
				filter->process(it_s, it_e,	filter_result);
				filter_cache.insert(config_row,filter_result);
			}

			//map
			CoRegMapper::MapperOutDataset map_result;
			FeatureRFGMap map_input;
			gen_feature_rfg_map(filter_result, map_input);//convert filter output to mapper input.
			mapper->process(map_input.begin(),map_input.end(),map_result);

			//reduce
			(*row)[config_row.index + data_table.data_index()] = 
					reduer->process(map_result.begin(), map_result.end());;
		}
		data_table.push_back(row);
	}
	return;
}

// See above
ENABLE_GCC_WARNING("-Wshadow")

void
GPlatesDataMining::DataSelector::get_target_collections(
		std::vector<const FeatureCollectionHandle*>&  target_collection) const
{
	const CoRegConfigurationTable& table = d_cfg_table;
	BOOST_FOREACH(const ConfigurationTableRow& row, table)
	{
		if(	std::find(
					target_collection.begin(),
					target_collection.end(),
					row.target_fc.handle_ptr()) == target_collection.end() )
		{
			target_collection.push_back(row.target_fc.handle_ptr());
		}
	}
}


bool
GPlatesDataMining::DataSelector::is_cfg_table_valid() const
{
	const CoRegConfigurationTable& table = d_cfg_table;
	BOOST_FOREACH(const ConfigurationTableRow &config_row, table)
	{
		if(!config_row.target_fc.is_valid())
			return false;
	}
	return true;
}


void
GPlatesDataMining::DataSelector::fill_seed_info(
		const GPlatesModel::FeatureHandle* feature,
		DataRowSharedPtr row)
{
	//Write out feature id as the first column so that each data row can be correlated. 
	//This is a temporary solution and will be removed when layer framework is ready to handle this.
	QString feature_id = feature->feature_id().get().qstring();
	row->append_cell(OpaqueData(feature_id));
	//seed valid time.
	row->append_cell(DataMiningUtils::get_property_value_by_name(feature,"validTime"));
}


void
GPlatesDataMining::DataSelector::populate_table_header(
		GPlatesDataMining::DataTable& data_table) const 
{
	//populate the table header
	TableHeader table_header;
	table_header.push_back("Seed Feature ID");
	table_header.push_back("Seed Valid Time");
	data_table.set_data_index(2);

	const CoRegConfigurationTable& table = d_cfg_table;
	
	BOOST_FOREACH(const ConfigurationTableRow& row, table)
	{
		table_header.push_back(row.attr_name);
	}
	data_table.set_table_header(table_header);
}

// Suppress warning with boost::variant with Boost 1.34 and g++ 4.2.
// This is here at the end of the file because the problem resides in a template
// being instantiated at the end of the compilation unit.
DISABLE_GCC_WARNING("-Wshadow")

