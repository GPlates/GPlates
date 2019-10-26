/**
 * \file
 * $Revision: 8651 $
 * $Date: 2010-06-06 20:15:55 +0200 (Sun, 06 Jun 2010) $
 *
 * Copyright (C) 2014, 2015 Geological Survey of Norway
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

#include <cmath> // std::pow

#include <QMessageBox>
#include <QStandardItemModel>
#include <QVector>

#include "qwt_scale_engine.h"
#include "qwt_plot.h"
#include "qwt_plot_canvas.h"
#include "qwt_plot_curve.h"
#include "qwt_plot_layout.h"
#include "qwt_plot_picker.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/MotionPathUtils.h"
#include "app-logic/PlateVelocityUtils.h"
#include "app-logic/ReconstructionTree.h"
#include "app-logic/ReconstructionTreeCreator.h"
#include "app-logic/RotationUtils.h"
#include "app-logic/UserPreferences.h"
#include "feature-visitors/GeometryFinder.h"
#include "gui/AnimationController.h"
#include "gui/CsvExport.h"
#include "gui/FeatureFocus.h"
#include "maths/FiniteRotation.h"
#include "maths/GeometryOnSphere.h"
#include "maths/UnitQuaternion3D.h"
#include "model/FeatureHandle.h"
#include "presentation/ViewState.h"
#include "qt-widgets/KinematicGraphsConfigurationDialog.h"
#include "qt-widgets/SaveFileDialog.h"
#include "utils/FeatureUtils.h"
#include "view-operations/GeometryBuilder.h" // for GeometryVertexFinder
#include "KinematicGraphPicker.h"

#include "KinematicGraphsDialog.h"

#define NUM_ELEMS(a) (sizeof(a) / sizeof((a)[0]))

const double VERTICAL_SCALE_MULTIPLIER = 0.7;
const unsigned int MAX_VERTICAL_SCALE_POWER = 5;
const unsigned int MIN_VERTICAL_SCALE_POWER = 0;
const double INITIAL_BEGIN_TIME = 200.;
const double INITIAL_END_TIME = 0.;
//const double INITIAL_TIME_STEP = 10.;

// Set start-up time step to 5 Ma for 2.0
const double INITIAL_TIME_STEP = 5.;

typedef GPlatesGui::ConfigGuiUtils::ConfigButtonGroupAdapter::button_enum_to_description_map_type velocity_method_map_type;

// TODO: Implement the "create-motion-path-feature" option.

namespace
{
	// Some FileDialogFilter-related functions largely copied from TotalReconstructionPolesDialog class.

	/**
	 * Struct to build the following table of file dialog filters / options.
	 * Typedef for the resulting QMap.
	 */
	struct FileDialogFilterOption
	{
		const char *text;
		const GPlatesGui::CsvExport::ExportOptions options;
	};
	typedef QMap<QString, GPlatesGui::CsvExport::ExportOptions> FileDialogFilterMapType;

	/**
	 * Table of filter options to present to the user when exporting CSV.
	 */
	static const FileDialogFilterOption file_dialog_filter_table[] = {
		{ QT_TRANSLATE_NOOP("KinematicGraphsDialog",
		  "CSV file, comma-delimited"),
		  { ',' } },
		{ QT_TRANSLATE_NOOP("KinematicGraphsDialog",
		  "CSV file, semicolon-delimited"),
		  { ';' } },
		{ QT_TRANSLATE_NOOP("KinematicGraphsDialog",
		  "CSV file, tab-delimited"),
		  { '\t' } },
	};

	/**
	 * This map is built for a quick, easy way to get back the CSV options
	 * based on what filter the QFileDialog says was selected.
	 */
	const FileDialogFilterMapType &
	build_export_filter_map()
	{
		static FileDialogFilterMapType map;
		const FileDialogFilterOption *begin = file_dialog_filter_table;
		const FileDialogFilterOption *end = begin + NUM_ELEMS(file_dialog_filter_table);
		for (; begin != end; ++begin)
		{
			map.insert(
						QObject::tr(begin->text) + " (*.csv)",
						begin->options);
		}
		return map;
	}

	/**
	 * Construct filters to give to SaveFileDialog.
	 */
	GPlatesQtWidgets::SaveFileDialog::filter_list_type
	build_save_file_dialog_filters()
	{
		GPlatesQtWidgets::SaveFileDialog::filter_list_type result;
		const FileDialogFilterOption *begin = file_dialog_filter_table;
		const FileDialogFilterOption *end = begin + NUM_ELEMS(file_dialog_filter_table);
		for (; begin != end; ++begin)
		{
			result.push_back(GPlatesQtWidgets::FileDialogFilter(begin->text, "csv"));
		}

		return result;
	}



	void
	append_row(
			QStandardItemModel *model,
			const GPlatesQtWidgets::KinematicGraphsDialog::table_entries &values)
	{
		static const QLocale locale;
		unsigned int row = model->rowCount();
		model->insertRow(row);
		model->setData(model->index(row,GPlatesQtWidgets::KinematicGraphsDialog::TIME_COLUMN),
					   values.d_time);

		model->setData(model->index(row,GPlatesQtWidgets::KinematicGraphsDialog::LAT_COLUMN),
					   locale.toString(values.d_lat,'f',4));
		model->setData(model->index(row,GPlatesQtWidgets::KinematicGraphsDialog::LON_COLUMN),
					   locale.toString(values.d_lon,'f',4));
		model->setData(model->index(row,GPlatesQtWidgets::KinematicGraphsDialog::VELOCITY_MAG_COLUMN),
					   locale.toString(values.d_velocity_mag,'f',4));
		model->setData(model->index(row,GPlatesQtWidgets::KinematicGraphsDialog::VELOCITY_AZIMUTH_COLUMN),
					   locale.toString(values.d_velocity_azimuth,'f',4));
		model->setData(model->index(row,GPlatesQtWidgets::KinematicGraphsDialog::VELOCITY_COLAT_COLUMN),
					   locale.toString(values.d_velocity_colat,'f',4));
		model->setData(model->index(row,GPlatesQtWidgets::KinematicGraphsDialog::VELOCITY_LON_COLUMN),
					   locale.toString(values.d_velocity_lon,'f',4));
		model->setData(model->index(row,GPlatesQtWidgets::KinematicGraphsDialog::ANGULAR_VELOCITY_COLUMN),
					   locale.toString(values.d_angular_velocity,'f',4));
#if 0
		model->setData(model->index(row,GPlatesQtWidgets::KinematicGraphsDialog::ROTATION_RATE_COLUMN),
					   values.d_rotation_rate);
#endif
	}

	double
	get_data_from_result_structure(
			const GPlatesQtWidgets::KinematicGraphsDialog::KinematicGraphType &graph_type,
			const GPlatesQtWidgets::KinematicGraphsDialog::table_entries &result)
	{
		// NOTE: absolute value returned for angular-velocity and rotation-rate.

		switch(graph_type)
		{
		case GPlatesQtWidgets::KinematicGraphsDialog::LATITUDE_GRAPH_TYPE:
			return result.d_lat;
			break;
		case GPlatesQtWidgets::KinematicGraphsDialog::LONGITUDE_GRAPH_TYPE:
			return result.d_lon;
			break;
		case GPlatesQtWidgets::KinematicGraphsDialog::VELOCITY_MAG_GRAPH_TYPE:
			return result.d_velocity_mag;
			break;
		case GPlatesQtWidgets::KinematicGraphsDialog::VELOCITY_AZIMUTH_GRAPH_TYPE:
			return result.d_velocity_azimuth;
			break;
		case GPlatesQtWidgets::KinematicGraphsDialog::VELOCITY_COLAT_GRAPH_TYPE:
			return result.d_velocity_colat;
			break;
		case GPlatesQtWidgets::KinematicGraphsDialog::VELOCITY_LON_GRAPH_TYPE:
			return result.d_velocity_lon;
			break;
		case GPlatesQtWidgets::KinematicGraphsDialog::ANGULAR_VELOCITY_GRAPH_TYPE:
			return std::abs(result.d_angular_velocity);
			break;
#if 0
		case GPlatesQtWidgets::KinematicGraphsDialog::ROTATION_RATE_GRAPH_TYPE:
			return std::abs(result.d_rotation_rate);
			break;
#endif
		default:
			return 0.;
			break;
		}
	}

	void
	check_model_for_bad_velocity_values(
			QStandardItemModel *model,
			double velocity_threshold,
			std::vector<QModelIndex> &bad_indices)
	{
		int rows = model->rowCount();
		for (int row = 0; row < rows; ++row)
		{
			QStandardItem *item = model->item(row,GPlatesQtWidgets::KinematicGraphsDialog::VELOCITY_MAG_COLUMN);
			if (item)
			{
				double velocity = item->data(Qt::EditRole).toDouble();
				if (velocity > velocity_threshold)
				{
					bad_indices.push_back(item->index());
				}
			}
		}
	}

	void
	highlight_bad_rows_in_table(
			QStandardItemModel *model,
			const std::vector<QModelIndex> &bad_indices,
			const QBrush &brush)
	{

		BOOST_FOREACH(QModelIndex index, bad_indices)
		{
			int row = index.row();
			for (int column = 0; column < GPlatesQtWidgets::KinematicGraphsDialog::NUM_COLUMNS; ++column)
			{
				model->setData(model->index(row,column),brush,Qt::BackgroundRole);
			}
		}
	}

	void
	reset_table_background_colours(
			QStandardItemModel *model)
	{
		static QBrush default_background_brush(Qt::white);

		int rows = model->rowCount();
		for (int row = 0; row < rows; ++row)
		{
			for (int column = 0; column < GPlatesQtWidgets::KinematicGraphsDialog::NUM_COLUMNS; ++column)
			{
				model->setData(model->index(row,column),default_background_brush,Qt::BackgroundRole);
			}
		}
	}

	/**
	 * @brief get_older_and_younger_times - on return @time_older and @time_younger will hold
	 * the appropriate times for the velocity calculation at the @current_time.
	 */
	void
	get_older_and_younger_times(
			const GPlatesQtWidgets::KinematicGraphsDialog::Configuration &configuration,
			const double &current_time,
			double &time_older,
			double &time_younger)
	{
		switch(configuration.d_velocity_method)
		{
		case GPlatesQtWidgets::KinematicGraphsConfigurationWidget::T_TO_T_MINUS_DT:
			time_older = current_time;
			time_younger = current_time - configuration.d_delta_t;
			break;
		case GPlatesQtWidgets::KinematicGraphsConfigurationWidget::T_PLUS_DT_TO_T:
			time_older = current_time + configuration.d_delta_t;
			time_younger = current_time;
			break;
		case GPlatesQtWidgets::KinematicGraphsConfigurationWidget::T_PLUS_MINUS_HALF_DT:
			time_older = current_time + configuration.d_delta_t/2.;
			time_younger = current_time - configuration.d_delta_t/2.;
			break;
		default:
			time_older = current_time;
			time_younger = current_time - configuration.d_delta_t;
		}
	}


}


