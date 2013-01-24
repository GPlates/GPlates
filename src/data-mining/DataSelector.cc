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

#include <algorithm>
#include <map>
#include <QCoreApplication>
#include <QDebug>

#include "global/CompilerWarnings.h"
#include <boost/assign.hpp>
#include <boost/foreach.hpp>

#include "CoRegFilterCache.h"
#include "CoRegFilterMapReduceFactory.h"
#include "DataSelector.h"
#include "DataMiningUtils.h"
#include "IsCloseEnoughChecker.h"
#include "RegionOfInterestFilter.h"

#include "app-logic/RasterLayerProxy.h"
#include "app-logic/ReconstructionFeatureProperties.h"
#include "app-logic/ReconstructLayerProxy.h"

#include "feature-visitors/TotalReconstructionSequenceTimePeriodFinder.h"

#include "maths/Real.h"
#include "maths/SphericalArea.h"

#include "opengl/GLRasterCoRegistration.h"

#include "utils/Profile.h"

GPlatesDataMining::DataTable GPlatesDataMining::DataSelector::d_data_table;


// The BOOST_FOREACH macro in versions of boost before 1.37 uses the same local
// variable name in each instantiation. Nested BOOST_FOREACH macros therefore
// cause GCC to warn about shadowed declarations.
DISABLE_GCC_WARNING("-Wshadow")

void
GPlatesDataMining::DataSelector::select(
		const std::vector<GPlatesAppLogic::ReconstructContext::ReconstructedFeature> &reconstructed_seed_features,	
		const std::vector<GPlatesAppLogic::LayerProxy::non_null_ptr_type> &target_layer_proxies,
		const double &reconstruction_time,
		GPlatesDataMining::DataTable &result_data_table,
		boost::optional<RasterCoRegistration> co_register_rasters)
{
	if (!is_config_table_valid(target_layer_proxies))
	{
		qWarning() << "Co-registration configuration table invalid - skipping.";
		return;
	}
		
	result_data_table.set_data_index(d_data_index);
	result_data_table.set_table_header(d_table_header);

	//
	// Set up the co-registration result table.
	//

	BOOST_FOREACH(
			const GPlatesAppLogic::ReconstructContext::ReconstructedFeature &reconstructed_seed_feature,
			reconstructed_seed_features)
	{
		DataRowSharedPtr row = DataRowSharedPtr(new DataRow); 
		fill_seed_info(reconstructed_seed_feature, row);
		//append placeholder for real data below.
		row->append(d_cfg_table.size(), EmptyData);

		result_data_table.push_back(row);
	}

	//
	// Handle the configuration rows that co-register target *rasters*.
	//

	// If the necessary OpenGL extensions for raster co-registration are available the co-register,
	// otherwise leave the result data table entries are they are.
	if (co_register_rasters)
	{
		co_register_target_reconstructed_rasters(
				co_register_rasters->renderer,
				co_register_rasters->co_registration,
				reconstructed_seed_features,	
				reconstruction_time,
				result_data_table);
	}

	//
	// Handle the configuration rows that co-register target reconstructed *geometries*.
	//

	co_register_target_reconstructed_geometries(
			reconstructed_seed_features,	
			reconstruction_time,
			result_data_table);
}

// See above
ENABLE_GCC_WARNING("-Wshadow")


