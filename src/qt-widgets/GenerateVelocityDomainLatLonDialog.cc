/**
 * \file 
 * 
 * Copyright (C) 2010, 2011 The University of Sydney, Australia
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

#include <sstream>
#include <string>
#include <QDebug>
#include <QDir>
#include <QMessageBox>

#include "GenerateVelocityDomainLatLonDialog.h"

#include "ProgressDialog.h"
#include "QtWidgetUtils.h"
#include "ViewportWindow.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/FeatureCollectionFileIO.h"
#include "app-logic/ReconstructGraph.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "gui/FileIOFeedback.h"

#include "maths/LatLonPoint.h"
#include "maths/MultiPointOnSphere.h"

#include "model/FeatureCollectionHandle.h"
#include "model/FeatureHandle.h"
#include "model/FeatureType.h"
#include "model/ModelInterface.h"
#include "model/Model.h"
#include "model/ModelUtils.h"
#include "model/NotificationGuard.h"

#include "presentation/ViewState.h"

#include "property-values/GmlMultiPoint.h"
#include "property-values/GpmlPlateId.h"


namespace
{
	const std::string NUM_LATITUDE_GRID_INTERVALS_PLACE_HOLDER = "%n";

	const char *HELP_DIALOG_TITLE_CONFIGURATION = QT_TR_NOOP("Configuration parameters");

	const char *HELP_DIALOG_TEXT_CONFIGURATION = QT_TR_NOOP(
			"<html><body>"
			"<p/>"
			"<p>The <i>'number of latitudinal grid intervals'</i> parameter specifies the number of "
			"intervals in the latitude direction (along meridians). The number of latitudinal grid nodes "
			"(points) will be the number of grid intervals plus one. "
			"The top and bottom points will lie on the north and south poles. "
			"Note that the density of grid nodes is much higher near the poles than at the equator.</p>"
			"<p>The latitude and longitude grid <i>spacings</i> are the same, resulting in roughly "
			"twice the number of nodes along longitudes - it is actually '2 * (NUM_LATITUDE_NODES - 1)' "
			"to avoid duplicating nodes when wrapping from longitude 360 degrees to 0 degrees.</p>"
			"</body></html>");

	const char *HELP_DIALOG_TITLE_OUTPUT = QT_TR_NOOP("Setting output directory and file name");

	const char *HELP_DIALOG_TEXT_OUTPUT = QT_TR_NOOP(
			"<html><body>"
			"<p/>"
			"<p>A single generated GPML file of the specified filename will be saved to the specifed output directory.</p>"
			"<p>You can <i>optionally</i> use the template parameter '%n' in the file name and it will be "
			"replaced by the <i>'number of grid intervals along latitude'</i> parameter.</p>"
			"</body></html>\n");


	/**
	 * Replace all occurrences of a place holder substring with its replacement substring.
	 */
	void
	replace_place_holder(
			std::string &str,
			const std::string &place_holder,
			const std::string &replacement)
	{
		while (true)
		{
			const std::string::size_type str_index = str.find(place_holder);
			if (str_index == std::string::npos)
			{
				break;
			}
			str.replace(str_index, place_holder.size(), replacement);
		}
	}
}


GPlatesQtWidgets::GenerateVelocityDomainLatLonDialog::GenerateVelocityDomainLatLonDialog(
		ViewportWindow &main_window_,
		QWidget *parent_ ) :
	GPlatesDialog(
			parent_,
			Qt::CustomizeWindowHint |
			Qt::WindowTitleHint |
			Qt::WindowSystemMenuHint |
			Qt::MSWindowsFixedSizeDialogHint),
	d_main_window(main_window_),
	d_num_latitude_grid_intervals(6),
	d_file_name_template("lat_lon_velocity_domain_%n"),
	d_help_dialog_configuration(
			new InformationDialog(
					tr(HELP_DIALOG_TEXT_CONFIGURATION), 
					tr(HELP_DIALOG_TITLE_CONFIGURATION), 
					this)),
	d_help_dialog_output(
			new InformationDialog(
					tr(HELP_DIALOG_TEXT_OUTPUT), 
					tr(HELP_DIALOG_TITLE_OUTPUT), 
					this)),
	d_open_directory_dialog(
			this,
			tr("Select Path"),
			main_window_.get_view_state())
{
	setupUi(this);

	QObject::connect(
			lattitude_grid_intervals_spinbox,
			SIGNAL(valueChanged(int)), 
			this,
			SLOT(handle_num_latitude_grid_intervals_value_changed(int)));

	QObject::connect(
			button_path,
			SIGNAL(clicked()),
			this,
			SLOT(select_path()));
	QObject::connect(
			lineEdit_path,
			SIGNAL(editingFinished()),
			this,
			SLOT(set_path()));
	QObject::connect(
			lineEdit_file_name_template,
			SIGNAL(editingFinished()),
			this,
			SLOT(set_file_name_template()));

	QObject::connect(
			pushButton_info_output,
			SIGNAL(clicked()),
			d_help_dialog_output,
			SLOT(show()));
	QObject::connect(
			pushButton_info_configuration,
			SIGNAL(clicked()),
			d_help_dialog_configuration,
			SLOT(show()));

	QObject::connect(
			main_buttonbox,
			SIGNAL(accepted()),
			this,
			SLOT(generate_velocity_domain()));
	QObject::connect(
			main_buttonbox,
			SIGNAL(rejected()),
			this,
			SLOT(reject()));

	QtWidgetUtils::resize_based_on_size_hint(this);

	//
	// Initialise the GUI from the initial parameter values.
	//

	lattitude_grid_intervals_spinbox->setValue(d_num_latitude_grid_intervals);
	
	lineEdit_path->setText(QDir::toNativeSeparators(QDir::currentPath()));

	lineEdit_file_name_template->setText(d_file_name_template.c_str());
}


