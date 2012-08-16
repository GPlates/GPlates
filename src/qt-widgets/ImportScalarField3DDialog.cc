/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
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
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <QDebug>
#include <QMessageBox>
#include <QString>
#include <QStringList>
#include <QTextStream>

#include "ImportScalarField3DDialog.h"

#include "GeoreferencingPage.h"
#include "GlobeAndMapWidget.h"
#include "RasterFeatureCollectionPage.h"
#include "ReconstructionViewWidget.h"
#include "ScalarField3DDepthLayersPage.h"
#include "ViewportWindow.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/FeatureCollectionFileIO.h"
#include "app-logic/FeatureCollectionFileState.h"
#include "app-logic/Layer.h"
#include "app-logic/ReconstructGraph.h"

#include "file-io/FeatureCollectionFileFormat.h"
#include "file-io/File.h"
#include "file-io/FileInfo.h"
#include "file-io/RasterReader.h"
#include "file-io/ReadErrorAccumulation.h"

#include "global/GPlatesAssert.h"
#include "global/AssertionFailureException.h"
#include "global/PreconditionViolationError.h"

#include "gui/FileIOFeedback.h"
#include "gui/UnsavedChangesTracker.h"

#include "maths/MathsUtils.h"

#include "model/FeatureHandle.h"
#include "model/ModelUtils.h"
#include "model/NotificationGuard.h"
#include "model/TopLevelPropertyInline.h"

#include "opengl/GLContext.h"
#include "opengl/GLRenderer.h"
#include "opengl/GLScalarField3D.h"
#include "opengl/GLScalarField3DGenerator.h"

#include "property-values/GmlFile.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlPiecewiseAggregation.h"
#include "property-values/GpmlScalarField3DFile.h"
#include "property-values/GpmlTimeWindow.h"
#include "property-values/XsString.h"

#include "utils/Parse.h"
#include "utils/UnicodeStringUtils.h"


const QString
GPlatesQtWidgets::ImportScalarField3DDialog::GPML_EXT = ".gpml";

const QString
GPlatesQtWidgets::ImportScalarField3DDialog::GPSF_EXT = ".gpsf";

const double GPlatesQtWidgets::ScalarField3DDepthLayersSequence::DEFAULT_RADIUS_OF_EARTH = 6378.1;


bool
GPlatesQtWidgets::ScalarField3DDepthLayersSequence::empty() const
{
	return d_sequence.empty();
}


void
GPlatesQtWidgets::ScalarField3DDepthLayersSequence::push_back(
		boost::optional<double> depth,
		const QString &absolute_file_path,
		const QString &file_name,
		unsigned int width,
		unsigned int height)
{
	d_sequence.push_back(
			element_type(
				depth,
				absolute_file_path,
				file_name,
				width,
				height));
}


void
GPlatesQtWidgets::ScalarField3DDepthLayersSequence::add_all(
		const ScalarField3DDepthLayersSequence &other)
{
	d_sequence.reserve(d_sequence.size() + other.d_sequence.size());
	std::copy(
			other.d_sequence.begin(),
			other.d_sequence.end(),
			std::back_inserter(d_sequence));
}


void
GPlatesQtWidgets::ScalarField3DDepthLayersSequence::clear()
{
	d_sequence.clear();
}


void
GPlatesQtWidgets::ScalarField3DDepthLayersSequence::erase(
		unsigned int begin_index,
		unsigned int end_index)
{
	d_sequence.erase(
			d_sequence.begin() + begin_index,
			d_sequence.begin() + end_index);
}


void
GPlatesQtWidgets::ScalarField3DDepthLayersSequence::set_depth(
		unsigned int index,
		const boost::optional<double> &depth)
{
	d_sequence[index].depth = depth;
}


void
GPlatesQtWidgets::ScalarField3DDepthLayersSequence::sort_by_depth()
{
	std::sort(d_sequence.begin(), d_sequence.end(),
			boost::bind(&element_type::depth, _1) <
			boost::bind(&element_type::depth, _2));
}


void
GPlatesQtWidgets::ScalarField3DDepthLayersSequence::sort_by_file_name()
{
	std::sort(d_sequence.begin(), d_sequence.end(),
			boost::bind(&element_type::file_name, _1) <
			boost::bind(&element_type::file_name, _2));
}


