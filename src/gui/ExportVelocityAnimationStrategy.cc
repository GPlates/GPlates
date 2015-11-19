/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2009, 2010 The University of Sydney, Australia
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
 

#include <iostream>

#include <algorithm>
#include <list>
#include <map>
#include <vector>

#include <QFileInfo>
#include <QString>
#include <QDebug>
#include <QFileInfo>

#include "ExportVelocityAnimationStrategy.h"

#include "model/FeatureHandle.h"
#include "model/types.h"

#include "app-logic/AppLogicUtils.h"
#include "app-logic/ApplicationState.h"
#include "app-logic/MultiPointVectorField.h"
#include "app-logic/LayerProxyUtils.h"
#include "app-logic/ReconstructGraph.h"
#include "app-logic/ReconstructionGeometryUtils.h"
#include "app-logic/ReconstructionTree.h"
#include "app-logic/VelocityFieldCalculatorLayerProxy.h"
#include "app-logic/VelocityParams.h"

#include "file-io/ExportTemplateFilenameSequence.h"
#include "file-io/File.h"
#include "file-io/MultiPointVectorFieldExport.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/NotYetImplementedException.h"

#include "gui/ExportAnimationContext.h"
#include "gui/AnimationController.h"

#include "presentation/ViewState.h"


namespace
{
	/**
	 * Typedef for sequence of velocity field calculator layer proxies.
	 */
	typedef std::vector<GPlatesAppLogic::VelocityFieldCalculatorLayerProxy::non_null_ptr_type>
			velocity_field_calculator_layer_proxy_seq_type;

	/**
	 * Typedef for a sequence of @a MultiPointVectorField pointers.
	 */
	typedef std::vector<const GPlatesAppLogic::MultiPointVectorField *> vector_field_seq_type;


	void
	get_velocity_field_calculator_layer_proxies(
			velocity_field_calculator_layer_proxy_seq_type &velocity_field_outputs,
			const GPlatesAppLogic::ApplicationState &application_state)
	{
		using namespace GPlatesAppLogic;

		const Reconstruction &reconstruction = application_state.get_current_reconstruction();

		// Get the velocity field calculator layer outputsx.
		// Note that an active layer does not necessarily mean a visible layer.
		reconstruction.get_active_layer_outputs<VelocityFieldCalculatorLayerProxy>(velocity_field_outputs);
	}


	void
	get_vector_field_seq(
			vector_field_seq_type &vector_field_seq,
			const std::vector<GPlatesAppLogic::multi_point_vector_field_non_null_ptr_type> &multi_point_velocity_fields)
	{
		using namespace GPlatesAppLogic;

		// Convert sequence of non_null_ptr_type's to a sequence of raw pointers expected by the caller.
		BOOST_FOREACH(
				const multi_point_vector_field_non_null_ptr_type &multi_point_velocity_field,
				multi_point_velocity_fields)
		{
			vector_field_seq.push_back(multi_point_velocity_field.get());
		}
	}


	void
	populate_vector_field_seq(
			vector_field_seq_type &vector_field_seq,
			const GPlatesAppLogic::ApplicationState &application_state,
			const GPlatesGui::ExportOptionsUtils::ExportVelocitySmoothingOptions &velocity_smoothing_options)
	{
		using namespace GPlatesAppLogic;

		// Get the velocity field calculator layer outputs.
		std::vector<VelocityFieldCalculatorLayerProxy::non_null_ptr_type> velocity_field_outputs;
		get_velocity_field_calculator_layer_proxies(velocity_field_outputs, application_state);

		// Iterate over the layers that have velocity field calculator outputs.
		std::vector<multi_point_vector_field_non_null_ptr_type> multi_point_velocity_fields;
		BOOST_FOREACH(
				const VelocityFieldCalculatorLayerProxy::non_null_ptr_type &velocity_field_output,
				velocity_field_outputs)
		{
			VelocityParams velocity_params = velocity_field_output->get_current_velocity_params();

			// Override with any layer velocity params explicitly set in the export options.
			velocity_params.set_is_boundary_smoothing_enabled(
					velocity_smoothing_options.is_boundary_smoothing_enabled);
			velocity_params.set_boundary_smoothing_angular_half_extent_degrees(
					velocity_smoothing_options.boundary_smoothing_angular_half_extent_degrees);
			velocity_params.set_exclude_deforming_regions_from_smoothing(
					velocity_smoothing_options.exclude_deforming_regions);

			velocity_field_output->get_velocity_multi_point_vector_fields(
					multi_point_velocity_fields,
					velocity_params);
		}

		// Convert sequence of non_null_ptr_type's to a sequence of raw pointers expected by the caller.
		get_vector_field_seq(vector_field_seq, multi_point_velocity_fields);
	}
}