void
GPlatesQtWidgets::GenerateVelocityDomainLatLonDialog::handle_num_latitude_grid_intervals_value_changed(
		int num_latitude_grid_intervals)
{
	d_num_latitude_grid_intervals = num_latitude_grid_intervals;

	// Update the number of nodes.
	set_num_nodes();
}


void
GPlatesQtWidgets::GenerateVelocityDomainLatLonDialog::set_num_nodes()
{
	const int num_nodes = (d_num_latitude_grid_intervals + 1) * (2 * d_num_latitude_grid_intervals);

	num_nodes_line_edit->setText(QString("%1").arg(num_nodes));
}


void
GPlatesQtWidgets::GenerateVelocityDomainLatLonDialog::set_path()
{
	QString new_path = lineEdit_path->text();
	
	QFileInfo new_path_info(new_path);

	if (new_path_info.exists() && 
		new_path_info.isDir() && 
		new_path_info.isWritable()) 
	{
		d_path = new_path;
		
		// Make sure the path ends with a directory separator
		if (!d_path.endsWith(QDir::separator()))
		{
			d_path = d_path + QDir::separator();
		}
	}
	else
	{
		// If the new path is invalid, we don't allow the path change.
		lineEdit_path->setText(QDir::toNativeSeparators(d_path));
	}
}


void
GPlatesQtWidgets::GenerateVelocityDomainLatLonDialog::select_path()
{
	d_open_directory_dialog.select_directory(lineEdit_path->text());
	QString pathname = d_open_directory_dialog.get_existing_directory();

	if (!pathname.isEmpty())
	{
		lineEdit_path->setText(QDir::toNativeSeparators(pathname));
		set_path();
	}
}


void
GPlatesQtWidgets::GenerateVelocityDomainLatLonDialog::set_file_name_template()
{
	const QString text = lineEdit_file_name_template->text();

	// A placeholder is not required in the filename since there's only a single file being output
	// and hence no filename variation is required to generate unique filenames.
	if (text.isEmpty())
	{
		QMessageBox::warning(
				this, 
				tr("Invalid file name"), 
				tr("The file name is empty. "),
				QMessageBox::Ok, QMessageBox::Ok);
		lineEdit_file_name_template->setText(d_file_name_template.c_str());
		return;
	}

	d_file_name_template = text.toStdString();
}


void
GPlatesQtWidgets::GenerateVelocityDomainLatLonDialog::generate_velocity_domain()
{
	GPlatesModel::ModelInterface model = d_main_window.get_application_state().get_model_interface();

	// We want to merge model events across this scope so that only one model event
	// is generated instead of many as we incrementally modify the feature below.
	GPlatesModel::NotificationGuard model_notification_guard(model.access_model());

	// Block any signaled calls to 'ApplicationState::reconstruct' until we exit this scope.
	GPlatesAppLogic::ApplicationState::ScopedReconstructGuard scoped_reconstruct_guard(
			d_main_window.get_application_state(), true/*reconstruct_on_scope_exit*/);

	// Loading files will trigger layer additions.
	// As an optimisation (ie, not required), put all layer additions in a single add layers group.
	// It dramatically improves the speed of the Visual Layers dialog when there's many layers.
	//
	// NOTE: Actually this is not necessary since we're only adding a single file.
	// But we'll keep it in case that changes.
	GPlatesAppLogic::ReconstructGraph::AddOrRemoveLayersGroup add_layers_group(
			d_main_window.get_application_state().get_reconstruct_graph());
	add_layers_group.begin_add_or_remove_layers();

	main_buttonbox->setDisabled(true);

	// No need for a progress dialog since we're only outputting a single file.

	// Generate the sub-domain points for the current local processor.
	const GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type velocity_domain =
			generate_lat_lon_domain();

	// Save to a new file.
	if (!save_velocity_domain_file(velocity_domain))
	{
		main_buttonbox->setDisabled(false);
		close();
		return;
	}

	add_layers_group.end_add_or_remove_layers();

	main_buttonbox->setDisabled(false);

	accept();
}


GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type
GPlatesQtWidgets::GenerateVelocityDomainLatLonDialog::generate_lat_lon_domain()
{
	std::vector<GPlatesMaths::PointOnSphere> points;

	const double grid_spacing = GPlatesMaths::PI / d_num_latitude_grid_intervals;

	// Generate grid starting at north pole, ending at south pole and incrementing points along
	// lines of constant latitude (parallels).
	for (unsigned int j = 0; j <= d_num_latitude_grid_intervals; ++j)
	{
		const double latitude = GPlatesMaths::convert_rad_to_deg(GPlatesMaths::HALF_PI - j * grid_spacing);

		for (unsigned int i = 0; i < 2 * d_num_latitude_grid_intervals; ++i)
		{
			const double longitude = GPlatesMaths::convert_rad_to_deg(i * grid_spacing);

			points.push_back(
					GPlatesMaths::make_point_on_sphere(
							GPlatesMaths::LatLonPoint(latitude, longitude)));
		}
	}

	return GPlatesMaths::MultiPointOnSphere::create_on_heap(points.begin(), points.end());
}


bool
GPlatesQtWidgets::GenerateVelocityDomainLatLonDialog::save_velocity_domain_file(
		const GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type &velocity_sub_domain)
{
	// Create a feature collection that is not added to the model.
	const GPlatesModel::FeatureCollectionHandle::non_null_ptr_type feature_collection = 
			GPlatesModel::FeatureCollectionHandle::create();

	// Get a weak reference so we can add features to the feature collection.
	const GPlatesModel::FeatureCollectionHandle::weak_ref feature_collection_ref =
			feature_collection->reference();

	static const GPlatesModel::FeatureType mesh_node_feature_type = 
			GPlatesModel::FeatureType::create_gpml("MeshNode");

	GPlatesModel::FeatureHandle::weak_ref feature = GPlatesModel::FeatureHandle::create(
			feature_collection_ref,
			mesh_node_feature_type);

	// Create the geometry property and append to feature.
	feature->add(
			GPlatesModel::TopLevelPropertyInline::create(
				GPlatesModel::PropertyName::create_gpml("meshPoints"),
				GPlatesPropertyValues::GmlMultiPoint::create(velocity_sub_domain)));

	//
	// Add 'reconstructionPlateId' and 'validTime' to mesh points feature
	// These two properties are needed to show mesh points on globe.
	//

	GPlatesPropertyValues::GpmlPlateId::non_null_ptr_type gpml_plate_id = 
		GPlatesPropertyValues::GpmlPlateId::create(0);
	feature->add(
			GPlatesModel::TopLevelPropertyInline::create(
				GPlatesModel::PropertyName::create_gpml("reconstructionPlateId"),
				GPlatesModel::ModelUtils::create_gpml_constant_value(gpml_plate_id)));


	GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_type gml_valid_time = 
		GPlatesModel::ModelUtils::create_gml_time_period(
		GPlatesPropertyValues::GeoTimeInstant::create_distant_past(),
		GPlatesPropertyValues::GeoTimeInstant::create_distant_future());
	feature->add(
			GPlatesModel::TopLevelPropertyInline::create(
				GPlatesModel::PropertyName::create_gml("validTime"),
				gml_valid_time));

	// Get the integer parameters in string form.
	std::stringstream num_latitude_grid_intervals_stream;
	num_latitude_grid_intervals_stream << d_num_latitude_grid_intervals;

	// Generate the filename from the template by replacing the place holders (if any) with parameter values.
	std::string file_name = d_file_name_template;	
	file_name.append(".gpml");
	replace_place_holder(file_name, NUM_LATITUDE_GRID_INTERVALS_PLACE_HOLDER, num_latitude_grid_intervals_stream.str());
	file_name = d_path.toStdString() + file_name;

	// Make a new FileInfo object for saving to a new file.
	// This also copies any other info stored in the FileInfo.
	GPlatesFileIO::FileInfo new_fileinfo(file_name.c_str());
	GPlatesFileIO::File::non_null_ptr_type new_file =
			GPlatesFileIO::File::create_file(new_fileinfo, feature_collection);

	// Save the feature collection to a file that is registered with
	// FeatureCollectionFileState (maintains list of all loaded files).
	// This will pop up an error dialog if there's an error.
	return d_main_window.file_io_feedback().create_file(new_file);
}
