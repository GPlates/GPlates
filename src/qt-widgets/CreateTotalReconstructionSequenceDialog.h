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

#ifndef GPLATES_QTWIDGETS_CREATETOTALRECONSTRUCTIONSEQUENCEDIALOG_H
#define GPLATES_QTWIDGETS_CREATETOTALRECONSTRUCTIONSEQUENCEDIALOG_H

#include <QDialog>

#include "model/FeatureHandle.h"
#include "property-values/GpmlIrregularSampling.h"
#include "ChooseFeatureCollectionWidget.h"

#include "CreateTotalReconstructionSequenceDialogUi.h"

namespace GPlatesAppLogic
{
	class ApplicationState;
}

namespace GPlatesQtWidgets
{
	class EditTotalReconstructionSequenceWidget;
	class TotalReconstructionSequencesDialog;

	/**
	 * This dialog displays, and allows editing of, the TotalReconstructionSequence @a trs_feature.                                                                  
	 */
	class CreateTotalReconstructionSequenceDialog:
		public QDialog,
		protected Ui_CreateTotalReconstructionSequenceDialog
	{
		Q_OBJECT
	public:
		
		enum StackedWidgetPage
		{
			TRS_PAGE,
			COLLECTION_PAGE
		};	

		CreateTotalReconstructionSequenceDialog(
			GPlatesQtWidgets::TotalReconstructionSequencesDialog &trs_dialog,
			GPlatesAppLogic::ApplicationState &app_state,
			QWidget *parent = 0);

                void
                init();
	
        virtual
        ~CreateTotalReconstructionSequenceDialog();

		boost::optional<GPlatesModel::FeatureHandle::weak_ref>
		created_feature()
		{
			return d_trs_feature;
		}

	private Q_SLOTS:

		/**
		 * Handle the create button being clicked.                                                                    
		 */
		void
		handle_create();

		/**
		 * Handle the cancel button being clicked.                                                                    
		 */
		void
		handle_cancel();

		void
		handle_table_validity_changed(
			bool);

		void
		handle_previous();

		void
		handle_next();

	private:

		void
		make_connections();

		void
		setup_pages();

		void
		make_trs_page_current();

		void
		make_feature_collection_page_current();

		/**
		 * The TRS dialog.                                                                    
		 */
		GPlatesQtWidgets::TotalReconstructionSequencesDialog &d_trs_dialog;


		/**
		 * The widget for editing the TRS.                                                                   
		 */
		boost::scoped_ptr<EditTotalReconstructionSequenceWidget> d_edit_widget_ptr;

		/**
		 * The widget for choosing the feature collection.                                                                    
		 */
		GPlatesQtWidgets::ChooseFeatureCollectionWidget *d_choose_feature_collection_widget_ptr;

		/**
		 * The irregular sampling property.
		 */
		boost::optional<GPlatesPropertyValues::GpmlIrregularSampling::non_null_ptr_type> d_irregular_sampling;

		/**
		 * The moving plate id.                                                                    
		 */
		boost::optional<GPlatesModel::integer_plate_id_type> d_moving_plate_id;

		/**
		 * The fixed plate id.                                                                    
		 */
		boost::optional<GPlatesModel::integer_plate_id_type> d_fixed_plate_id;

		/**
		 * The app state, for getting the feature collections.
		 */
		GPlatesAppLogic::ApplicationState &d_app_state;

		/**
		 * The created TRS feature.                                                                    
		 */
		boost::optional<GPlatesModel::FeatureHandle::weak_ref> d_trs_feature;

	};

}

#endif // GPLATES_QTWIDGETS_CREATETOTALRECONSTRUCTIONSEQUENCEDIALOG_H
