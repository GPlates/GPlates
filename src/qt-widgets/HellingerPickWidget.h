/* $Id$ */

/**
 * \file
 * $Revision$
 * $Date$
 *
 * Copyright (C) 2015, 2016 Geological Survey of Norway
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

#ifndef GPLATES_QTWIDGETS_HELLINGERPICKWIDGET_H
#define GPLATES_QTWIDGETS_HELLINGERPICKWIDGET_H

#include "boost/optional.hpp"


#include "HellingerModel.h"
#include "HellingerPickWidgetUi.h"


namespace GPlatesQtWidgets
{

	class HellingerDialog;
	class HellingerModel;

	class HellingerPickWidget:
			public QWidget,
			protected Ui_HellingerPickWidget
	{
		Q_OBJECT

		typedef std::map<int,bool> expanded_status_map_type;


	public:

		typedef std::vector<QTreeWidgetItem*> tree_items_collection_type;

		HellingerPickWidget(
				HellingerDialog *hellinger_dialog,
				HellingerModel *hellinger_model);


		virtual
		~HellingerPickWidget()
		{  }

		void
		update_after_switching_tabs();

		void
		update_from_model(
				bool expand_tree_after_update = false);

		void
		update_buttons();

		boost::optional<unsigned int>
		segment_number_of_selected_pick();

		boost::optional<unsigned int>
		selected_segment();

		boost::optional<unsigned int>
		selected_row();

		boost::optional<hellinger_model_type::const_iterator>
		selected_pick();

		tree_items_collection_type
		tree_items() const;

		void
		restore();

		void
		handle_close();

		bool
		picks_loaded();

		void
		update_hovered_item(
				const unsigned int geometry_index,
				bool is_enabled);

		void
		set_selected_pick_from_geometry_index(
				const unsigned int index);

		void
		clear_hovered_item();

		void
		renumber_segments();

		void
		update_after_new_or_edited_pick(
				const hellinger_model_type::const_iterator &it,
				const int segment_number);

		void
		update_after_new_or_edited_segment(
				const int segment_number);

		void
		store_scrollbar_status();

		void
		restore_scrollbar_status();


	Q_SIGNALS:

		void
		edit_pick_signal();

		void
		add_new_pick_signal();

		void
		add_new_segment_signal();

		void
		edit_segment_signal();

		void
		tree_updated_signal();

	private Q_SLOTS:

		void
		handle_expand_all();

		void
		handle_collapse_all();

		void
		handle_edit_pick();

		void
		handle_add_new_pick();

		void
		handle_remove_pick();

		void
		handle_remove_segment();

		void
		handle_add_new_segment();

		void
		handle_edit_segment();

		void
		handle_selection_changed(
				const QItemSelection &,
				const QItemSelection &);

		void
		handle_pick_state_changed();

		void
		handle_clear();

		void
		handle_renumber_segments();

		void
		store_expanded_status();

		void
		restore_expanded_status();

	private:

		void
		initialise_widgets();

		void
		set_up_connections();

		void
		update_tree_from_model();

		void
		update_selected_pick_and_segment();

		void
		update_enable_disable_buttons();

		void
		expand_segment(
				const unsigned int segment_number);

		void
		set_selected_segment(
				const unsigned int segment_number);

		void
		set_selected_pick(
				const hellinger_model_type::const_iterator &it);


		HellingerDialog *d_hellinger_dialog_ptr;
		HellingerModel *d_hellinger_model_ptr;

		tree_items_collection_type d_tree_items;

		/**
		 * @brief d_selected_segment - the number of the selected segment, if a segment
		 * has been selected in the tree_widget
		 */
		boost::optional<unsigned int> d_selected_segment;

		/**
		 * @brief d_selected_pick - the selected pick in the tree_widget, if a pick
		 * has been selected
		 */
		boost::optional<hellinger_model_type::const_iterator> d_selected_pick;

		/**
		 * @brief d_segment_number_of_selected_pick - if a pick has been selected,
		 * the segment number of that pick.
		 */
		boost::optional<unsigned int> d_segment_number_of_selected_pick;

		/**
		 * @brief d_segment_expanded_status - map storing the status of expanded/collapsed parts of the tree widget, so
		 * that this can be restored when necessary.
		 */
		expanded_status_map_type d_segment_expanded_status;

		int d_scrollbar_position;
		int d_scrollbar_maximum;

		boost::optional<QTreeWidgetItem*> d_hovered_item;
		bool d_hovered_item_original_state;
	};

}


#endif // GPLATES_QTWIDGETS_HELLINGERPICKWIDGET_H
