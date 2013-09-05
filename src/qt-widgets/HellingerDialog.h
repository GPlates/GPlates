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


	enum EditOperationType{
		EDIT_POINT_OPERATION,
		EDIT_SEGMENT_OPERATION
	};

	class HellingerDialog:
			public GPlatesDialog,
			protected Ui_HellingerDialog
	{
		Q_OBJECT
	public:

		typedef std::map<int,bool> expanded_status_map_type;
		typedef std::vector<hellinger_model_type::const_iterator > geometry_to_model_map_type;
		typedef std::vector<QTreeWidgetItem*> geometry_to_tree_item_map_type;

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
				const unsigned int segment_number);

		GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type
		get_pick_layer();

		void
		set_hovered_pick(
				const unsigned int index);

		void
		set_selected_pick(
				const unsigned int index);

		void
		set_selected_pick(
				const hellinger_model_type::const_iterator &it);

		void
		clear_hovered_layer();

		void
		clear_selection_layer();

		void
		clear_editing_layer();

		void
		edit_current_pick();


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

		void
		update_buttons();


		void
		load_pick_file_to_model(
				const QString &filename,
				const GPlatesModel::integer_plate_id_type &moving_plate_id,
				const GPlatesModel::integer_plate_id_type &fixed_plate_id,
				const double &chron_time);

		void
		highlight_selected_pick(
				const HellingerPick& pick);

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
		draw_picks();

		const GPlatesGui::Colour &get_segment_colour(
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

		void
		update_edit_layer(
				const EditOperationType &type);

		void
		update_selected_geometries();

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

		void
		update_hovered_item(
				boost::optional<QTreeWidgetItem*> item = boost::none,
				bool current_state = true);


		GPlatesPresentation::ViewState &d_view_state;

		/// For creating child layers
		GPlatesViewOperations::RenderedGeometryCollection *d_rendered_geom_collection_ptr;

		//! For drawing picks
		child_layer_ptr_type d_pick_layer_ptr;

		//! For highlighting picks which are hovered over.
		child_layer_ptr_type d_hover_layer_ptr;

		//! For selected pick / segment
		child_layer_ptr_type d_selection_layer_ptr;

		//! For fitted pole, uncertainty, and rotated picks
		child_layer_ptr_type d_result_layer_ptr;

		//! For geometries being edited
		child_layer_ptr_type d_editing_layer_ptr;

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

		ThreadType d_thread_type;
		QString d_python_path;
		QString d_python_file;
		QString d_temporary_path;

		expanded_status_map_type d_segment_expanded_status;
		geometry_to_model_map_type d_geometry_to_model_map;
		geometry_to_tree_item_map_type d_geometry_to_tree_item_map;

		boost::optional<QTreeWidgetItem*> d_hovered_item;
		bool d_hovered_item_original_state;

		boost::optional<unsigned int> d_selected_segment;
		boost::optional<hellinger_model_type::const_iterator> d_selected_pick;

	};
}

#endif //GPLATES_QTWIDGETS_HELLINGERDIALOG_H
