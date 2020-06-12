/* $Id: HellingerDialog.h 254 2012-03-01 12:00:21Z robin.watson@ngu.no $ */

/**
 * \file
 * $Revision: 254 $
 * $Date: 2012-03-01 13:00:21 +0100 (Thu, 01 Mar 2012) $
 *
 * Copyright (C) 2011, 2012, 2013, 2014, 2016 Geological Survey of Norway
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

#ifndef GPLATES_QTWIDGETS_HELLINGERDIALOG_H
#define GPLATES_QTWIDGETS_HELLINGERDIALOG_H

#include <vector>

#include "gui/Colour.h"
#include "gui/Symbol.h"
#include "model/types.h"
#include "view-operations/RenderedGeometryCollection.h"
#include "GPlatesDialog.h"
#include "HellingerConfigurationWidget.h"
#include "HellingerModel.h"
#include "ui_HellingerDialogUi.h"
#include "HellingerThread.h"
#include "OpenDirectoryDialog.h"



namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesViewOperations
{
	class RenderedGeometryLayer;
}

namespace GPlatesQtWidgets
{

	class HellingerConfigurationDialog;
	class HellingerPointDialog;
	class HellingerSegmentDialog;
	class HellingerFitWidget;
	class HellingerPickWidget;
	class HellingerStatsDialog;
	class HellingerModel;
	class HellingerThread;
	class ReadErrorAccumulationDialog;


	enum CanvasOperationType{
		SELECT_OPERATION,
		EDIT_POINT_OPERATION,
		NEW_POINT_OPERATION,
		EDIT_SEGMENT_OPERATION,
		NEW_SEGMENT_OPERATION
	};

	const HellingerConfigurationWidget::HellingerColour INITIAL_BEST_FIT_POLE_COLOUR =
			HellingerConfigurationWidget::RED;
	const float INITIAL_POLE_SIZE = 1.;
	const HellingerConfigurationWidget::HellingerColour INITIAL_ELLIPSE_COLOUR =
			HellingerConfigurationWidget::RED;
	const int INITIAL_ELLIPSE_THICKNESS = 2;
	const float INITIAL_POLE_ARROW_HEIGHT = 0.3f;
	const float INITIAL_POLE_ARROW_RADIUS = 0.12f;
	const GPlatesGui::Symbol::SymbolType INITIAL_PLATE_ONE_PICK_SYMBOL = GPlatesGui::Symbol::CROSS;
	const GPlatesGui::Symbol::SymbolType INITIAL_PLATE_TWO_PICK_SYMBOL = GPlatesGui::Symbol::SQUARE;
	const GPlatesGui::Symbol::SymbolType INITIAL_PLATE_THREE_PICK_SYMBOL = GPlatesGui::Symbol::TRIANGLE;
	const int INITIAL_PLATE_ONE_PICK_SYMBOL_SIZE = 2;
	const int INITIAL_PLATE_TWO_PICK_SYMBOL_SIZE = 2;
	const int INITIAL_PLATE_THREE_PICK_SYMBOL_SIZE = 2;

	// Initial colours - control this from the settings dialog eventually.
	const HellingerConfigurationWidget::HellingerColour POLE_12_COLOUR = HellingerConfigurationWidget::RED;
	const HellingerConfigurationWidget::HellingerColour POLE_13_COLOUR = HellingerConfigurationWidget::YELLOW;
	const HellingerConfigurationWidget::HellingerColour POLE_23_COLOUR = HellingerConfigurationWidget::GREEN;

	const HellingerConfigurationWidget::HellingerColour POLE_ESTIMATE_12_COLOUR = HellingerConfigurationWidget::AQUA;
	const HellingerConfigurationWidget::HellingerColour POLE_ESTIMATE_13_COLOUR = HellingerConfigurationWidget::TEAL;



	class HellingerDialog:
			public GPlatesDialog,
			protected Ui_HellingerDialog
	{
		Q_OBJECT
	public:



		struct Configuration
		{
			Configuration():
//				d_best_fit_pole_symbol(INITIAL_BEST_FIT_POLE_SYMBOL),
				d_best_fit_pole_colour(INITIAL_BEST_FIT_POLE_COLOUR),
				d_best_fit_pole_size(INITIAL_POLE_SIZE),
				d_ellipse_colour(INITIAL_ELLIPSE_COLOUR),
				d_ellipse_line_thickness(INITIAL_ELLIPSE_THICKNESS),
				d_initial_estimate_pole_colour(POLE_ESTIMATE_12_COLOUR),
				d_pole_arrow_height(INITIAL_POLE_ARROW_HEIGHT),
				d_pole_arrow_radius(INITIAL_POLE_ARROW_RADIUS),
				d_plate_one_pick_symbol(INITIAL_PLATE_ONE_PICK_SYMBOL),
				d_plate_two_pick_symbol(INITIAL_PLATE_TWO_PICK_SYMBOL),
				d_plate_three_pick_symbol(INITIAL_PLATE_THREE_PICK_SYMBOL),
				d_plate_one_pick_symbol_size(INITIAL_PLATE_ONE_PICK_SYMBOL_SIZE),
				d_plate_two_pick_symbol_size(INITIAL_PLATE_TWO_PICK_SYMBOL_SIZE),
				d_plate_three_pick_symbol_size(INITIAL_PLATE_THREE_PICK_SYMBOL_SIZE)
			{}

//			SYMBOL_TYPE d_best_fit_pole_symbol;
			GPlatesQtWidgets::HellingerConfigurationWidget::HellingerColour d_best_fit_pole_colour;
			float d_best_fit_pole_size;
			GPlatesQtWidgets::HellingerConfigurationWidget::HellingerColour d_ellipse_colour;
			int d_ellipse_line_thickness;
			GPlatesQtWidgets::HellingerConfigurationWidget::HellingerColour d_initial_estimate_pole_colour;
			float d_pole_arrow_height;
			float d_pole_arrow_radius;
			GPlatesGui::Symbol::SymbolType d_plate_one_pick_symbol;
			GPlatesGui::Symbol::SymbolType d_plate_two_pick_symbol;
			GPlatesGui::Symbol::SymbolType d_plate_three_pick_symbol;
			int d_plate_one_pick_symbol_size;
			int d_plate_two_pick_symbol_size;
			int d_plate_three_pick_symbol_size;


		};

		typedef std::map<int,bool> expanded_status_map_type;
		typedef std::vector<hellinger_model_type::const_iterator > geometry_to_model_map_type;

		HellingerDialog(
				GPlatesPresentation::ViewState &view_state,
				GPlatesQtWidgets::ReadErrorAccumulationDialog &read_error_dialog,
				QWidget *parent_ = NULL);

		/**
		 * Set the pick layer active, and draw the model contents on the canvas
		 */
		void
		restore();

		/**
		 * Update whole dialog from model, and then update the canvas
		 */
		void
		update_widgets_from_model();

		GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type
		get_pick_layer();

		GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type
		get_editing_layer();

		GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type
		get_feature_highlight_layer();

		GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type
		get_pole_estimate_layer();

		void
		set_hovered_pick(
				const unsigned int index);

		void
		set_selected_pick(
				const unsigned int index);

		void
		clear_hovered_layer_and_table();

		void
		clear_selection_layer();

		void
		clear_editing_layer();

		void
		clear_feature_highlight_layer();

		void
		edit_current_pick();

		void
		update_edit_layer(
				const GPlatesMaths::PointOnSphere &pos);

		void
		set_enlarged_edit_geometry(
				bool enlarged = true);

		bool
		is_in_edit_point_state()
		{
			return (d_canvas_operation_type == EDIT_POINT_OPERATION);
		}

		bool
		is_in_new_point_state()
		{
			return (d_canvas_operation_type == NEW_POINT_OPERATION);
		}

		bool
		is_in_edit_segment_state()
		{
			return (d_canvas_operation_type == EDIT_SEGMENT_OPERATION);
		}

		bool
		is_in_new_segment_state()
		{
			return (d_canvas_operation_type == NEW_SEGMENT_OPERATION);
		}

		void
		set_feature_highlight(
				const GPlatesMaths::PointOnSphere &pos);

		void
		update_after_new_or_edited_pick(
				const hellinger_model_type::const_iterator &it,
				const int segment_number);

		void
		update_after_new_or_edited_segment(
				const int segment_number);

		void
		enable_pole_estimate_widgets(
				bool enable);

		void
		set_layer_state_for_active_pole_tool(
				bool pole_tool_is_active);

		const GPlatesMaths::LatLonPoint &
		get_pole_estimate_12_lat_lon();

		const double &
		get_pole_estimate_12_angle();

		const GPlatesMaths::LatLonPoint &
		get_pole_estimate_13_lat_lon();

		const double &
		get_pole_estimate_13_angle();

		void
		update_pole_estimates(
				const GPlatesMaths::PointOnSphere &point_12,
				double &angle_12,
				const GPlatesMaths::PointOnSphere &point_13,
				double &angle_13);

		bool
		adjust_pole_tool_is_active()
		{
			return d_adjust_pole_tool_is_active;
		}


		void
		set_adjust_pole_tool_is_active(
				bool active)
		{
			d_adjust_pole_tool_is_active = active;
		}

		void
		set_state_for_pole_adjustment_tool(
				bool pole_adjustment_tool_is_active);

		QString
		output_file_path() const
		{
			return d_output_file_path;
		}

		bool
		output_file_path_is_valid() const
		{
			return d_output_path_is_valid;
		}

		const HellingerFitType &
		get_fit_type();

		const Configuration &
		configuration()
		{
			return d_configuration;
		}

	public Q_SLOTS:


		void
		close();

		/**
		 * @brief hide - override the QDialog method so that we can hide child dialogs too.
		 */
		void
		hide();

		void
		keyPressEvent(QKeyEvent *event);

	Q_SIGNALS:

		void
		begin_edit_pick();

		void
		begin_new_pick();

		void
		finished_editing();

		void
		pole_estimate_12_lat_lon_changed(
				double,
				double);

		void
		pole_estimate_12_angle_changed(
				double);

		void
		pole_estimate_13_lat_lon_changed(
				double,
				double);

		void
		pole_estimate_13_angle_changed(
				double);

	private:

		void
		check_and_highlight_output_path();

		void
		set_up_connections();

		void
		set_up_child_layers();

		void
		activate_layers(
				bool activate = true);

		void
		clear_rendered_geometries();

		void
		clear_pick_geometries();

		void
		highlight_selected_pick(
				const HellingerPick& pick);

		void
		highlight_selected_segment(
				const int &segment_number);

		void
		draw_pole_result(
				const double &lat,
				const double &lon,
				const HellingerConfigurationWidget::HellingerColour &colour);

		void
		update_results_on_canvas();

		void
		update_estimates_on_canvas();

		void
		draw_error_ellipse(
				const GPlatesQtWidgets::HellingerPlatePairType &type = GPlatesQtWidgets::PLATES_1_2_PAIR_TYPE);


		/**
		 * Import the currently loaded hellinger pick data into the
		 * main gplates model.
		 */
		void
		create_feature_collection();

		void
		draw_picks_of_plate_index(
				const HellingerPlateIndex &fixed_plate_index);

		void
		draw_picks();

		void
		draw_pole_estimate(
				const HellingerPoleEstimate &estimate,
				const HellingerConfigurationWidget::HellingerColour &colour);

		void
		hide_child_dialogs();

		/**
		 * @brief update_chron_time - check the chron string against the active age model, and convert
		 * it to an age if appropriate.
		 */
		void
		update_chron_time();

		void
		update_model_from_file_related_data();

		void
		enable_pole_estimate_signals(
				bool enable);


	private Q_SLOTS:

		void
		handle_show_estimate_checkboxes_clicked();

		void
		handle_show_result_checkboxes_clicked();

		void
		handle_thread_finished();

		void
		handle_calculate_fit();

		void
		handle_import_hellinger_file();

		void
		handle_show_details();


		void
		handle_add_new_pick();

		void
		handle_export_pick_file();

		void
		handle_export_com_file();

		void
		handle_edit_pick();

		void
		handle_add_new_segment();

		void
		handle_edit_segment();

		void
		handle_calculate_uncertainties();

		void
		handle_close();

		void
		handle_pole_estimate_12_changed(double, double);

		void
		handle_pole_estimate_12_angle_changed(double);

		void
		handle_pole_estimate_13_changed(double, double);

		void
		handle_pole_estimate_13_angle_changed(double);

		void
		handle_chron_time_changed(
				const double &time);

		void
		handle_recon_time_spinbox_changed(
				const double &time);

		void
		handle_recon_time_slider_changed(
				const int &value);

		void
		handle_pick_dialog_updated();

		void
		handle_cancel();

		void
		handle_finished_editing();

		void
		handle_update_point_editing();

		void
		handle_update_segment_editing();

		void
		handle_active_age_model_changed();

		void
		handle_settings_clicked();

		void
		handle_configuration_changed();

		void
		handle_tab_changed(int);

		void
		handle_output_path_button_clicked();

		void
		handle_output_path_changed();

		void
		handle_output_path_editing_finished();

	private:

		//! Convenience typedef for GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type
		typedef GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type child_layer_ptr_type;

		/**
		 * @brief initialise_widgets - set-up initial state of widgets.
		 */
		void
		initialise_widgets();

		/**
		 * Draw the model contents on the globe/map.
		 */
		void
		update_canvas();

		void
		update_selected_geometries();

		/**
		 * Reconstruct the moving picks
		 */
		void
		reconstruct_picks();

		GPlatesPresentation::ViewState &d_view_state;

		/// For creating child layers
		GPlatesViewOperations::RenderedGeometryCollection *d_rendered_geom_collection_ptr;

		//! For drawing picks
		child_layer_ptr_type d_pick_layer_ptr;

		//! For highlighting picks which are hovered over.
		child_layer_ptr_type d_hover_layer_ptr;

		//! For selected pick / segment
		child_layer_ptr_type d_selection_layer_ptr;

		//! For fitted pole, uncertainty
		child_layer_ptr_type d_result_layer_ptr;

		//! For geometries being edited
		child_layer_ptr_type d_editing_layer_ptr;

		//! For highlighting feature geometries which can be selected
		child_layer_ptr_type d_feature_highlight_layer_ptr;

		//! For displaying the pole estimate
		child_layer_ptr_type d_pole_estimate_layer_ptr;

		ReadErrorAccumulationDialog &d_read_error_accumulation_dialog;
		HellingerModel d_hellinger_model;
		HellingerStatsDialog *d_hellinger_stats_dialog;
		HellingerPointDialog *d_hellinger_edit_point_dialog;
		HellingerPointDialog *d_hellinger_new_point_dialog;
		HellingerSegmentDialog *d_hellinger_edit_segment_dialog;
		HellingerSegmentDialog *d_hellinger_new_segment_dialog;
		HellingerThread *d_hellinger_thread;

		/**
		 * @brief For storing moving and fixed plate IDs for later insertion of rotation pole into model.
		 * Not currently used.
		 */
		GPlatesModel::integer_plate_id_type d_plate_1_id;
		GPlatesModel::integer_plate_id_type d_plate_2_id;
		GPlatesModel::integer_plate_id_type d_plate_3_id;

		double d_recon_time;
		double d_chron_time;

		/**
		 * @brief  symbols for depicting moving and fixed picks.
		 */
		GPlatesGui::Symbol d_moving_symbol;
		GPlatesGui::Symbol d_fixed_symbol;

		/**
		 * @brief d_thread_type - enum describing the thread type - i.e .pole calculation thread or stats calculation thread.
		 */
		ThreadType d_thread_type;

		/**
		 * @brief d_python_path - At present we need to pass the hellinger python file to boost::python::exec_file. This
		 * stores the path to the hellinger python file.
		 */
		QString d_python_path;

		/**
		 * @brief d_python_file - the full filename (including path) of the hellinger python file.
		 */
		QString d_python_file;

		/**
		 * @brief d_path_for_temporary_files - location for storing temporary files used for passing data between the python scripts and GPlates.
		 */
		QString d_path_for_temporary_files;

		QString d_output_file_path;

		geometry_to_model_map_type d_geometry_to_model_map;

		bool d_edit_point_is_enlarged;

		CanvasOperationType d_canvas_operation_type;

		GPlatesMaths::LatLonPoint d_pole_estimate_12_llp;
		GPlatesMaths::LatLonPoint d_pole_estimate_13_llp;
		double d_pole_estimate_12_angle;
		double d_pole_estimate_13_angle;

		/**
		 * @brief d_spin_box_palette - the palette used in begin/end spinboxes. Stored so that we can
		 * restore the original palette after changing to a warning palette.
		 */
		QPalette d_spin_box_palette;

		/**
		 * @brief d_settings_dialog - Dialog for changing configuration
		 */
		HellingerConfigurationDialog *d_configuration_dialog;

		/**
		 * @brief d_configuration - instance of structure holding settings for rendering of
		 * certain hellinger-related geometries
		 */
		Configuration d_configuration;

		HellingerPickWidget *d_pick_widget;
		HellingerFitWidget *d_fit_widget;

		bool d_adjust_pole_tool_is_active;

		OpenDirectoryDialog d_open_directory_dialog;

		bool d_output_path_is_valid;

		bool d_three_way_fitting_is_enabled;

	};
}

#endif //GPLATES_QTWIDGETS_HELLINGERDIALOG_H