void
GPlatesDataMining::DataSelector::co_register_target_reconstructed_rasters(
		GPlatesOpenGL::GLRenderer &renderer,
		GPlatesOpenGL::GLRasterCoRegistration &raster_co_registration,
		const std::vector<GPlatesAppLogic::ReconstructContext::ReconstructedFeature> &reconstructed_seed_features,	
		const double &reconstruction_time,
		GPlatesDataMining::DataTable &result_data_table)
{
	// Need to iterate over 'const' table.
	const CoRegConfigurationTable &const_cfg_table = d_cfg_table;

	// Group rows by raster layer - it's more efficient to submit multiple operations per raster.
	// A raster is identified by its layer and the selected raster band name.
	typedef std::pair<GPlatesAppLogic::Layer, GPlatesUtils::UnicodeString/*band name*/> raster_id_type;
	// A list of config row indices.
	typedef std::vector<unsigned int> config_row_indices_seq_type;
	// Lookup a list of config row indices associated with a particular raster.
	typedef std::map<raster_id_type, config_row_indices_seq_type> config_rows_from_raster_layer_lookup_type;
	config_rows_from_raster_layer_lookup_type config_rows_from_raster_layer_lookup;

	// Iterate over the rows in the configuration table and group rows by raster layer.
	for (unsigned int config_row_index = 0; config_row_index < const_cfg_table.size(); ++config_row_index)
	{
		const ConfigurationTableRow &config_row = const_cfg_table[config_row_index];

		// If it's not a raster co-registration then ignore it - it's handled in a separate code path.
		if (config_row.attr_type != CO_REGISTRATION_RASTER_ATTRIBUTE)
		{
			continue;
		}

		// The target raster layer.
		const GPlatesAppLogic::Layer target_layer = config_row.target_layer;

		// The raster band name is the configuration attribute.
		const GPlatesUtils::UnicodeString raster_band_name(config_row.attr_name);

		// Associate the config row with the raster.
		const raster_id_type raster_id = std::make_pair(config_row.target_layer, raster_band_name);
		config_rows_from_raster_layer_lookup[raster_id].push_back(config_row_index);
	}

	// Iterate over the raster layers and co-register all operations for each raster as a group.
	BOOST_FOREACH(
			config_rows_from_raster_layer_lookup_type::value_type &config_rows_from_raster_layer,
			config_rows_from_raster_layer_lookup)
	{
		// The raster id.
		const raster_id_type &raster_id = config_rows_from_raster_layer.first;

		// The config row indices associated with the current raster.
		const config_row_indices_seq_type &raster_config_row_indices = config_rows_from_raster_layer.second;

		// The target raster layer.
		const GPlatesAppLogic::Layer target_layer = raster_id.first;

		// The raster band name.
		const GPlatesUtils::UnicodeString raster_band_name = raster_id.second;

		// Get the target raster layer proxy.
		boost::optional<GPlatesAppLogic::RasterLayerProxy::non_null_ptr_type> target_layer_proxy =
				target_layer.get_layer_output<GPlatesAppLogic::RasterLayerProxy>();
		if (!target_layer_proxy)
		{
			qWarning() << "DataSelector: Unable to get raster layer output - skipping co-registration.";
			continue;
		}

		// Get the (possibly) reconstructed raster.
		boost::optional<GPlatesOpenGL::GLMultiResolutionRasterInterface::non_null_ptr_type> reconstructed_raster =
				target_layer_proxy.get()->get_multi_resolution_data_raster(
						renderer,
						reconstruction_time,
						raster_band_name);
		if (!reconstructed_raster)
		{
			// Shouldn't get here because raster should have already been verified to contain numerical
			// data, the band name should be valid - could be a time-dependent raster with the
			// reconstruction time outside the time sequence.
			qWarning() << "DataSelector: Unable to get raster for specified reconstruction time - skipping co-registration.";
			continue;
		}

		// The operations to co-register for the current raster.
		std::vector<GPlatesOpenGL::GLRasterCoRegistration::Operation> raster_operations;

		// Config row indices that are indexed using the operation index.
		// Maps operation index to config row index (in case one or more operations are not recognised).
		std::vector<unsigned int> operation_config_row_indices;

		// Start with the lowest resolution level-of-detail and select the highest resolution
		// requested for the current raster.
		unsigned int raster_level_of_detail = reconstructed_raster.get()->get_num_levels_of_detail() - 1;

		// Iterate over the config rows associated with the current raster and add to the list
		// of operations to be co-registered for the current raster.
		for (unsigned int config_row_indices_index = 0;
			config_row_indices_index < raster_config_row_indices.size();
			++config_row_indices_index)
		{
			const unsigned int config_row_index = raster_config_row_indices[config_row_indices_index];
			const ConfigurationTableRow &config_row = const_cfg_table[config_row_index];

			// The reducer operation.
			GPlatesOpenGL::GLRasterCoRegistration::OperationType operation_type;
			switch (config_row.reducer_type)
			{
			case REDUCER_MIN:
				operation_type = GPlatesOpenGL::GLRasterCoRegistration::OPERATION_MINIMUM;
				break;
			case REDUCER_MAX:
				operation_type = GPlatesOpenGL::GLRasterCoRegistration::OPERATION_MAXIMUM;
				break;
			case REDUCER_MEAN:
				operation_type = GPlatesOpenGL::GLRasterCoRegistration::OPERATION_MEAN;
				break;
			case REDUCER_STANDARD_DEVIATION:
				operation_type = GPlatesOpenGL::GLRasterCoRegistration::OPERATION_STANDARD_DEVIATION;
				break;
			default:
				// Should not get any other reducer types for rasters - skip this config row.
				qWarning() << "DataSelector: Unexpected reduce operation for raster - skipping co-registration.";
				continue;
			}

			// The region-of-interest range in Kms.
			const double range = dynamic_cast<const RegionOfInterestFilter::Config &>(*config_row.filter_cfg).range();

			// Choose the highest resolution level-of-detail requested for the current raster.
			if (config_row.raster_level_of_detail < raster_level_of_detail)
			{
				raster_level_of_detail = config_row.raster_level_of_detail;
			}

			// Add the raster operation.
			raster_operations.push_back(
					GPlatesOpenGL::GLRasterCoRegistration::Operation(
							range / DEFAULT_RADIUS_OF_EARTH_KMS/* angular radial extent in radians */,
							operation_type,
							config_row.raster_fill_polygons));

			// Add the config row index associated with the current operation.
			operation_config_row_indices.push_back(config_row_index);
		}
		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				raster_operations.size() == operation_config_row_indices.size(),
				GPLATES_ASSERTION_SOURCE);

		// Co-register the reconstructed seed features with the reconstructed raster for all the
		// operations associated with the current raster.
		raster_co_registration.co_register(
				renderer,
				raster_operations,
				reconstructed_seed_features,
				reconstructed_raster.get(),
				raster_level_of_detail);

		// Iterate over the operations associated with the current raster and distribute the
		// co-registration results back to the appropriate config row.
		for (unsigned int operation_index = 0; operation_index < raster_operations.size(); ++operation_index)
		{
			const unsigned int config_row_index = operation_config_row_indices[operation_index];
			const ConfigurationTableRow &config_row = const_cfg_table[config_row_index];

			// Get the co-registration results.
			const GPlatesOpenGL::GLRasterCoRegistration::Operation::result_seq_type &co_reg_results =
					raster_operations[operation_index].get_co_registration_results();

			// Should have a result for each seed feature.
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					co_reg_results.size() == reconstructed_seed_features.size(),
					GPLATES_ASSERTION_SOURCE);
			// Store the results in the result data table.
			for (unsigned int reconstructed_seed_feature_index = 0;
				reconstructed_seed_feature_index < reconstructed_seed_features.size();
				++reconstructed_seed_feature_index)
			{
				// If there's a result for the current seed feature then set it in the result data table,
				// otherwise leave the table entry as it is (empty) to signal "N/A".
				if (co_reg_results[reconstructed_seed_feature_index])
				{
					DataRow &result_data_row = *result_data_table[reconstructed_seed_feature_index];

					result_data_row[config_row.index + result_data_table.data_index()] = 
							co_reg_results[reconstructed_seed_feature_index].get();
				}
			}
		}
	}
}


