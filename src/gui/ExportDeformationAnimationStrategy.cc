/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2016 The University of Sydney, Australia
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

#include "ExportDeformationAnimationStrategy.h"

#include "model/FeatureHandle.h"
#include "model/types.h"

#include "app-logic/AppLogicUtils.h"
#include "app-logic/ApplicationState.h"
#include "app-logic/LayerProxyUtils.h"
#include "app-logic/ReconstructionGeometryUtils.h"
#include "app-logic/ReconstructionTree.h"
#include "app-logic/ReconstructLayerProxy.h"
#include "app-logic/TopologyReconstructedFeatureGeometry.h"

#include "file-io/DeformationExport.h"
#include "file-io/ExportTemplateFilenameSequence.h"
#include "file-io/File.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "gui/ExportAnimationContext.h"
#include "gui/AnimationController.h"

#include "presentation/ViewState.h"
#include "presentation/VisualLayer.h"
#include "presentation/VisualLayers.h"


namespace
{
	/**
	 * Typedef for a sequence of @a TopologyReconstructedFeatureGeometry pointers.
	 */
	typedef std::vector<const GPlatesAppLogic::TopologyReconstructedFeatureGeometry *> deformed_feature_geometry_seq_type;


	/**
	 * Get the visible reconstruct visual layers.
	 */
	void
	get_visible_reconstruct_visual_layers(
			std::vector< boost::shared_ptr<const GPlatesPresentation::VisualLayer> > &visible_reconstruct_visual_layers,
			const GPlatesPresentation::ViewState &view_state)
	{
		const GPlatesPresentation::VisualLayers &visual_layers = view_state.get_visual_layers();

		// Iterate over the *visible* visual layers.
		const unsigned int num_layers = visual_layers.size();
		for (unsigned int n = 0; n < num_layers; ++n)
		{
			boost::shared_ptr<const GPlatesPresentation::VisualLayer> visual_layer = visual_layers.visual_layer_at(n).lock();
			if (visual_layer &&
				visual_layer->is_visible() &&
				visual_layer->get_layer_type() == static_cast<unsigned int>(GPlatesAppLogic::LayerTaskType::RECONSTRUCT))
			{
				visible_reconstruct_visual_layers.push_back(visual_layer);
			}
		}
	}


	void
	get_visible_reconstruct_layer_proxies(
			std::vector<GPlatesAppLogic::ReconstructLayerProxy::non_null_ptr_type> &visible_reconstruct_outputs,
			const GPlatesPresentation::ViewState &view_state)
	{
		std::vector< boost::shared_ptr<const GPlatesPresentation::VisualLayer> > visible_reconstruct_visual_layers;
		get_visible_reconstruct_visual_layers(visible_reconstruct_visual_layers, view_state);

		// Iterate over the reconstruct visual layers.
		for (unsigned int n = 0; n < visible_reconstruct_visual_layers.size(); ++n)
		{
			const boost::shared_ptr<const GPlatesPresentation::VisualLayer> visible_reconstruct_visual_layer =
					visible_reconstruct_visual_layers[n];

			const GPlatesAppLogic::Layer &reconstruct_layer = visible_reconstruct_visual_layer->get_reconstruct_graph_layer();
			boost::optional<GPlatesAppLogic::ReconstructLayerProxy::non_null_ptr_type> reconstruct_output =
					reconstruct_layer.get_layer_output<GPlatesAppLogic::ReconstructLayerProxy>();
			if (reconstruct_output)
			{
				visible_reconstruct_outputs.push_back(reconstruct_output.get());
			}
		}
	}


	void
	populate_visible_deformed_feature_geometry_seq(
			deformed_feature_geometry_seq_type &deformed_feature_geometry_seq,
			const GPlatesPresentation::ViewState &view_state)
	{
		// Get the reconstruct layer outputs.
		std::vector<GPlatesAppLogic::ReconstructLayerProxy::non_null_ptr_type> reconstruct_outputs;
		get_visible_reconstruct_layer_proxies(reconstruct_outputs, view_state);

		// Iterate over the layers that have reconstruct outputs.
		std::vector<GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_type> reconstructed_feature_geometries;
		BOOST_FOREACH(
				const GPlatesAppLogic::ReconstructLayerProxy::non_null_ptr_type &reconstruct_output,
				reconstruct_outputs)
		{
			reconstruct_output->get_reconstructed_feature_geometries(reconstructed_feature_geometries);
		}

		// Get any ReconstructionGeometry objects that are of type TopologyReconstructedFeatureGeometry.
		// Also converts sequence of non_null_ptr_type's to a sequence of raw pointers expected by the caller.
		GPlatesAppLogic::ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type_sequence(
				reconstructed_feature_geometries.begin(),
				reconstructed_feature_geometries.end(),
				deformed_feature_geometry_seq);
	}
}


