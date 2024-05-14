/**
 * \file
 * $Revision: 8188 $
 * $Date: 2010-04-23 12:25:09 +0200 (Fri, 23 Apr 2010) $
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

#ifndef GPLATES_QTWIDGETS_KINEMATICGRAPHSDIALOG_H
#define GPLATES_QTWIDGETS_KINEMATICGRAPHSDIALOG_H

#include <QDialog>
#include <QScopedPointer>
#include "qwt_series_data.h"

#include "maths/LatLonPoint.h"

#include "model/types.h"
#include "GPlatesDialog.h"
#include "SaveFileDialog.h"
#include "KinematicGraphsConfigurationWidget.h"

#include "ui_KinematicGraphsDialogUi.h"

// TODO: Get vertical picker line to display all the time
// update: don't recall what I was meaning with the above comment...
// TODO: Graph export - how do we do this? QwtPlotRenderer, possibly
// TODO: Think about how to integrate this with MotionPaths. For example: if user has selected a motion path feature,
// and opens the kinematic dialog, then on the left hand side of the dialog is a table of point members of the motion path
// (i.e. seed points). There may only be one, if it's a single-point motion path seed point. If there's only one, then this is
// selected by default, and the values are reproduced in the "lat/lon" boxes. If it's a multipoint motion path seed, then
// the user can choose which one is taken as the "used" lat-lon point. The first point in the multipoint could be selected
// by default. The user can override the motion path points by entering their own lat-lon point if desired.


class QwtPlot;
class QwtPlotCurve;

class QStandardItemModel;

namespace GPlatesAppLogic
{
	class ApplicationState;
}

namespace GPlatesGui
{
	class FeatureFocus;
}

namespace GPlatesPresentation
{
	class ViewState;
}

// These values should be overridden by values read from preferences when the dialog is
// created.
const double INITIAL_DELTA_T = 5.;
const double INITIAL_THRESHOLD_YELLOW = 20. ; // Velocity threshold (cm/yr) for checking crazy values.
const double INITIAL_THRESHOLD_RED = 30. ; // Velocity threshold (cm/yr) for checking crazy values.

namespace GPlatesQtWidgets
{

	class KinematicGraphPicker;
	class KinematicGraphsConfigurationDialog;
	class KinematicGraphsConfigurationWidget;




	class KinematicGraphsDialog:
			public GPlatesDialog,
			protected Ui_KinematicGraphsDialog
	{
		Q_OBJECT

	public:

		struct Configuration
		{
			Configuration():
				d_delta_t(INITIAL_DELTA_T),
				d_yellow_threshold(INITIAL_THRESHOLD_YELLOW),
				d_red_threshold(INITIAL_THRESHOLD_RED),
				d_velocity_method(GPlatesQtWidgets::KinematicGraphsConfigurationWidget::T_TO_T_MINUS_DT)
			{}

			double d_delta_t;
			double d_yellow_threshold;
			double d_red_threshold;
			GPlatesQtWidgets::KinematicGraphsConfigurationWidget::VelocityMethod d_velocity_method;
		};

		enum KinematicGraphType{
			LATITUDE_GRAPH_TYPE,
			LONGITUDE_GRAPH_TYPE,
			VELOCITY_MAG_GRAPH_TYPE,
			VELOCITY_AZIMUTH_GRAPH_TYPE,
			VELOCITY_COLAT_GRAPH_TYPE,
			VELOCITY_LON_GRAPH_TYPE,
			ANGULAR_VELOCITY_GRAPH_TYPE,

			NUM_GRAPH_TYPES,
			// Temp re-ordering to disable rotation rate.
			ROTATION_RATE_GRAPH_TYPE,
		};



		enum KinematicTableColumns{
			TIME_COLUMN,
			LAT_COLUMN,
			LON_COLUMN,
			VELOCITY_MAG_COLUMN,
			VELOCITY_AZIMUTH_COLUMN,
			VELOCITY_COLAT_COLUMN,
			VELOCITY_LON_COLUMN,
			ANGULAR_VELOCITY_COLUMN,
			NUM_COLUMNS,
			// Temp re-ordering to disable rotation rate.
			ROTATION_RATE_COLUMN,
		};




		struct table_entries{
			double d_time;
			double d_lat;
			double d_lon;
			double d_velocity_mag;
			double d_velocity_azimuth;
			double d_velocity_colat;
			double d_velocity_lon;
			double d_angular_velocity;
		};

		typedef std::vector<table_entries> results_type;
		typedef results_type::const_iterator results_type_const_iterator;


		KinematicGraphsDialog(
				GPlatesPresentation::ViewState &view_state,
				QWidget *parent_ = NULL);

	private Q_SLOTS:

		void
		handle_close();

		/**
		 * @brief handle_update
		 *
		 * Calculate new values for the table, and update the graph as appropriate.
		 */
		void
		handle_update();

		void
		handle_settings_clicked();

		void
		handle_export_table();

		void
		handle_export_graph();

		void
		handle_graph_type_radio_toggled(
				bool state);

		/**
		 * Handle the "use last-selected feature" button being clicked.
		 *
		 * This will set the lat-lon to that of the last-selected point feature.
		 * If it wasn't a point feature we could use the first point of other feature types.
		 *
		 * We also set the plate-id to that of the last-selected feature.
		 */
		void
		handle_use_feature();

		/**
		 * @brief handle_use_animation - handle the "use animation values" being clicked.
		 *
		 * This sets the start, end, and step times to those of the animation control dialog.
		 *
		 * We can also set the relative plate-id to that of the anchor plate. Should we do
		 * this here or use a separate button?
		 */
		void
		handle_use_animation();

		/**
		 * @brief handle_auto_y_clicked - handle the "autoscale y axis" button.
		 */
		void
		handle_auto_y_clicked();

		/**
		 * @brief handle_compress_y_clicked - handle the "compress y axis" button
		 *
		 */
		void
		handle_compress_y_clicked();

		/**
		 * @brief handle_stretch_y_clicked - handle the "stretch y axis" button.
		 */
		void
		handle_stretch_y_clicked();

		/**
		 * @brief handle_flip_horizontal_axis -handle the "flip horizontal axis" button.
		 */
		void
		handle_flip_horizontal_axis();


	private:

		void
		update_values_from_widgets();

		void
		update_table();

		void
		update_graph();

		void
		initialise_widgets();

		void
		set_up_connections();

		void
		set_graph_axes_and_titles();

		void
		set_up_axes_ranges();

		void
		set_up_plot();

		void
		check_and_highlight_bad_velocity_values();

		void
		read_values_from_preferences();

		/**
		 * @brief m_plot
		 *
		 * This widget is given the KinematicGraphsDialog as parent in the initialiser, so should be memory-managed by Qt.
		 */
		QwtPlot *d_plot;

		/**
		 * @brief m_plot_curve
		 *
		 */
		QwtPlotCurve *d_plot_curve;

		/**
		 * @brief m_point_series_data
		 *
		 */
		QwtPointSeriesData *d_point_series_data;

		/**
		 * @brief d_samples
		 */
		QVector<QPointF> d_samples;

		/**
		 * @brief d_picker - A QwtPicker (http://qwt.sourceforge.net/class_qwt_picker.html#details)
		 * is used to select data from a Qwt widget. Here we use it to display the plot coordinates
		 * as we mouse over the plot.
		 */
		GPlatesQtWidgets::KinematicGraphPicker *d_picker;

		/**
		 * Graph titles,units,ranges etc.
		 */
		QString d_title;
		QString d_x_axis_title;
		QString d_y_axis_title;
		QString d_x_axis_unit;
		QString d_y_axis_unit;
		double d_x_min;
		double d_x_max;
		double d_y_min;
		double d_y_max;

		/**
		 * @brief d_vertical_scale_factor
		 *
		 * For stretching/compressing the y-axis.
		 * The minimum and maximum values of the y-axis are multipled by
		 * VERTICAL_SCALE_MULTIPLER**d_vertical_scale_factor before being
		 * applied to the Qwt plot. So to compress the axis (and make "tall" graphs looks smaller so that
		 * we can see a larger range of data) we want to increase this factor; to stretch the axis (to zoom in to to see
		 * lower values better) we want to decrease this factor. The max and min allowed values are defined
		 * in body as MAX_VERTICAL_SCALE_FACTOR and MIN_VERTICAL_SCALE_FACTOR.
		 */
		unsigned int d_vertical_scale_power;
		unsigned int d_vertical_scale_powers[NUM_GRAPH_TYPES];
		double d_vertical_scale_maxes[NUM_GRAPH_TYPES];
		double d_vertical_scale_mins[NUM_GRAPH_TYPES];


		/**
		 * User-specified variables required for velocity calculations
		 */
		GPlatesModel::integer_plate_id_type d_moving_id;
		GPlatesModel::integer_plate_id_type d_anchor_id;
		double d_begin_time;
		double d_end_time;
		double d_step_time;
		double d_lat;
		double d_lon;

		/**
		 * The type of graph (e.g. velocity vs time, latitude vs time...)
		 */
		KinematicGraphType d_graph_type;

		/**
		 * App state, for getting reconstruction features, preferences etc.
		 */
		GPlatesAppLogic::ApplicationState &d_application_state;

		/**
		 * @brief d_view_state, for getting animation control values etc.
		 */
		GPlatesPresentation::ViewState &d_view_state;

		/**
		 * The focussed feature - for pre-filling the
		 * lat/lon/plate-id etc. fields from the focussed feature.
		 */
		const GPlatesGui::FeatureFocus &d_feature_focus;

		/**
		 * Data for the table-view.
		 *
		 * This dialog is set as the parent in the initialiser, and so should be memory-managed by Qt.
		 */
		QStandardItemModel *d_model;

		/**
		 * @brief d_results - instance of a structure to hold the results of the kinematical calculations.
		 */
		results_type d_results;

		/**
		 * @brief d_save_file_dialog - for exporting table.
		 */
		SaveFileDialog d_save_file_dialog;

		/**
		 * @brief d_spin_box_palette - the palette used in begin/end spinboxes. Stored so that we can
		 * restore the original palette after changing to a warning palette.
		 */
		QPalette d_spin_box_palette;

		/**
		 * @brief d_settings_dialog - Dialog for letting user change details relating to velocity calculations.
		 */
		KinematicGraphsConfigurationDialog *d_settings_dialog;

		/**
		 * @brief d_configuration - instance of structure holding configuration for the velocity
		 * calculations - (e.g. time-step)
		 */
		Configuration d_configuration;

	};
}

#endif  // GPLATES_QTWIDGETS_KINEMATICGRAPHSDIALOG_H
