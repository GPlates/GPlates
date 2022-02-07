/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
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
 
#ifndef GPLATES_QTWIDGETS_PREFERENCESDIALOG_H
#define GPLATES_QTWIDGETS_PREFERENCESDIALOG_H

#include <QDialog>
#include <QTableView>

#include "GPlatesDialog.h"

#include "PreferencesDialogUi.h"


namespace GPlatesAppLogic
{
	class ApplicationState;
}

namespace GPlatesQtWidgets
{
	class ConfigTableView :
			public QTableView
	{
	public:

		explicit
		ConfigTableView(
				QWidget *parent_ = NULL) :
			QTableView(parent_)
		{ }

		void
		commit_current_editor_data()
		{
			 QModelIndex index = currentIndex();
			 QWidget * widget = indexWidget(index);
			 commitData(widget);
		}
	};

	/**
	 * This dialog provides users with controls for various preference settings available
	 * in GPlates via GPlatesAppLogic::UserPreferences.
	 *
	 * As it only uses a 'Close' button instead of an 'Apply/OK/Cancel' set of buttons,
	 * it should be used as a modal dialog - exec() it, don't show() it.
	 * FIXME: As PreferencesDialog has been converted to a GPlatesDialog, we might need
	 * to ensure this is still the case.
	 *
	 * TO ADD A NEW PREFERENCE CATEGORY:-
	 *    1. Create a new PreferencesPaneXXXUi.ui in Designer, along with PreferencesPaneXXX.h and
	 *       PreferencesPaneXXX.cc - you may wish to base it off one of the existing panes.
	 *    2. Add to subversion and cmake/add_sources.py
	 *    3. #include it in PreferencesDialog.cc
	 *    4. Add a call to add_pane() inside PreferencesDialog's constructor, in the order you
	 *       want it to appear.
	 *    5. The Advanced pane is special and always last.
	 */
	class PreferencesDialog: 
			public GPlatesDialog,
			protected Ui_PreferencesDialog 
	{
		Q_OBJECT
		
	public:

		explicit
		PreferencesDialog(
				GPlatesAppLogic::ApplicationState &app_state,
				QWidget *parent_ = NULL);


		virtual
		~PreferencesDialog()
		{  }

	
		void 
		reject()
		{
			d_cfg_table->commit_current_editor_data();
			GPlatesDialog::reject();
		}

	protected:
	
	private:
	
		void
		add_pane(
				int index,
				const QString &category_label,
				QWidget *pane_widget,
				bool scrolling);
		
		ConfigTableView * d_cfg_table;
	};

}

#endif  // GPLATES_QTWIDGETS_PREFERENCESDIALOG_H
