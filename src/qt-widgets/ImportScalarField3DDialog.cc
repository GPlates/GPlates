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
#include <QString>
#include <QStringList>
#include <QMessageBox>

#include "ImportScalarField3DDialog.h"

#include "RasterFeatureCollectionPage.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/FeatureCollectionFileIO.h"
#include "app-logic/FeatureCollectionFileState.h"
#include "app-logic/Layer.h"
#include "app-logic/ReconstructGraph.h"

#include "file-io/FeatureCollectionFileFormat.h"
#include "file-io/File.h"
#include "file-io/FileInfo.h"
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


GPlatesQtWidgets::ImportScalarField3DDialog::ImportScalarField3DDialog(
		GPlatesAppLogic::ApplicationState &application_state,
		GPlatesPresentation::ViewState &view_state,
		GPlatesGui::UnsavedChangesTracker *unsaved_changes_tracker,
		GPlatesGui::FileIOFeedback *file_io_feedback,
		QWidget *parent_) :
	QWizard(parent_, Qt::Dialog | Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
	d_application_state(application_state),
	d_view_state(view_state),
	d_unsaved_changes_tracker(unsaved_changes_tracker),
	d_file_io_feedback(file_io_feedback),
	d_open_file_dialog(
			parentWidget(),
			tr("Import 3D Scalar Field"),
			OpenFileDialog::filter_list_type(1, FileDialogFilter(QObject::tr("All files"))), // no extensions = matches all
			view_state),
	d_save_after_finish(true),
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
	QString filename = d_open_file_dialog.get_open_file_name();
	if (filename.isEmpty())
	{
		return;
	}

	d_view_state.get_last_open_directory() = QFileInfo(filename).path();

	// Check whether there is a GPML file of the same name in the same directory.
	// If so, ask the user if they actually meant to open that.
	QFileInfo scalar_field_file_info(filename);
	QString base_gpml_filename = scalar_field_file_info.completeBaseName() + GPML_EXT;
	QString dir = scalar_field_file_info.absolutePath();
	if (!dir.endsWith("/"))
	{
		dir.append("/");
	}
	QString gpml_filename = dir + base_gpml_filename;
	if (QFile(gpml_filename).exists())
	{
		static const QString QUESTION =
				"There is a GPML file %1 in the same directory as the scalar field file that you selected. "
				"Do you wish to open this existing GPML file instead of importing the scalar field file?";
		QMessageBox::StandardButton answer = QMessageBox::question(
				parentWidget(),
				"Import 3D Scalar Field",
				QUESTION.arg(base_gpml_filename),
				QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
		if (answer == QMessageBox::Yes)
		{
			d_file_io_feedback->open_files(QStringList(gpml_filename));
			return;
		}
		else if (answer == QMessageBox::Cancel)
		{
			return;
		}
		// else fall through.
	}

	// TODO: Convert the imported 2D rasters into a GPlates-format 3D scalar field file that
	// is referenced by the scalar field GPML file - from here on the GPlates-format file will
	// be used instead of the imported 2D raster files.

	// Jump past the time-dependent raster sequence page.
	setStartId(d_scalar_field_feature_collection_page_id);

	setWindowTitle("Import 3D Scalar Field");

	if (exec() == QDialog::Accepted)
	{
		// We want to merge model events across this scope so that only one model event
		// is generated instead of many as we incrementally modify the feature below.
		GPlatesModel::NotificationGuard model_notification_guard(
				d_application_state.get_model_interface().access_model());

		// By the time that we got up to here, we would've collected all the
		// information we need to create the scalar field feature.
		//
		// TODO: Assert we've got all the information.

		GPlatesModel::PropertyValue::non_null_ptr_type scalar_field_3d_file =
				create_scalar_field_3d_file(scalar_field_file_info);

		static const GPlatesModel::FeatureType SCALAR_FIELD_3D =
				GPlatesModel::FeatureType::create_gpml("ScalarField3D");
		static const GPlatesModel::PropertyName SCALAR_FIELD_FILE =
				GPlatesModel::PropertyName::create_gpml("file");

		GPlatesModel::FeatureHandle::non_null_ptr_type feature = GPlatesModel::FeatureHandle::create(SCALAR_FIELD_3D);
		feature->add(GPlatesModel::TopLevelPropertyInline::create(SCALAR_FIELD_FILE, scalar_field_3d_file));

		// Create a new file and add it to file state.
		QString gpml_file_path = create_gpml_file_path(scalar_field_file_info);
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


GPlatesModel::PropertyValue::non_null_ptr_type
GPlatesQtWidgets::ImportScalarField3DDialog::create_scalar_field_3d_file(
		const QFileInfo &file_info)
{
	using namespace GPlatesPropertyValues;

	const XsString::non_null_ptr_type filename = XsString::create(
			GPlatesUtils::make_icu_string_from_qstring(file_info.absoluteFilePath()));

	GpmlScalarField3DFile::non_null_ptr_type scalar_field_3d_file = GpmlScalarField3DFile::create(filename);

	static const TemplateTypeParameterType VALUE_TYPE =
			TemplateTypeParameterType::create_gpml("ScalarField3DFile");

	return GpmlConstantValue::create(scalar_field_3d_file, VALUE_TYPE);
}


QString
GPlatesQtWidgets::ImportScalarField3DDialog::create_gpml_file_path(
		const QFileInfo &file_info) const
{
	QString base_name = QFileInfo(file_info.fileName()).completeBaseName();

	QString fixed_file_name;

	fixed_file_name = base_name + GPML_EXT;

	QString dir = QFileInfo(file_info.absoluteFilePath()).absolutePath();
	if (!dir.endsWith("/"))
	{
		dir += "/";
	}

	return dir + fixed_file_name;
}
