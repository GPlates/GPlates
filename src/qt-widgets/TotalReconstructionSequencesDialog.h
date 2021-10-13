/* $Id$ */

/**
 * \file 
 * Contains the definition of class TotalReconstructionSequencesDialog.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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

#ifndef GPLATES_QTWIDGETS_TOTALRECONSTRUCTIONSEQUENCESDIALOG_H
#define GPLATES_QTWIDGETS_TOTALRECONSTRUCTIONSEQUENCESDIALOG_H

#include <QDialog>
#include <boost/shared_ptr.hpp>

#include "TotalReconstructionSequencesDialogUi.h"
#include "model/types.h"


namespace GPlatesAppLogic
{
	class ApplicationState;
	class FeatureCollectionFileState;
	class ReconstructionTree;
}

namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesQtWidgets
{
	/**
	 * A predicate to filter by plate ID.
	 *
	 * This is the abstract base class in an instance of the Interpreter pattern.
	 *
	 * The concrete derivations are defined in the source file.
	 */
	class PlateIdFilteringPredicate
	{
	public:
		virtual
		~PlateIdFilteringPredicate()
		{  }

		virtual
		bool
		allow_plate_id(
				GPlatesModel::integer_plate_id_type plate_id) const = 0;
	};


	class TotalReconstructionSequencesSearchIndex;

	class TotalReconstructionSequencesDialog:
		public QDialog,
		protected Ui_TotalReconstructionSequencesDialog
	{
		Q_OBJECT

	public:
		explicit
		TotalReconstructionSequencesDialog(
				GPlatesAppLogic::FeatureCollectionFileState &file_state,
				GPlatesPresentation::ViewState &view_state,
				QWidget *parent_ = NULL);

	public slots:

		/**
		 * Update the dialog (after a file has been loaded or unloaded, for example).
		 */
		void
		update();

		/**
		 * Respond to the "Apply Filter" button.
		 */
		void
		apply_filter();

		/**
		 * Respond to the "Reset Filter" button.
		 */
		void
		reset_filter();

		/**
		 * React when the "current item" of the QTreeWidget has changed.
		 */
		void
		handle_current_item_changed(
				QTreeWidgetItem *current,
				QTreeWidgetItem *previous);

	protected:

		const boost::shared_ptr<PlateIdFilteringPredicate>
		parse_plate_id_filtering_text() const;

	private:

		/**
		 * The loaded feature collection files.
		 */
		GPlatesAppLogic::FeatureCollectionFileState *d_file_state_ptr;

		/**
		 * The search index used to search by plate ID and text-in-comment.
		 */
		boost::shared_ptr<TotalReconstructionSequencesSearchIndex> d_search_index_ptr;

		/**
		 * Initialise the signal-slot connections in the constructor.
		 */
		void
		make_signal_slot_connections();

		/**
		 * Connect to signals from a @a FeatureCollectionFileState object.
		 *
		 * FIXME:  Define this function.
		 */
		void
		connect_to_file_state_signals();
	};

}

#endif // GPLATES_QTWIDGETS_TOTALRECONSTRUCTIONSEQUENCESDIALOG_H

