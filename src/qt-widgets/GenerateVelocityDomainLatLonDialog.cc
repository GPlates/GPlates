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

#include <cmath>
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
#include "maths/MathsUtils.h"
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
	const std::string NUM_LONGITUDE_GRID_INTERVALS_PLACE_HOLDER = "%m";

	const char *HELP_DIALOG_TITLE_CONFIGURATION = QT_TR_NOOP("Configuration parameters");

	const char *HELP_DIALOG_TEXT_CONFIGURATION = QT_TR_NOOP(
			"<html><body>"
			"<p/>"
			"<p>The latitudinal and longitudinal extents can be used to limit the generated node points "
			"to a specific geographic region (the default is global).</p>"
			"<p>The <i>'Place node points at centre of latitude/longitude cells'</i> check box determines "
			"whether generated nodes (points) are placed at the centres of latitude/longitude cells "
			"or at cell corners.</p>"
			"<p>The <i>'number of latitudinal grid intervals'</i> parameter specifies the number of "
			"intervals in the latitude direction (along meridians). A similar parameter specifies "
			"longitudinal intervals. The number of latitudinal grid nodes (points) will be the number "
			"of latitudinal grid intervals when the nodes are at the centres of the latitude/longitude "
			"cells (and plus one when nodes are at cell corners). The <i>'number of longitudinal grid "
			"grid intervals'</i> has the same relation to the number of longitudinal grid nodes (points) "
			"as the latitude case above, except in the case where the longitude interval is the full "
			"360 degrees in which case the end line of nodes is not generated to avoid duplicating nodes "
			"with the start line.</p>"
			"<p>Note that the density of grid nodes on the globe is much higher near the poles than "
			"at the equator due to sampling in latitude/longitude space.</p>"
			"</body></html>");

	const char *HELP_DIALOG_TITLE_OUTPUT = QT_TR_NOOP("Setting output directory and file name");

	const char *HELP_DIALOG_TEXT_OUTPUT = QT_TR_NOOP(
			"<html><body>"
			"<p/>"
			"<p>A single generated GPML file of the specified filename will be saved to the specifed output directory.</p>"
			"<p>You can <i>optionally</i> use the template parameters '%n' and '%m' in the file name and "
			"they will be replaced by the <i>'number of latitudinal grid intervals'</i> and "
			"<i>'number of longitudinal grid intervals'</i> parameters.</p>"
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
	d_num_latitude_grid_intervals(9),
	d_num_longitude_grid_intervals(18),
	d_extents_top(90),
	d_extents_bottom(-90),
	d_extents_left(-180),
	d_extents_right(180),
	d_cell_centred_nodes(false),
	d_file_name_template("lat_lon_velocity_domain_%n_%m"),
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
			top_extents_spinbox,
			SIGNAL(valueChanged(double)),
			this,
			SLOT(react_top_extents_spin_box_value_changed(double)));
	QObject::connect(
			bottom_extents_spinbox,
			SIGNAL(valueChanged(double)),
			this,
			SLOT(react_bottom_extents_spin_box_value_changed(double)));
	QObject::connect(
			left_extents_spinbox,
			SIGNAL(valueChanged(double)),
			this,
			SLOT(react_left_extents_spin_box_value_changed(double)));
	QObject::connect(
			right_extents_spinbox,
			SIGNAL(valueChanged(double)),
			this,
			SLOT(react_right_extents_spin_box_value_changed(double)));
	QObject::connect(
			use_global_extents_button,
			SIGNAL(clicked()),
			this,
			SLOT(handle_use_global_extents_button_clicked()));
	QObject::connect(
			lattitude_grid_intervals_spinbox,
			SIGNAL(valueChanged(int)), 
			this,
			SLOT(handle_num_latitude_grid_intervals_value_changed(int)));
	QObject::connect(
			longitude_grid_intervals_spinbox,
			SIGNAL(valueChanged(int)), 
			this,
			SLOT(handle_num_longitude_grid_intervals_value_changed(int)));
	QObject::connect(
			cell_centred_checkbox, SIGNAL(stateChanged(int)),
			this, SLOT(react_cell_centred_check_box_changed()));

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

	// Set the min/max longitude values.
	left_extents_spinbox->setMinimum(d_extents_right - 360);
	left_extents_spinbox->setMaximum(d_extents_right + 360);
	right_extents_spinbox->setMinimum(d_extents_left - 360);
	left_extents_spinbox->setMaximum(d_extents_left + 360);

	top_extents_spinbox->setValue(d_extents_top);
	bottom_extents_spinbox->setValue(d_extents_bottom);
	left_extents_spinbox->setValue(d_extents_left);
	right_extents_spinbox->setValue(d_extents_right);

	lattitude_grid_intervals_spinbox->setValue(d_num_latitude_grid_intervals);
	longitude_grid_intervals_spinbox->setValue(d_num_longitude_grid_intervals);

	cell_centred_checkbox->setChecked(d_cell_centred_nodes);

	lineEdit_path->setText(QDir::toNativeSeparators(QDir::currentPath()));

	lineEdit_file_name_template->setText(d_file_name_template.c_str());
}


void
GPlatesQtWidgets::GenerateVelocityDomainLatLonDialog::react_top_extents_spin_box_value_changed(
		double value)
{
	d_extents_top = value;
}


void
GPlatesQtWidgets::GenerateVelocityDomainLatLonDialog::react_bottom_extents_spin_box_value_changed(
		double value)
{
	d_extents_bottom = value;
}


