/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2007 The University of Sydney, Australia
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
 
#include "AboutDialog.h"
#include "LicenseDialog.h"
#include "global/Constants.h"  // the copyright string macro

GPlatesQtWidgets::AboutDialog::~AboutDialog()
{
}

GPlatesQtWidgets::AboutDialog::AboutDialog(
		QWidget *parent_):
	QDialog(parent_),
	d_license_dialog_ptr(NULL)
{
	setupUi(this);

	QObject::connect(button_License, SIGNAL(clicked()),
			this, SLOT(pop_up_license_dialog()));

	QString version(QObject::tr(GPlatesGlobal::VersionString));
	label_GPlates->setText(version);

	QString copyright(QObject::tr(GPlatesGlobal::HtmlCopyrightString));
	text_Copyright->setHtml(copyright);
}

void
GPlatesQtWidgets::AboutDialog::pop_up_license_dialog()
{
	if (!d_license_dialog_ptr)
	{
		d_license_dialog_ptr.reset(new LicenseDialog(this));
	}

	d_license_dialog_ptr->show();
	// In most cases, 'show()' is sufficient. However, selecting the menu entry
	// a second time, when the dialog is still open, should make the dialog 'active'
	// and return keyboard focus to it.
	d_license_dialog_ptr->activateWindow();
	// On platforms which do not keep dialogs on top of their parent, a call to
	// raise() may also be necessary to properly 're-pop-up' the dialog.
	d_license_dialog_ptr->raise();
}

