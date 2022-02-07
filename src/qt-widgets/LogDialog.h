/* $Id$ */

/**
 * \file
 *
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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
 
#ifndef GPLATES_QTWIDGETS_LOGDIALOG_H
#define GPLATES_QTWIDGETS_LOGDIALOG_H


#include <QPointer>
#include <QTimer>

#include "GPlatesDialog.h"
#include "LogDialogUi.h"

#include "app-logic/ApplicationState.h"


namespace GPlatesGui
{
	class LogFilterModel;
}

namespace GPlatesQtWidgets
{
	/**
	 * Simple dialog to show messages which would otherwise go to a terminal window,
	 * to aid users who do not start GPlates from a terminal.
	 */
	class LogDialog :
			public GPlatesDialog,
			protected Ui_LogDialog
	{
		Q_OBJECT
	public:
	
		explicit
		LogDialog(
				GPlatesAppLogic::ApplicationState &app_state,
				QWidget *_parent);
		
		virtual
		~LogDialog()
		{  };
	
	public Q_SLOTS:
		
		void
		copy_selection_to_clipboard();
	
	private Q_SLOTS:
	
		void
		handle_filter_typing();

		void
		handle_filter_changed();

		void
		handle_selection_changed();
	
	private:
	
		/**
		 * This model acts as a proxy between this dialog and the real LogModel.
		 * We keep a pointer to it so we can update the filtering text etc.
		 */
		QPointer<GPlatesGui::LogFilterModel> d_log_filter_model_ptr;
		
		/**
		 * A find-as-you-type filter that immediately responds to keypresses can be
		 * a bit annoying, so we include a small delay before it responds.
		 * Pressing Enter in the field immediately does the filtering.
		 */
		QPointer<QTimer> d_filter_timeout;
	};
}


#endif // GPLATES_QTWIDGETS_LOGDIALOG_H
