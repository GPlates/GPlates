/* $Id: HellingerDialog.h 254 2012-03-01 12:00:21Z robin.watson@ngu.no $ */

/**
 * \file
 * $Revision: 254 $
 * $Date: 2012-03-01 13:00:21 +0100 (Thu, 01 Mar 2012) $
 *
 * Copyright (C) 2011, 2012, 2013 Geological Survey of Norway
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
#include <QItemSelection>
#include <QWidget>

#include "gui/Colour.h"
#include "gui/Symbol.h"
#include "model/types.h"
#include "view-operations/RenderedGeometryCollection.h"
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
	class HellingerModel;
	class ReadErrorAccumulationDialog;


	class HellingerDialog:
			public GPlatesDialog,
			protected Ui_HellingerDialog
	{
		Q_OBJECT
	public:

		typedef std::map<int,bool> expanded_status_map_type;

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
		 * Update whole dialog from model, and then update the canvas
		 */
		void
		update_from_model();

		/**
		 * @brief update_tree_from_model
		 * Update pick-widget from model, and then update the canvas
		 */
		void
		update_tree_from_model();

		void
		restore_expanded_status();

		void
		expand_segment(
				const int segment_number);


	public Q_SLOTS:

		/**
		 * Renumber segments so that they are contiguous.
		 */
		void
		renumber_segments();

		void
		store_expanded_status();

	private:

		void
		update_model_with_com_data();

		void
		set_up_connections();

		void
		set_up_child_layers();

		void
		activate_layers(
				bool activate);

		void
		clear_rendered_geometries();

		void
		add_point(
				const QString &add_value);

		void
		add_segment(
				const QStringList &add_list);

		// FIXME: what's happening here?? Where is the function body??
		void
		change_point(
				const QString &point_value);



		void
		update_buttons();


		void
		load_pick_file_to_model(
				const QString &filename,
				const GPlatesModel::integer_plate_id_type &moving_plate_id,
				const GPlatesModel::integer_plate_id_type &fixed_plate_id,
				const double &chron_time);

		void
		highlight_selected_point(
				const double &lat,
				const double &lon,
				const int &type_segment,
				bool enabled);

		void
		highlight_selected_segment(
				const int &segment_number);

		void
		draw_pole(
				const double &lat,
				const double &lon);

		// TODO: This appears to update the fit from the model, and also to draw the fit point on the
		// canvas. Check this, and perhaps separate out the functionality.
		void
		update_result();

		void
		draw_error_ellipse();

		void
		update_initial_guess();

		/**
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
		set_segment_colours(
				int num_color);



	private Q_SLOTS:

		void
		handle_clear();

		void
		handle_thread_finished();

		void
		handle_calculate_fit();

		void
		import_hellinger_file();

		void
		show_stat_details();


		void
		handle_add_new_pick();

		void
		handle_export_pick_file();

		void
		handle_export_com_file();

		void
		handle_expand_all();

		void
		handle_collapse_all();

		void
		handle_edit_pick();

		void
		handle_remove_pick();

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
		handle_close();

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

		void
		handle_cancel();

	private:

		//! Convenience typedef for GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type
		typedef GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type child_layer_ptr_type;

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

		bool
		picks_loaded();

		void
		set_buttons_for_no_selection();

		void
		set_buttons_for_segment_selected();

		void
		set_buttons_for_pick_selected(
				bool state_is_active);


		GPlatesPresentation::ViewState &d_view_state;

		/// For creating child layers
		GPlatesViewOperations::RenderedGeometryCollection *d_rendered_geom_collection_ptr;

		//! For drawing picks
		child_layer_ptr_type d_pick_layer_ptr;

		//! For highlights
		child_layer_ptr_type d_highlight_layer_ptr;

		//! For rotated picks
		child_layer_ptr_type d_rotated_layer_ptr;

		ReadErrorAccumulationDialog &d_read_error_accumulation_dialog;
		HellingerModel *d_hellinger_model;
		HellingerStatsDialog *d_hellinger_stats_dialog;
		HellingerThread *d_hellinger_thread;
		QString d_path;
		QString d_file_name;
		QString d_filename_dat;
		QString d_filename_up;
		QString d_filename_down;
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

		expanded_status_map_type d_segment_expanded_status;


	};
}

#endif //GPLATES_QTWIDGETS_HELLINGERDIALOG_H
