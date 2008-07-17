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
 
#ifndef GPLATES_QTWIDGETS_DIGITISATIONWIDGET_H
#define GPLATES_QTWIDGETS_DIGITISATIONWIDGET_H

#include <QWidget>
#include <QTreeWidget>
#include <QUndoStack>
#include "DigitisationWidgetUi.h"


namespace GPlatesQtWidgets
{
	// Forward declaration to avoid the GUI header spaghetti as much as possible.
	class ExportCoordinatesDialog;
	
	
	class DigitisationWidget:
			public QWidget, 
			protected Ui_DigitisationWidget
	{
		Q_OBJECT
		
	public:

		/**
		 * What kinds of geometry the DigitisationWidget can be configured for.
		 */
		enum GeometryType
		{
			POLYLINE, MULTIPOINT, POLYGON
		};

		explicit
		DigitisationWidget(
				QWidget *parent_ = NULL);
		
		/**
		 * Accessor so the QUndoCommands can get at the table of points.
		 */
		QTreeWidget *
		coordinates_table()
		{
			return treewidget_coordinates;
		}
					
	public slots:
		
		/**
		 * Resets all fields to their defaults.
		 */
		void
		clear_widget();

		/**
		 * Configures widgets to accept new geometry of a specific type.
		 * This will clear the coordinates table.
		 */
		void
		initialise_geometry(
				GeometryType geom_type);
		
		/**
		 * Configures widgets for a new geometry type while preserving the
		 * points that are currently being digitised.
		 * 
		 * Triggered when the user changes the combobox, or switches to a
		 * different CanvasTool. Note that this will set the combobox's
		 * index, which unfortunately causes (another) currentIndexChanged()
		 * signal to be emitted.
		 */
		void
		change_geometry_type(
				GeometryType geom_type);
		
		/**
		 *	The slot that gets called when the user clicks "Create".
		 */
		void
		create_geometry();

		/**
		 *	The slot that gets called when the user clicks "Cancel".
		 */
		void
		cancel_geometry();

		/**
		 * Adds a new lat,lon to the specified geometry (defaults to NULL,
		 * indicating the 'default geometry', which for now is just the
		 * first geometry available. This is convenient because to start
		 * with, I'm only handling unbroken LineString and MultiPoint.)
		 *
		 * If the specified geometry is a gml:Point, the given lat,lon will
		 * NOT be added to it but will instead replace it - I believe this
		 * would be the appropriate behaviour when (re)digitising a gml:position.
		 */
		void
		append_point_to_geometry(
				double lat,
				double lon,
				QTreeWidgetItem *target_geometry = NULL);
		
		/**
		 * Updates the text on all top-level QTreeWidgetItems (the labels)
		 * in the table to reflect what geometry (parts) you'll actually get.
		 */
		void
		update_table_labels();
	
	private slots:
		
		/**
		 * Converts the geometry combobox signal into one appropriate for
		 * DigitisationWidget::change_geometry_type(GeometryType).
		 *
		 * This slot is linked to the combobox's currentIndexChanged(int) signal,
		 * so it is assumed that the combobox has already been set to the
		 * appropriate index; however, it needs to call the public
		 * change_geometry_type(), and that method cannot make that assumption.
		 */
		void
		change_geometry_type(
				int idx);
		
		/**
		 * Feeds the ExportCoordinatesDialog a GeometryOnSphere (TODO), and
		 * then displays it.
		 */
		void
		handle_export();
	
	private:
		
		/**
		 * Causes the Geometry Type QComboBox to display the appropriate slot.
		 */
		void
		set_geometry_combobox(
				GeometryType geom_type);
		
		/**
		 * May want to move this stack into e.g. ViewState,
		 * or use a @a QUndoGroup to manage this stack and others.
		 */
		QUndoStack d_undo_stack;
		
		/**
		 * Memory managed by Qt.
		 */
		ExportCoordinatesDialog *d_export_coordinates_dialog;
		
		/**
		 * What kind of geometry are we -supposed- to be digitising?
		 * Note that what we actually get when the user hits Create may be
		 * different (A LineString with only one point?! That's unpossible.)
		 */
		GeometryType d_geometry_type;
		
	};
}

#endif  // GPLATES_QTWIDGETS_DIGITISATIONWIDGET_H