void
GPlatesQtWidgets::GenerateVelocityDomainLatLonDialog::react_left_extents_spin_box_value_changed(
		double value)
{
	d_extents_left = value;

	// Make sure longitude extent cannot exceed 360 degrees (either direction).
	right_extents_spinbox->setMinimum(d_extents_left - 360);
	right_extents_spinbox->setMaximum(d_extents_left + 360);

	// Update the number of nodes.
	display_num_nodes();
}


void
GPlatesQtWidgets::GenerateVelocityDomainLatLonDialog::react_right_extents_spin_box_value_changed(
		double value)
{
	d_extents_right = value;

	// Make sure longitude extent cannot exceed 360 degrees (either direction).
	left_extents_spinbox->setMinimum(d_extents_right - 360);
	left_extents_spinbox->setMaximum(d_extents_right + 360);

	// Update the number of nodes.
	display_num_nodes();
}


void
GPlatesQtWidgets::GenerateVelocityDomainLatLonDialog::handle_use_global_extents_button_clicked()
{
	const double left = -180;
	const double right = 180;

	// Reset the min/max longitude values.
	left_extents_spinbox->setMinimum(right - 360);
	left_extents_spinbox->setMaximum(right + 360);
	right_extents_spinbox->setMinimum(left - 360);
	left_extents_spinbox->setMaximum(left + 360);

	top_extents_spinbox->setValue(90);
	bottom_extents_spinbox->setValue(-90);
	left_extents_spinbox->setValue(left);
	right_extents_spinbox->setValue(right);
}


void
GPlatesQtWidgets::GenerateVelocityDomainLatLonDialog::handle_num_latitude_grid_intervals_value_changed(
		int num_latitude_grid_intervals)
{
	d_num_latitude_grid_intervals = num_latitude_grid_intervals;

	// Update the number of nodes.
	display_num_nodes();
}


void
GPlatesQtWidgets::GenerateVelocityDomainLatLonDialog::handle_num_longitude_grid_intervals_value_changed(
		int num_longitude_grid_intervals)
{
	d_num_longitude_grid_intervals = num_longitude_grid_intervals;

	// Update the number of nodes.
	display_num_nodes();
}


void
GPlatesQtWidgets::GenerateVelocityDomainLatLonDialog::react_cell_centred_check_box_changed()
{
	d_cell_centred_nodes = cell_centred_checkbox->isChecked();

	// Update the number of nodes.
	display_num_nodes();
}


unsigned int
GPlatesQtWidgets::GenerateVelocityDomainLatLonDialog::get_num_latitude_nodes() const
{
	return d_cell_centred_nodes
			? d_num_latitude_grid_intervals
			: d_num_latitude_grid_intervals + 1;
}


unsigned int
GPlatesQtWidgets::GenerateVelocityDomainLatLonDialog::get_num_longitude_nodes() const
{
	if (d_cell_centred_nodes)
	{
		return d_num_longitude_grid_intervals;
	}

	// If the longitude extent is 360 degrees then there's one less node due to wraparound.
	// Note that the longitude extents are already constrained to the range [-360, 360].
	return GPlatesMaths::are_almost_exactly_equal(std::fabs(d_extents_right - d_extents_left), 360)
			? d_num_longitude_grid_intervals
			: d_num_longitude_grid_intervals + 1;
}


void
GPlatesQtWidgets::GenerateVelocityDomainLatLonDialog::display_num_nodes()
{
	const int num_nodes = get_num_latitude_nodes() * get_num_longitude_nodes();

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
	GPlatesModel::NotificationGuard model_notification_guard(*model.access_model());

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

	const double grid_latitude_length = d_extents_bottom - d_extents_top;
	const double grid_latitude_spacing = grid_latitude_length / d_num_latitude_grid_intervals;
	const double grid_latitude_top = d_extents_top +
			(d_cell_centred_nodes ? 0.5 * grid_latitude_spacing : 0);
	const unsigned int num_latitude_grid_nodes = get_num_latitude_nodes();

	const double grid_longitude_length = d_extents_right - d_extents_left;
	const double grid_longitude_spacing = grid_longitude_length / d_num_longitude_grid_intervals;
	const double grid_longitude_left = d_extents_left +
			(d_cell_centred_nodes ? 0.5 * grid_longitude_spacing : 0);
	const unsigned int num_longitude_grid_nodes = get_num_longitude_nodes();

	// Generate a grid within the lat/lon extents.
	for (unsigned int j = 0; j < num_latitude_grid_nodes; ++j)
	{
		const double latitude = grid_latitude_top + j * grid_latitude_spacing;

		for (unsigned int i = 0; i < num_longitude_grid_nodes; ++i)
		{
			const double longitude = grid_longitude_left + i * grid_longitude_spacing;

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
	std::stringstream num_longitude_grid_intervals_stream;
	num_longitude_grid_intervals_stream << d_num_longitude_grid_intervals;

	// Generate the filename from the template by replacing the place holders (if any) with parameter values.
	std::string file_name = d_file_name_template;	
	file_name.append(".gpml");
	replace_place_holder(file_name, NUM_LATITUDE_GRID_INTERVALS_PLACE_HOLDER, num_latitude_grid_intervals_stream.str());
	replace_place_holder(file_name, NUM_LONGITUDE_GRID_INTERVALS_PLACE_HOLDER, num_longitude_grid_intervals_stream.str());
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