void
GPlatesDataMining::DataSelector::co_register_target_reconstructed_geometries(
		const std::vector<GPlatesAppLogic::ReconstructContext::ReconstructedFeature> &reconstructed_seed_features,	
		const double &reconstruction_time,
		GPlatesDataMining::DataTable &result_data_table)
{
	//for each seed feature
	for (unsigned int reconstructed_seed_feature_index = 0;
		reconstructed_seed_feature_index < reconstructed_seed_features.size();
		++reconstructed_seed_feature_index)
	{
		const GPlatesAppLogic::ReconstructContext::ReconstructedFeature &reconstructed_seed_feature =
				reconstructed_seed_features[reconstructed_seed_feature_index];

		if(reconstructed_seed_feature.get_reconstructions().size() == 0)
		{
			//No reconstructed-feature-geometry means the seed feature is inactive at this time.
			//if the seed is inactive, no need to calculate any data for it.
			//leave all data in inactive seed row as "Nan". -- fix for Jo
			continue;
		}

		DataRow &result_data_row = *result_data_table[reconstructed_seed_feature_index];

		CoRegFilterCache filter_cache;

		// Need to iterate over 'const' table.
		const CoRegConfigurationTable &const_cfg_table = d_cfg_table;

		//for each row in cfg table
		BOOST_FOREACH(const ConfigurationTableRow &config_row, const_cfg_table)
		{
			// If it's a raster co-registration then ignore it - it's handled in a separate code path.
			if (config_row.attr_type == CO_REGISTRATION_RASTER_ATTRIBUTE)
			{
				continue;
			}

			// Get the target reconstructed geometries layer proxy.
			const GPlatesAppLogic::Layer target_layer = config_row.target_layer;
			boost::optional<GPlatesAppLogic::ReconstructLayerProxy::non_null_ptr_type> target_layer_proxy =
					target_layer.get_layer_output<GPlatesAppLogic::ReconstructLayerProxy>();
			if (!target_layer_proxy)
			{
				qWarning() << "DataSelector: Unable to get reconstructed geometries layer output - skipping co-registration.";
				continue;
			}

			// Get the reconstructed target features.
			std::vector<GPlatesAppLogic::ReconstructContext::ReconstructedFeature> reconstructed_target_features;
			target_layer_proxy.get()->get_reconstructed_features(
					reconstructed_target_features,
					reconstruction_time);

			boost::shared_ptr< CoRegFilter > filter;
			boost::shared_ptr< CoRegMapper > mapper;
			boost::shared_ptr<CoRegReducer> reducer;
			boost::tie(filter,mapper,reducer)=
					create_filter_map_reduce(config_row, reconstructed_seed_feature);

			//filter
			CoRegFilter::reconstructed_feature_vector_type filter_result, cache_hit;
			if(filter_cache.find(config_row, cache_hit))
			{
				filter->process(
						cache_hit.begin(),
						cache_hit.end(),
						filter_result);
			}
			else
			{
				filter->process(
						reconstructed_target_features.begin(),
						reconstructed_target_features.end(),
						filter_result);
			}
			filter_cache.insert(config_row,filter_result);

			//map
			CoRegMapper::MapperOutDataset map_result;
			mapper->process(filter_result.begin(), filter_result.end(), map_result);

			//reduce
			result_data_row[config_row.index + result_data_table.data_index()] = 
					reducer->process(map_result.begin(), map_result.end());
		}
	}
}


