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

#include "global/License.h"  // the copyright string macro
#include "global/Version.h"

#include "gui/Dialogs.h"

#include "model/Gpgim.h"
#include "model/GpgimVersion.h"


GPlatesQtWidgets::AboutDialog::AboutDialog(
		GPlatesGui::Dialogs &dialogs,
		QWidget *parent_):
	GPlatesDialog(
			parent_,
			Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::MSWindowsFixedSizeDialogHint),
	d_license_dialog_ptr(new LicenseDialog(this))
{
	setupUi(this);

	QObject::connect(button_License, SIGNAL(clicked()),
			d_license_dialog_ptr, SLOT(show()));

	// Set GPlates version label text.
	QString version(QObject::tr(GPlatesGlobal::Version::get_GPlates_version().toLatin1().constData()));
	label_GPlates->setText(version);

	// Set the GPGIM version label.
	const QString gpgim_version_string = GPlatesModel::Gpgim::instance().get_version().get_version_string();
	label_GPGIM_version->setText(QString("GPlates Geological Information Model: %1").arg(gpgim_version_string));

	// Set contents of copyright box.
	QString copyright(QObject::tr(GPlatesGlobal::License::get_html_copyright_string().toLatin1().constData()));
	text_Copyright->setHtml(copyright);
}
