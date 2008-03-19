/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, Geological Survey of Norway
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

#ifndef GPLATES_QTWIDGETS_EULERPOLEDIALOG_H
#define GPLATES_QTWIDGETS_EULERPOLEDIALOG_H

#include <QDialog>

#include "EulerPoleDialogUi.h"

#include "model/Reconstruction.h"


namespace GPlatesQtWidgets{

	class ViewportWindow;

	class EulerPoleDialog:
		public QDialog,
		protected Ui_EulerPoleDialog
	{
		Q_OBJECT

	public:
	EulerPoleDialog(
		ViewportWindow &viewport_window,
		QWidget *parent_ = NULL);

	/**
	 * Set the dialog reconstruction time. 
	 */ 
	void 
	set_time(
		const double time);

	/**
	 * Set the dialog stationary plate id.
	 */
	void
	set_plate(
		unsigned long plate);

	/**
	 * Fill the equivalent-rotation QTableWidget. 
	 */
	void 
	fill_equivalent_table();

	/**
	 * Fill the relative-rotation QTableWidget.
	 */
	void
	fill_relative_table();


	/**
	 * Fill the reconstruction tree QTreeWidget.
	 */
	void
	fill_reconstruction_tree();

	/**
	 * Fill the circuit-to-stationary-plate QTreeWidget.
	 */
	void
	fill_circuit_tree();

	/**
	 * Update the dialog. (After the reconstruction time/plate has been
	 * changed in the Viewport Window, for example). 
	 */
	void
	update();

	public slots:

	/**
	 * Export the relative-rotation data in csv form. 
	 */ 
	void
	export_relative();

	/** 
	 * Export the equivalent-rotation data in csv form.
	 */ 
	void
	export_equivalent();

	private:

		/**
		 * The viewport window.
		 */
		ViewportWindow *d_viewport_window_ptr;

		/**
		 * The stationary plate id.
		 */
		unsigned long d_plate;

		/**
		 * The reconstruction time.
		 */
		double d_time;

	};

}

#endif // GPLATES_QTWIDGETS_EULERPOLEDIALOG_H

