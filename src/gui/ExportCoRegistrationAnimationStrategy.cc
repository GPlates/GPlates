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
#include <QLocale> 

#include "ExportCoRegistrationAnimationStrategy.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/CoRegistrationLayerProxy.h"
#include "app-logic/Reconstruction.h"

#include "data-mining/DataSelector.h"

#include "gui/ExportAnimationContext.h"
#include "gui/AnimationController.h"
#include "gui/CsvExport.h"

#include "opengl/GLContext.h"
#include "opengl/GLRenderer.h"

#include "presentation/ViewState.h"

#include "qt-widgets/GlobeAndMapWidget.h"
#include "qt-widgets/ReconstructionViewWidget.h"
#include "qt-widgets/ViewportWindow.h"


namespace
{
	QString
	substitute_placeholder(
			const QString &output_filebasename,
			const QString &placeholder,
			const QString &placeholder_replacement)
	{
		return QString(output_filebasename).replace(placeholder, placeholder_replacement);
	}

	QString
	calculate_output_basename(
			const QString &output_filename_prefix,
			const QString &layer_name)
	{
		const QString output_basename = substitute_placeholder(
				output_filename_prefix,
				GPlatesFileIO::ExportTemplateFilename::PLACEHOLDER_FORMAT_STRING,
				layer_name);

		return output_basename;
	}
}


GPlatesGui::ExportCoRegistrationAnimationStrategy::ExportCoRegistrationAnimationStrategy(
		GPlatesGui::ExportAnimationContext &export_animation_context,
		const const_configuration_ptr &configuration):
	ExportAnimationStrategy(export_animation_context),
	d_configuration(configuration)
{
	set_template_filename(d_configuration->get_filename_template());
}

bool
GPlatesGui::ExportCoRegistrationAnimationStrategy::do_export_iteration(
		std::size_t frame_index)
{	
	GPlatesFileIO::ExportTemplateFilenameSequence::const_iterator &filename_it = 
		*d_filename_iterator_opt;

	//CsvExport::ExportOptions option;
	//option.delimiter = ',';
	
	QString basename = *filename_it;
	filename_it++;

	GPlatesAppLogic::ApplicationState &application_state =
			d_export_animation_context_ptr->view_state().get_application_state();

	// Get the current reconstruction (of all (enabled) layers).
	const GPlatesAppLogic::Reconstruction &reconstruction = application_state.get_current_reconstruction();

	// Get the co-registration layer outputs (likely only one layer but could be more).
	std::vector<GPlatesAppLogic::CoRegistrationLayerProxy::non_null_ptr_type> co_registration_layer_outputs;
	if (reconstruction.get_active_layer_outputs<GPlatesAppLogic::CoRegistrationLayerProxy>(
			co_registration_layer_outputs))
	{
		// Iterate over the co-registration layers.
		for (unsigned int coreg_layer_index = 0;
			coreg_layer_index < co_registration_layer_outputs.size();
			++coreg_layer_index)
		{
			const GPlatesAppLogic::CoRegistrationLayerProxy::non_null_ptr_type &co_registration_layer_output =
					co_registration_layer_outputs[coreg_layer_index];

			//
			// Co-registration of rasters requires an OpenGL renderer (for co-registration of rasters).
			//

			// Get an OpenGL context for the (raster) co-registration since it accelerates it with OpenGL.
			GPlatesOpenGL::GLContext::non_null_ptr_type gl_context =
					d_export_animation_context_ptr->viewport_window().reconstruction_view_widget()
							.globe_and_map_widget().get_active_gl_context();

			// Make sure the context is currently active.
			gl_context->make_current();

			// Pass in the viewport of the window currently attached to the OpenGL context.
			const GPlatesOpenGL::GLViewport viewport(
					0, 0,
					d_export_animation_context_ptr->view_state().get_main_viewport_dimensions().first/*width*/,
					d_export_animation_context_ptr->view_state().get_main_viewport_dimensions().second/*height*/);

			// Start a begin_render/end_render scope.
			// NOTE: Before calling this, OpenGL should be in the default OpenGL state.
			GPlatesOpenGL::GLRenderer::non_null_ptr_type renderer = gl_context->create_renderer();
			GPlatesOpenGL::GLRenderer::RenderScope render_scope(*renderer, viewport);

			//
			// Get the co-registration results (perform the co-registration).
			//

			// Get the co-registration result data for the current reconstruction time.
			boost::optional<GPlatesAppLogic::CoRegistrationData::non_null_ptr_type> coregistration_data =
					co_registration_layer_output->get_coregistration_data(*renderer);

			// If there's no co-registration data then it means the user has not yet configured co-registration.
			if (!coregistration_data)
			{
				continue;
			}

			// If there's more then one co-registration layer in total then we'll need to have different
			// export filenames - so just substitute "_layer1", etc (incrementing the number)
			// for the template filename placeholder, otherwise substitute the empty string.
			QString placeholder_replacement("");
			if (co_registration_layer_outputs.size() > 1)
			{
				placeholder_replacement = QString("_layer%1").arg(coreg_layer_index + 1);
			}
			QString output_basename =
					calculate_output_basename(
							basename,
							placeholder_replacement);
			QString full_filename = d_export_animation_context_ptr->target_dir().absoluteFilePath(output_basename);

			// Export the co-registration data.
			coregistration_data.get()->data_table().export_as_CSV(full_filename);
		}
	}

	// Normal exit, all good, ask the Context process the next iteration please.
	return true;
}
