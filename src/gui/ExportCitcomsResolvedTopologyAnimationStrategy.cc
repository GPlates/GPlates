/* $Id$ */
 
/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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
#include <iterator>
#include <boost/foreach.hpp>
#include <boost/optional.hpp>
#include <QFileInfo>
#include <QString>

#include "ExportCitcomsResolvedTopologyAnimationStrategy.h"

#include "AnimationController.h"
#include "ExportAnimationContext.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/AppLogicUtils.h"
#include "app-logic/ReconstructionGeometryUtils.h"
#include "app-logic/Reconstruction.h"
#include "app-logic/ResolvedTopologicalBoundary.h"
#include "app-logic/ResolvedTopologicalNetwork.h"

#include "presentation/ViewState.h"

#include "view-operations/RenderedGeometryUtils.h"

#include <boost/foreach.hpp>

GPlatesGui::ExportCitcomsResolvedTopologyAnimationStrategy::ExportCitcomsResolvedTopologyAnimationStrategy(
		GPlatesGui::ExportAnimationContext &export_animation_context,
		const const_configuration_ptr &cfg) :
	ExportAnimationStrategy(export_animation_context),
	d_configuration(cfg)
{
	set_template_filename(d_configuration->get_filename_template());
	
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

	const GPlatesAppLogic::ReconstructGraph &reconstruct_graph =
			d_export_animation_context_ptr->view_state().get_application_state().get_reconstruct_graph();

	// Check all the active reconstruction layers, and get their input files.
	GPlatesAppLogic::ReconstructGraph::const_iterator layers_iter = reconstruct_graph.begin();
	GPlatesAppLogic::ReconstructGraph::const_iterator layers_end = reconstruct_graph.end();
	for ( ; layers_iter != layers_end; ++layers_iter)
	{
		const GPlatesAppLogic::Layer layer = *layers_iter;

		if (layer.get_type() == GPlatesAppLogic::LayerTaskType::RECONSTRUCTION &&
			layer.is_active())
		{

			// The 'reconstruction tree' layer has input feature collections on its main input channel.
			const GPlatesAppLogic::LayerInputChannelName::Type main_input_channel =
					layer.get_main_input_feature_collection_channel();
			const std::vector<GPlatesAppLogic::Layer::InputConnection> main_inputs =
					layer.get_channel_inputs(main_input_channel);

			// Loop over all input connections to get the files (feature collections) for the current target layer.
			BOOST_FOREACH(const GPlatesAppLogic::Layer::InputConnection& main_input_connection, main_inputs)
			{
				boost::optional<GPlatesAppLogic::Layer::InputFile> input_file = main_input_connection.get_input_file();
				// If it's not a file (ie, it's a layer) then continue to the next file.
				// This shouldn't happen for 'reconstruction tree' layers though.
				if (!input_file)
				{
					continue;
				}
				d_loaded_reconstruction_files.push_back(&input_file->get_file().get_file());
			}
		}
	}
}


void
GPlatesGui::ExportCitcomsResolvedTopologyAnimationStrategy::set_template_filename(
		const QString &filename)
{
	// We want "Polygons" to look like "Polygons.%P.%d" as that
	// is what is expected by the workflow (external to GPlates) that uses
	// this export.
	// The '%P' placeholder string will get replaced for each type of export
	// in 'do_export_iteration()'.
	// The "%d" tells ExportTemplateFilenameSequence to insert the reconstruction time.
#if 0
	//The placeholders, %P and %d", have been taken care by "export" dialog.
	//So there is no need to add them here again.
	const QString suffix =
			"." +
			GPlatesFileIO::ExportTemplateFilename::PLACEHOLDER_FORMAT_STRING +
			".%d";

	const QString modified_template_filename =
			append_suffix_to_template_filebasename(filename, suffix);

#endif
	// Call base class method to actually set the template filename.
	//ExportAnimationStrategy::set_template_filename(modified_template_filename);
	ExportAnimationStrategy::set_template_filename(filename);
}


