/* $Id: EditTimeSequenceWidget.cc 8310 2010-05-06 15:02:23Z rwatson $ */

/**
 * \file 
 * $Revision: 8310 $
 * $Date: 2010-05-06 17:02:23 +0200 (to, 06 mai 2010) $ 
 * 
 * Copyright (C) 2011 Geological Survey of Norway
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

#ifndef GPLATES_QTWIDGETS_EDITTOTALRECONSTRUCTIONSEQUENCEWIDGET_H
#define GPLATES_QTWIDGETS_EDITTOTALRECONSTRUCTIONSEQUENCEWIDGET_H

#include <QStandardItemModel>
#include <QWidget>

#include "model/FeatureHandle.h"
#include "property-values/GpmlIrregularSampling.h"

#include "EditTableActionWidget.h"
#include "EditTableWidget.h"
#include "ui_EditTotalReconstructionSequenceWidgetUi.h"

namespace GPlatesQtWidgets
{
	class TotalReconstructionSequencesDialog;
	class EditPoleActionWidget:
		public EditTableActionWidget
	{
		Q_OBJECT
	public:
		explicit
		EditPoleActionWidget(
				EditTableWidget *table_widget,
				bool enable_is_on = true,
				QWidget *parent_ = NULL);

		void
		set_enable_flag(
				bool flag)
		{
			d_enable_is_on = flag;
			refresh_buttons();
		}

	protected:
		void
		refresh_buttons()
		{
			if(d_enable_is_on)
			{
				enable_button->setVisible(true);
				disable_button->setVisible(false);
			}
			else
			{
				enable_button->setVisible(false);
				disable_button->setVisible(true);
			}
		}
	
	private Q_SLOTS:
		void
		enable_pole();

		void
		disable_pole();
	private:
		QPushButton *disable_button, *enable_button;
		bool d_enable_is_on;
	};

	/**
	 * This widget displays, and allows editing of, the irregular sampling property of a TotalReconstructionSequence feature.                                                             
	 */
	class EditTotalReconstructionSequenceWidget:
		public QWidget,
		public EditTableWidget,
		protected Ui_EditTotalReconstructionSequenceWidget
	{
		Q_OBJECT
	public:
		
		explicit
		EditTotalReconstructionSequenceWidget(
			QWidget *parent = 0);
	
		/**
		 * Fill table with data from TRS feature.                                                                    
		 */
		void
		update_table_widget_from_property(
			GPlatesPropertyValues::GpmlIrregularSampling::non_null_ptr_type irregular_sampling);

		GPlatesModel::TopLevelProperty::non_null_ptr_type
		get_irregular_sampling_property_value_from_table_widget();

		GPlatesModel::integer_plate_id_type
		moving_plate_id() const;

		GPlatesModel::integer_plate_id_type
		fixed_plate_id() const;

		void
		set_moving_plate_id(
				const GPlatesModel::integer_plate_id_type &moving_plate_id);

		void
		set_fixed_plate_id(
				const GPlatesModel::integer_plate_id_type &fixed_plate_id);

		void
		sort_table_by_time();

		/**
		 * Validate the table of sequences.
		 * 
		 * The only sort of validation required is to check for duplicate time instants.
		 *
		 * Other numerical fields - lat/lon/angle - are taken care of by the spinbox limits.  
		 */
		bool
		validate();

		/**
		 * Set up an "empty" widget - but with an initial (zero-valued) row entry.                                                                     
		 */
		void
        initialise();

		/**
		 *                                                                     
		 */
		void
		set_action_widget_in_row(
				int row);

		void
		handle_disable_pole(
				EditPoleActionWidget*,
				bool);
		
	Q_SIGNALS:
		void
		table_validity_changed(
				bool);

		void
		plate_ids_have_changed();

	protected:
		/**
		* Creates an irregular sampling property from the values in @a table.                                                             
		*/
		GPlatesModel::TopLevelProperty::non_null_ptr_type
		make_irregular_sampling_from_table();

	private:
		virtual
		void
		handle_insert_row_above(
				const EditTableActionWidget *);

		virtual
		void
		handle_insert_row_below(
				const EditTableActionWidget *);

		virtual
		void
		handle_delete_row(
				const EditTableActionWidget *);

	private Q_SLOTS:
		void
		handle_insert_new_pole();

		void
		handle_item_changed(
				QTableWidgetItem* item);

		void
		handle_current_cell_changed(
				int,int,int,int);

		/**
		 * Handle the editFinished() signal from the spinbox in the active cell.                                                                     
		 */
		void
		handle_editing_finished();

		void
		handle_plate_ids_changed();

	private:

		int
		get_row_for_action_widget(
				const EditTableActionWidget *);

		void
		insert_blank_row(
				int row);

		void
		delete_row(
			  int row);

		/**
		 * Borrowed from TopologySectionsTable - used to prevent 
		 * update and related methods from triggering the itemChanged signal.
		 * 
		 * (The signals will still be sent, just that we can decide whether or not to handle them). 
		 * 
		 * Could also do this by disconnecting?
		 */
		bool d_suppress_update_notification_guard;

		// The row and column at which the spinbox is located. 
		int d_spinbox_row;
		int d_spinbox_column;

		bool d_moving_plate_changed;
		bool d_fixed_plate_changed;
	};

}

#endif // GPLATES_QTWIDGETS_EDITTOTALRECONSTRUCTIONSEQUENCEWIDGET_H
