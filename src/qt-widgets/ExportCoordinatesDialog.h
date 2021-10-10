/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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
 
#ifndef GPLATES_QTWIDGETS_EXPORTCOORDINATESDIALOG_H
#define GPLATES_QTWIDGETS_EXPORTCOORDINATESDIALOG_H

#include <QFileDialog>
#include <boost/optional.hpp>
#include "ExportCoordinatesDialogUi.h"

#include "maths/GeometryOnSphere.h"


namespace GPlatesQtWidgets
{
	class InformationDialog;

	class ExportCoordinatesDialog:
			public QDialog, 
			protected Ui_ExportCoordinatesDialog
	{
		Q_OBJECT
		
	public:

		explicit
		ExportCoordinatesDialog(
				QWidget *parent_ = NULL);
		
		/**
		 * Rather than simply exec()ing the dialog, you should call this method to
		 * ensure you are feeding the ExportCoordinatesDialog some valid geometry
		 * at the same time.
		 */
		bool
		set_geometry_and_display(
				GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry_);
					
	private slots:
		
		/**
		 * Fired when user (or code..) selects a format from the combobox.
		 */
		void
		handle_format_selection(
				int idx);
		
		void
		select_to_file_stack()
		{
			stack_destination->setCurrentWidget(page_export_select_file);
		}

		void
		select_to_clipboard_stack()
		{
			stack_destination->setCurrentWidget(page_export_clipboard);
		}
		
		/**
		 * Handles the seleciton of a filename to export to via QFileDialog.
		 */
		void
		pop_up_file_browser();
		
		/**
		 * The slot that gets called when the user clicks the Export button.
		 */
		void
		handle_export();
		
	private:
	
		/**
		 * The geometry that is to be exported when the user clicks the Export
		 * button and triggers the handle_export() slot.
		 * 
		 * This may be boost::none if the export dialog has not been fed any geometry yet.
		 */
		boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> d_geometry_opt_ptr;


		/**
		 * A QFileDialog instance that we use for specifying the destination file.
		 * We keep it as a member so that it will remember where the user last
		 * saved to, for convenience.
		 *
		 * Memory managed by Qt.
		 */
		QFileDialog *d_export_file_dialog;
		
		/**
		 * The small information dialog that pops up to explain the reason for the
		 * checkbox_polygon_terminating_point option.
		 *
		 * Memory managed by Qt.
		 */
		InformationDialog *d_terminating_point_information_dialog;
		
		/**
		 * The text of the terminating point information dialog.
		 */
		static const QString s_terminating_point_information_text;
		
	};
}

#endif  // GPLATES_QTWIDGETS_EXPORTCOORDINATESDIALOG_H

