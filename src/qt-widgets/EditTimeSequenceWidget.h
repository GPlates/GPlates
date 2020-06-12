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
#include <QItemDelegate>

#include "AbstractEditWidget.h"
#include "EditTableWidget.h"

#include "model/types.h"
#include "property-values/GpmlArray.h"

#include "ui_EditTimeSequenceWidgetUi.h"

namespace GPlatesAppLogic
{
    class ApplicationState;
}

namespace GPlatesQtWidgets
{

	/**
	 * @brief The SpinBoxDelegate class
	 *
	 * This lets us customise the spinbox behaviour in the TableView. Borrowed largely from the
	 * Qt example here:
	 * http://qt-project.org/doc/qt-4.8/itemviews-spinboxdelegate.html
	 *
	 */
	class EditTimeSequenceSpinBoxDelegate : public QItemDelegate
	{
		Q_OBJECT

	public:

		Q_SIGNAL void editing_finished() const;

		EditTimeSequenceSpinBoxDelegate(QObject *parent = 0);

		QWidget *createEditor(
				QWidget *parent,
				const QStyleOptionViewItem &option,
				const QModelIndex &index) const;

		void setEditorData(
				QWidget *editor,
				const QModelIndex &index) const;

		void setModelData(
				QWidget *editor,
				QAbstractItemModel *model,
				const QModelIndex &index) const;

		void updateEditorGeometry(
				QWidget *editor,
				const QStyleOptionViewItem &option,
				const QModelIndex &index) const;

		void paint(
				QPainter *painter,
				const QStyleOptionViewItem &option,
				const QModelIndex &index) const;
	};




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
		
		/**
		 * @a gpml_array must have a value type of 'gml:TimePeriod'.
		 */
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

	private Q_SLOTS:
	
		void
		handle_spinbox_editing_finished();

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
		 * Sorts the table, removes duplicates, and emits commit signal.
		 */
		void
		sort_and_commit();

		/**
		 * Updates the time samples in the GpmlIrregularSampling.
		 */
		void
		update_time_array_from_widget();

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

		EditTimeSequenceSpinBoxDelegate *d_spin_box_delegate;
	};
}

#endif  // GPLATES_QTWIDGETS_EDITTIMESEQUENCEWIDGET_H

