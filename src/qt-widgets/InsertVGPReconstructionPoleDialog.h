/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2010 Geological Survey of Norway
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
 
#ifndef GPLATES_QTWIDGETS_INSERTVGPRECONSTRUCTIONPOLEDIALOG_H
#define GPLATES_QTWIDGETS_INSERTVGPRECONSTRUCTIONPOLEDIALOG_H

#include <QDialog>

#include "PoleSequenceTableWidget.h"
#include "ReconstructionPoleWidget.h"
#include "InsertVGPReconstructionPoleDialogUi.h"

namespace GPlatesAppLogic
{
	class FeatureCollectionFileState;
	class FeatureCollectionFileIO;
	class Reconstruct;
}

namespace GPlatesMaths
{
	class Rotation;
}

namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesQtWidgets
{

	class InsertVGPReconstructionPoleDialog:
		public QDialog,
		protected Ui_InsertVGPReconstructionPoleDialog
	{
		Q_OBJECT
	public:

		InsertVGPReconstructionPoleDialog(
			GPlatesPresentation::ViewState &view_state_,
			QWidget *parent_ = NULL);
		
		void
		setup(
			const GPlatesQtWidgets::ReconstructionPole &reconstruction_pole);
			
			
	private:
	
		ReconstructionPole d_reconstruction_pole;
		
		PoleSequenceTableWidget *d_pole_sequence_table_widget_ptr;
	
		ReconstructionPoleWidget *d_reconstruction_pole_widget_ptr;
		
		GPlatesAppLogic::Reconstruct *d_reconstruct_ptr;
		
		/**
		 * The loaded feature collection files.
		 */
		GPlatesAppLogic::FeatureCollectionFileState &d_file_state;

		/**
		 * Used to create an empty feature collection file.
		 */
		GPlatesAppLogic::FeatureCollectionFileIO &d_file_io;		
				
	};
}

#endif // GPLATES_QTWIDGETS_INSERTVGPRECONSTRUCTIONPOLEDIALOG_H

 