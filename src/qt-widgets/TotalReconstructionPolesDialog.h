/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008 Geological Survey of Norway
 * Copyright (C) 2010, 2011 The University of Sydney, Australia
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

#ifndef GPLATES_QTWIDGETS_TOTALRECONSTRUCTIONPOLESDIALOG_H
#define GPLATES_QTWIDGETS_TOTALRECONSTRUCTIONPOLESDIALOG_H

#include <boost/weak_ptr.hpp>
#include <QDialog>

#include "TotalReconstructionPolesDialogUi.h"

#include "GPlatesDialog.h"
#include "SaveFileDialog.h"

#include "presentation/VisualLayer.h"


namespace GPlatesAppLogic
{
	class ApplicationState;
	class ReconstructionTree;
}

namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesQtWidgets
{
	class VisualLayersComboBox;

	class TotalReconstructionPolesDialog:
			public GPlatesDialog,
			protected Ui_TotalReconstructionPolesDialog
	{
		Q_OBJECT

	public:

		explicit
		TotalReconstructionPolesDialog(
				GPlatesPresentation::ViewState &view_state,
				QWidget *parent_ = NULL);

	public Q_SLOTS:

		/**
		 * Updates the dialog. (After the reconstruction time/plate has been
		 * changed in the Viewport Window, for example). 
		 */
		void
		update();

		/**
		 * Updates the dialog to show a particular @a visual_layer.
		 */
		void
		update(
				boost::weak_ptr<GPlatesPresentation::VisualLayer> visual_layer);

	protected:

		void
		showEvent(
				QShowEvent *event_);

	private Q_SLOTS:

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

		void
		update_if_visible();

		void
		update_if_layer_changed();

	private:
	
		/**
		 * Called from @a export_relative and @a export_equivalent to handle
		 * getting the filename from the user and different export options.
		 */
		void
		handle_export(
				const QTableWidget &table);

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
		fill_equivalent_table(
				const GPlatesAppLogic::ReconstructionTree &reconstruction_tree);

		/**
		 * Fill the relative-rotation QTableWidget.
		 */
		void
		fill_relative_table(
				const GPlatesAppLogic::ReconstructionTree &reconstruction_tree);


		/**
		 * Fill the reconstruction tree QTreeWidget.
		 */
		void
		fill_reconstruction_tree(
				const GPlatesAppLogic::ReconstructionTree &reconstruction_tree);

		/**
		 * Fill the circuit-to-stationary-plate QTreeWidget.
		 */
		void
		fill_circuit_tree(
				const GPlatesAppLogic::ReconstructionTree &reconstruction_tree);
		
		void
		make_signal_slot_connections();

		void
		reset_everything();

		GPlatesPresentation::ViewState &d_view_state;

		/**
		 * To query the reconstruction.
		 */
		GPlatesAppLogic::ApplicationState &d_application_state;

		/**
		 * The stationary plate id.
		 */
		unsigned long d_plate;

		/**
		 * The reconstruction time.
		 */
		double d_time;

		/**
		 * Used by @a handle_export to obtain a file name from the user.
		 */
		SaveFileDialog d_save_file_dialog;

		VisualLayersComboBox *d_visual_layers_combobox;
		boost::weak_ptr<GPlatesPresentation::VisualLayer> d_curr_visual_layer;
	};
}


#endif // GPLATES_QTWIDGETS_TOTALRECONSTRUCTIONPOLESDIALOG_H

