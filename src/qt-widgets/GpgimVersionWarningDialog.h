/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
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
 
#ifndef GPLATES_QTWIDGETS_GPGIMVERSIONWARNINGDIALOG_H
#define GPLATES_QTWIDGETS_GPGIMVERSIONWARNINGDIALOG_H

#include <QDialog>
#include <QDialogButtonBox>
#include <QPushButton>

#include "GpgimVersionWarningDialogUi.h"


namespace GPlatesModel
{
	class Gpgim;
}

namespace GPlatesQtWidgets
{
	/**
	 * This dialog is the one which pops up if the user loads files that were created with a different
	 * GPGIM version than the current GPlates or if the user attempts to save those files.
	 *
	 * This essentially warns the user about overwriting GPML files with a different version which
	 * could make the GPML files unreadable by older versions of GPlates.
	 *
	 * The reason QDialogButtonBox is used is so that Qt can handle the platform-specific
	 * button ordering conventions.
	 */
	class GpgimVersionWarningDialog: 
			public QDialog,
			protected Ui_GpgimVersionWarningDialog
	{
		Q_OBJECT
		
	public:

		enum ActionRequested { LOAD_FILES, SAVE_FILES };

		explicit
		GpgimVersionWarningDialog(
				const GPlatesModel::Gpgim &gpgim,
				QWidget *parent_ = NULL);

		virtual
		~GpgimVersionWarningDialog()
		{  }


		/**
		 * Changes the list of older and newer version filenames displayed in the dialog.
		 *
		 * Also changes the label text and button labels to be appropriate
		 * for the corresponding action requested by the user.
		 */
		void
		set_action_requested(
				ActionRequested act,
				QStringList older_version_filenames,
				QStringList newer_version_filenames);

	private Q_SLOTS:

		void
		save_changes();

		void
		abort_save();

		void
		close();

	private:

		/**
		 * Used to query the current GPGIM version.
		 */
		const GPlatesModel::Gpgim &d_gpgim;


		/**
		 * Overrides the default labels on the StandardButtons Qt provides, and adds icons.
		 */
		void
		tweak_buttons(
				ActionRequested act);

		/**
		 * Sets the dialog's main descriptive label (as defined in UI)
		 * to something more context-sensitive.
		 */
		void
		tweak_label(
				ActionRequested act);
			
	};
}

#endif  // GPLATES_QTWIDGETS_GPGIMVERSIONWARNINGDIALOG_H
