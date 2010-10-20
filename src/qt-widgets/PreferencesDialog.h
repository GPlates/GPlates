/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
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
 
#ifndef GPLATES_QTWIDGETS_PREFERENCESDIALOG_H
#define GPLATES_QTWIDGETS_PREFERENCESDIALOG_H

#include <QDialog>
#include "PreferencesDialogUi.h"


namespace GPlatesQtWidgets
{
	/**
	 * This dialog provides users with controls for various preference settings available
	 * in GPlates via GPlatesAppLogic::UserPreferences.
	 *
	 * As it only uses a 'Close' button instead of an 'Apply/OK/Cancel' set of buttons,
	 * it should be used as a modal dialog - exec() it, don't show() it.
	 */
	class PreferencesDialog: 
			public QDialog,
			protected Ui_PreferencesDialog 
	{
		Q_OBJECT
		
	public:

		explicit
		PreferencesDialog(
				QWidget *parent_ = NULL);


		virtual
		~PreferencesDialog()
		{  }
		
	};
}

#endif  // GPLATES_QTWIDGETS_PREFERENCESDIALOG_H
