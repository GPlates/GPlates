/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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
 
#ifndef GPLATES_QTWIDGETS_EXPORTCOORDINATESDIALOG_H
#define GPLATES_QTWIDGETS_EXPORTCOORDINATESDIALOG_H

#include <boost/optional.hpp>
#include <QTextStream>

#include "ui_ExportCoordinatesDialogUi.h"

#include "SaveFileDialog.h"

#include "maths/GeometryOnSphere.h"


namespace GPlatesFileIO
{
	class GeometryExporter;
}

namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesQtWidgets
{
	class InformationDialog;

	class ExportCoordinatesDialog:
			public QDialog, 
			protected Ui_ExportCoordinatesDialog
	{
		Q_OBJECT
		
	public:

		/**
		* Enumeration for the possible formats to export to.
		* The order of these should match the setup of the @a combobox_format
		* as set up in the designer.
		*
		* FIXME: When we implement the writers, we will probably want to associate
		* information with these enums; this would be a good point to include the
		* combobox text so we can set it up in code.
		*/
		enum OutputFormat
		{
			PLATES4, GMT, OGRGMT, SHAPEFILE,  WKT, CSV
		};

		explicit
		ExportCoordinatesDialog(
				GPlatesPresentation::ViewState &view_state,
				QWidget *parent_ = NULL);
		
		/**
		 * Rather than simply exec()ing the dialog, you should call this method to
		 * ensure you are feeding the ExportCoordinatesDialog some valid geometry
		 * at the same time.
		 */
		bool
		set_geometry_and_display(
				GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry_);
					
	private Q_SLOTS:
		
		/**
		 * Fired when user (or code..) selects a format from the combobox.
		 */
		void
		handle_format_selection(
				int idx);
		
		/**
		 * The slot that gets called when the user clicks the Export button.
		 */
		void
		handle_export();

	private:



		/**
		 * Enumeration for the order of coordinates to export with.
		 * The order of these should match the setup of the @a combobox_coordinate_order
		 * as set up in the designer.
		 */
		enum CoordinateOrder
		{
			LAT_LON, LON_LAT
		};
	
		/**
		 * The geometry that is to be exported when the user clicks the Export
		 * button and triggers the handle_export() slot.
		 * 
		 * This may be boost::none if the export dialog has not been fed any geometry yet.
		 */
		boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> d_geometry_opt_ptr;

		GPlatesPresentation::ViewState &d_view_state_ref;

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
	

		/**
		 * Export geometry in specified format.
		 *
		 * @param format format of geometry export.
		 * @param text_stream output stream.
		 */
		void export_geometry_to_file( OutputFormat format, QString &filename);

		/**
		 * Export geometry in specified format.
		 *
		 * @param format format of geometry export.
		 * @param text_stream output stream.
		 */
		void export_geometry_to_text_stream( OutputFormat format, QTextStream &text_stream);

	};
}

#endif  // GPLATES_QTWIDGETS_EXPORTCOORDINATESDIALOG_H