GPlatesQtWidgets::KinematicGraphsDialog::KinematicGraphsDialog(
		GPlatesPresentation::ViewState &view_state,
		QWidget *parent_):
	GPlatesDialog(parent_,Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
	d_plot(new QwtPlot(this)),
	d_plot_curve(new QwtPlotCurve()),
	d_point_series_data(new QwtPointSeriesData()),
	d_picker(new KinematicGraphPicker(
				 d_point_series_data,
				 d_plot_curve,
				 QwtPlot::xBottom,
				 QwtPlot::yLeft,
				 QwtPicker::VLineRubberBand,
				 QwtPicker::AlwaysOn,
				 dynamic_cast<QwtPlotCanvas *>(d_plot->canvas()))),
	d_vertical_scale_power(1.),
	d_application_state(view_state.get_application_state()),
	d_view_state(view_state),
	d_feature_focus(view_state.get_feature_focus()),
	d_model(new QStandardItemModel(0,NUM_COLUMNS,this)),
	d_save_file_dialog(
		this,
		tr("Export Tabular Data"),
		build_save_file_dialog_filters(),
		view_state),
	d_settings_dialog(0)
{
	setupUi(this);

	read_values_from_preferences();

	initialise_widgets();
	set_up_connections();
	set_up_plot();
	set_up_axes_ranges();
	set_graph_axes_and_titles();
#if 0
	qDebug() << "Using Qwt version " << QWT_VERSION_STR;
	qDebug() << QWT_MAJOR_VERSION;
	qDebug() << QWT_MINOR_VERSION;
	qDebug() << QWT_PATCH_VERSION;
#endif
}

void
GPlatesQtWidgets::KinematicGraphsDialog::handle_close()
{
	reject();
}

void
GPlatesQtWidgets::KinematicGraphsDialog::handle_update()
{
	update_values_from_widgets();
	update_table();
	update_graph();
	check_and_highlight_bad_velocity_values();
}

void
GPlatesQtWidgets::KinematicGraphsDialog::handle_settings_clicked()
{
	if (!d_settings_dialog)
	{
		d_settings_dialog = new KinematicGraphsConfigurationDialog(d_configuration,this);
	}

	d_settings_dialog->show();
}

void
GPlatesQtWidgets::KinematicGraphsDialog::handle_export_table()
{
	// Borrowed largely from TRPDialog's "handle_export" method.

	// Build a map to let us look up the options the user wants based on what
	// file filter was selected in the dialog.
	static const FileDialogFilterMapType &filter_map = build_export_filter_map();

	// Pop up and ask for file.
	QString filter;
	boost::optional<QString> filename = d_save_file_dialog.get_file_name(&filter);
	if (filename)
	{
		if (filter_map.contains(filter))
		{
			GPlatesGui::CsvExport::ExportOptions options = filter_map.value(filter);
			GPlatesGui::CsvExport::export_table_view(*filename, options, *table_results);
		}
		else
		{
			// Somehow, user chose filter that we didn't put in there.
			QMessageBox::critical(this, tr("Invalid export filter"), tr("Please specify a CSV file format variant in the save dialog."));
		}
	}
}

void
GPlatesQtWidgets::KinematicGraphsDialog::handle_export_graph()
{

}

void
GPlatesQtWidgets::KinematicGraphsDialog::handle_graph_type_radio_toggled(
		bool state)
{
	if (radio_latitude->isChecked())
	{
		d_graph_type = LATITUDE_GRAPH_TYPE;
	}

	if (radio_longitude->isChecked())
	{
		d_graph_type = LONGITUDE_GRAPH_TYPE;
	}

	if (radio_velocity_mag->isChecked())
	{
		d_graph_type = VELOCITY_MAG_GRAPH_TYPE;
	}

	if (radio_velocity_azimuth->isChecked())
	{
		d_graph_type = VELOCITY_AZIMUTH_GRAPH_TYPE;
	}

	if (radio_velocity_colat->isChecked())
	{
		d_graph_type = VELOCITY_COLAT_GRAPH_TYPE;
	}

	if (radio_velocity_lon->isChecked())
	{
		d_graph_type = VELOCITY_LON_GRAPH_TYPE;
	}

	if (radio_angular_velocity->isChecked())
	{
		d_graph_type = ANGULAR_VELOCITY_GRAPH_TYPE;
	}
	d_picker->set_graph_type(d_graph_type);
	update_graph();
}

void
GPlatesQtWidgets::KinematicGraphsDialog::handle_use_feature()
{
	if (!d_feature_focus.is_valid())
	{
		return;
	}

	boost::optional<GPlatesModel::integer_plate_id_type> plate_id =
			GPlatesUtils::get_recon_plate_id_as_int(d_feature_focus.focused_feature().handle_ptr());

	if (plate_id)
	{
		d_moving_id = *plate_id;
		spinbox_plateid->setValue(d_moving_id);
	}

	// TODO: we can also use featured_focus().associated_reconstruction_geometry() for example.
	GPlatesFeatureVisitors::GeometryFinder finder;

	finder.visit_feature(d_feature_focus.focused_feature());

	GPlatesFeatureVisitors::GeometryFinder::geometry_container_const_iterator
			it = finder.found_geometries_begin();

	if (it != finder.found_geometries_end())
	{
		GPlatesViewOperations::GeometryVertexFinder vertex_finder(0);
		(*it)->accept_visitor(vertex_finder);

		boost::optional<GPlatesMaths::PointOnSphere> vertex = vertex_finder.get_vertex();

		if(vertex)
		{
			GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(*vertex);

			d_lat = llp.latitude();
			d_lon = llp.longitude();
			spinbox_latitude->setValue(d_lat);
			spinbox_longitude->setValue(d_lon);
		}
	}


	// Check if the feature is a motion path feature; if so, extract time information from it.
	// NOTE: the motion path can have arbitrarily spaced time values, whereas the kinematic graphs dialog
	// has a constant time step between time values. So we have a potential discrepancy there. As
	// a simple initial solution, we can determine the average time step of the motion path and use
	// that to generate the graph. A fuller solution would need us to allow varying
	// time-steps in the graphs dialog (not impossible, but more work, and adds to the complexity of the
	// graphing dialog). Probably a lot of motion-path users will be using a uniform
	// time step for their paths anyway, and if that's the case, there will be no discrepancy.

	// TODO: check if we need to extract the relative plate-id and use this as the anchor.
	static const GPlatesModel::FeatureType motion_path_feature_type =
			GPlatesModel::FeatureType::create_gpml("MotionPath");
	if (d_feature_focus.focused_feature().handle_ptr()->feature_type() == motion_path_feature_type)
	{
		qDebug() << "We have a motion path";

		GPlatesAppLogic::MotionPathUtils::MotionPathPropertyFinder property_finder;
		property_finder.visit_feature(d_feature_focus.focused_feature());

		std::vector<double> times = property_finder.get_times();

		// Motion path times are stored in increasing order, i.e. youngest (end-time) to oldest (begin-time)
		d_begin_time = times.back();
		d_end_time = times.front();
		int steps = static_cast<int>(times.size());

		// If we're not able to get a sensible time step for some reason, d_step_time
		// will not be updated.
		if (steps > 1)
		{
			d_step_time = (d_begin_time - d_end_time)/(steps-1);
		}

		spinbox_begin_time->setValue(d_begin_time);
		spinbox_end_time->setValue(d_end_time);
		spinbox_dt->setValue(d_step_time);
	}

	// And we might as well do the whole calculation thing here as well.
	handle_update();
}

void
GPlatesQtWidgets::KinematicGraphsDialog::handle_use_animation()
{
	d_begin_time = d_view_state.get_animation_controller().start_time();
	d_end_time = d_view_state.get_animation_controller().end_time();
	d_step_time = d_view_state.get_animation_controller().time_increment();

	spinbox_begin_time->setValue(d_begin_time);
	spinbox_end_time->setValue(d_end_time);
	spinbox_dt->setValue(d_step_time);
}

void
GPlatesQtWidgets::KinematicGraphsDialog::handle_auto_y_clicked()
{

	d_plot->setAxisAutoScale(QwtPlot::yLeft, button_auto_y->isChecked());
	button_compress_y->setEnabled(!button_auto_y->isChecked());
	button_stretch_y->setEnabled(!button_auto_y->isChecked());
#if ((QWT_MAJOR_VERSION >= 6) && (QWT_MINOR_VERSION >= 1))
	double upper_ = d_plot->axisScaleDiv(QwtPlot::yLeft).upperBound();
	double lower_ = d_plot->axisScaleDiv(QwtPlot::yLeft).lowerBound();
#else
	double upper_ = d_plot->axisScaleDiv(QwtPlot::yLeft)->upperBound();
	double lower_ = d_plot->axisScaleDiv(QwtPlot::yLeft)->lowerBound();
#endif
	double bigger_of_upper_lower = (std::max)(std::abs(upper_),std::abs(lower_));

	double scale_factor = (2*bigger_of_upper_lower)/(d_vertical_scale_maxes[d_graph_type]-d_vertical_scale_mins[d_graph_type]);

	unsigned int power = static_cast<unsigned int>(std::log(scale_factor)/std::log(VERTICAL_SCALE_MULTIPLIER));
	d_vertical_scale_powers[d_graph_type] = power;
	d_plot->replot();
}

void
GPlatesQtWidgets::KinematicGraphsDialog::handle_compress_y_clicked()
{
	unsigned int scale = d_vertical_scale_powers[d_graph_type];
	if (scale > MIN_VERTICAL_SCALE_POWER){
		--scale;
	}

	d_vertical_scale_powers[d_graph_type] = scale;
	set_graph_axes_and_titles();
}

void
GPlatesQtWidgets::KinematicGraphsDialog::handle_stretch_y_clicked()
{
	unsigned int scale = d_vertical_scale_powers[d_graph_type];
	if (scale < MAX_VERTICAL_SCALE_POWER){
		++scale;
	}

	d_vertical_scale_powers[d_graph_type] = scale;
	set_graph_axes_and_titles();
}

void
GPlatesQtWidgets::KinematicGraphsDialog::handle_flip_horizontal_axis()
{
	bool current_axis_state =
			d_plot->axisScaleEngine(QwtPlot::xBottom)->testAttribute(QwtScaleEngine::Inverted);

	d_plot->axisScaleEngine(QwtPlot::xBottom)->setAttribute(QwtScaleEngine::Inverted,!current_axis_state);

	update_graph();
}

void
GPlatesQtWidgets::KinematicGraphsDialog::update_values_from_widgets()
{
	d_lat = spinbox_latitude->value();
	d_lon = spinbox_longitude->value();
	d_moving_id = spinbox_plateid->value();
	d_anchor_id = spinbox_anchor->value();

	d_begin_time = spinbox_begin_time->value();
	d_end_time = spinbox_end_time->value();
	d_step_time = spinbox_dt->value();


	//	Warn if begin/end times don't make sense
	static QPalette red_palette;
	red_palette.setColor(QPalette::Active, QPalette::Base, Qt::red);
	if (d_begin_time <= d_end_time)
	{
		spinbox_begin_time->setPalette(red_palette);
		spinbox_end_time->setPalette(red_palette);
	}
	else
	{
		spinbox_begin_time->setPalette(d_spin_box_palette);
		spinbox_end_time->setPalette(d_spin_box_palette);
	}
	// FIXME: change logic so that we bail out of the whole update process here.
}

void
GPlatesQtWidgets::KinematicGraphsDialog::update_table()
{

	using namespace GPlatesMaths;

	d_results.clear();
	d_model->setRowCount(0);

	if (d_end_time >= d_begin_time)
	{
		qDebug() << "End time " << d_end_time << " is before begin time " << d_begin_time;
		return;
	}

	// Time interval for velocity calculation, Ma.
	double dtime = d_configuration.d_delta_t;

	if (GPlatesMaths::Real(dtime) == 0.0)
	{
		qDebug() << "Zero value for dt.";
		return;
	}

	// The default reconstruction tree creator.
	GPlatesAppLogic::ReconstructionTreeCreator tree_creator =
			d_application_state.get_current_reconstruction()
			.get_default_reconstruction_layer_output()->get_reconstruction_tree_creator();

	LatLonPoint llp(d_lat,d_lon);
	PointOnSphere pos_ = make_point_on_sphere(llp);

#if 0
	double previous_azimuth = 0.; // temp for calculating delta(azimuth)
#endif
	// From oldest time to youngest time.
	for (double time = d_begin_time; time >= d_end_time; time-=d_step_time)
	{
		// Older and younger times used in the velocity calculation.
		double time_older, time_younger;
		get_older_and_younger_times(d_configuration,time,time_older,time_younger);

		GPlatesAppLogic::ReconstructionTree::non_null_ptr_to_const_type tree =
				tree_creator.get_reconstruction_tree(time, d_anchor_id);
		FiniteRotation rot = tree->get_composed_absolute_rotation(d_moving_id).first;

		PointOnSphere p = rot*pos_;

		LatLonPoint l = make_lat_lon_point(p);

		// t1 is younger than t2, as required by the calculate_velocity_vector_and_omaga function used below.
		GPlatesAppLogic::ReconstructionTree::non_null_ptr_to_const_type tree_t1 =
				tree_creator.get_reconstruction_tree(time_younger, d_anchor_id);
		GPlatesAppLogic::ReconstructionTree::non_null_ptr_to_const_type tree_t2 =
				tree_creator.get_reconstruction_tree(time_older, d_anchor_id);


		FiniteRotation rot_1 = tree_t1->get_composed_absolute_rotation(d_moving_id).first;
		FiniteRotation rot_2 = tree_t2->get_composed_absolute_rotation(d_moving_id).first;

		PointOnSphere p_1 = rot_1*pos_;
		PointOnSphere p_2 = rot_2*pos_;

		// The velocity calculation assumes a time step of 1Ma. As we have used dtime here to generate the finite rotations, we need to correct for this.
		// The position here should represent the position of the point *at the desired time instant*, not the present day point.

		// I've added an axis hint to the velocity routine in order to get the sign of the rotation angle, but in order to get the axis hint
		// I have to generate the stage pole, so I'm probably duplicating work here. There may be a neater way of getting this.
		FiniteRotation stage_pole_rotation = GPlatesAppLogic::RotationUtils::get_stage_pole(*tree_t1,*tree_t2,d_moving_id,d_anchor_id);
		boost::optional<UnitVector3D> stage_pole_axis = stage_pole_rotation.axis_hint();
		std::pair<Vector3D,real_t> velocity_and_omega_pair = calculate_velocity_vector_and_omega(p,rot_1,rot_2,dtime,stage_pole_axis);
		Vector3D v = velocity_and_omega_pair.first;

		VectorColatitudeLongitude vcl = convert_vector_from_xyz_to_colat_lon(p,v);
		std::pair<real_t,real_t> mag_azimuth =
				calculate_vector_components_magnitude_and_azimuth(p,v);

		table_entries results;

#if 0
		qDebug() << velocity_cm_per_year << ", " << v.magnitude().dval();
		qDebug() << "Time: " << time;
		qDebug() << "V colat/lon" << vcl.get_vector_colatitude()<< "," << vcl.get_vector_longitude();
		qDebug() << "V mag: " << v.magnitude() << ", " << mag_azimuth.first;
		qDebug();
#endif

		results.d_time = time;
		results.d_lat = l.latitude();
		results.d_lon = l.longitude();
		results.d_velocity_mag = mag_azimuth.first.dval();
		results.d_velocity_azimuth = convert_rad_to_deg(mag_azimuth.second.dval());
		results.d_velocity_colat = vcl.get_vector_colatitude().dval(); // south component
		results.d_velocity_lon = vcl.get_vector_longitude().dval(); // east component
		results.d_angular_velocity = convert_rad_to_deg(velocity_and_omega_pair.second.dval());
#if 0
		qDebug() << "rot: " << results.d_angular_velocity;
		double delta_azimuth = previous_azimuth - results.d_velocity_azimuth;
		qDebug() << "delta_azimuth 1: " << delta_azimuth;
		if (delta_azimuth > 180.){
			delta_azimuth -= 360.;
		}
		else if (delta_azimuth < -180.)
		{
			delta_azimuth += 360.;
		}
		qDebug() << "delta_azimuth 2: " << delta_azimuth;

		results.d_rotation_rate = delta_azimuth*one_over_dtime;
		qDebug() << "rotation: " << results.d_rotation_rate;
		qDebug();
		previous_azimuth = results.d_velocity_azimuth;
#endif
		d_results.push_back(results);


	} // for-loop over times


	BOOST_FOREACH(table_entries result_, d_results)
	{
		append_row(d_model,result_);
	}

	// Put the oldest times at the top of the table.
	table_results->sortByColumn(TIME_COLUMN,Qt::DescendingOrder);
}

void
GPlatesQtWidgets::KinematicGraphsDialog::update_graph()
{
	set_graph_axes_and_titles();

	d_samples.clear();

	BOOST_FOREACH(table_entries result_, d_results)
	{
		double data_ = get_data_from_result_structure(d_graph_type,result_);
		d_samples.push_back(QPointF(result_.d_time,data_));
	}
	d_point_series_data->setSamples(d_samples);
	d_plot->replot();

}


void
GPlatesQtWidgets::KinematicGraphsDialog::set_up_plot()
{

	QGridLayout *layout_ = new QGridLayout(widget_plot);
	layout_->addWidget(d_plot);

	d_plot->setTitle("Test qwt");
	d_plot->setAxisScale(QwtPlot::xBottom,INITIAL_BEGIN_TIME,0);
	d_plot->setAxisScale(QwtPlot::yLeft,-90,90);

	// Reverse x axis so we go from oldest (left) to youngest (right).
	d_plot->axisScaleEngine(QwtPlot::xBottom)->setAttribute(QwtScaleEngine::Inverted);

	d_plot->setCanvasBackground(QBrush(Qt::white));

	d_plot_curve->setData(d_point_series_data);

	d_plot_curve->attach(d_plot);
	d_plot->show();
}

void
GPlatesQtWidgets::KinematicGraphsDialog::check_and_highlight_bad_velocity_values()
{
	static QBrush yellow_brush(Qt::yellow);
	static QBrush red_brush(Qt::red);
	std::vector<QModelIndex> yellow_indices;
	std::vector<QModelIndex> red_indices;
	check_model_for_bad_velocity_values(d_model,d_configuration.d_yellow_threshold,yellow_indices);
	check_model_for_bad_velocity_values(d_model,d_configuration.d_red_threshold,red_indices);
	reset_table_background_colours(d_model);
	highlight_bad_rows_in_table(d_model,yellow_indices,yellow_brush);
	highlight_bad_rows_in_table(d_model,red_indices,red_brush);

}

void
GPlatesQtWidgets::KinematicGraphsDialog::read_values_from_preferences()
{
	GPlatesAppLogic::UserPreferences &prefs = d_application_state.get_user_preferences();
	d_configuration.d_delta_t = prefs.get_value("tools/kinematics/velocity_delta_time").toDouble();
	d_configuration.d_yellow_threshold = prefs.get_value("tools/kinematics/velocity_warning_1").toDouble();
	d_configuration.d_red_threshold = prefs.get_value("tools/kinematics/velocity_warning_2").toDouble();

	QVariant velocity_method_variant = prefs.get_value("tools/kinematics/velocity_method");

	static const velocity_method_map_type
			map = KinematicGraphsConfigurationWidget::build_velocity_method_description_map();

	GPlatesGui::ConfigGuiUtils::MapValueEquals map_value_equals(velocity_method_variant.toString());

	const velocity_method_map_type::const_iterator
			it = std::find_if(map.begin(),map.end(),map_value_equals);

	if (it != map.end())
	{
		d_configuration.d_velocity_method = static_cast<KinematicGraphsConfigurationWidget::VelocityMethod>(it.key());
	}
}

void
GPlatesQtWidgets::KinematicGraphsDialog::initialise_widgets()
{

	//	  Qt documentation says (http://qt-project.org/doc/qt-4.8/qobject.html):
	//		"For portability reasons, we recommend that you use escape sequences for
	//		specifying non-ASCII characters in string literals to trUtf8(). For example:
	//			label->setText(tr("F\374r \310lise"));  "
	//
	//		So we use \260 for the degree symbol.




	// Initialise spinboxes
	spinbox_latitude->setValue(0);
	spinbox_longitude->setValue(0);
	spinbox_plateid->setValue(0);
	spinbox_anchor->setValue(d_application_state.get_current_anchored_plate_id());
	spinbox_begin_time->setValue(INITIAL_BEGIN_TIME);
	spinbox_end_time->setValue(INITIAL_END_TIME);
	spinbox_dt->setValue(INITIAL_TIME_STEP);

	// Set up table widget
	d_model->setHorizontalHeaderItem(TIME_COLUMN,new QStandardItem(QObject::tr("Time (Ma)")));
	d_model->setHorizontalHeaderItem(LAT_COLUMN,new QStandardItem(QObject::tr("Lat")));
	d_model->setHorizontalHeaderItem(LON_COLUMN,new QStandardItem(QObject::tr("Lon")));
	d_model->setHorizontalHeaderItem(VELOCITY_MAG_COLUMN,new QStandardItem(QObject::tr("V mag (cm/yr)")));
	d_model->setHorizontalHeaderItem(VELOCITY_AZIMUTH_COLUMN,new QStandardItem(QObject::tr("V azimuth (\260)")));
	d_model->setHorizontalHeaderItem(VELOCITY_COLAT_COLUMN,new QStandardItem(QObject::tr("V colat (cm/yr)")));
	d_model->setHorizontalHeaderItem(VELOCITY_LON_COLUMN,new QStandardItem(QObject::tr("V lon (cm/yr)")));
	d_model->setHorizontalHeaderItem(ANGULAR_VELOCITY_COLUMN,new QStandardItem(QObject::tr("Ang V (\260/Ma)")));


	d_model->horizontalHeaderItem(TIME_COLUMN)->setToolTip(QObject::tr("Time (Ma)"));
	d_model->horizontalHeaderItem(LAT_COLUMN)->setToolTip(QObject::tr("Latitude"));
	d_model->horizontalHeaderItem(LON_COLUMN)->setToolTip(QObject::tr("Longitude"));
	d_model->horizontalHeaderItem(VELOCITY_MAG_COLUMN)->setToolTip(QObject::tr("Magnitude of velocity (cm/yr)"));
	d_model->horizontalHeaderItem(VELOCITY_AZIMUTH_COLUMN)->setToolTip(QObject::tr("Azimuth of velocity (\260)"));
	d_model->horizontalHeaderItem(VELOCITY_COLAT_COLUMN)->setToolTip(QObject::tr("Colatitude component of velocity (cm/yr)"));
	d_model->horizontalHeaderItem(VELOCITY_LON_COLUMN)->setToolTip(QObject::tr("Longitude component of velocity (cm/yr)"));
	d_model->horizontalHeaderItem(ANGULAR_VELOCITY_COLUMN)->setToolTip(QObject::tr("Angular velocity (\260/Ma)"));


	table_results->setModel(d_model);

	table_results->horizontalHeader()->resizeSection(TIME_COLUMN,100);
	table_results->horizontalHeader()->resizeSection(LAT_COLUMN,90);
	table_results->horizontalHeader()->resizeSection(LON_COLUMN,90);
	table_results->horizontalHeader()->resizeSection(VELOCITY_MAG_COLUMN,130);
	table_results->horizontalHeader()->resizeSection(VELOCITY_AZIMUTH_COLUMN,130);
	table_results->horizontalHeader()->resizeSection(VELOCITY_COLAT_COLUMN,130);
	table_results->horizontalHeader()->resizeSection(VELOCITY_LON_COLUMN,130);
	table_results->horizontalHeader()->resizeSection(ANGULAR_VELOCITY_COLUMN,130);
	table_results->horizontalHeader()->setStretchLastSection(true);

	table_results->setEditTriggers(QAbstractItemView::NoEditTriggers);
	//FIXME: This (alternating row colours) is not getting picked up, either here or in the Designer.
	table_results->setAlternatingRowColors(true);
#if 0
	// Temp addition to table
	d_model->setHorizontalHeaderItem(ROTATION_RATE_COLUMN,new QStandardItem(QObject::tr("Rot rate (\260/Ma)")));
	d_model->horizontalHeaderItem(ROTATION_RATE_COLUMN)->setToolTip(QObject::tr("Rate of change of velocity azimuth"));
	table_results->horizontalHeader()->resizeSection(ROTATION_RATE_COLUMN,100);
#endif
	radio_latitude->setChecked(true);
	d_graph_type = LATITUDE_GRAPH_TYPE;

	button_update->setFocus();
	table_results->verticalHeader()->setVisible(false);

	//One day....
	button_export_graph->setEnabled(false);
	button_export_graph->setVisible(false);
	button_create_motion_path->setVisible(false);
	radio_rotation_rate->setVisible(false);

	d_spin_box_palette = spinbox_begin_time->palette();
}

void
GPlatesQtWidgets::KinematicGraphsDialog::set_up_connections()
{
	QObject::connect(button_close,SIGNAL(clicked()),this,SLOT(handle_close()));
	QObject::connect(button_update,SIGNAL(clicked()),this,SLOT(handle_update()));
	QObject::connect(button_use_animation,SIGNAL(clicked()),this,SLOT(handle_use_animation()));
	QObject::connect(button_use_feature,SIGNAL(clicked()),this,SLOT(handle_use_feature()));
	QObject::connect(radio_latitude,SIGNAL(toggled(bool)),this,SLOT(handle_graph_type_radio_toggled(bool)));
	QObject::connect(radio_longitude,SIGNAL(toggled(bool)),this,SLOT(handle_graph_type_radio_toggled(bool)));
	QObject::connect(radio_velocity_mag,SIGNAL(toggled(bool)),this,SLOT(handle_graph_type_radio_toggled(bool)));
	QObject::connect(radio_velocity_azimuth,SIGNAL(toggled(bool)),this,SLOT(handle_graph_type_radio_toggled(bool)));
	QObject::connect(radio_velocity_colat,SIGNAL(toggled(bool)),this,SLOT(handle_graph_type_radio_toggled(bool)));
	QObject::connect(radio_velocity_lon,SIGNAL(toggled(bool)),this,SLOT(handle_graph_type_radio_toggled(bool)));
	QObject::connect(radio_angular_velocity,SIGNAL(toggled(bool)),this,SLOT(handle_graph_type_radio_toggled(bool)));
	QObject::connect(radio_rotation_rate,SIGNAL(toggled(bool)),this,SLOT(handle_graph_type_radio_toggled(bool)));
	QObject::connect(button_auto_y,SIGNAL(clicked()),this,SLOT(handle_auto_y_clicked()));
	QObject::connect(button_compress_y,SIGNAL(clicked()),this,SLOT(handle_compress_y_clicked()));
	QObject::connect(button_stretch_y,SIGNAL(clicked()),this,SLOT(handle_stretch_y_clicked()));
	QObject::connect(button_flip_x,SIGNAL(clicked()),this,SLOT(handle_flip_horizontal_axis()));
	QObject::connect(button_export_table,SIGNAL(clicked()),this,SLOT(handle_export_table()));
	QObject::connect(button_settings,SIGNAL(clicked()),this,SLOT(handle_settings_clicked()));

}

void
GPlatesQtWidgets::KinematicGraphsDialog::set_graph_axes_and_titles()
{
	QString axis_title = QObject::tr("Axis Title");
	QString graph_title = QObject::tr("Graph Title");
	switch(d_graph_type)
	{
	case LATITUDE_GRAPH_TYPE:
		axis_title = QObject::tr("Latitude");
		graph_title = QObject::tr("Latitude vs time");
		break;
	case LONGITUDE_GRAPH_TYPE:
		axis_title = QObject::tr("Longitude");
		graph_title = QObject::tr("Longitude vs time");
		break;
	case VELOCITY_MAG_GRAPH_TYPE:
		axis_title = QObject::tr("Velocity (cm/yr)");
		graph_title = QObject::tr("Velocity magnitude vs time");
		break;
	case VELOCITY_AZIMUTH_GRAPH_TYPE:
		axis_title = QObject::tr("Azimuth (\260)");
		graph_title = QObject::tr("Velocity azimuth vs time");
		break;
	case VELOCITY_COLAT_GRAPH_TYPE:
		axis_title = QObject::tr("Velocity (cm/yr)");
		graph_title = QObject::tr("Velocity colatitude component vs time");
		break;
	case VELOCITY_LON_GRAPH_TYPE:
		axis_title = QObject::tr("Velocity (cm/yr)");
		graph_title = QObject::tr("Velocity longitude component vs time");
		break;
	case ANGULAR_VELOCITY_GRAPH_TYPE:
		axis_title = QObject::tr("Angular velocity (\260/Ma)");
		graph_title = QObject::tr("Angular velocity vs time");
		break;
	case ROTATION_RATE_GRAPH_TYPE:
		axis_title = QObject::tr("Rotation rate (\260/Ma)");
		graph_title = QObject::tr("Rotation rate vs time");
		break;
	default:
		break;

	}

	double y_min = d_vertical_scale_mins[d_graph_type];
	double y_max = d_vertical_scale_maxes[d_graph_type];

	d_plot->setTitle(graph_title);

	bool auto_scale_y = button_auto_y->isChecked();
	d_plot->setAxisAutoScale(QwtPlot::yLeft, auto_scale_y);
	if (!auto_scale_y)
	{
		double factor = std::pow(VERTICAL_SCALE_MULTIPLIER,static_cast<int>(d_vertical_scale_powers[d_graph_type]));
		d_plot->setAxisScale(QwtPlot::yLeft,y_min*factor,y_max*factor);
	}
	d_plot->setAxisTitle(QwtPlot::yLeft,axis_title);
	d_plot->setAxisAutoScale(QwtPlot::xBottom);
	d_plot->setAxisTitle(QwtPlot::xBottom,"Time (Ma)");

	d_plot->replot();
}

void
GPlatesQtWidgets::KinematicGraphsDialog::set_up_axes_ranges()
{
	for(int i = 0; i < NUM_GRAPH_TYPES; ++i)
	{
		d_vertical_scale_powers[i] = 0;
	}

	d_vertical_scale_maxes[LATITUDE_GRAPH_TYPE] = 90.;
	d_vertical_scale_mins[LATITUDE_GRAPH_TYPE] = -90.;

	d_vertical_scale_maxes[LONGITUDE_GRAPH_TYPE] = 180.;
	d_vertical_scale_mins[LONGITUDE_GRAPH_TYPE] = -180.;

	d_vertical_scale_maxes[VELOCITY_MAG_GRAPH_TYPE] = 20.;
	d_vertical_scale_mins[VELOCITY_MAG_GRAPH_TYPE] = -20.;

	d_vertical_scale_maxes[VELOCITY_AZIMUTH_GRAPH_TYPE] = 360.;
	d_vertical_scale_mins[VELOCITY_AZIMUTH_GRAPH_TYPE] = 0.;

	d_vertical_scale_maxes[VELOCITY_COLAT_GRAPH_TYPE] = 20.;
	d_vertical_scale_mins[VELOCITY_COLAT_GRAPH_TYPE] = -20.;

	d_vertical_scale_maxes[VELOCITY_LON_GRAPH_TYPE] = 20.;
	d_vertical_scale_mins[VELOCITY_LON_GRAPH_TYPE] = -20.;

	d_vertical_scale_maxes[ANGULAR_VELOCITY_GRAPH_TYPE] = 2.;
	d_vertical_scale_mins[ANGULAR_VELOCITY_GRAPH_TYPE] = 0.;
#if 0
	d_vertical_scale_maxes[ROTATION_RATE_GRAPH_TYPE] = 15.;
	d_vertical_scale_mins[ROTATION_RATE_GRAPH_TYPE] = 0.;
#endif
}