bool
GPlatesDataMining::DataSelector::is_config_table_valid(
		const std::vector<GPlatesAppLogic::LayerProxy::non_null_ptr_type> &target_layer_proxies)
{
	const CoRegConfigurationTable& table = d_cfg_table;
	BOOST_FOREACH(const ConfigurationTableRow &config_row, table)
	{
		// The layer handle itself should reference a valid, existing layer.
		if (!config_row.target_layer.is_valid())
		{
			return false;
		}

		// The configuration table should not include deactivated layers so this should not return boost::none.
		boost::optional<GPlatesAppLogic::LayerProxy::non_null_ptr_type> config_row_target_layer_proxy =
				config_row.target_layer.get_layer_output();
		if (!config_row_target_layer_proxy)
		{
			return false;
		}

		// The configuration table row should reference a layer that has been connected to the
		// co-registration layer.
		if (std::find(
			target_layer_proxies.begin(),
			target_layer_proxies.end(),
			config_row_target_layer_proxy.get()) == target_layer_proxies.end())
		{
			return false;
		}
	}

	// Config table is valid.
	return true;
}


void
GPlatesDataMining::DataSelector::fill_seed_info(
		const GPlatesAppLogic::ReconstructContext::ReconstructedFeature &reconstructed_seed_feature,
		DataRowSharedPtr row)
{
	//Write out feature id as the first column so that each data row can be correlated. 
	//This is a temporary solution and will be removed when layer framework is ready to handle this.
	QString feature_id = reconstructed_seed_feature.get_feature()->feature_id().get().qstring();
	row->append_cell(OpaqueData(feature_id));

	//seed valid time.
	row->append_cell(
			DataMiningUtils::get_property_value_by_name(
					reconstructed_seed_feature.get_feature(),
					"validTime"));
}


void
GPlatesDataMining::DataSelector::populate_table_header()  
{
	//populate the table header
	
	d_table_header.push_back("Seed Feature ID");
	d_table_header.push_back("Seed Valid Time");
	d_data_index = 2;

	// Size the table header since we're not going to write to it sequentially.
	d_table_header.resize(d_table_header.size() + d_cfg_table.size());

	// Iterate through the configuration rows and write the table header for each.
	const CoRegConfigurationTable& table = d_cfg_table;
	BOOST_FOREACH(const ConfigurationTableRow& row, table)
	{
		// Display the attribute name and the reducer operation - that helps the user to
		// visually identify which configuration row the current table column is referring to.
		QString column_header = row.assoc_name+ "_" + row.attr_name + "_";
		switch (row.reducer_type)
		{
		case REDUCER_MIN:
			column_header += "(min)";
			break;
		case REDUCER_MAX:
			column_header += "(max)";
			break;
		case REDUCER_MEAN:
			column_header += "(mean)";
			break;
		case REDUCER_STANDARD_DEVIATION:
			column_header += "(std-dev)";
			break;
		case REDUCER_MEDIAN:
			column_header += "(median)";
			break;
		case REDUCER_LOOKUP:
			column_header += "(lookup)";
			break;
		case REDUCER_VOTE:
			column_header += "(vote)";
			break;
		case REDUCER_WEIGHTED_MEAN:
			column_header += "(weighted-mean)";
			break;
		case REDUCER_PERCENTILE:
			column_header += "(percentile)";
			break;
		case REDUCER_MIN_DISTANCE:
			column_header += "(min-distance)";
			break;
		case REDUCER_PRESENCE:
			column_header += "(presence)";
			break;
		case REDUCER_NUM_IN_ROI:
			column_header += "(number-in-region)";
			break;

		default:
			// Do nothing.
			qDebug() << "GPlatesDataMining::DataSelector::populate_table_header: reducer type not recognised.";
			break;
		}

		d_table_header[d_data_index + row.index] = column_header;
	}
	
}

// Suppress warning with boost::variant with Boost 1.34 and g++ 4.2.
// This is here at the end of the file because the problem resides in a template
// being instantiated at the end of the compilation unit.
DISABLE_GCC_WARNING("-Wshadow")

