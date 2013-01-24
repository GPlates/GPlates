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
	 *
	 * It may also pop up if the user attempts to load a new session while there
	 * are yet files unsaved, as doing so would replace the current files.
	 *
	 * It is triggered from GPlatesGui::UnsavedChangesTracker.
	 *
	 * When you exec() this dialog, the return value is the QDialogButtonBox::StandardButton
	 * enum corresponding to the clicked button; this is one of:-
	 * 	QDialogButtonBox::Discard - do not save, just {close gplates,replace session}.
	 * 	QDialogButtonBox::Abort - do not {close gplates, replace session}.
	 * 	QDialogButtonBox::SaveAll - save all modified files first, then {close,replace}.
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
		enum ActionRequested { CLOSE_GPLATES, REPLACE_SESSION };

		explicit
		UnsavedChangesWarningDialog(
				QWidget *parent_ = NULL):
			QDialog(parent_, Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint)
		{
			setupUi(this);
			set_action_requested(CLOSE_GPLATES);
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

		/**
		 * Changes the label text and button labels to be appropriate
		 * for the corresponding action requested by the user (that
		 * GPlates is interrupting on account of the unsaved changes).
		 */
		void
		set_action_requested(
				ActionRequested act)
		{
			tweak_buttons(act);
			tweak_label(act);
			adjustSize();
			ensurePolished();
		}


	private Q_SLOTS:

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
		tweak_buttons(
				ActionRequested act)
		{
			static QPushButton *saveall = buttonbox->button(QDialogButtonBox::SaveAll);
			static QPushButton *discard = buttonbox->button(QDialogButtonBox::Discard);
			static QPushButton *abort = buttonbox->button(QDialogButtonBox::Abort);
			
			switch (act)
			{
			default:
			case CLOSE_GPLATES:
					saveall->setText(tr("&Save all modified feature collections"));
					saveall->setIcon(QIcon(":/gnome_save_22.png"));
					saveall->setIconSize(QSize(22, 22));

					discard->setText(tr("&Discard changes"));
					discard->setIcon(QIcon(":/discard_changes_22.png"));
					discard->setIconSize(QSize(22, 22));

					abort->setText(tr("D&on't close"));
					abort->setIcon(QIcon(":/tango_process_stop_22.png"));
					abort->setIconSize(QSize(22, 22));
					break;

			case REPLACE_SESSION:
					saveall->setText(tr("&Save all first"));
					saveall->setIcon(QIcon(":/gnome_save_22.png"));
					saveall->setIconSize(QSize(22, 22));

					discard->setText(tr("&Discard changes, load session"));
					discard->setIcon(QIcon(":/discard_changes_22.png"));
					discard->setIconSize(QSize(22, 22));

					abort->setText(tr("D&on't open the new session"));
					abort->setIcon(QIcon(":/tango_process_stop_22.png"));
					abort->setIconSize(QSize(22, 22));
					break;
			}
			buttonbox->adjustSize();
		}

		/**
		 * Sets the dialog's main descriptive label (as defined in UI)
		 * to something more context-sensitive.
		 */
		void
		tweak_label(
				ActionRequested act)
		{
			switch (act)
			{
			default:
			case CLOSE_GPLATES:
					label_context->setText(tr("GPlates is closing."));
					break;

			case REPLACE_SESSION:
					label_context->setText(tr(
							"Opening a previous session will replace all currently loaded"
							" feature collections. You may wish to save changes you have made"
							" before continuing."));
					break;
			}
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
