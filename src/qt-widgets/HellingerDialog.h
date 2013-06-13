/* $Id: HellingerDialog.h 254 2012-03-01 12:00:21Z robin.watson@ngu.no $ */

/**
 * \file
 * $Revision: 254 $
 * $Date: 2012-03-01 13:00:21 +0100 (Thu, 01 Mar 2012) $
 *
 * Copyright (C) 2011, 2012 Geological Survey of Norway
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

#include <QDialog>
#include <QWidget>

#include "gui/Colour.h"
#include "gui/Symbol.h"
#include "model/types.h"
#include "GPlatesDialog.h"
#include "HellingerModel.h"
#include "HellingerDialogUi.h"
#include "HellingerThread.h"

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

	class HellingerStatsDialog;
	class HellingerNewPoint;
	class HellingerEditPoint;
	class HellingerNewSegment;
	class HellingerEditSegment;
	class HellingerRemoveError;
	class HellingerModel;
	class ReadErrorAccumulationDialog;


	class HellingerDialog:
			public GPlatesDialog,
			protected Ui_HellingerDialog
	{
		Q_OBJECT
	public:


		HellingerDialog(
				GPlatesPresentation::ViewState &view_state,
				GPlatesQtWidgets::ReadErrorAccumulationDialog &read_error_dialog,
				QWidget *parent_ = NULL);

		/**
				 * Set initial values for dialog.
				 */
		void
		initialise();

		/**
				 * Update dialog from hellinger-model.
				 */
		void
		update();

	public Q_SLOTS:

		/**
				 * Renumber segments so that they are contiguous.
				 */
		void
		renumber_segments();

	private:
		void
		set_up_connections();

		void
		add_point(
				const QString &add_value);

		void
		add_segment(
				const QStringList &add_list);

		void
		change_point(
				const QString &point_value);

		void
		load_data_from_model();

		void
		update_buttons();

		void
		update_buttons_statistics(bool info);

		void
		update_continue_button(bool info);


		void
		load_pick_file_to_model(
				const QString &filename,
				const GPlatesModel::integer_plate_id_type &moving_plate_id,
				const GPlatesModel::integer_plate_id_type &fixed_plate_id,
				const double &chron_time);

		void
		show_point_globe(
				const double &lat,
				const double &lon,
				const int &type_segment);

		void
		show_results(
				const double &lat,
				const double &lon);

		void
		results_python();

		void
		show_data_points();

		void
		reset_picks_globe();



		void
		update_initial_guess();

		/*
				 * Import the currently loaded hellinger pick data into the
				 * main gplates model.
				 */
		void
		create_feature_collection();

		void
		draw_fixed_picks();

		void
		draw_moving_picks();

		void
		set_segment_colour(
				int &num_color);

		void
		reorder_picks();

		void
		reset_expanded_status();

	private Q_SLOTS:

		void
		handle_thread_finished();

		void
		update_expanded_status();

		void
		handle_calculate();

		void
		import_pick_file();

		void
		show_stat_details();


		void
		handle_add_new_point();

		void
		handle_export_file();

		void
		handle_expand_all();

		void
		handle_collapse_all();

		void
		handle_edit_point();

		void
		handle_remove_point();

		void
		handle_remove_segment();

		void
		handle_add_new_segment();

		void
		handle_edit_segment();

		void
		handle_calculate_stats();

		void
		handle_pick_state_changed();

		void
		close_dialog();

		void
		handle_checkbox_grid_search_changed();

		void
		handle_spinbox_radius_changed();

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
		handle_fit_spinboxes_changed();

		void
		handle_selection_changed(
				const QItemSelection &,
				const QItemSelection &);

	private:

		/**
				 * Draw the model contents on the globe/map.
				 */
		void
		update_canvas();

		/**
				 * Reconstruct the moving picks
				 */
		void
		reconstruct_picks();




		GPlatesPresentation::ViewState &d_view_state;
		GPlatesViewOperations::RenderedGeometryLayer &d_hellinger_layer;
		ReadErrorAccumulationDialog &d_read_error_accumulation_dialog;
		HellingerModel *d_hellinger_model;
		HellingerStatsDialog *d_hellinger_stats_dialog;
		HellingerNewPoint *d_hellinger_new_point;
		HellingerEditPoint *d_hellinger_edit_point;
		HellingerNewSegment *d_hellinger_new_segment;
		HellingerEditSegment *d_hellinger_edit_segment;
		HellingerRemoveError *d_hellinger_remove_error;
		HellingerThread *d_hellinger_thread;
		QString d_path;
		QString d_file_name;
		QString d_filename_dat;
		QString d_filename_up;
		QString d_filename_do;
		GPlatesModel::integer_plate_id_type d_moving_plate_id;
		GPlatesModel::integer_plate_id_type d_fixed_plate_id;
		double d_recon_time;
		double d_chron_time;
		GPlatesGui::Symbol d_moving_symbol;
		GPlatesGui::Symbol d_fixed_symbol;
		GPlatesGui::Colour d_segment_colour;

		ThreadType d_thread_type;
		QString d_python_path;
		QString d_python_file;
		QString d_temporary_path;
		QString d_temp_pick_file;
		QString d_temp_result;
		QString d_temp_par;
		QString d_temp_res;
		std::vector<QString> d_expanded_segments;


	};
}

#endif //GPLATES_QTWIDGETS_HELLINGERDIALOG_H