GPlatesQtWidgets::ImportScalarField3DDialog::ImportScalarField3DDialog(
		GPlatesAppLogic::ApplicationState &application_state,
		GPlatesPresentation::ViewState &view_state,
		ViewportWindow &viewport_window,
		GPlatesGui::UnsavedChangesTracker *unsaved_changes_tracker,
		GPlatesGui::FileIOFeedback *file_io_feedback,
		QWidget *parent_) :
	QWizard(parent_, Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
	d_application_state(application_state),
	d_view_state(view_state),
	d_viewport_window(viewport_window),
	d_unsaved_changes_tracker(unsaved_changes_tracker),
	d_file_io_feedback(file_io_feedback),
	d_open_file_dialog(
			parentWidget(),
			tr("Import 3D Scalar Field"),
			// We only want formats handled by GDAL...
			// FIXME: We really just want non-RGBA formats (which doesn't necessarily mean GDAL).
			GPlatesFileIO::RasterReader::get_file_dialog_filters(GPlatesFileIO::RasterReader::GDAL),
			view_state),
	d_georeferencing(
			GPlatesPropertyValues::Georeferencing::create()),
	d_save_after_finish(true),
	d_depth_layers_page_id(addPage(
				new ScalarField3DDepthLayersPage(
					view_state,
					d_raster_width,
					d_raster_height,
					d_depth_layers_sequence,
					this))),
	d_georeferencing_page_id(addPage(
				new GeoreferencingPage(
					d_georeferencing,
					d_raster_width,
					d_raster_height,
					this))),
	d_scalar_field_feature_collection_page_id(addPage(
				new RasterFeatureCollectionPage(
					d_save_after_finish,
					this)))
{
	setOptions(options() | QWizard::NoDefaultButton /* by default, the dialog eats Enter keys */);

	// Note: I would've preferred to use resize() instead, but at least on
	// Windows Vista with Qt 4.4, the dialog doesn't respect the call to resize().
	QSize desired_size(724, 600);
	setMinimumSize(desired_size);
}


void
GPlatesQtWidgets::ImportScalarField3DDialog::display(
		GPlatesFileIO::ReadErrorAccumulation *read_errors)
{
	// If the runtime system cannot generate a scalar field from depth layers...
	if (!is_scalar_field_import_supported())
	{
		QString message;
		QTextStream(&message)
				<< tr("Error: Cannot import or render scalar fields on this system - necessary OpenGL functionality missing.\n");
		QMessageBox::critical(&d_viewport_window, tr("Error Importing Scalar Field"), message,
				QMessageBox::Ok, QMessageBox::Ok);
		qWarning() << message; // Also log the detailed error message.
		return;
	}

	// Start at the depth layers sequence page.
	setStartId(d_depth_layers_page_id);

	setWindowTitle("Import 3D Scalar Field");

	if (exec() == QDialog::Accepted)
	{
		// We want to merge model events across this scope so that only one model event
		// is generated instead of many as we incrementally modify the feature below.
		GPlatesModel::NotificationGuard model_notification_guard(
				d_application_state.get_model_interface().access_model());

		// Ensure sorted by depth before we iterate over the sequence.
		d_depth_layers_sequence.sort_by_depth();

		// The name of the 3D scalar field file we will generate from the depth layers.
		const QString gpsf_file_path = create_gpsf_file_path();

		// Create the 3D scalar field file from the depth layers.
		if (!generate_scalar_field(gpsf_file_path, read_errors))
		{
			return;
		}

		GPlatesModel::PropertyValue::non_null_ptr_type scalar_field_3d_file =
				create_scalar_field_3d_file_property_value(gpsf_file_path);

		static const GPlatesModel::FeatureType SCALAR_FIELD_3D =
				GPlatesModel::FeatureType::create_gpml("ScalarField3D");
		static const GPlatesModel::PropertyName SCALAR_FIELD_FILE =
				GPlatesModel::PropertyName::create_gpml("file");

		GPlatesModel::FeatureHandle::non_null_ptr_type feature = GPlatesModel::FeatureHandle::create(SCALAR_FIELD_3D);
		feature->add(GPlatesModel::TopLevelPropertyInline::create(SCALAR_FIELD_FILE, scalar_field_3d_file));

		// Create a new file and add it to file state.
		QString gpml_file_path = create_gpml_file_path();
		GPlatesFileIO::FileInfo gpml_file_info(gpml_file_path);
		GPlatesFileIO::File::non_null_ptr_type file = GPlatesFileIO::File::create_file(gpml_file_info);
		GPlatesAppLogic::FeatureCollectionFileState::file_reference app_logic_file_ref =
			d_application_state.get_feature_collection_file_state().add_file(file);
		GPlatesFileIO::File::Reference &file_io_file_ref = app_logic_file_ref.get_file();

		// Add feature to feature collection in file.
		GPlatesModel::FeatureCollectionHandle::weak_ref feature_collection =
				file_io_file_ref.get_feature_collection();
		feature_collection->add(feature);
		
		// Release the model notification guard now that we've finished modifying the feature.
		// Provided there are no nested guards this should notify model observers.
		// We want any observers to see the changes before continuing so that everyone's in sync.
		model_notification_guard.release_guard();

		// Then save the file.
		if (d_save_after_finish)
		{
			try
			{
				d_file_io_feedback->save_file(app_logic_file_ref);
			}
			catch (std::exception &exc)
			{
				QString message = tr("An error occurred while saving the file '%1': '%2' -"
						" Please use the Manage Feature Collections dialog "
						"on the File menu to save the new feature collection manually.")
						.arg(gpml_file_info.get_display_name(false/*use_absolute_path_name*/)
						.arg(exc.what()));
				QMessageBox::critical(parentWidget(), "Save 3D Scalar Field", message);
			}
			catch (...)
			{
				QMessageBox::critical(parentWidget(), "Save 3D Scalar Field",
						"The GPML file could not be saved. Please use the Manage Feature Collections dialog "
						"on the File menu to save the new feature collection manually.");
			}
		}
	}
}


GPlatesOpenGL::GLRenderer::non_null_ptr_type
GPlatesQtWidgets::ImportScalarField3DDialog::create_gl_renderer(
		GPlatesOpenGL::GLViewport &viewport) const
{
	// Get an OpenGL context.
	GPlatesOpenGL::GLContext::non_null_ptr_type gl_context =
			d_viewport_window.reconstruction_view_widget().globe_and_map_widget().get_active_gl_context();

	// Make sure the context is currently active.
	gl_context->make_current();

	// Pass in the viewport of the window currently attached to the OpenGL context.
	viewport.set_viewport(
			0, 0,
			d_view_state.get_main_viewport_dimensions().first/*width*/,
			d_view_state.get_main_viewport_dimensions().second/*height*/);

	// Start a begin_render/end_render scope.
	// NOTE: Before calling this, OpenGL should be in the default OpenGL state.
	return gl_context->create_renderer();
}


bool
GPlatesQtWidgets::ImportScalarField3DDialog::is_scalar_field_import_supported() const
{
	//
	// First get an OpenGL context from the main viewport window and create a renderer from it.
	//

	// We need an OpenGL renderer before we can query support.
	GPlatesOpenGL::GLViewport viewport;
	GPlatesOpenGL::GLRenderer::non_null_ptr_type renderer = create_gl_renderer(viewport);

	// Start a begin_render/end_render scope.
	GPlatesOpenGL::GLRenderer::RenderScope render_scope(*renderer, viewport);

	//
	// Now see if we can generate a 3D scalar field from depth layers.
	// Also test that we can actually render a scalar field (this is actually stricter).
	//

	return GPlatesOpenGL::GLScalarField3DGenerator::is_supported(*renderer) &&
		GPlatesOpenGL::GLScalarField3D::is_supported(*renderer);
}


bool
GPlatesQtWidgets::ImportScalarField3DDialog::generate_scalar_field(
		const QString &gpsf_file_path,
		GPlatesFileIO::ReadErrorAccumulation *read_errors)
{
	//
	// First get an OpenGL context from the main viewport window and create a renderer from it.
	//

	// We need an OpenGL renderer before we can query support.
	GPlatesOpenGL::GLViewport viewport;
	GPlatesOpenGL::GLRenderer::non_null_ptr_type renderer = create_gl_renderer(viewport);

	// Start a begin_render/end_render scope.
	GPlatesOpenGL::GLRenderer::RenderScope render_scope(*renderer, viewport);

	//
	// Now generate the 3D scalar field file from the depth layers.
	//

	// Collect information on the depth layers.
	GPlatesOpenGL::GLScalarField3DGenerator::depth_layer_seq_type depth_layers;
	BOOST_FOREACH(
			const ScalarField3DDepthLayersSequence::element_type &depth_layer,
			d_depth_layers_sequence.get_sequence())
	{
		// Convert layer depth in Kms to normalised [0,1] sphere radius.
		const double depth_radius =
				(ScalarField3DDepthLayersSequence::DEFAULT_RADIUS_OF_EARTH - depth_layer.depth.get()) /
						ScalarField3DDepthLayersSequence::DEFAULT_RADIUS_OF_EARTH;

		depth_layers.push_back(
				GPlatesOpenGL::GLScalarField3DGenerator::DepthLayer(
						depth_layer.absolute_file_path,
						depth_radius));
	}

	GPlatesOpenGL::GLScalarField3DGenerator::non_null_ptr_type scalar_field_generator =
			GPlatesOpenGL::GLScalarField3DGenerator::create(
					*renderer,
					gpsf_file_path,
					d_georeferencing,
					// All depth layers have been verified to have the same width and height...
					d_raster_width,
					d_raster_height,
					depth_layers,
					read_errors);

	// Generate the scalar field file.
	if (!scalar_field_generator->generate_scalar_field(*renderer, read_errors))
	{
		return false;
	}

	return true;
}


GPlatesModel::PropertyValue::non_null_ptr_type
GPlatesQtWidgets::ImportScalarField3DDialog::create_scalar_field_3d_file_property_value(
		const QString &gpsf_file_path)
{
	using namespace GPlatesPropertyValues;

	const XsString::non_null_ptr_type filename = XsString::create(
			GPlatesUtils::make_icu_string_from_qstring(gpsf_file_path));

	GpmlScalarField3DFile::non_null_ptr_type scalar_field_3d_file = GpmlScalarField3DFile::create(filename);

	static const TemplateTypeParameterType VALUE_TYPE =
			TemplateTypeParameterType::create_gpml("ScalarField3DFile");

	return GpmlConstantValue::create(scalar_field_3d_file, VALUE_TYPE);
}


QString
GPlatesQtWidgets::ImportScalarField3DDialog::create_gpml_file_path() const
{
	return create_file_basename_with_path() + GPML_EXT;
}


QString
GPlatesQtWidgets::ImportScalarField3DDialog::create_gpsf_file_path() const
{
	return create_file_basename_with_path() + GPSF_EXT;
}


QString
GPlatesQtWidgets::ImportScalarField3DDialog::create_file_basename_with_path() const
{
	// Get the first file in the depth layer sequence.
	const ScalarField3DDepthLayersSequence::sequence_type &sequence = d_depth_layers_sequence.get_sequence();
	const ScalarField3DDepthLayersSequence::element_type &first_file = sequence[0];

	QString base_name = QFileInfo(first_file.file_name).completeBaseName();

	QString fixed_file_basename;

	// Strip off the depth from the file name if it is there.
	QStringList tokens = base_name.split("-");

	if (tokens.count() >= 2)
	{
		try
		{
			GPlatesUtils::Parse<double> parse;
			if (GPlatesMaths::are_almost_exactly_equal(
				parse(tokens.last()),
				first_file.depth.get()))
			{
				tokens.removeLast();
			}
			fixed_file_basename = tokens.join("-");
		}
		catch (const GPlatesUtils::ParseError &)
		{
			fixed_file_basename = base_name;
		}
	}
	else
	{
		fixed_file_basename = base_name;
	}

	QString dir = QFileInfo(first_file.absolute_file_path).absolutePath();
	if (!dir.endsWith("/"))
	{
		dir += "/";
	}

	return dir + fixed_file_basename;
}
