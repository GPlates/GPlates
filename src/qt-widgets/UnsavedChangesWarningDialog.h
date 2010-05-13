/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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
 
#ifndef GPLATES_QTWIDGETS_UNSAVEDCHANGESWARNINGDIALOG_H
#define GPLATES_QTWIDGETS_UNSAVEDCHANGESWARNINGDIALOG_H

#include <QDialog>
#include <QDialogButtonBox>
#include <QPushButton>
#include "UnsavedChangesWarningDialogUi.h"


namespace GPlatesQtWidgets
{
	/**
	 * This dialog is the one which pops up if the user attempts to close GPlates
	 * while there are yet files unsaved.
	 * It is triggered from GPlatesGui::UnsavedChangesTracker.
	 *
	 * When you exec() this dialog, the return value is the QDialogButtonBox::StandardButton
	 * enum corresponding to the clicked button; this is one of:-
	 * 	QDialogButtonBox::Discard - do not save, just close.
	 * 	QDialogButtonBox::Abort - do not close.
	 * 	QDialogButtonBox::SaveAll - save all modified files, then close.
	 *
	 * The reason QDialogButtonBox is used is so that Qt can handle the platform-specific
	 * button ordering conventions.
	 */
	class UnsavedChangesWarningDialog: 
			public QDialog,
			protected Ui_UnsavedChangesWarningDialog 
	{
		Q_OBJECT
		
	public:

		explicit
		UnsavedChangesWarningDialog(
				QWidget *parent_ = NULL):
			QDialog(parent_, Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint)
		{
			setupUi(this);
			tweak_buttons();
			connect_buttons();
		}

		virtual
		~UnsavedChangesWarningDialog()
		{  }
		
		/**
		 * Changes the list of filenames displayed in the dialog.
		 */
		void
		set_filename_list(
				QStringList filenames)
		{
			list_files->clear();
			list_files->addItems(filenames);
		}

	private slots:

		void
		discard_changes()
		{
			done(QDialogButtonBox::Discard);
		}

		void
		abort_close()
		{
			done(QDialogButtonBox::Abort);
		}

		void
		save_all_first()
		{
			done(QDialogButtonBox::SaveAll);
		}

	private:

		/**
		 * Overrides the default labels on the StandardButtons Qt provides,
		 * and adds icons.
		 */
		void
		tweak_buttons()
		{
			buttonbox->button(QDialogButtonBox::SaveAll)->setText(tr("&Save all modified feature collections"));
			buttonbox->button(QDialogButtonBox::SaveAll)->setIcon(QIcon(":/gnome_save_22.png"));
			buttonbox->button(QDialogButtonBox::SaveAll)->setIconSize(QSize(22, 22));

			buttonbox->button(QDialogButtonBox::Discard)->setText(tr("&Discard changes"));
			buttonbox->button(QDialogButtonBox::Discard)->setIcon(QIcon(":/discard_changes_22.png"));
			buttonbox->button(QDialogButtonBox::Discard)->setIconSize(QSize(22, 22));

			buttonbox->button(QDialogButtonBox::Abort)->setText(tr("D&on't close"));
			buttonbox->button(QDialogButtonBox::Abort)->setIcon(QIcon(":/tango_process_stop_22.png"));
			buttonbox->button(QDialogButtonBox::Abort)->setIconSize(QSize(22, 22));
		}

		/**
		 * Connects all the buttons to a single signal that we can emit
		 * to indicate what button was clicked on this dialog.
		 */
		void
		connect_buttons()
		{
			connect(buttonbox->button(QDialogButtonBox::Discard), SIGNAL(clicked()),
					this, SLOT(discard_changes()));
			connect(buttonbox->button(QDialogButtonBox::Abort), SIGNAL(clicked()),
					this, SLOT(abort_close()));
			connect(buttonbox->button(QDialogButtonBox::SaveAll), SIGNAL(clicked()),
					this, SLOT(save_all_first()));
		}
			
	};
}

#endif  // GPLATES_QTWIDGETS_UNSAVEDCHANGESWARNINGDIALOG_H
