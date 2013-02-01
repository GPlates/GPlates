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

#include "file-io/ExportTemplateFilenameSequence.h"
#include "file-io/File.h"
#include "file-io/MultiPointVectorFieldExport.h"

#include "global/NotYetImplementedException.h"

#include "gui/ExportAnimationContext.h"
#include "gui/AnimationController.h"

#include "presentation/ViewState.h"


namespace
{
	/**
	 * Typedef for a sequence of @a MultiPointVectorField pointers.
	 */
	typedef std::vector<const GPlatesAppLogic::MultiPointVectorField *> vector_field_seq_type;


	void
	populate_vector_field_seq(
			vector_field_seq_type &vector_field_seq,
			const GPlatesAppLogic::ApplicationState &application_state)
	{
		using namespace GPlatesAppLogic;

		const Reconstruction &reconstruction = application_state.get_current_reconstruction();

		// Get the velocity field calculator layer outputs.
		// Note that an active layer does not necessarily mean a visible layer.
		std::vector<GPlatesAppLogic::VelocityFieldCalculatorLayerProxy::non_null_ptr_type> velocity_field_outputs;
		if (!reconstruction.get_active_layer_outputs<GPlatesAppLogic::VelocityFieldCalculatorLayerProxy>(
				velocity_field_outputs))
		{
			return;
		}

		// Iterate over the layers that have velocity field calculator outputs.
		std::vector<multi_point_vector_field_non_null_ptr_type> multi_point_velocity_fields;
		BOOST_FOREACH(
				const GPlatesAppLogic::VelocityFieldCalculatorLayerProxy::non_null_ptr_type &velocity_field_calculator_layer_proxy,
				velocity_field_outputs)
		{
			velocity_field_calculator_layer_proxy->get_velocity_multi_point_vector_fields(multi_point_velocity_fields);
		}

		// Convert sequence of non_null_ptr_type's to a sequence of raw pointers expected by the caller.
		BOOST_FOREACH(
				const multi_point_vector_field_non_null_ptr_type &multi_point_velocity_field,
				multi_point_velocity_fields)
		{
			vector_field_seq.push_back(multi_point_velocity_field.get());
		}
	}
}


const QString GPlatesGui::ExportVelocityAnimationStrategy::Configuration::TERRA_MT_PLACE_HOLDER = "%MT";
const QString GPlatesGui::ExportVelocityAnimationStrategy::Configuration::TERRA_NT_PLACE_HOLDER = "%NT";
const QString GPlatesGui::ExportVelocityAnimationStrategy::Configuration::TERRA_ND_PLACE_HOLDER = "%ND";
const QString GPlatesGui::ExportVelocityAnimationStrategy::Configuration::TERRA_PROCESSOR_PLACE_HOLDER = "%NP";


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

	// Get all the MultiPointVectorFields from the current reconstruction.
	vector_field_seq_type velocity_vector_field_seq;
	populate_vector_field_seq(
			velocity_vector_field_seq,
			d_export_animation_context_ptr->view_state().get_application_state());

	// Here's where we do the actual work of exporting the velocity vector fields.
	try
	{
		switch (d_configuration->file_format)
		{
		case Configuration::GPML:
			GPlatesFileIO::MultiPointVectorFieldExport::export_velocity_vector_fields_to_gpml_format(
				full_filename,
				velocity_vector_field_seq,
				d_export_animation_context_ptr->view_state().get_application_state().get_gpgim(),
				d_export_animation_context_ptr->view_state().get_application_state().get_model_interface(),
				d_loaded_files,
				d_export_animation_context_ptr->view_state().get_application_state().get_current_anchored_plate_id(),
				d_export_animation_context_ptr->view_time(),
				d_configuration->file_options.export_to_a_single_file,
				d_configuration->file_options.export_to_multiple_files,
				d_configuration->file_options.separate_output_directory_per_file);
			break;

		case Configuration::GMT:
			GPlatesFileIO::MultiPointVectorFieldExport::export_velocity_vector_fields_to_gmt_format(
				full_filename,
				velocity_vector_field_seq,
				d_loaded_files,
				d_export_animation_context_ptr->view_state().get_application_state().get_current_anchored_plate_id(),
				d_export_animation_context_ptr->view_time(),
				d_configuration->velocity_vector_format,
				d_configuration->file_options.export_to_a_single_file,
				d_configuration->file_options.export_to_multiple_files,
				d_configuration->file_options.separate_output_directory_per_file);
			break;

		case Configuration::TERRA_TEXT:
			GPlatesFileIO::MultiPointVectorFieldExport::export_velocity_vector_fields_to_terra_text_format(
				d_configuration->terra_grid_filename_template,
				full_filename,
				Configuration::TERRA_MT_PLACE_HOLDER,
				Configuration::TERRA_NT_PLACE_HOLDER,
				Configuration::TERRA_ND_PLACE_HOLDER,
				Configuration::TERRA_PROCESSOR_PLACE_HOLDER,
				GPlatesFileIO::ExportTemplateFilename::PLACEHOLDER_FORMAT_STRING,
				velocity_vector_field_seq,
				d_loaded_files,
				static_cast<int>(d_export_animation_context_ptr->view_time()));
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