const QString GPlatesGui::ExportVelocityAnimationStrategy::TerraTextConfiguration::MT_PLACE_HOLDER = "%MT";
const QString GPlatesGui::ExportVelocityAnimationStrategy::TerraTextConfiguration::NT_PLACE_HOLDER = "%NT";
const QString GPlatesGui::ExportVelocityAnimationStrategy::TerraTextConfiguration::ND_PLACE_HOLDER = "%ND";
const QString GPlatesGui::ExportVelocityAnimationStrategy::TerraTextConfiguration::PROCESSOR_PLACE_HOLDER = "%NP";

const QString GPlatesGui::ExportVelocityAnimationStrategy::CitcomsGlobalConfiguration::DENSITY_PLACE_HOLDER = "%D";
const QString GPlatesGui::ExportVelocityAnimationStrategy::CitcomsGlobalConfiguration::CAP_NUM_PLACE_HOLDER = "%C";


GPlatesGui::ExportVelocityAnimationStrategy::ExportVelocityAnimationStrategy(
		GPlatesGui::ExportAnimationContext &export_animation_context,
		const const_configuration_ptr &configuration):
	ExportAnimationStrategy(export_animation_context),
	d_configuration(configuration)
{
	set_template_filename(d_configuration->get_filename_template());

	// This code is copied from "gui/ExportReconstructedGeometryAnimationStrategy.cc".
	GPlatesAppLogic::FeatureCollectionFileState &file_state =
			d_export_animation_context_ptr->view_state().get_application_state()
					.get_feature_collection_file_state();

	// From the file state, obtain the list of all currently loaded files.
	const std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference> loaded_files =
			file_state.get_loaded_files();

	// Add them to our list of loaded files.
	BOOST_FOREACH(GPlatesAppLogic::FeatureCollectionFileState::file_reference file_ref, loaded_files)
	{
		d_loaded_files.push_back(&file_ref.get_file());
	}
}


void
GPlatesGui::ExportVelocityAnimationStrategy::set_template_filename(
		const QString &filename)
{
	ExportAnimationStrategy::set_template_filename(filename);
}


