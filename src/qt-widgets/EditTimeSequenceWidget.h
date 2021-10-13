/* $Id: EditTimeSequenceWidget.h 8310 2010-05-06 15:02:23Z rwatson $ */

/**
 * \file 
 * $Revision: 8310 $
 * $Date: 2010-05-06 17:02:23 +0200 (to, 06 mai 2010) $ 
 * 
 * Copyright (C) 2009, 2010 Geological Survey of Norway
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
 
#ifndef GPLATES_QTWIDGETS_EDITTIMESEQUENCE_H
#define GPLATES_QTWIDGETS_EDITTIMESEQUENCE_H

#include <boost/intrusive_ptr.hpp>

#include "AbstractEditWidget.h"
#include "EditTableWidget.h"

#include "model/types.h"
#include "property-values/GpmlArray.h"

#include "EditTimeSequenceWidgetUi.h"

namespace GPlatesAppLogic
{
    class ApplicationState;
}

namespace GPlatesQtWidgets
{
	class EditTableActionWidget;

	class EditTimeSequenceWidget:
			public AbstractEditWidget,
			public EditTableWidget,
			protected Ui_EditTimeSequenceWidget
	{
		Q_OBJECT
		
	public:
		explicit
		EditTimeSequenceWidget(
			GPlatesAppLogic::ApplicationState &app_state_,
			QWidget *parent_ = NULL);
		
		virtual
		void
		reset_widget_to_default_values();
		
		void
		update_widget_from_time_period_array(
			GPlatesPropertyValues::GpmlArray &gpml_array);

		virtual
		GPlatesModel::PropertyValue::non_null_ptr_type
		create_property_value_from_widget() const;

		virtual
		bool
		update_property_value_from_widget();
	
		virtual
		void
		handle_insert_row_above(
				const EditTableActionWidget *action_widget);

		virtual
		void
		handle_insert_row_below(
				const EditTableActionWidget *action_widget);
	
		virtual
		void
		handle_delete_row(
				const EditTableActionWidget *action_widget);
	
		/**
		 * Adds a new time to the table.
		 */
		void
		insert_single(
			double time);
				
		/**
		 * Fill the table with values determined by the "Fill with times" group box.                                                                     
		 */		
		void
		insert_multiple();

	private slots:
	
		/**
		 * Fired when the data of a cell has been modified.
		 *
		 * FIXME: ... by the user?
		 */
		void
		handle_cell_changed(int row, int column);
		
		
		/**
		 * Fired when a cell has been clicked.
		 *
		 */
		void
		handle_cell_clicked(int row, int column);

		/**
		 * Fired when Fill-with-times widget's Fill button has been clicked.                                                                   
		 */
		
		void
		handle_remove_all();

		void
		handle_remove();

		void
		handle_insert_single()
		{			
			insert_single(spinbox_time->value());
			spinbox_time->setFocus();
			spinbox_time->selectAll();
			sort_and_commit();
		}

		void
		handle_insert_multiple()
		{
			insert_multiple();
			spinbox_from_time->setFocus();
			spinbox_from_time->selectAll();
			sort_and_commit();
		};

		/**
		 * Creates an EditTableActionWidget item in the current row.
		 */
		void
		handle_current_cell_changed(
				int currentRow, int currentColumn, int previousRow, int previousColumn);
				
		void
		handle_cell_activated(int row, int column);

		/**
		 * Use main window time for the insert-single-time time-value
		 */
		void
		handle_use_main_single();

		/**
		 * Use main window time for the insert-multiple-times from-value
		 */
		void
		handle_use_main_from();

		/**
		 * Use main window time for the insert-multiple-times to-value
		 */
		void
		handle_use_main_to();

		/**
		 * Listen for the time spinbox having had a value entered. We
		 * can use this to auto-fill the table.
		 */
		void
		handle_single_time_entered();
		
	private:
		
		/**
		 * Finds the current table row associated with the EditTableActionWidget.
		 * Will return -1 in the event that the action widget provided is not in
		 * the table.
		 */
		int
		get_row_for_action_widget(
				const EditTableActionWidget *action_widget);
		
		/**
		 * Adds a new blank point to the current geometry in the table.
		 * The new row will be inserted to take the place of row index @a row.
		 */
		void
		insert_blank_time_into_table(
				int row);
		
		/**
		 * Removes a single point from the current geometry in the table.
		 */
		void
		delete_time_from_table(
				int row);

		/**
		 * Sorts the table, and emits commit signal.
		 */
		void
		sort_and_commit();

		/**
		 * Updates the time samples in the GpmlIrregularSampling.
		 */
		void
		update_times();

		void
		update_buttons();

        void
		setup_connections();
		
		/**
		 * This boost::intrusive_ptr is used to remember the property value which
		 * was last loaded into this editing widget. This is done so that the
		 * edit widget can directly update the property value later.
		 *
		 * We need to use a reference-counting pointer to make sure the property
		 * value doesn't disappear while this edit widget is active; however, since
		 * the property value is not known at the time the widget is created,
		 * the pointer may be NULL and this must be checked for.
		 *
		 * The pointer will also be NULL when the edit widget is being used for
		 * adding brand new properties to the model.
		 */

         boost::intrusive_ptr<GPlatesPropertyValues::GpmlArray> d_array_ptr;
		
		 double d_current_reconstruction_time;


	};
}

#endif  // GPLATES_QTWIDGETS_EDITTIMESEQUENCEWIDGET_H

