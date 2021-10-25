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

#ifndef GPLATES_QTWIDGETS_EDITTOTALRECONSTRUCTIONSEQUENCEDIALOG_H
#define GPLATES_QTWIDGETS_EDITTOTALRECONSTRUCTIONSEQUENCEDIALOG_H

#include <QDialog>

#include "model/FeatureHandle.h"
#include "property-values/GpmlIrregularSampling.h"

#include "EditTotalReconstructionSequenceDialogUi.h"

namespace GPlatesQtWidgets
{
	class EditTotalReconstructionSequenceWidget;
	class TotalReconstructionSequencesDialog;

	/**
	 * This dialog displays, and allows editing of, the TotalReconstructionSequence @a trs_feature.                                                                  
	 */
	class EditTotalReconstructionSequenceDialog:
		public QDialog,
		protected Ui_EditTotalReconstructionSequenceDialog
	{
		Q_OBJECT
	public:
		
		EditTotalReconstructionSequenceDialog(
			GPlatesModel::FeatureHandle::weak_ref &trs_feature,
			GPlatesQtWidgets::TotalReconstructionSequencesDialog &trs_dialog,
			QWidget *parent = 0);
	
        virtual
        ~EditTotalReconstructionSequenceDialog();

	private Q_SLOTS:

		/**
		 * Handle the apply button being clicked.                                                                    
		 */
		void
		handle_apply();

		/**
		 * Handle the cancel button being clicked.                                                                    
		 */
		void
		handle_cancel();

		void
		handle_table_validity_changed(
			bool);

		void
		handle_plate_ids_changed();

	private:

		void
		make_connections();

		/**
		 * The TRS feature which we will edit.                                                                    
		 */
		GPlatesModel::FeatureHandle::weak_ref d_trs_feature;

		/**
		 * The TRS dialog.                                                                    
		 */
		GPlatesQtWidgets::TotalReconstructionSequencesDialog &d_trs_dialog;


		/**
		 * The widget for editing the TRS.                                                                   
		 */
		boost::scoped_ptr<EditTotalReconstructionSequenceWidget> d_edit_widget_ptr;


		/**
		 * The property iterators from d_trs_feature that refer to the properties we may want to edit.                                                                    
		 */
		boost::optional<GPlatesModel::FeatureHandle::iterator> d_irregular_sampling_property_iterator;
		boost::optional<GPlatesModel::FeatureHandle::iterator> d_moving_ref_frame_iterator;
		boost::optional<GPlatesModel::FeatureHandle::iterator> d_fixed_ref_frame_iterator;

		/**
		 * A clone of the irregular sampling property.
		 *
		 * (Does this really need to be a clone??)
		 */
		boost::optional<GPlatesPropertyValues::GpmlIrregularSampling::non_null_ptr_type> d_irregular_sampling;

		/**
		 * The moving plate id                                                                    
		 */
		boost::optional<GPlatesModel::integer_plate_id_type> d_moving_plate_id;

		/**
		 * The fixed plate id                                                                    
		 */
		boost::optional<GPlatesModel::integer_plate_id_type> d_fixed_plate_id;

	};

}

#endif // GPLATES_QTWIDGETS_EDITTOTALRECONSTRUCTIONSEQUENCEDIALOG_H