bool
GPlatesGui::ExportVelocityAnimationStrategy::do_export_iteration(
		std::size_t frame_index)
{
	GPlatesFileIO::ExportTemplateFilenameSequence::const_iterator &filename_it = 
		*d_filename_iterator_opt;

	// Figure out a filename from the template filename sequence.
	QString basename = *filename_it++;
	// Add the target dir to that to figure out the absolute path + name.
	QString full_filename = d_export_animation_context_ptr->target_dir().absoluteFilePath(basename);

	// Write status message.
	d_export_animation_context_ptr->update_status_message(
			QObject::tr("Writing velocity vector fields at frame %2 to file \"%1\"...")
			.arg(basename)
			.arg(frame_index) );

	// Here's where we do the actual work of exporting the velocity vector fields.
	try
	{
		switch (d_configuration->file_format)
		{
		case Configuration::GPML:
			{
				// Throws bad_cast if fails.
				const GpmlConfiguration &configuration =
						dynamic_cast<const GpmlConfiguration &>(*d_configuration);

				// Get all the MultiPointVectorFields from the current reconstruction.
				vector_field_seq_type velocity_vector_field_seq;
				populate_vector_field_seq(
						velocity_vector_field_seq,
						d_export_animation_context_ptr->view_state().get_application_state(),
						configuration.velocity_smoothing_options);

				GPlatesFileIO::MultiPointVectorFieldExport::export_velocity_vector_fields_to_gpml_format(
					full_filename,
					velocity_vector_field_seq,
					d_export_animation_context_ptr->view_state().get_application_state().get_model_interface(),
					d_loaded_files,
					d_export_animation_context_ptr->view_state().get_application_state().get_current_anchored_plate_id(),
					d_export_animation_context_ptr->view_time(),
					configuration.file_options.export_to_a_single_file,
					configuration.file_options.export_to_multiple_files,
					configuration.file_options.separate_output_directory_per_file);
			}
			break;

		case Configuration::GMT:
			{
				// Throws bad_cast if fails.
				const GMTConfiguration &configuration =
						dynamic_cast<const GMTConfiguration &>(*d_configuration);

				// Get all the MultiPointVectorFields from the current reconstruction.
				vector_field_seq_type velocity_vector_field_seq;
				populate_vector_field_seq(
						velocity_vector_field_seq,
						d_export_animation_context_ptr->view_state().get_application_state(),
						configuration.velocity_smoothing_options);

				GPlatesFileIO::MultiPointVectorFieldExport::export_velocity_vector_fields_to_gmt_format(
					full_filename,
					velocity_vector_field_seq,
					d_loaded_files,
					d_export_animation_context_ptr->view_state().get_application_state().get_current_anchored_plate_id(),
					d_export_animation_context_ptr->view_time(),
					configuration.velocity_vector_format,
					configuration.velocity_scale,
					configuration.velocity_stride,
					(configuration.domain_point_format == GMTConfiguration::LON_LAT),
					configuration.include_plate_id,
					configuration.include_domain_point,
					configuration.include_domain_meta_data,
					configuration.file_options.export_to_a_single_file,
					configuration.file_options.export_to_multiple_files,
					configuration.file_options.separate_output_directory_per_file);
			}
			break;

		case Configuration::TERRA_TEXT:
			{
				// Throws bad_cast if fails.
				const TerraTextConfiguration &configuration =
						dynamic_cast<const TerraTextConfiguration &>(*d_configuration);

				// Get all the MultiPointVectorFields from the current reconstruction.
				vector_field_seq_type velocity_vector_field_seq;
				populate_vector_field_seq(
						velocity_vector_field_seq,
						d_export_animation_context_ptr->view_state().get_application_state(),
						configuration.velocity_smoothing_options);

				GPlatesFileIO::MultiPointVectorFieldExport::export_velocity_vector_fields_to_terra_text_format(
					configuration.terra_grid_filename_template,
					full_filename,
					TerraTextConfiguration::MT_PLACE_HOLDER,
					TerraTextConfiguration::NT_PLACE_HOLDER,
					TerraTextConfiguration::ND_PLACE_HOLDER,
					TerraTextConfiguration::PROCESSOR_PLACE_HOLDER,
					GPlatesFileIO::ExportTemplateFilename::PLACEHOLDER_FORMAT_STRING,
					velocity_vector_field_seq,
					d_loaded_files,
					static_cast<int>(d_export_animation_context_ptr->view_time()));
			}
			break;

		case Configuration::CITCOMS_GLOBAL:
			{
				// Throws bad_cast if fails.
				const CitcomsGlobalConfiguration &configuration =
						dynamic_cast<const CitcomsGlobalConfiguration &>(*d_configuration);

				// Get all the MultiPointVectorFields from the current reconstruction.
				vector_field_seq_type velocity_vector_field_seq;
				populate_vector_field_seq(
						velocity_vector_field_seq,
						d_export_animation_context_ptr->view_state().get_application_state(),
						configuration.velocity_smoothing_options);

				// Export the raw CitcomS velocity files.
				// Also export CitcomS-compatible GMT format files, if requested.
				GPlatesFileIO::MultiPointVectorFieldExport::export_velocity_vector_fields_to_citcoms_global_format(
					configuration.citcoms_grid_filename_template,
					full_filename,
					CitcomsGlobalConfiguration::DENSITY_PLACE_HOLDER,
					CitcomsGlobalConfiguration::CAP_NUM_PLACE_HOLDER,
					GPlatesFileIO::ExportTemplateFilename::PLACEHOLDER_FORMAT_STRING,
					velocity_vector_field_seq,
					d_loaded_files,
					static_cast<int>(d_export_animation_context_ptr->view_time()),
					configuration.include_gmt_export,
					configuration.gmt_velocity_scale,
					configuration.gmt_velocity_stride);
			}
			break;

		default:
			// Shouldn't get here.
			GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
			break;
		}
	}
	catch (std::exception &exc)
	{
		d_export_animation_context_ptr->update_status_message(
			QObject::tr("Error writing velocity vector field file \"%1\": %2")
					.arg(full_filename)
					.arg(exc.what()));
		return false;
	}
	catch (...)
	{
		// FIXME: Catch all proper exceptions we might get here.
		d_export_animation_context_ptr->update_status_message(
			QObject::tr("Error writing reconstructed geometry file \"%1\": unknown error!").arg(full_filename));
		return false;
	}
	
	// Normal exit, all good, ask the Context process the next iteration please.
	return true;
}


void
GPlatesGui::ExportVelocityAnimationStrategy::wrap_up(
		bool export_successful)
{
	// If we need to do anything after writing a whole batch of velocity files,
	// here's the place to do it.
	// Of course, there's also the destructor, which should free up any resources
	// we acquired in the constructor; this method is intended for any "last step"
	// iteration operations that might need to occur. Perhaps all iterations end
	// up in the same file and we should close that file (if all steps completed
	// successfully).
}