GPlatesGui::ExportDeformationAnimationStrategy::ExportDeformationAnimationStrategy(
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
GPlatesGui::ExportDeformationAnimationStrategy::set_template_filename(
		const QString &filename)
{
	ExportAnimationStrategy::set_template_filename(filename);
}


bool
GPlatesGui::ExportDeformationAnimationStrategy::do_export_iteration(
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
			QObject::tr("Writing deformation at frame %2 to file \"%1\"...")
			.arg(basename)
			.arg(frame_index) );

	// Here's where we do the actual work of exporting deformation.
	try
	{
		switch (d_configuration->file_format)
		{
		case Configuration::GPML:
			{
				// Throws bad_cast if fails.
				const GpmlConfiguration &configuration = dynamic_cast<const GpmlConfiguration &>(*d_configuration);

				// Get all visible TopologyReconstructedFeatureGeometry's from the current reconstruction.
				deformed_feature_geometry_seq_type deformed_feature_geometry_seq;
				populate_visible_deformed_feature_geometry_seq(
						deformed_feature_geometry_seq,
						d_export_animation_context_ptr->view_state());

				// Principal strain options.
				boost::optional<GPlatesFileIO::DeformationExport::PrincipalStrainOptions> include_principal_strain_options;
				if (configuration.include_principal_strain)
				{
					include_principal_strain_options = configuration.principal_strain_options;
				}

				GPlatesFileIO::DeformationExport::export_deformation_to_gpml_format(
					full_filename,
					deformed_feature_geometry_seq,
					d_export_animation_context_ptr->view_state().get_application_state().get_model_interface(),
					d_loaded_files,
					include_principal_strain_options,
					configuration.include_dilatation_strain,
					configuration.include_dilatation_strain_rate,
					configuration.include_second_invariant_strain_rate,
					configuration.include_strain_rate_style,
					configuration.file_options.export_to_a_single_file,
					configuration.file_options.export_to_multiple_files,
					configuration.file_options.separate_output_directory_per_file);
			}
			break;

		case Configuration::GMT:
			{
				// Throws bad_cast if fails.
				const GMTConfiguration &configuration = dynamic_cast<const GMTConfiguration &>(*d_configuration);

				// Get all visible TopologyReconstructedFeatureGeometry's from the current reconstruction.
				deformed_feature_geometry_seq_type deformed_feature_geometry_seq;
				populate_visible_deformed_feature_geometry_seq(
						deformed_feature_geometry_seq,
						d_export_animation_context_ptr->view_state());

				// Principal strain options.
				boost::optional<GPlatesFileIO::DeformationExport::PrincipalStrainOptions> include_principal_strain_options;
				if (configuration.include_principal_strain)
				{
					include_principal_strain_options = configuration.principal_strain_options;
				}

				GPlatesFileIO::DeformationExport::export_deformation_to_gmt_format(
					full_filename,
					deformed_feature_geometry_seq,
					d_loaded_files,
					d_export_animation_context_ptr->view_state().get_application_state().get_current_anchored_plate_id(),
					d_export_animation_context_ptr->view_time(),
					(configuration.domain_point_format == GMTConfiguration::LON_LAT),
					include_principal_strain_options,
					configuration.include_dilatation_strain,
					configuration.include_dilatation_strain_rate,
					configuration.include_second_invariant_strain_rate,
					configuration.include_strain_rate_style,
					configuration.file_options.export_to_a_single_file,
					configuration.file_options.export_to_multiple_files,
					configuration.file_options.separate_output_directory_per_file);
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
			QObject::tr("Error writing deformation file \"%1\": %2")
					.arg(full_filename)
					.arg(exc.what()));
		return false;
	}
	catch (...)
	{
		// FIXME: Catch all proper exceptions we might get here.
		d_export_animation_context_ptr->update_status_message(
			QObject::tr("Error writing deformation file \"%1\": unknown error!").arg(full_filename));
		return false;
	}
	
	// Normal exit, all good, ask the Context process the next iteration please.
	return true;
}


void
GPlatesGui::ExportDeformationAnimationStrategy::wrap_up(
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
