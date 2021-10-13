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
 
#ifndef GPLATES_QTWIDGETS_CALCULATERECONSTRUCTIONPOLEDIALOG_H
#define GPLATES_QTWIDGETS_CALCULATERECONSTRUCTIONPOLEDIALOG_H

#include <QWidget>

#include "maths/Rotation.h"
#include "ReconstructionPoleWidget.h"

#include "CalculateReconstructionPoleDialogUi.h"

namespace GPlatesAppLogic
{
	class ApplicationState;
}

namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesQtWidgets
{

	class InsertVGPReconstructionPoleDialog;

	/**
	 * Dialog to calculate a reconstruction pole from a VGP.                                                                     
	 */
	class CalculateReconstructionPoleDialog:
			public QDialog,
			protected Ui_CalculateReconstructionPoleDialog
	{
		Q_OBJECT
	public:

		CalculateReconstructionPoleDialog(
			GPlatesPresentation::ViewState &view_state,
			QWidget *parent_ = NULL);
	
	private slots:
		
		void
		handle_calculate();
		
		void
		handle_button_clicked(
				QAbstractButton *button);

	private:
	
		void
		update_buttons();
		
		InsertVGPReconstructionPoleDialog *d_dialog_ptr;

		ReconstructionPoleWidget *d_reconstruction_pole_widget_ptr;
		
		boost::optional<ReconstructionPole> d_reconstruction_pole;
		
		/**
		 * We need to pass this onto the InsertVGPReconstructionDialog so that
		 * the rotation model can be updated if necessary. 
		 */
		GPlatesAppLogic::ApplicationState *d_application_state_ptr;
		
		
	};
}

#endif //GPLATES_QTWIDGETS_CALCULATERECONSTRUCTIONPOLEDIALOGDIALOG_H