bool
GPlatesGui::ExportCitcomsResolvedTopologyAnimationStrategy::do_export_iteration(
		std::size_t frame_index)
{
	GPlatesFileIO::ExportTemplateFilenameSequence::const_iterator &filename_it = 
		*d_filename_iterator_opt;

	// Assemble parts of this iteration's filename from the template filename sequence.
	QString output_filebasename = *filename_it++;

	// All that's really expected of us at this point is maybe updating
	// the dialog status message, then calculating what we want to calculate,
	// and writing whatever file we feel like writing.
	d_export_animation_context_ptr->update_status_message(
			QObject::tr("Writing resolved topologies at frame %2 to \"%1\"...")
			.arg(output_filebasename)
			.arg(frame_index) );

	//
	// Here's where we would do the actual exporting of the resolved topologies.
	// The View is already set to the appropriate reconstruction time for
	// this frame; all we have to do is the maths and the file-writing (to @a full_filename)
	//

	GPlatesAppLogic::ApplicationState &application_state =
			d_export_animation_context_ptr->view_state().get_application_state();

	const double &reconstruction_time = application_state.get_current_reconstruction_time();

	// Get any ReconstructionGeometry objects that are visible in any active layers
	// of the RenderedGeometryCollection.
	GPlatesViewOperations::RenderedGeometryUtils::reconstruction_geom_seq_type reconstruction_geom_seq;
	GPlatesViewOperations::RenderedGeometryUtils::get_unique_reconstruction_geometries(
			reconstruction_geom_seq,
			d_export_animation_context_ptr->view_state().get_rendered_geometry_collection(),
			// Don't want to export a duplicate resolved topology if one is currently in focus...
			GPlatesViewOperations::RenderedGeometryCollection::RECONSTRUCTION_LAYER);

	// Get any ReconstructionGeometry objects that are of type ResolvedTopologicalBoundary or
	// ResolvedTopologicalNetwork since both these types have topological boundaries.
	resolved_geom_seq_type resolved_topological_geometries;

	// Get the ResolvedTopologicalBoundary objects...
	std::vector<const GPlatesAppLogic::ResolvedTopologicalBoundary *> resolved_topological_boundaries;
	GPlatesAppLogic::ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type_sequence(
			reconstruction_geom_seq.begin(),
			reconstruction_geom_seq.end(),
			resolved_topological_boundaries);
	std::copy(
			resolved_topological_boundaries.begin(),
			resolved_topological_boundaries.end(),
			std::back_inserter(resolved_topological_geometries));

	// Get the ResolvedTopologicalNetwork objects...
	std::vector<const GPlatesAppLogic::ResolvedTopologicalNetwork *> resolved_topological_networks;
	GPlatesAppLogic::ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type_sequence(
			reconstruction_geom_seq.begin(),
			reconstruction_geom_seq.end(),
			resolved_topological_networks);
	std::copy(
			resolved_topological_networks.begin(),
			resolved_topological_networks.end(),
			std::back_inserter(resolved_topological_geometries));


	// Here's where we do the actual work of exporting of the resolved topologies,
	// given frame_index, filename, reconstructable files and geoms, and target_dir. Etc.
	try
	{
		// Export the various files.
		export_files(resolved_topological_geometries, reconstruction_time, output_filebasename);
	}
	catch (std::exception &exc)
	{
		d_export_animation_context_ptr->update_status_message(
			QObject::tr("Error writing resolved topological geometries\"%1\": %2")
					.arg(output_filebasename)
					.arg(exc.what()));
		return false;
	}
	catch (...)
	{
		// FIXME: Catch all proper exceptions we might get here.
		d_export_animation_context_ptr->update_status_message(
			QObject::tr("Error writing resolved topological geometries \"%1\": unknown error!")
					.arg(output_filebasename));
		return false;
	}
	
	// Normal exit, all good, ask the Context to process the next iteration please.
	return true;
}


void
GPlatesGui::ExportCitcomsResolvedTopologyAnimationStrategy::wrap_up(
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


void
GPlatesGui::ExportCitcomsResolvedTopologyAnimationStrategy::export_files(
		const resolved_geom_seq_type &resolved_geom_seq,
		const double &recon_time,
		const QString &filebasename)
{
	const QDir &target_dir = d_export_animation_context_ptr->target_dir();

	GPlatesFileIO::CitcomsResolvedTopologicalBoundaryExport::export_resolved_topological_boundaries(
			target_dir,
			filebasename,
			GPlatesFileIO::ExportTemplateFilename::PLACEHOLDER_FORMAT_STRING,
			d_configuration->output_options,
			GPlatesFileIO::CitcomsResolvedTopologicalBoundaryExport::get_export_file_format(
					filebasename,
					d_export_animation_context_ptr->view_state().get_application_state()
							.get_feature_collection_file_format_registry()),
			resolved_geom_seq,
			d_loaded_files,
			d_loaded_reconstruction_files,
			d_export_animation_context_ptr->view_state().get_application_state().get_current_anchored_plate_id(),
			d_export_animation_context_ptr->view_time());
}
