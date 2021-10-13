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
#include <boost/foreach.hpp>
#include <boost/range/concepts.hpp>
#include <boost/range/begin.hpp>
#include <boost/weak_ptr.hpp>

#include "AppLogicUtils.h"
#include "CoRegistrationLayerTask.h"
#include "CoRegistrationData.h"

#include "data-mining/CoRegConfigurationTable.h"
#include "data-mining/DataTable.h"
#include "data-mining/DataSelector.h"

#include "qt-widgets/CoRegLayerConfigurationDialog.h"

bool
GPlatesAppLogic::CoRegistrationLayerTask::can_process_feature_collection(
		const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection)
{
	return false;
}


std::vector<GPlatesAppLogic::Layer::input_channel_definition_type>
GPlatesAppLogic::CoRegistrationLayerTask::get_input_channel_definitions() const
{
	std::vector<Layer::input_channel_definition_type> input_channel_definitions;

	// Channel definition for the reconstruction tree.
	input_channel_definitions.push_back(
			boost::make_tuple(
					get_reconstruction_tree_channel_name(),
					Layer::INPUT_RECONSTRUCTION_TREE_DATA,
					Layer::ONE_DATA_IN_CHANNEL));

	input_channel_definitions.push_back(
			boost::make_tuple(
					"CoRegistration seed Channel",
					Layer::INPUT_RECONSTRUCTED_GEOMETRY_COLLECTION_DATA,
					Layer::MULTIPLE_DATAS_IN_CHANNEL));

	input_channel_definitions.push_back(
			boost::make_tuple(
					"CoRegistration input Channel",
					Layer::INPUT_RECONSTRUCTED_GEOMETRY_COLLECTION_DATA,
					Layer::MULTIPLE_DATAS_IN_CHANNEL));

	return input_channel_definitions;
}


QString
GPlatesAppLogic::CoRegistrationLayerTask::get_main_input_feature_collection_channel() const
{
	return "CoRegistration Channel";
}


GPlatesAppLogic::Layer::LayerOutputDataType
GPlatesAppLogic::CoRegistrationLayerTask::get_output_definition() const
{
	return Layer::OUTPUT_RECONSTRUCTED_GEOMETRY_COLLECTION_DATA;
}


boost::optional<GPlatesAppLogic::layer_task_data_type>
GPlatesAppLogic::CoRegistrationLayerTask::process(
		const Layer &layer_handle /* the layer invoking this */,
		const input_data_type &input_data,
		const double &reconstruction_time,
		GPlatesModel::integer_plate_id_type anchored_plate_id,
		const ReconstructionTree::non_null_ptr_to_const_type &default_reconstruction_tree)
{

	// Get the reconstruction tree input.
	boost::optional<ReconstructionTree::non_null_ptr_to_const_type> reconstruction_tree =
			extract_reconstruction_tree(
					input_data,
					default_reconstruction_tree);
	if (!reconstruction_tree)
	{
		// Expecting a single reconstruction tree.
		return boost::none;
	}

	// Get seeds.
	std::vector<ReconstructionGeometryCollection::non_null_ptr_to_const_type> seeds_collection;
	extract_input_channel_data(
			seeds_collection,
			"CoRegistration seed Channel",
			input_data);
	
	if (seeds_collection.empty()) 
	{
		return boost::none;
	}

	// Get co-registration features collection.
	std::vector<ReconstructionGeometryCollection::non_null_ptr_to_const_type> co_reg_collection;
	extract_input_channel_data(
			co_reg_collection,
			"CoRegistration input Channel",
			input_data);

	if (co_reg_collection.empty()) 
	{
		return boost::none;
	}

	CoRegistrationData::non_null_ptr_type data_ptr = 
		CoRegistrationData::non_null_ptr_type(
				new CoRegistrationData(*reconstruction_tree),
				GPlatesUtils::NullIntrusivePointerHandler());

	boost::scoped_ptr< GPlatesDataMining::DataSelector > selector( 
			GPlatesDataMining::DataSelector::create(
					GPlatesQtWidgets::CoRegLayerConfigurationDialog::CoRegCfgTable) );
	
	selector->select(
			seeds_collection, 
			co_reg_collection, 
			data_ptr->data_table());
	
	//TODO:
	//Temporary code to export co-registration data.
	try
	{
		data_ptr->data_table().export_as_CSV(
				get_export_file_name(
						GPlatesQtWidgets::CoRegLayerConfigurationDialog::CoRegCfgTable.export_path(),
						"co_registration_data",  //temporary hard code.
						reconstruction_time));
	}catch(...)
	{
		qDebug()<< "oops, exception happened.....";
	}

#ifdef _DEBUG
	std::cout << data_ptr->data_table() << std::endl;
#endif

	ReconstructionGeometryCollection::non_null_ptr_type CoRegistrationDataCollection =
			ReconstructionGeometryCollection::create(*reconstruction_tree);
	CoRegistrationDataCollection->add_reconstruction_geometry(data_ptr);
	return layer_task_data_type(
			ReconstructionGeometryCollection::non_null_ptr_to_const_type(
					CoRegistrationDataCollection));
}

const QString
GPlatesAppLogic::CoRegistrationLayerTask::get_export_file_name(
		const QString path,
		const QString base_file_name,
		const double time)
{
	QString time_str;
	time_str.setNum(time);
	return path + "/" + base_file_name + "_" + time_str + ".csv";
}


